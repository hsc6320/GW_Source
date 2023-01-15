/*
 * Main.cpp
 *
 *  Created on: 2021. 10. 22.
 *      Author: hong
 */

#include "pch.h"
#include "uart/uart.h"
#include "MsgHandler.h"
#include "Socket/Socket.h"
#include "Socket/SocketHandler.h"
#include "Socket/SocketMsgQueue.h"

#include <sys/resource.h>
#include <sched.h>
#include <vector>
#include <memory>
#include <cmath>


using namespace std;
#define SETPANID(u8data)

Socket*	m_pSocket;
MsgHandler* m_pMsgHandler;
UartComThread* m_MainComport;
MsgQueue* m_pMsgQueue;
SocketHandler*	m_pSocketHandle;
pthread_t Main_thread[2];
pthread_mutex_t Main_mutex;
pthread_mutex_t Main_Uartmutex;
pthread_mutex_t Main_Tagmutex;

timer_t firstTimerID;
timer_t secondTimerID;
timer_t thirdTimerID;


PRE_DEFINE::S_PACKET	m_GetInforPacket;
#define BUF_MAX 512

template <typename T>
std::vector<uint8_t> vec;
std::vector<std::vector<BYTE>> m_ArrayDataAcknowledge;
int nlsChecksum =0;
int nflagRegistration_Req =0;
int nTagCnt =0;
int socket_fd =0;
int nBeaconCnt =0, nTempBeaconCnt =0, nTemp2BeaconCnt = 0;
int nBeaconValue =0;
int bReDownloadFlag =0, bDataAckFlag =0;
int GetUartMsg();
BYTE GetChecksum(BYTE* puData, int len);
WORD ByteToWord(BYTE puData, BYTE puData1);
bool EraseZero(BYTE x);
BYTE m_SendData[1024];


int Main_Socket_Init();
int Socket_Registration_Req();
int Socket_Connect_Req();
int ServiceStart_Cfm();
int UartToSocket_TagAssociation();
int Main_SendSocketMsgToUart(int msgtype);
int Main_ServiceStart_TagAssociation_Init();
int UartToSocket_Service_cfm();
int Main_ByPass_SocketToUart();
int Main_ByPass_UartToSocket();
int uart_SetTimer();
uint32_t Set_WaitTimer(timer_t *timerID, int expireMS, int intervalMS);
void PrintfHello(int sig, siginfo_t* si, void* uc);
void th_delay(int millsec);
int ServerReConn();
int TagAssociation_Init();
void Main_Service_Stop();
int Main_TagVal_SortAndArrange(std::vector<std::vector<BYTE>> V_ArrayData);
int Main_TagVal_SortAndArrange2(std::vector<std::vector<BYTE>> V_ArrayData);
int Main_Check_TagVal(int* iTemp, int* iTemp2);
int Main_Check_TagVal2(int* iTemp, int* iTemp2);

int Main_TagSort_Arrange(int* iTemp, int* iTemp2);
int Main_TagSort_Arrange2(int* iTemp, int* iTemp2);
void Main_DataAck_Handling();
int Main_DownDataSort();

static void timer_handler(int signum)
{

	printf("Send Timer %d\n\n", signum);
}


int uart_SetTimer()
{
	struct sigaction sa;
	struct itimerval timer;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &timer_handler;
	sigaction(/*SIGVTALRM*/SIGRTMIN, &sa, NULL);

	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;

	timer.it_value.tv_sec = 5;
	timer.it_value.tv_usec = 0;

	setitimer(/*ITIMER_REAL*/CLOCK_REALTIME, &timer, NULL);

	while(1) {
		if(getitimer(/*ITIMER_REAL*/CLOCK_REALTIME, &timer) == -1) {
			printf("timer error\n");
			exit(1);
		}
		printf("%d sec, %d msec.\n", (int)timer.it_value.tv_sec, (int)timer.it_value.tv_usec);
		sleep(1);
	}

	return 1;
}


int Main_ByPass_UartToSocket()
{
	int msg = 0;

	if(m_pMsgQueue->m_bReadEnd_UartMessage) {
		printf("Main_ByPass_UartToSocket\n");
		m_pMsgQueue->m_bReadEnd_UartMessage =0;
		msg = (int)m_pMsgQueue->m_u8SendData[MSGTYPE];

		switch(msg)
		{
		case COORDINATOR_RESET_CONFIRM:
			nBeaconCnt = 0;
			m_pMsgHandler->m_nDataIndiCount =0;
			m_pMsgHandler->m_nDataDownCount =0;
			m_pMsgHandler->m_UartArrayDataDownMsg.clear();
			m_pMsgHandler->m_UartArrayDataIndicateMsg.clear();
			m_pMsgQueue->m_nSendTagCount =0;
			m_pSocket->m_SocketArrayDataDownMsg.clear();
			m_pSocket->m_SocketArrayDataIndicateMsg.clear();
			m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.clear();
			m_pMsgQueue->m_MsgQueueDataAcknowledge.clear();
			m_pMsgQueue->m_nMapParity =0;
			m_pMsgQueue->Map_dataAcknowParityCheck.clear();
			m_pMsgHandler->Map_dataParityCheck.clear();
			m_pMsgHandler->Map_AcknowParityCheck.clear();
			m_pMsgHandler->bClear();
			m_pSocketHandle->SetMsg_StartCfm_Remalloc();
			m_pMsgQueue->m_MsgQueueTagData.clear();
			m_pSocketHandle->m_nTagDataCount =0;
			bReDownloadFlag = 0;
			bDataAckFlag =0;
			nTempBeaconCnt =0;
			nTemp2BeaconCnt =0;
			memset(m_pMsgHandler->m_GetDownTagID, 0, 512);
			break;

		case SERVICESTART_CONFIRM:
			printf("SERVICESTART_CONFIRM\n");
			if(GetUartMsg()) {
				UartToSocket_Service_cfm();
			}
			break;
		case TAG_ASSOCIATION:
			printf("TAG_ASSOCIATION\n");
			m_pMsgQueue->m_bReadEnd_UartMessage =0;
			UartToSocket_TagAssociation();
			break;
		default :
			break;
		}
	}
	return 1;
}


int Main_ServiceStart_TagAssociation_Init()
{
	int msg =0, loopEnd =0, disConn =0;

	printf("Main_ServiceStart_TagAssociation_Init\n");
	m_pSocket->m_Main_ServiceStart_TagAssociation_InitFlag = 1;
	m_pMsgQueue->m_Uart_ServiceStart_TagAssociation_InitFlag =1;

	while(1) {
		switch(msg) {
			case 0:
				printf("case0\n");
				Socket_Registration_Req();
				msg++;
				break;
			case 1:
				if(m_pSocket->m_iSocketReceiveEnd) {
					printf("case 1:\n");
					m_pSocket->m_iSocketReceiveEnd =0;
					m_pSocketHandle->GetServerID(ByteToWord(m_pSocket->m_SocketMsg_vec[MSG_SADDRONE], m_pSocket->m_SocketMsg_vec[MSG_SADDRZERO]));
					msg++;
				}
				break;
			case 2:
				printf("case 2\n");
				Socket_Connect_Req();
				msg++;
				break;
			case 3:
			/*	if(m_pSocket->m_iSocketReceiveEnd) {
					if(m_pSocket->m_SocketMsg_vec[MSGTYPE] == CONNECT_CONFIRM) {
						m_pSocket->m_iSocketReceiveEnd =0;
						printf("main switch 3-1 SERVIadCESTART_msgtype: 0x%x\n", m_pSocket->m_SocketMsg_vec[MSGTYPE]);
						msg++;
					}
				}
				break;
			case 4:*/
			//	printf("MESSAGE TYPE : 0x%x(%d)\n", m_pSocket->m_SocketMsg_vec[MSGTYPE]);
				if(m_pSocket->m_iSocketReceiveEnd || m_pSocket->m_iBypassSocketToUart) {
					if(m_pSocket->m_SocketMsg_vec[MSGTYPE] == SERVICESTART_REQUEST) {
						printf("main switch 3-2 SERVICESTART_REQUEST 0x%x\n", m_pSocket->m_SocketMsg_vec[MSGTYPE]);
						Main_SendSocketMsgToUart(m_pSocket->m_SocketMsg_vec[MSGTYPE]);
					//	m_pSocket->m_iBypassSocketToUart =0;
						msg++;
					}
				}
				break;
			case 4:
				if(ServiceStart_Cfm()) {
					msg = msg+2;
					break;
				}
				break;
			case 5:
				Main_ByPass_SocketToUart();
				msg = msg+2;
				break;
			default :
				m_pSocket->m_iBypassSocketToUart =0;
				loopEnd =1;
				m_pSocket->m_Main_ServiceStart_TagAssociation_InitFlag =0;
				break;
		}
		if(m_pSocket->bWorkingThread == 0) {
			disConn =1;
			break;
		}
		if(loopEnd) {
			break;
		}
		usleep(200);
	}
	printf("Main_ServiceStart_TagAssociation_Init() END \n");

	if(disConn)
		return 0;
	else
		return 1;
}

int Main_ByPass_SocketToUart()
{

	if(m_pSocket->m_iBypassSocketToUart && m_pSocket->m_iSocketReceiveEnd) {
		printf("Main_ByPass_SocketToUart() : ");
		for(int i=0; i<m_pSocket->m_ReceiveData_len; i++) {
			printf("%x ", m_pSocket->m_p8uData[i]);
		}
		m_pMsgHandler->BypassSocketToUart(m_pSocket->m_p8uData,	m_pSocket->m_ReceiveData_len, m_pSocket->m_SocketMsg_vec[MSGTYPE]);

		delete[] m_pSocket->m_p8uData;
		m_pSocket->m_p8uData = NULL;

		m_pSocket->m_iBypassSocketToUart =0;
		m_pSocket->m_iSocketReceiveEnd =0;

	}
	else if(m_pSocket->m_iSocketReceiveQueue) {
		m_pSocket->m_iSocketReceiveQueue =0;

		m_pMsgHandler->SetSocketArray(m_pSocket->m_SocketArrayDataDownMsg, m_pSocket->m_SocketArrayDataIndicateMsg,
				m_pSocket->m_nSocketArrayDataDownCnt, m_pSocket->m_nSocketArrayDataIndicateCnt, m_pMsgQueue->Map_dataAcknowParityCheck);

		printf("Socket Communication End\n");
	}
	else if(m_pMsgQueue->m_bUartCommuniFlag) {
		m_pMsgQueue->m_bUartCommuniFlag =0;

		switch((int)m_pMsgQueue->m_vcemsg.MsgPacket.u8MsgType)
		{
		case DOWNLOAD_START_ACK:
			printf("DOWNLOAD_START_ACK Compare Tag ID : %x, %x\n", m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[0]| m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[1], m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRZERO)|m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRONE));
			Main_DataAck_Handling();

			if(!bReDownloadFlag) {
				if( (m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[0]| m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[1])
						== (m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRZERO)|m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRONE)))
				{
					if(bDataAckFlag) {
						m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]] = PASS;
						printf("!bReDownloadFlag Parity Pass %d %d\n", m_pMsgHandler->m_nDataDownCount,
													m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]]);
					}
					else {
						m_pMsgHandler->Map_dataParityCheck.insert({m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount], PASS});
						printf("!bReDownloadFlag Parity Pass %d %d\n", m_pMsgHandler->m_nDataDownCount,
													m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]]);
					}
				}
				else {
					if(bDataAckFlag) {
						m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]] = FAIL;
					}
					else {
						m_pMsgHandler->Map_dataParityCheck.insert({m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount], FAIL});
						printf("Parity Fail\n");
					}
				}
			}
			else {
				printf("Before Parity Pass %d %d\n", m_pMsgHandler->m_nDataDownCount,	m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]]);

				if( (m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[0]| m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[1]) ==
						(m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRZERO)|m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRONE))) {

					m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]] = PASS;
					printf("After Parity Pass %d %d\n", m_pMsgHandler->m_nDataDownCount,
							m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]]);
				}
				else {
					m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]] = FAIL;
					printf("Parity Ack Fail\n");

				}
			}

			printf("!bReDownloadFlag %d && !bDataAckFlag %d m_nDataDownCount : %d\n", bReDownloadFlag, bDataAckFlag, m_pMsgHandler->m_nDataDownCount);
			if(!bReDownloadFlag && !bDataAckFlag) {
				m_pMsgHandler->UartPacket_DataIndicateStart(nBeaconValue);
				m_pMsgHandler->m_nDataDownCount++;
			}
			else if(bDataAckFlag) {
				m_pMsgHandler->UartPacket_ReDataAcknowledge_DataIndicateStart(nBeaconValue);
			}
			else if(bReDownloadFlag) {
				m_pMsgHandler->UartPacket_ReDataIndicateStart(nBeaconValue);
			}
			break;

		case DATAINDICATION_ACK:
			printf("DATAINDICATION_ACK Compare Tag ID : %x,  %x\n", m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[0]|m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[1],m_pMsgHandler->m_UartArrayDataIndicateMsg[m_pMsgHandler->m_nDataIndiCount].at(3)|m_pMsgHandler->m_UartArrayDataIndicateMsg[m_pMsgHandler->m_nDataIndiCount].at(4));

			if(!bReDownloadFlag && !bDataAckFlag) {
				m_pMsgHandler->m_nDataIndiCount++;
				m_pMsgHandler->UartPacket_DataDownStart(nBeaconValue);
			}
			else if(bDataAckFlag) {
				m_pMsgHandler->UartPacket_ReDataAcknowledge_DownStart(nBeaconValue);
			}
			else if(bReDownloadFlag){
				m_pMsgHandler->UartPacket_ReDataDownStart(nBeaconValue);
			}
			break;

		case BSN_START_ACK:
			nBeaconValue = m_pMsgQueue->m_vcemsg.MsgPacket.data[0];
			m_pMsgHandler->m_DataFlag =0;
			m_pMsgHandler->m_DataCnt =0;
			m_pMsgHandler->Map_AcknowCnt =0;
			m_pMsgHandler->Map_AcknowCnt2 = (16 * (nBeaconValue+1))-1;

			if(m_pMsgHandler->m_nUartArrayDataDownCnt == m_pMsgQueue->m_nMapParity) {
				Main_Service_Stop();
				return 0;
			}

			if(!bDataAckFlag && !bReDownloadFlag && ((int)m_pMsgQueue->m_vcemsg.MsgPacket.data[0] <= BEACON_MAX) && (nBeaconCnt < BEACON_MAX+1)) {
				m_pMsgHandler->UartPacket_DataDownStart(m_pMsgQueue->m_vcemsg.MsgPacket.data[0]);
				nBeaconCnt++;
				printf("First nBeaconCnt : %d\n", nBeaconCnt);
			}
			else if(bReDownloadFlag && ((int)m_pMsgQueue->m_vcemsg.MsgPacket.data[0] <= BEACON_MAX) && (nBeaconCnt < BEACON_MAX+1)) {
				m_pMsgHandler->UartPacket_ReDataDownStart(m_pMsgQueue->m_vcemsg.MsgPacket.data[0]);
				nBeaconCnt++;
				printf("Second RenBeaconCnt : %d\n", nBeaconCnt);
			}
			else if(bDataAckFlag && ((int)m_pMsgQueue->m_vcemsg.MsgPacket.data[0] <= BEACON_MAX) && (nBeaconCnt < BEACON_MAX+1)) {
				m_pMsgHandler->UartPacket_ReDataAcknowledge_DownStart(m_pMsgQueue->m_vcemsg.MsgPacket.data[0]);
				nBeaconCnt++;
				printf("Third RenBeaconCnt : %d\n", nBeaconCnt);
			}
			else if(nBeaconCnt >= BEACON_MAX+1) {
				int Redown =0, Redown2 =0;
				int j =0;
				m_pMsgHandler->m_nReDownCnt =0;
				m_pMsgHandler->m_nDownloadedSuccessCnt =0;
				printf("nBeaconCnt : %d > BEACON_MAX\n", nBeaconCnt);

				if(!bDataAckFlag) {
					for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
						if(m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[i]] != PASS) {
							printf("Map_dataParityCheck[%d] FAIL \n", i);
							m_pMsgHandler->m_GetDownTagID[j] = (int)m_pMsgHandler->m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO);
							printf("TagID : 0x%x\n", m_pMsgHandler->m_GetDownTagID[j]);
							j++;
							Redown = 1;
						}
					}
				}

				if(Redown) {
					m_pMsgHandler->m_nReDownCnt = j;
					nBeaconCnt =0;
					bReDownloadFlag =1;
					m_pMsgHandler->m_nDataDownCount = 0;
					m_pMsgHandler->m_nDataIndiCount = 0;
					m_pMsgHandler->UartPacket_ReDataDownStart(m_pMsgQueue->m_vcemsg.MsgPacket.data[0]);
				}
				else if(!Redown && (m_pMsgHandler->m_nDownloadedSuccessCnt == m_pMsgHandler->m_nReDownCnt)) {
					nTempBeaconCnt++;
					printf("nTempBeaconCnt : %d\n", nTempBeaconCnt);

					if(nTempBeaconCnt > BEACON_MAX/2) {
						bReDownloadFlag =0;
						memset(m_pMsgHandler->m_GetDownTagID, 0, 512);
						printf("bDataAckFlag : %d, m_nUartArrayDataDownCnt : %d, m_nMapParity : %d\n", bDataAckFlag, m_pMsgHandler->m_nUartArrayDataDownCnt, m_pMsgQueue->m_nMapParity);

						if(!bDataAckFlag && (m_pMsgHandler->m_nUartArrayDataDownCnt > m_pMsgQueue->m_nMapParity) ) {
							Main_TagSort_Arrange(&j, &Redown2);
						}
						else if(bDataAckFlag && (m_pMsgHandler->m_nUartArrayDataDownCnt > m_pMsgQueue->m_nMapParity) ) {
							j =0;
							Main_TagSort_Arrange2(&j, &Redown2);
						}
						printf("Main j: %d, Redown : %d, Redown2 : %d\n", j, Redown, Redown2);
					}
					else {
						printf("return 1\n");
						return 1;
					}

					if(Redown2) {
						nTemp2BeaconCnt++;
						printf("Redown2 nTemp2BeaconCnt : %d\n", nTemp2BeaconCnt);
						if(nTemp2BeaconCnt > 2) {
							Main_Service_Stop();
						}
						
						printf("Redown2\n");
						nBeaconCnt =0;
						bDataAckFlag =1;
						m_pMsgHandler->m_nReDownCnt = j;
						m_pMsgHandler->m_nDownloadedSuccessCnt =0;
						m_pMsgHandler->m_nDataDownCount = 0;
						m_pMsgHandler->m_nDataIndiCount = 0;
						m_pMsgHandler->UartPacket_ReDataAcknowledge_DownStart(m_pMsgQueue->m_vcemsg.MsgPacket.data[0]);
					}
					else {
						if(m_pMsgHandler->m_nUartArrayDataDownCnt == m_pMsgQueue->m_nMapParity) {
							Main_Service_Stop();
						}
						else {
							nTempBeaconCnt =0;
							nTemp2BeaconCnt++;
							printf("Redown2 nTemp2BeaconCnt : %d\n", nTemp2BeaconCnt);
							if(nTemp2BeaconCnt > 1) {
								Main_Service_Stop();
							}
						}
					}
				}
			}
			break;
		}
	}

	return 1;
}


void Main_DataAck_Handling()
{

}
void Main_Service_Stop()
{
	printf("Beacon Stop\n");
	m_pMsgHandler->BSN_Stop_Packet();
	nBeaconCnt = 0;
	for(int i=0; i<m_pSocket->m_nSocketArrayDataDownCnt; i++) {
		m_pSocket->m_SocketArrayDataDownMsg[i].clear();
		m_pSocket->m_SocketArrayDataIndicateMsg[i].clear();
	}
	m_pSocket->m_nSocketArrayDataDownCnt = 0;
	m_pSocket->m_nSocketArrayDataIndicateCnt = 0;
	m_pSocket->m_SocketArrayDataDownMsg.clear();
	m_pSocket->m_SocketArrayDataIndicateMsg.clear();
	m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.clear();
	m_pMsgQueue->m_MsgQueueDataAcknowledge.clear();
	m_pMsgQueue->m_nMapParity =0;
	m_pMsgQueue->Map_dataAcknowParityCheck.clear();
	m_pMsgQueue->m_ArrayDataAcknowledge.clear();

	m_pMsgHandler->bClear();
	memset(m_pMsgHandler->m_GetDownTagID, 0, 512);
	bReDownloadFlag = 0;
	bDataAckFlag =0;
	nTempBeaconCnt =0;
	nTemp2BeaconCnt =0;
}

int Main_SendSocketMsgToUart(int msgtype)
{
	if(!msgtype) {
		printf("msgtype %d return\n", msgtype);
		return 0;
	}
	printf("Main_SendSocketMsgToUart() : ");

	if(/*m_pSocket->m_iBypassSocketToUart ||*/ m_pSocket->m_iSocketReceiveEnd) {
		m_pSocket->m_iSocketReceiveEnd =0;

		for(int i=0; i<m_pSocket->m_ReceiveData_len; i++) {
			printf("%x ", m_pSocket->m_p8uData[i]);
		}
		printf("\n");
		m_pMsgHandler->BypassSocketToUart(m_pSocket->m_p8uData, m_pSocket->m_ReceiveData_len, msgtype);
			delete[] m_pSocket->m_p8uData;
			m_pSocket->m_p8uData = NULL;
	}

	return 1;
}

int Main_TagSort_Arrange2(int* iTemp, int* iTemp2)
{
	printf("\nBefore Sort2: \n");
	printf("m_pMsgQueue->m_ArrayDataAcknowledge.size() : %d m_nUartArrayDataDownCnt : %d, m_nMapParity : %d\n", (int)m_pMsgQueue->m_ArrayDataAcknowledge.size(), m_pMsgHandler->m_nUartArrayDataDownCnt, m_pMsgQueue->m_nMapParity);
	for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge.size(); i++) {
		for(int k=0; k<(int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size(); k++) {
		//	printf("size : %d\n", (int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size());
			printf("%x ", m_pMsgQueue->m_ArrayDataAcknowledge[i][k]);
		}
	}
	printf("\n");
	Main_TagVal_SortAndArrange2(m_pMsgQueue->m_ArrayDataAcknowledge);

	//j = *iTemp;
	//Redown2 = *iTemp2;
	printf("Main_TagSort_Arrange2 () j : %d, Redown2 : %d\n", *iTemp, *iTemp2);
	Main_Check_TagVal2(iTemp, iTemp2);
	m_pMsgHandler->vMsgDataAcknowledge.clear();
	m_pMsgHandler->vMsgDataAcknowledge = m_pMsgQueue->m_ArrayDataAcknowledge;
	m_pMsgHandler->SetMapValueArray(m_pMsgQueue->m_ArrayDataAcknowledge, m_pMsgQueue->m_nMapParity);

	return 0;
}

int Main_DownDataSort()
{
	printf("Main_DownDataSort()\n");
	printf("Before Sort:\n");
	for(int i=0; i< (int)m_pSocket->m_SocketArrayDataDownMsg.size(); i++) {
		for(int j=0; j<(int)m_pSocket->m_SocketArrayDataDownMsg[i].size(); j++ ) {
			printf("%x ", m_pSocket->m_SocketArrayDataDownMsg[i][j]);
		}
		printf("\n");
	}

	sort(m_pSocket->m_SocketArrayDataDownMsg.end(), m_pSocket->m_SocketArrayDataDownMsg.begin());

	printf("Check\n");
	for(int i=0; i< (int)m_pSocket->m_SocketArrayDataDownMsg.size(); i++) {
		for(int j=0; j< (int)m_pSocket->m_SocketArrayDataDownMsg[i].size(); j++ ) {
			printf("%x ", m_pSocket->m_SocketArrayDataDownMsg[i][j]);
		}
		printf("\n");
	}
	return 1;
}

int Main_TagSort_Arrange(int* iTemp, int* iTemp2)
{
	printf("Main_TagSort_Arrange()\n");
	printf("Before Sort:");
	for(int i=0; i<=m_pMsgQueue->m_nMapParity; i++) {
		for(int j=0; j<(int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].size(); j++) {
			printf("%x ", m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(j));
		}
	}
	printf("\n");
	printf("m_nMapParity : %d\n", m_pMsgQueue->m_nMapParity);
	sort(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.begin(), m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.end());

	m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.erase(unique(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.begin(), m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.end()),
																	m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.end());		//Delete overlap

	for(int i=0; i<=m_pMsgQueue->m_nMapParity; i++) {
		for(int j=0; j<(int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].size(); j++) {
			printf("%x ", m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(j));
		}
	}
	printf("\n");
	m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.erase(unique(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.begin(), m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.end()),
																m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.end());		//Delete overlap
	for(int i=0; i<=m_pMsgQueue->m_nMapParity; i++) {
		for(int j=0; j<(int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].size(); j++) {
			printf("%x ", m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(j));
		}
	}
	printf("\n");

	printf("CHECK \n");
	Main_TagVal_SortAndArrange(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge);
	//j = *iTemp;
	//Redown2 = *iTemp2;

	printf("\nMain_TagSort_Arrange () j : %d, Redown2 : %d\n", *iTemp, *iTemp2);
	Main_Check_TagVal(iTemp, iTemp2);

	m_pMsgHandler->vMsgDataAcknowledge.clear();
	m_pMsgHandler->vMsgDataAcknowledge = m_pMsgQueue->m_MsgQueueArrayDataAcknowledge;
	m_pMsgHandler->SetMapValueArray(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge, m_pMsgQueue->m_nMapParity);

	printf("m_ArrayDataAcknowledge size 1 :%d\n", m_pMsgQueue->m_ArrayDataAcknowledge.size());
	m_pMsgQueue->m_ArrayDataAcknowledge.resize((int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.size());
	printf("m_ArrayDataAcknowledge size 2 :%d\n", m_pMsgQueue->m_ArrayDataAcknowledge.size());
	std::copy(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.begin(), m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.end(), m_pMsgQueue->m_ArrayDataAcknowledge.begin());

	printf("m_ArrayDataAcknowledge\n");
	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		for(int k=0; k<(int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size(); k++) {
			printf("%x ", m_pMsgQueue->m_ArrayDataAcknowledge[i].at(k));
		//	printf("%x", m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(k));
		}
	}
	printf("\n");
	return 0;
}

int Main_Check_TagVal(int* iTemp, int* iTemp2)
{
	int icnt =1, cnt =1;
	int j = 0;
	int Redown2 =0;

	printf("Main_Check_TagVal()\n");
	while(icnt <=m_pMsgHandler->m_nUartArrayDataDownCnt) {

		if(cnt != ((int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[icnt-1].at(0))) {
			m_pMsgHandler->Map_AcknowParityCheck.insert({m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[icnt-1], FAIL});
			j++;
			Redown2 = 1;
			printf("TagID(%d): %x, for I : %d\n",icnt-1, m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[icnt-1].at(0), cnt);
			printf("Map_AcknowParityCheck Fail\n");
		}
		else if(cnt == ((int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[icnt-1].at(0))) {
			m_pMsgHandler->Map_AcknowParityCheck.insert({m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[icnt-1], PASS});
			printf("Map_AcknowParityCheck Pass\n");
		}
		icnt++;
		cnt++;

		if(cnt > m_pMsgHandler->m_nUartArrayDataDownCnt) {
			m_pMsgQueue->Redown =1;
			m_pMsgQueue->m_DataAckCnt = m_pMsgHandler->m_nUartArrayDataDownCnt;
			printf("m_pMsgQueue->Redown :%d\n", m_pMsgQueue->Redown);
			break;
		}
	}
	printf("2th Check!!! Complete Data Sort && Arrange!!!  %d \n", (int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.size());
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		printf("%d[%x] ",j, m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[j].at(0));
	}
	printf("\n");
	*iTemp = j;
	*iTemp2 = Redown2;
	printf("Main_Check_TagVal() j : %d, Redown2 : %d\n", j, Redown2);

	return 1;
}

int Main_Check_TagVal2(int* iTemp, int* iTemp2)
{
	int icnt =0, cnt =1;
	int j = 0;
	int Redown2 =0;
	BYTE temp2 =0;
	printf("Main_Check_TagVal2()\n");
	while(icnt <= m_pMsgHandler->m_nUartArrayDataDownCnt) {
		printf("TagID(%d): %x, for I : %d\n",icnt, m_pMsgQueue->m_ArrayDataAcknowledge[icnt].at(0), cnt);

		if(cnt != ((int)m_pMsgQueue->m_ArrayDataAcknowledge[icnt].at(0))) {
			temp2 = cnt;

		/*	m_pMsgQueue->m_ArrayDataAcknowledge[icnt-1].at(0) = temp2;
			printf("[%x]\n", m_pMsgQueue->m_ArrayDataAcknowledge[icnt-1].at(0));*/

			m_pMsgHandler->Map_AcknowParityCheck[m_pMsgQueue->m_ArrayDataAcknowledge[icnt]] = FAIL;
			j++;
			Redown2 = 1;
			nTempBeaconCnt =0;
			printf("TagID(%d): %x, for I : %d\n",icnt, m_pMsgQueue->m_ArrayDataAcknowledge[icnt].at(0), cnt);
			printf("Map_AcknowParityCheck Fail : [%d]\n", m_pMsgHandler->Map_AcknowParityCheck[m_pMsgQueue->m_ArrayDataAcknowledge[icnt-1]]);
		}
		else if(cnt == ((int)m_pMsgQueue->m_ArrayDataAcknowledge[icnt-1].at(0))) {
			m_pMsgHandler->Map_AcknowParityCheck[m_pMsgQueue->m_ArrayDataAcknowledge[icnt]] = PASS;
		//	printf("Map_AcknowParityCheck Pass\n");
		}
		icnt++;
		cnt++;

		if(cnt > m_pMsgHandler->m_nUartArrayDataDownCnt) {
			printf("m_MsgQueueArrayDataAcknowledge Clear\n\n");
			m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.clear();
			m_pMsgQueue->m_MsgQueueArrayDataAcknowledge = m_pMsgQueue->m_ArrayDataAcknowledge;
			break;
		}
	}

	*iTemp = j;
	*iTemp2 = Redown2;
	printf("Main_Check_TagVal() j : %d, Redown2 : %d\n", j, Redown2);
	return 0;
}

#if 1
int Main_TagVal_SortAndArrange(std::vector<std::vector<BYTE>> V_ArrayData)
{
	std::vector<BYTE> TempVec;

	printf("After Sort(size[%d]) : ", (int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.size());
	BYTE temp2 =0;
	//m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.resize(m_pMsgHandler->m_nUartArrayDataDownCnt);
	printf("m_MsgQueueArrayDataAcknowledge resize : %d \n", (int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.size());

	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		if(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].size() <= 0) {
			TempVec.clear();
			temp2 = 0xff;
			TempVec.push_back(temp2);
			m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.insert(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.begin()+i,TempVec);
		}
		if( (i == 0) && (m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(0) != i+1) ) {
			printf("\ni == %d \n", i);
			TempVec.clear();
			temp2 = 0;
			TempVec.push_back(temp2);
			m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.insert(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.begin(),TempVec);
		}
		else if(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(0) != i+1) {
			printf("\nParity Error [%d] , ",i);
			TempVec.clear();
			temp2 = 0;
			TempVec.push_back(temp2);
		//	m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.resize((int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.size()+1);
			m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.insert(m_pMsgQueue->m_MsgQueueArrayDataAcknowledge.begin()+i,TempVec);
			printf("insert value : %d\n", m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(0));
		}

		for(int j=0; j<(int)m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].size(); j++) {
			printf("[%x] ", m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[i].at(j));
		}

	}
	printf("\n\n");
	printf("Complete Data Sort && Arrange !!! : ");
	for(int j=0; j<(int)m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		printf("%d[%x] ",j, m_pMsgQueue->m_MsgQueueArrayDataAcknowledge[j].at(0));
	}
	printf("\n");

	return 0;
}
#else
int Main_TagVal_SortAndArrange(std::vector<std::vector<BYTE>> V_ArrayData)
{
	std::vector<BYTE> TempVec;


	printf("m_nMapParity : %d\n", m_pMsgQueue->m_nMapParity);
	sort(V_ArrayData.begin(), V_ArrayData.end());

	V_ArrayData.erase(unique(V_ArrayData.begin(), V_ArrayData.end()),
																V_ArrayData.end());		//Delete overlap

	m_pMsgQueue->m_ArrayDataAcknowledge = V_ArrayData;


	printf("After Sort(size[%d]) : ", (int)V_ArrayData[0].size());
	BYTE temp2 =0;
	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		if(V_ArrayData[i].size() <= 0) {
			TempVec.clear();
			temp2 = 0xff;
			TempVec.push_back(temp2);
			V_ArrayData.insert(V_ArrayData.begin()+i,TempVec);
		}
		if( (i == 0) && (V_ArrayData[i].at(0) != i+1) ) {
			printf("\ni == %d \n", i);
			TempVec.clear();
			temp2 = 0;
			TempVec.push_back(temp2);
			V_ArrayData.insert(V_ArrayData.begin(),TempVec);
		}
		else if(V_ArrayData[i].at(0) != i+1) {
			printf("\nParity Error [%d] , ",i);
			TempVec.clear();
			temp2 = 0;
			TempVec.push_back(temp2);
			V_ArrayData.insert(V_ArrayData.begin()+i,TempVec);
			printf("insert value : %d\n", V_ArrayData[i].at(0));
		}

		for(int j=0; j<(int)V_ArrayData[i].size(); j++) {
			printf("[%x] ", V_ArrayData[i].at(j));
		}

	}
	printf("\n\n");
	printf("Complete Data Sort && Arrange !!! : ");
	for(int j=0; j<(int)V_ArrayData.size(); j++) {
		printf("[%x] ", V_ArrayData[j].at(0));
	}
	printf("\n");

	return 0;
}
#endif
int Main_Socket_Init()
{
	int Ret =0;

	m_MainComport->Create_Uart_thread(Main_thread[0]);
	m_pMsgHandler->ServiceIdle();

	while(1) {
		if(m_pMsgQueue->m_bReadEnd_UartMessage) {
			printf("m_pMsgQueue->m_bReadEnd_UartMessage %d\n", m_pMsgQueue->m_bReadEnd_UartMessage);
			if(GetUartMsg()) {
				while(1) {
					m_pSocket->m_IP_String = "";
					socket_fd = m_pSocket->Socket_Init();
					if(socket_fd == -1)
						continue;
					else
						break;
				}
				th_delay(1000);
				m_pSocket->SetMutex(Main_mutex, socket_fd);
				m_pSocket->Create_Socket_Thread(Main_thread[1], socket_fd);

				printf("Socket End\n");
				m_pSocket->bWorkingThread = 1;
				m_pMsgQueue->m_bReadEnd_UartMessage =0;
				Ret =1;
				break;
			}
			else {
				printf("main server init fail\n");
				m_pMsgQueue->m_bReadEnd_UartMessage =0;
				break;
			}
		}
	}
	return Ret;
}

int Main_TagVal_SortAndArrange2(std::vector<std::vector<BYTE>> V_ArrayData)
{
	std::vector<BYTE> TempVec;
	BYTE temp2 =0;
	int Cnt[m_pMsgHandler->m_nUartArrayDataDownCnt] = {0, };
	int iCnt =0;

	printf("Sort Ago m_pMsgQueue->m_ArrayDataAcknowledge.size %d\n", (int)m_pMsgQueue->m_ArrayDataAcknowledge.size());
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			printf("[%x] ", m_pMsgQueue->m_ArrayDataAcknowledge[j].at(i));
		}
	}
	printf("\n");
	sort(m_pMsgQueue->m_ArrayDataAcknowledge.begin(), m_pMsgQueue->m_ArrayDataAcknowledge.end());
	printf("Sort after m_pMsgQueue->m_ArrayDataAcknowledge.size %d\n", (int)m_pMsgQueue->m_ArrayDataAcknowledge.size());
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			printf("[%x] ", m_pMsgQueue->m_ArrayDataAcknowledge[j].at(i));
		}
	}
	m_pMsgQueue->m_ArrayDataAcknowledge.erase(unique(m_pMsgQueue->m_ArrayDataAcknowledge.begin(), m_pMsgQueue->m_ArrayDataAcknowledge.end()),
																			m_pMsgQueue->m_ArrayDataAcknowledge.end());		//Delete overlap
/*	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		m_pMsgQueue->m_ArrayDataAcknowledge.erase(std::remove_if(m_pMsgQueue->m_ArrayDataAcknowledge[j].begin(), m_pMsgQueue->m_ArrayDataAcknowledge[j].end(), EraseZero)
				,m_pMsgQueue->m_ArrayDataAcknowledge[j].end());
	}*/

	printf("nAfter Erase_0 m_pMsgQueue->m_ArrayDataAcknowledge.size %d\n", (int)m_pMsgQueue->m_ArrayDataAcknowledge.size());
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			printf("[%x] ", m_pMsgQueue->m_ArrayDataAcknowledge[j].at(i));
		}
	}
	printf("\n");

	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		for(int j=0; j<(int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size(); j++) {
			printf("m_pMsgQueue->m_ArrayDataAcknowledge[%d][%d] :%x \n",i,j, m_pMsgQueue->m_ArrayDataAcknowledge[i][j]);
			if(m_pMsgQueue->m_ArrayDataAcknowledge[i][j] == 0) {
				Cnt[iCnt] = i;
				printf("Cnt[%d] :%d\n",iCnt, Cnt[iCnt]);
				m_pMsgQueue->m_ArrayDataAcknowledge.erase(m_pMsgQueue->m_ArrayDataAcknowledge.begin()+i, m_pMsgQueue->m_ArrayDataAcknowledge.begin()+i+1);
				iCnt++;
			}
			
		}
	}

	temp2 = 0;
	TempVec.push_back(temp2);
	/*for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			printf("[%x] ", m_pMsgQueue->m_ArrayDataAcknowledge[j].at(i));
		}
	}*/
	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		for(int j=0; j<(int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size(); j++) {
			printf("i:%d, j:%d [%x] ",i,j, m_pMsgQueue->m_ArrayDataAcknowledge[i].at(j));			
		}
	}
	printf("\n");

	printf("After Erase_0 1\n");
	printf("nAfter Erase_0 1 m_pMsgQueue->m_ArrayDataAcknowledge.size %d\n", (int)m_pMsgQueue->m_ArrayDataAcknowledge.size());
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			printf("%d[%x] ", j, m_pMsgQueue->m_ArrayDataAcknowledge[j].at(0));
		}
	}
	printf("\n");

	sort(m_pMsgQueue->m_ArrayDataAcknowledge.begin(), m_pMsgQueue->m_ArrayDataAcknowledge.end());
	printf("Sort \n");
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			printf("%d[%x] ",(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(), m_pMsgQueue->m_ArrayDataAcknowledge[j].at(0));
		}
	}

	th_delay(20);
	/*TempVec.clear();
	temp2 = 0;
	TempVec.push_back(temp2);
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			if(Cnt[j] == j) {
				m_pMsgQueue->m_ArrayDataAcknowledge.insert(m_pMsgQueue->m_ArrayDataAcknowledge.begin()+j,TempVec);
			}
		}
	}

	printf("Insert Zero \n");
	for(int j=0; j<m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		for(int i=0; i<(int)m_pMsgQueue->m_ArrayDataAcknowledge[j].size(); i++) {
			printf("[%x] ",m_pMsgQueue->m_ArrayDataAcknowledge[j].at(i));
		}
	}*/
	printf("\nAfter Sort2 %d: \n", (int)m_pMsgQueue->m_ArrayDataAcknowledge[0].size());

	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
	/*	if((int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size() <= 0) {
			TempVec.clear();
			temp2 = 0;
			TempVec.push_back(temp2);
			m_pMsgQueue->m_ArrayDataAcknowledge.insert(m_pMsgQueue->m_ArrayDataAcknowledge.begin()+i,TempVec);
			printf("insert value : %d\n", m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0));
		}*/
		if( ((i == 0) && (m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0) != i+1)) ) {
			printf("\nParity Error m_ArrayDataAcknowledge[%d].at(0) : %x \n, ",i, m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0));
			TempVec.clear();
			temp2 = 0;
			TempVec.push_back(temp2);
			m_pMsgQueue->m_ArrayDataAcknowledge.insert(m_pMsgQueue->m_ArrayDataAcknowledge.begin()+i,TempVec);
			printf("insert value : %d\n", m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0));
		}
		else if(m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0) != i+1) {
			printf("\nParity Error m_ArrayDataAcknowledge[%d].at(0) : %x \n, ",i, m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0));
			TempVec.clear();
			temp2 = 0;
			TempVec.push_back(temp2);
			m_pMsgQueue->m_ArrayDataAcknowledge.insert(m_pMsgQueue->m_ArrayDataAcknowledge.begin()+i,TempVec);
			printf("insert value : %d\n", m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0));
		}

		for(int j=0; j<(int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size(); j++) {
			printf("[%x] ", m_pMsgQueue->m_ArrayDataAcknowledge[i].at(j));
		}
	}	
	
	printf("\n\n");
	printf(" m_pMsgQueue->m_ArrayDataAcknowledge.size %d\n", (int)m_pMsgQueue->m_ArrayDataAcknowledge.size());
	printf("Complete Data Sort && Arrange2 !!! : ");
	for(int i=0; i<=m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		for(int j=0; j<(int)m_pMsgQueue->m_ArrayDataAcknowledge[i].size(); j++) {
			printf("%d[%x] ", i, m_pMsgQueue->m_ArrayDataAcknowledge[i].at(0));
		}
	}
	printf("\n");

	return 0;
}

bool EraseZero(BYTE x)
{
	if(x == 0) {
		return true;
	}
	return false;
}

int UartToSocket_TagAssociation()
{
	printf("UartToSocket_TagAssociation\n");
	m_pSocketHandle->TagData(m_pMsgQueue->m_MsgQueueTagData);
	//m_pSocketHandle->SendMessage(TAG_ASSOCIATION, m_GetInforPacket);
	return 1;
}

int UartToSocket_Service_cfm()
{
	printf("UartToSocket_Service_cfm()\n");
	if(m_pMsgQueue->m_nSendTagCount > 0) {
		m_pSocketHandle->SetMsg_StartCfm_Remalloc();
	}
	m_pSocketHandle->SendMessage(SERVICESTART_CONFIRM, m_GetInforPacket);
	m_pMsgQueue->m_bReadEnd_UartMessage =0;

	return 1;
}

int ServiceStart_Cfm()
{
	int Ret =0;

	while(m_pMsgQueue->m_bReadEnd_UartMessage) {
		if(GetUartMsg()) {
			m_pSocketHandle->SendMessage(SERVICESTART_CONFIRM, m_GetInforPacket);
			m_pMsgQueue->m_bReadEnd_UartMessage =0;
			Ret =1;
			break;
		}
		else {
			printf("main ServiceStart_Cfm\n");
			m_pMsgQueue->m_bReadEnd_UartMessage =0;
			break;
		}
	}

	return Ret;
}

int Socket_Registration_Req()
{
	m_pSocketHandle->SendMessage(REGISTRATION_REQUEST, m_GetInforPacket);
	printf("main_Socket_Registration_Req()\n");

	return 1;
}

int Socket_Connect_Req()
{
	m_pSocketHandle->SendMessage(CONNECT_REQUEST, m_GetInforPacket);
	printf("main_Socket_Connect_Req(), bWorkingThread : %d\n", m_pSocket->bWorkingThread);

	return 1;
}

int GetUartMsg()
{
	Vector<uint8_t> vec;// = m_pMsgQueue->m_vcemsg.m_UartMsg_vec;
	//std::vector<uint8_t> vSoceket;// = m_pMsgQueue->m_vcemsg.m_UartMsg_vec;
	PRE_DEFINE::S_PACKET	Getpacket;
	int size =0;
	int ipos =0;
	int loopEnd =0;

	//printf("GetUartMsg()\n");
	size = m_pMsgQueue->m_vcemsg.m_UartMsg_vec.size();
	//printf("size %d\n", size);
	if(size <= 0)	{
	//	printf("GetUartMsg() return 0\n");
		return 0;
	}
	for(int i =0; i< size; i++) {
		vec.push_back(m_pMsgQueue->m_vcemsg.m_UartMsg_vec[i]);
	}
	printf("\n");
	for(int i =0; i< size; i++) {
		m_pMsgQueue->m_vcemsg.m_UartMsg_vec.remove(i);
	}

	while(!loopEnd) {
		switch(ipos)
		{
		case 0:
			if(vec[MSG_STX] != STX) {
//				printf("\nSTX error %x\n", vec[MSG_STX]);
				return 0;
			}
			else {
				Getpacket.header.stx = vec[MSG_STX];
	//			printf("STX : [%x] ", Getpacket.header.stx);
				ipos++;
			}
			break;
		case 1:
			Getpacket.header.panID = ByteToWord(vec[MSG_PANIDONE], vec[MSG_PANIDZERO]);
	//		printf("panID [%x], ", Getpacket.header.panID);
			ipos = ipos +2;
			break;
		case 3:
			Getpacket.header.dAddr = ByteToWord(vec[MSG_DADDRZERO], vec[MSG_DADDRONE]);
	//		printf("Daddr[%x] , ",Getpacket.header.dAddr);
			ipos = ipos +2;
			break;
		case 5:
			Getpacket.header.sAddr = ByteToWord(vec[MSG_SADDRZERO], vec[MSG_SADDRONE]);
	//		printf("Saddr[%x], ", Getpacket.header.sAddr);
			ipos = ipos +2;
			break;
		case 7:
			Getpacket.header.type = vec[MSGTYPE];
	//		printf("MsgType[%x], ", Getpacket.header.type);
			ipos++;
			break;
		case 8:
			Getpacket.header.length = ByteToWord(vec[MSG_LENGTHONE], vec[MSG_LENGTHZERO]);
	//		printf("DataLen[%x], ", Getpacket.header.length);
			ipos = ipos +2;
			break;
		case 10:
			Getpacket.pu8Data = new BYTE[Getpacket.header.length];
			for(int i =0; i< Getpacket.header.length; i++) {
				Getpacket.pu8Data[i] = vec[MSG_LENGTHONE+1+i];
	//			printf("pu8Data [%x] ", Getpacket.pu8Data[i]);
			}
			ipos = ipos + (int)Getpacket.header.length;
			break;
		default:
			if(!nlsChecksum) {
				Getpacket.tail.checksum = vec[ipos];
	//			printf("checksum Val :%x ", Getpacket.tail.checksum);
				nlsChecksum =1;
				ipos++;
				break;
			}
			else {
				Getpacket.tail.ext[0] = vec[ipos];
				Getpacket.tail.ext[1] = vec[++ipos];
				Getpacket.tail.ext[2] = vec[++ipos];

				if((Getpacket.tail.ext[0] != 0xA5) || (Getpacket.tail.ext[1] != 0x5A) || (Getpacket.tail.ext[2] != 0x7E)) {
	//				printf("etx error return 0\n");
					return 0;
				}
				else {
	//				printf("etx : %x %x %x\n", Getpacket.tail.ext[0], Getpacket.tail.ext[1], Getpacket.tail.ext[2]);
					loopEnd =1;
					nlsChecksum =0;
					Getpacket.header.DataLength = vec.size();
					break;
				}
			}

		}		//END SWITCH
	}		//END WHILE
	m_GetInforPacket = Getpacket;

	if(Getpacket.header.type == 0x21) {
		uart_SetTimer();
	}

	if(Getpacket.header.type == REGISTRATION_CONFIRM) {
		m_pSocketHandle->GetServerID(Getpacket.header.sAddr);
	//	m_pSocket->m_pSocMsgqueue->GetServerID(Getpacket.header.sAddr);
	}
	if(Getpacket.header.type == GATEWAYID_RESPONSE) {
		m_pSocketHandle->GetPanID(Getpacket.header.panID);
	//	m_pSocket->m_pSocMsgqueue->GetPanID(Getpacket.header.panID);
	}

	printf("return 1\n");
	return 1;
}

WORD ByteToWord(BYTE puData, BYTE puData1)
{
	WORD p16Tempdata_HIGH, p16Tempdata_LOW;

	p16Tempdata_HIGH = (puData << 8);
	p16Tempdata_LOW = puData1;

	return p16Tempdata_HIGH|p16Tempdata_LOW;
}

void th_delay(int millsec)
{
	double time;
	double timedelay = millsec;
	struct timeval start1 = {};
	struct timeval end1 = {};

	clock_t end = timedelay* 1000;
	clock_t start = clock();

	//printf("th_dealy %.2f msec\n", timedelay*2);
	gettimeofday(&start1 , NULL);
	while(clock()-start < end) {;}
	gettimeofday(&end1 , NULL);
	time = end1.tv_sec + end1.tv_usec / 1000000.0 - start1.tv_sec - start1.tv_usec / 1000000.0;
	printf("%.2f sec\n", time);
}

uint32_t Set_WaitTimer(timer_t *timerID, int expireMS, int intervalMS)
{

#if 1
	struct sigevent te;
	struct itimerspec its;
	struct sigaction sa;
	int sigNo = SIGRTMIN;

	/* Set up signal handler. */
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &PrintfHello;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sigNo, &sa, NULL) == -1) {
		perror("sigaction");
	}

	/* Set and enable alarm */
	te.sigev_notify = SIGEV_SIGNAL;
	te.sigev_signo = sigNo;
	te.sigev_value.sival_ptr = timerID;
	timer_create(CLOCK_REALTIME, &te, timerID);

	its.it_interval.tv_sec = 1;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = expireMS;
	its.it_value.tv_nsec = 0;//expireMS * 1000;// * 1000000;
	timer_settime(*timerID, 0, &its, NULL);

#endif
	return 1;
}

void PrintfHello(int sig, siginfo_t* si, void* uc)
{
	timer_t* tidp;

	tidp = (void**)(si->si_value.sival_ptr);

	if(*tidp == firstTimerID) {
		printf("2ms\n");
	}
	else if(*tidp == secondTimerID) {
			printf("10ms\n");
	}
	else if (*tidp == thirdTimerID){
		//Main_Socket_Init();
			printf("1 S\n");

	}
}

int TagAssociation_Init()
{
	while (1) {
		if(Main_ServiceStart_TagAssociation_Init() == 0) {
			ServerReConn();
		}
		else
			break;
	}

	return 1;
}

int ServerReConn()
{
	while(1) {
		printf("Main bWorkingThread : %d\n", m_pSocket->bWorkingThread);
		m_pSocket->Exit_Socket_Thread();
		m_pSocket->m_IP_String = "";
		th_delay(500);
		socket_fd = m_pSocket->Socket_Init();
		if(socket_fd == -1)
			continue;
		th_delay(500);
		m_pSocket->Create_Socket_Thread(Main_thread[1], socket_fd);
		printf("Socket Re-Init End\n");
		m_pSocket->bWorkingThread = 1;
		m_pMsgQueue->m_bReadEnd_UartMessage =0;

		break;
	}

	return 1;
}
BYTE GetChecksum(BYTE* puData, int len)
{
	uint8_t sum =0;

	len += (sizeof(PRE_DEFINE::S_HEADER)-1);
	printf("sizeof(%d) len %d\n",sizeof(PRE_DEFINE::S_HEADER), len);
	for(;  0 < len; len--) {
		sum+= *(puData++);
		printf("sum : %x\n", sum);
	}
	return sum;
}



int main(int argc, char *argv[])
{
	pthread_mutex_init(&Main_mutex, NULL);
	pthread_mutex_init(&Main_Uartmutex, NULL);
	pthread_mutex_init(&Main_Tagmutex, NULL);
	setpriority(PRIO_PROCESS, getpid(), -10);

	m_MainComport->uart_init();

	m_MainComport->SetMutex(Main_Uartmutex);
	th_delay(30);
	th_delay(40);
	th_delay(50);
	th_delay(100);
	th_delay(200);
	th_delay(700);

	Main_Socket_Init();
	TagAssociation_Init();

	while(1)
	{
		Main_ByPass_SocketToUart();
		Main_ByPass_UartToSocket();

		if(m_pSocket->bWorkingThread == 0) {
			ServerReConn();
			TagAssociation_Init();
			printf("Main TagData\n");
			m_pSocketHandle->TagData(m_pMsgQueue->m_MsgQueueTagData);
		}
		usleep(100);
	}

	return 0;
}


void __attribute__((constructor)) NewObject()
{
	m_MainComport = new UartComThread;
	m_pMsgQueue = new MsgQueue;
	m_pMsgHandler = new MsgHandler;
	m_pSocket = new Socket;
	m_pSocketHandle = new SocketHandler;


	m_MainComport->SetUartComm(m_MainComport);
	m_MainComport->SetMsgQueue(m_pMsgQueue);
	m_pMsgHandler->SetHandle(m_MainComport);

	m_pSocket->SetMsgHwnd(m_pSocket);
	m_pMsgQueue->GetSocket(m_pSocket);
	m_pSocketHandle->SetSocketHwnd(m_pSocket);
	m_pSocketHandle->SetMsgQueueHwnd(m_pMsgQueue);

	printf("NewObject\n");
}

void __attribute__((destructor)) DeleteObject()
{
	m_MainComport->Uart_Close(m_MainComport->m_uartd);
	printf("Main Function Exit\n uart Close\n");

	m_pSocket->Exit_Socket_Thread();

	delete m_MainComport;
	delete m_pMsgQueue;
	delete m_pMsgHandler;
	delete m_pSocket;
	delete m_pSocketHandle;

	printf("DeleteObject\n");
	printf("Close\n");
}

