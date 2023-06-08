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
#include "Vector_SocketQueue.h"

#include <sys/resource.h>
#include <sched.h>
#include <vector>
#include <memory>
#include <cmath>


using namespace std;

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
timer_t DataDownTimerID;
timer_t DataIndecateTimerID;
int AckFail_Redown =0;

int firstTimerFlag =0, SecondTimerFlag=0;

PRE_DEFINE::S_PACKET	m_GetInforPacket;
#define BUF_MAX 512

template <typename T>
std::vector<uint8_t> vec;
std::vector<std::vector<BYTE>> m_ArrayDataAcknowledge;

std::map<WORD, int> mapTagDirectSet;

int nlsChecksum =0;
int socket_fd =0;
int nBeaconCnt =0, nTempBeaconCnt =0, nTemp2BeaconCnt = 0, nTemp1BeaconCnt=0;
BYTE nBeaconValue =0;
int bReDownloadFlag =0, bDataAckFlag =0;
int BEACON_MAX =0;
int bSocketAlive =1;
WORD nDirectDownTagNumber[1024] = {0, };

int GetUartMsg(PRE_DEFINE::S_PACKET* Getpacket);
BYTE GetChecksum(BYTE* puData, int len);
WORD ByteToWord(BYTE puData, BYTE puData1);


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

int Set_WaitTimer(timer_t *timerID, int expireMS, int intervalMS);
void PrintfHello(int sig, siginfo_t* si, void* uc);
void th_delay(int millsec);
int ServerReConn();
int TagAssociation_Init();
void Main_Service_Stop();
int Main_TagSort_Arrange(int* iTemp, int* iTemp2);
int Main_TagSort_Arrange2(int* iTemp, int* iTemp2);
int Main_TagAck_Arrange();

void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext);
void installSignal(int __sig);

int Main_TagArrayVal_CheckParity(int Temp);
int Main_Check_TagArrayPassFail(int* iTemp, int* iTemp2);
void Main_InsertArray(int idx, WORD sz, WORD* ar);

typedef struct _sig_ucontext {
  unsigned long uc_flags;
  struct ucontext *uc_link;
  stack_t uc_stack;
  struct sigcontext uc_mcontext;
  sigset_t uc_sigmask;
} sig_ucontext_t;


void timer_handler()
{
	printf("Send Timer \n\n");
}
void Main_deleteArray(int idx, int size, WORD* ar)
{
	memmove(ar+idx, ar+idx+1, size-idx);
}


int Main_GetSizeArray (WORD* ar)
{
	int ret =0;
	BYTE arr[4096];
	memcpy(arr, ar, 4096);
	int size = (sizeof(arr)/sizeof(*arr));
	
	for(int i=0; i< size; i++) {
		if(ar[i] > 0) {
			ret++;
		}
	}
	printf("GetSizeArray() ret :%d\n", ret);
	return ret;
}
void Main_InsertArray(int idx, WORD sz, WORD* ar)
{
	WORD arr[4096];
	memcpy(arr, ar, 4096);
	int size = (sizeof(arr)/sizeof(*arr));
	memmove(ar+idx+1, ar+idx, size-idx+1);
	ar[idx] = sz;
	//printf("Insert [%d]%x, size : %d\n", idx, ar[idx], size);
}

void Main_AppendArray(WORD sz, int size1, WORD* ar)
{
	Main_InsertArray(size1, sz, ar);
}

void Main_PrintArray1(WORD* ar, int size)
{
	WORD arr[4096];
	memcpy(arr, ar, 4096);
	for(int i=0; i<= size; i++) {
	//	if(ar[i] > 0)		
			printf("%x ", arr[i]);
	}	
	printf("\n\n");
}

int uart_SetTimer()
{
	/*struct sigaction sa;
	struct itimerval timer;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = timer_handler;*/
//	sigaction(/*SIGVTALRM*/SIGRTMIN, &sa, NULL);

/*	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;

	timer.it_value.tv_sec = 5;
	timer.it_value.tv_usec = 0;
*/
//	setitimer(/*ITIMER_REAL*/CLOCK_REALTIME, &timer, NULL);



//	while(1) {
//		if(getitimer(/*ITIMER_REAL*/CLOCK_REALTIME, &timer) == -1) {
//			printf("timer error\n");
//			exit(1);
//		}
//		printf("%d sec, %d msec.\n", (int)timer.it_value.tv_sec, (int)timer.it_value.tv_usec);
//		sleep(1);
//	}

	return 1;
}


int Main_ByPass_UartToSocket()
{
	int msg = 0;
	PRE_DEFINE::S_PACKET GetInforPacket;
	WORD Tag =0;
	std::set<WORD>::iterator iter;

	if(m_pMsgQueue->m_bReadEnd_UartMessage) {
	//	printf("Main_ByPass_UartToSocket\n");
		m_pMsgQueue->m_bReadEnd_UartMessage =0;
		msg = (int)m_pMsgQueue->m_u8SendData[MSGTYPE];

		switch(msg)
		{
		case COORDINATOR_RESET_CONFIRM:
			nBeaconCnt = 0;
			m_pMsgQueue->m_nSendTagCount =0;
			m_pSocket->m_SocketArrayDataDownMsg.clear();
			m_pSocket->m_SocketArrayDataIndicateMsg.clear();
			m_pMsgQueue->m_nMapParity =0;
			m_pMsgHandler->bClear();
			m_pSocketHandle->SetMsg_StartCfm_Remalloc(0);
			m_pSocketHandle->m_nTagDataCount =0;
			m_pMsgHandler->BSN_Stop_Packet();

			m_pMsgQueue->m_ArrayDataAcknowledge.clear();
			memset (m_pSocket->m_TagNumber, 0, sizeof(WORD)*4096);
			memset(m_pMsgQueue->m_Test, 0, sizeof(WORD)*4096);
			memset(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 0, sizeof(WORD)*4096);
			memset(m_pMsgHandler->m_pu16MsgDataAcknowledge, 0, sizeof(WORD)*4096);
			memset(nDirectDownTagNumber, 0, sizeof(WORD)*1024);
			m_pMsgHandler->Map_dataParityCheck.erase(m_pMsgHandler->Map_dataParityCheck.begin(), m_pMsgHandler->Map_dataParityCheck.end());
			m_pMsgHandler->Map_u16AcknowParityCheck.erase(m_pMsgHandler->Map_u16AcknowParityCheck.begin(), m_pMsgHandler->Map_u16AcknowParityCheck.end());
	
			bReDownloadFlag = 0;
			bDataAckFlag =0;
			nTempBeaconCnt =0;
			nTemp2BeaconCnt =0;
			break;

		case SERVICESTART_CONFIRM:
			UartToSocket_Service_cfm();
		//	ServiceStart_Cfm();
			break;
		case TAG_DIRECT_CHANGE_INDICATION:
			if(m_pMsgQueue->m_u8SendData[MSG_TAG_DIRECT_CHANGE_STATUS] == ENABLE){
				printf("TAG_DIRECT_CHANGE_INDICATION\n");
				m_pMsgHandler->iSmallDataDown =1;
				m_pMsgHandler->m_iTagDirectDown =1;
				Tag = ByteToWord(m_pMsgQueue->m_u8SendData[MSG_SADDRONE], m_pMsgQueue->m_u8SendData[MSG_SADDRZERO] );

				iter = m_pMsgQueue->setTagNumber.find(Tag);
				if(iter != m_pMsgQueue->setTagNumber.end()) {
					printf("Tag overlap\n");
				}
				else {					
					printf("Tag Insert : [%d] \n", Tag);
					m_pMsgQueue->setTagNumber.insert(Tag);
					for(auto str : m_pMsgQueue->setTagNumber) {
						if(str == Tag) {
							mapTagDirectSet.insert({str, ENABLE});
							printf("mapTagDirectSet.insert({%d, ENABLE})\n", Tag);
						}
					}
				}
				printf("TAG_DIRECT_CHANGE_INDICATION %d\n", m_pMsgHandler->iSmallDataDown);
			}
			else {
				Tag = ByteToWord(m_pMsgQueue->m_u8SendData[MSG_SADDRONE], m_pMsgQueue->m_u8SendData[MSG_SADDRZERO] );							

				m_pMsgQueue->setTagNumber.erase(m_pMsgQueue->setTagNumber.find(Tag));
				mapTagDirectSet[Tag] = DISABLE;
				if(m_pMsgQueue->setTagNumber.empty()) {
					m_pMsgHandler->iSmallDataDown =0;
					m_pMsgHandler->m_iTagDirectDown =0;
					m_pMsgQueue->setTagAckNumber.clear();
					printf("TAG_DIRECT_CHANGE_INDICATION %d\n", m_pMsgHandler->iSmallDataDown);
				}				
			}
			break;
		case TAG_ASSOCIATION:
			if(m_pMsgQueue->m_nSendTagCount > 0) {
				m_pSocketHandle->SetMsg_StartCfm_Remalloc(1);
			}
			UartToSocket_TagAssociation();
			break;
		default :			
			if(GetUartMsg(&GetInforPacket)) {
				m_pSocketHandle->SendMessage(m_GetInforPacket.header.type, GetInforPacket);
				m_pMsgQueue->m_bReadEnd_UartMessage =0;
			}
			else {
				m_pMsgQueue->m_bReadEnd_UartMessage =0;

			}
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
				Socket_Registration_Req();
				msg++;
				break;
			case 1:
				if(m_pSocket->m_iSocketReceiveEnd) {
					m_pSocket->m_iSocketReceiveEnd =0;
					m_pSocketHandle->GetServerID(ByteToWord(m_pSocket->m_SocketMsg_vec[MSG_DADDRONE], m_pSocket->m_SocketMsg_vec[MSG_DADDRZERO]));
					
					msg++;
				}
				break;
			case 2:
				Socket_Connect_Req();
				msg++;
				break;
			case 3:	
				if(m_pSocket->m_iSocketReceiveEnd || m_pSocket->m_iBypassSocketToUart) {
					if(m_pSocket->m_SocketMsg_vec[MSGTYPE] == SERVICESTART_REQUEST) {
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
	m_pMsgQueue->m_Uart_ServiceStart_TagAssociation_InitFlag =0;
	printf("Main_ServiceStart_TagAssociation_Init() END \n");

	if(disConn)
		return 0;
	else
		return 1;
}

int Main_ByPass_SocketToUart()
{	
	if(m_pSocket->m_iBypassSocketToUart && m_pSocket->m_iSocketReceiveEnd) {
		timer_delete(firstTimerID);
		Set_WaitTimer(&firstTimerID, 30, 0);

	//	printf(": m_iStatusAlive : %d\n", m_pSocket->m_iStatusAlive);
		if(m_pSocket->m_iStatusAlive) {
			m_pSocket->m_iStatusAlive =0;
			m_pSocket->m_iBypassSocketToUart =0;
			m_pSocket->m_iSocketReceiveEnd =0;
			return 1;
		}
	/*	printf("Main_ByPass_SocketToUart() : ");
		for(int i=0; i<m_pSocket->m_ReceiveData_len; i++) {
			printf("%x ", m_pSocket->m_p8uData[i]);
		}*/
		
		if(m_pSocket->m_p8uData[MSGTYPE] == CONNECT_SOCKET_ALIVE_CHECK) {
			printf("CONNECT_SOCKET_ALIVE_CHECK\n");
			delete[] m_pSocket->m_p8uData;
			m_pSocket->m_p8uData = NULL;
			m_pSocket->m_iBypassSocketToUart =0;
			m_pSocket->m_iSocketReceiveEnd =0;
			return 1;
		}		
		
		m_pMsgHandler->BypassSocketToUart(m_pSocket->m_p8uData,	m_pSocket->m_ReceiveData_len, m_pSocket->m_SocketMsg_vec[MSGTYPE]);

		delete[] m_pSocket->m_p8uData;
		m_pSocket->m_p8uData = NULL;

		m_pSocket->m_iBypassSocketToUart =0;
		m_pSocket->m_iSocketReceiveEnd =0;

	}
	else if(m_pSocket->m_iSocketReceiveQueue) {
		m_pSocket->m_iSocketReceiveQueue =0;
	
		memset(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 0, sizeof(WORD)*4096);
		memset(m_pMsgHandler->m_pu16MsgDataAcknowledge, 0, sizeof(WORD)*4096);
		
		m_pMsgQueue->GetDataDown(m_pSocket->m_nSocketArrayDataDownCnt, m_pSocket->m_TagNumber);
		m_pMsgHandler->SetSocketArray(m_pSocket->m_SocketArrayDataDownMsg, m_pSocket->m_SocketArrayDataIndicateMsg);
	//	printf("Socket Communication End\n");
	}
	else if(m_pMsgQueue->m_bUartCommuniFlag) {
		m_pMsgQueue->m_bUartCommuniFlag =0;
		switch((int)m_pMsgQueue->m_vcemsg.at(MSGTYPE)) 
		{
		case DOWNLOAD_START_ACK:
			if(firstTimerFlag) {
				firstTimerFlag =0;
				timer_delete(DataDownTimerID);
			}

			if(!bReDownloadFlag) {
				if(ByteToWord(m_pMsgQueue->m_vcemsg.at(MSG_SADDRONE), m_pMsgQueue->m_vcemsg.at(MSG_SADDRZERO)) == 
					ByteToWord(m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRONE), m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRZERO)) )
			//	if( (m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[0]| m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[1])
			//			== (m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRZERO)|m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRONE)))
				{
					if(bDataAckFlag) {
						m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]] = PASS;
			//			printf("!bReDownloadFlag Parity Pass %d %d\n", m_pMsgHandler->m_nDataDownCount,
			//										m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]]);
					}
					else {
						m_pMsgHandler->Map_dataParityCheck.insert({m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount], PASS});
			//			printf("!bReDownloadFlag Parity Pass %d %d\n", m_pMsgHandler->m_nDataDownCount,
			//										m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]]);
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
				printf("Before Parity Pass Tag : %d\n", m_pMsgHandler->m_nDataDownCount+1);
				if(ByteToWord(m_pMsgQueue->m_vcemsg.at(MSG_SADDRONE), m_pMsgQueue->m_vcemsg.at(MSG_SADDRZERO)) ==
					ByteToWord(m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRONE), m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRZERO)) )
	//			if( (m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[0]| m_pMsgQueue->m_vcemsg.MsgPacket.Saddr[1]) ==
	//						(m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRZERO)|m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount].at(MSG_DADDRONE))) 
				{

					m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]] = PASS;

				}
				else {
					m_pMsgHandler->Map_dataParityCheck[m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount]] = FAIL;
					printf("Parity Ack Fail\n");

				}
			}

		//	printf("!bReDownloadFlag %d && !bDataAckFlag %d m_nDataDownCount : %d\n", bReDownloadFlag, bDataAckFlag, m_pMsgHandler->m_nDataDownCount);
			if(!bReDownloadFlag && !bDataAckFlag) {
				if (m_pMsgHandler->UartPacket_DataIndicateStart(nBeaconValue) ) {
					Set_WaitTimer(&DataIndecateTimerID, 100, 1);
					SecondTimerFlag = 1;
				}
				if(!m_pMsgHandler->m_iTagDirectDown) 
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
			if(m_pMsgQueue->m_vcemsg.at(MSG_CFM_DATAINDICATE_STATUS) != 0x01) {
				printf("Data FAIL %d\n ", m_pMsgQueue->m_vcemsg.at(MSG_CFM_DATAINDICATE_STATUS));
				break;
			}
			
			if(SecondTimerFlag) {
				SecondTimerFlag =0;
				timer_delete(DataIndecateTimerID);
			}
			 
			if(!bReDownloadFlag && !bDataAckFlag) {
				m_pMsgHandler->m_nDataIndiCount++;
				if( m_pMsgHandler->UartPacket_DataDownStart(nBeaconValue) ) {
					Set_WaitTimer(&DataDownTimerID, 100, 1);
					firstTimerFlag =1;
				}
			}
			else if(bDataAckFlag) {
				m_pMsgHandler->UartPacket_ReDataAcknowledge_DownStart(nBeaconValue);
			}
			else if(bReDownloadFlag){
				m_pMsgHandler->UartPacket_ReDataDownStart(nBeaconValue);
			}
			break;	

		case BSN_START_ACK:
			if(firstTimerFlag) {
				firstTimerFlag =0;
				timer_delete(DataDownTimerID);
			}
			nBeaconValue = (BYTE)m_pMsgQueue->m_vcemsg.at(MSG_BSN_DATA);
			m_pMsgHandler->DataFlag_Initialize(nBeaconValue);
			

			if((!m_pMsgHandler->m_iTagDirectDown) && (m_pMsgHandler->m_nUartArrayDataDownCnt == m_pMsgQueue->m_nMapParity) ) {
				Main_Service_Stop();
				return 0;
			}
			
			if(m_pMsgHandler->m_iTagDirectDown) {
				if(!Main_TagAck_Arrange())
					return 0;
			/*
				if(m_pMsgHandler->m_iTagDirectDownCnt%4 == 0) {
					m_pMsgHandler->iSmallDataDown = 1;
				}
				else if(m_pMsgHandler->m_iTagDirectDownCnt >= 20) {
					m_pMsgHandler->m_iTagDirectDown =0;
					Main_Service_Stop();
					return 0;
				}
				*/
				m_pMsgHandler->m_iTagDirectDownCnt++;
				printf("m_iTagDirectDownCnt : %d\n",m_pMsgHandler->m_iTagDirectDownCnt);
			
				if(m_pMsgHandler->m_iTagDirectDownCnt%3 == 0) {
					m_pMsgHandler->iSmallDataDown = 1;
					
					if( m_pMsgHandler->UartPacket_DataDownStart(nBeaconValue ) ) {
						Set_WaitTimer(&DataDownTimerID, 100, 1);
						firstTimerFlag = 1;
					}
					nBeaconCnt++;
				}
				else if(m_pMsgHandler->m_iTagDirectDownCnt >= 30) {		//sent 8 times 
					m_pMsgHandler->m_iTagDirectDownCnt =0;
					m_pMsgHandler->m_nDataDownCount++;
				}
				nBeaconCnt++;
				printf("DataDown nBeaconCnt : %d\n", nBeaconCnt);
				return 0;
			}
			
			if(!bDataAckFlag && !bReDownloadFlag && ((int)nBeaconValue <= BEACON_MAX) && (nBeaconCnt < BEACON_MAX+1)) {	
				if( m_pMsgHandler->UartPacket_DataDownStart(nBeaconValue ) ) {
					Set_WaitTimer(&DataDownTimerID, 100, 1);
					firstTimerFlag = 1;
				}
				nBeaconCnt++;
				printf("DataDown nBeaconCnt : %d\n", nBeaconCnt);
			}
			else if(bReDownloadFlag && ((int)nBeaconValue <= BEACON_MAX) && (nBeaconCnt < BEACON_MAX+1)) {					
				m_pMsgHandler->UartPacket_ReDataDownStart(nBeaconValue);
				nBeaconCnt++;
				printf("ReDown nBeaconCnt : %d\n", nBeaconCnt);
			}
			else if(bDataAckFlag && ((int)nBeaconValue<= BEACON_MAX) && (nBeaconCnt < BEACON_MAX+1)) {
				m_pMsgHandler->UartPacket_ReDataAcknowledge_DownStart(nBeaconValue);
				nBeaconCnt++;
				printf("DataAck nBeaconCnt : %d\n", nBeaconCnt);
			}
			else if(nBeaconCnt >= BEACON_MAX+1) {
				int DataSendFail_Redown =0;
				int j =0;
				m_pMsgHandler->DataSendFail_RedownCnt =0;
				m_pMsgHandler->m_nDataSendFail_SuccessCnt =0;				

				nTempBeaconCnt++;
				printf("nTempBeaconCnt : %d\n", nTempBeaconCnt);


				if(nTempBeaconCnt >= 3/*BEACON_MAX/2*/) {
					bReDownloadFlag =0;
					printf("bDataAckFlag : %d, m_nUartArrayDataDownCnt : %d, m_nMapParity : %d\n", bDataAckFlag, m_pMsgHandler->m_nUartArrayDataDownCnt, m_pMsgQueue->m_nMapParity);

					if(!bDataAckFlag && (m_pMsgHandler->m_nUartArrayDataDownCnt > m_pMsgQueue->m_nMapParity) ) {
						Main_TagSort_Arrange(&j, &AckFail_Redown);
					}
					else if(bDataAckFlag && (m_pMsgHandler->m_nUartArrayDataDownCnt > m_pMsgQueue->m_nMapParity) ) {						
						Main_TagSort_Arrange2(&j, &AckFail_Redown);
					}
					m_pMsgHandler->DataSendFail_RedownCnt = j;
					printf("\nMain Fail tag count: %d, DataSendFail_Redown : %d, AckFail_Redown : %d\n", j, DataSendFail_Redown, AckFail_Redown);						
				}
				else {
					printf("return 1\n");
					return 1;
				}
				if(AckFail_Redown) {
					AckFail_Redown =0;
					nTemp2BeaconCnt++;
					printf("AckFail_Redown nTemp2BeaconCnt : %d\n", nTemp2BeaconCnt);
				
					if(nTemp2BeaconCnt > 3) {
						Main_Service_Stop();
						return 0;
					}
					nBeaconCnt =1;
					bDataAckFlag =1;
					m_pMsgHandler->m_nDataSendFail_SuccessCnt =0;
					m_pMsgHandler->m_nDataDownCount = 0;
					m_pMsgHandler->m_nDataIndiCount = 0;
					m_pMsgHandler->UartPacket_ReDataAcknowledge_DownStart(nBeaconValue/*m_pMsgQueue->m_vcemsg.MsgPacket.data[0]*/);
				}
				else {
					if(m_pMsgHandler->m_nUartArrayDataDownCnt == m_pMsgQueue->m_nMapParity) {
						Main_Service_Stop();
					}
					else {
						nTempBeaconCnt =0;
						nTemp2BeaconCnt++;
						printf("AckFail_Redown nTemp2BeaconCnt_______ : %d\n", nTemp2BeaconCnt);
						if(nTemp2BeaconCnt > 1) {
							Main_Service_Stop();
						}
					}
				}
			}
				break;
		}
	}
	return 1;
}

void Main_Service_Stop()
{
	printf("Beacon Stop\n");
	th_delay(3);
	m_pMsgHandler->BSN_Stop_Packet();
	m_pSocketHandle->Server_BSN_Stop_Packet();	
	
	nBeaconCnt = 0;
	for(int i=0; i<m_pSocket->m_nSocketArrayDataDownCnt; i++) {
		m_pSocket->m_SocketArrayDataDownMsg[i].clear();
		m_pSocket->m_SocketArrayDataIndicateMsg[i].clear();
	}
	m_pSocket->m_nSocketArrayDataDownCnt = 0;
	m_pSocket->m_nSocketArrayDataIndicateCnt = 0;
	m_pSocket->m_SocketArrayDataDownMsg.clear();
	m_pSocket->m_SocketArrayDataIndicateMsg.clear();	
	m_pSocket->m_SocketArrayDataDownMsg.shrink_to_fit();
	m_pSocket->m_SocketArrayDataIndicateMsg.shrink_to_fit();
	m_pSocket->m_SocketArrayDataDownMsg.reserve(5000);
	m_pSocket->m_SocketArrayDataIndicateMsg.reserve(5000);
	
	 
	m_pMsgQueue->m_nMapParity =0;
	m_pMsgQueue->m_ArrayDataAcknowledge.clear();	
	m_pMsgHandler->m_DataFlag =0;
	m_pMsgHandler->m_DataCnt =0;
	
	memset (m_pSocket->m_TagNumber, 0, sizeof(WORD)*4096);
	memset(m_pMsgQueue->m_Test, 0, sizeof(WORD)*4096);
	memset(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 0, sizeof(WORD)*4096);
	memset(m_pMsgHandler->m_pu16MsgDataAcknowledge, 0, sizeof(WORD)*4096);
	
	m_pMsgHandler->Map_dataParityCheck.erase(m_pMsgHandler->Map_dataParityCheck.begin(), m_pMsgHandler->Map_dataParityCheck.end());
	m_pMsgHandler->Map_u16AcknowParityCheck.erase(m_pMsgHandler->Map_u16AcknowParityCheck.begin(), m_pMsgHandler->Map_u16AcknowParityCheck.end());

	m_pMsgHandler->bClear();
	bReDownloadFlag = 0;
	bDataAckFlag =0;
	nTemp1BeaconCnt =0;
	nTempBeaconCnt =0;
	nTemp2BeaconCnt =0;
}

int Main_SendSocketMsgToUart(int msgtype)
{
	if(!msgtype) {
		printf("msgtype %d return\n", msgtype);
		return 0;
	}

	if(/*m_pSocket->m_iBypassSocketToUart ||*/ m_pSocket->m_iSocketReceiveEnd) {
		m_pSocket->m_iSocketReceiveEnd =0;
		for(int i=0; i<=m_pSocket->m_ReceiveData_len; i++) {
	//		printf("%x ", m_pSocket->m_p8uData[i] );
			if( (m_pSocket->m_p8uData[0] == STX) && (m_pSocket->m_p8uData[i-1] == 0x7e) && (m_pSocket->m_p8uData[i-2] == 0x5a) && (m_pSocket->m_p8uData[i-3] == 0xa5) ) {			
	//			printf("\n");
				m_pMsgHandler->BypassSocketToUart(m_pSocket->m_p8uData, i/*m_pSocket->m_ReceiveData_len*/, msgtype);
				delete[] m_pSocket->m_p8uData;
				m_pSocket->m_p8uData = NULL;
				break;
			}
		}
		printf("\n");
		
	}

	return 1;
}


int Main_TagSort_Arrange(int* iTemp, int* iTemp2)
{
	int Temp =0;
	Temp = *iTemp;
	printf("\nMain_TagSort_Arrange()\n");
	
	/*for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		printf("%d ", m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i]);
	}
	printf("\n");*/
	printf("\n");
	
	printf("m_nMapParity(Total 0x43) : %d\n", m_pMsgQueue->m_nMapParity);
	Main_TagArrayVal_CheckParity(Temp);
	Main_Check_TagArrayPassFail(iTemp, iTemp2);

	memcpy(m_pMsgHandler->m_pu16MsgDataAcknowledge, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 
		sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge)/sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[0]));

	
	/*printf("Final m_ArrayDataAcknowledge\n");
	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		printf("%x ", m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i]);
	}
	printf("\n");

	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		printf("%x ", m_pMsgHandler->m_pu16MsgDataAcknowledge[i]);
	}
	printf("\n");*/
	
	return 0;
}

int Main_TagSort_Arrange2(int* iTemp, int* iTemp2)
{
	int Temp=0;
	int j=0;
	Temp = *iTemp;
	printf("Main_TagSort_Arrange2 %d()\n", Temp);
	/*for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		printf("%d ", m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i]);
	}
	printf("\n");*/
	printf("\n");
	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
	//	printf("[%d] %d ", i, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i]);
		if(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i] == 0) {
			Main_deleteArray(i, 4096, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge);
	//		printf("Main_deleteArray[%d] %d ", i, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i]);

			if(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i] == 0 ) {
		//		printf("i-- \n");
				i--;
			}
		}
		j++;
		if(j == m_pMsgHandler->m_nUartArrayDataDownCnt) {
	//		printf("%d == %d\n", j, m_pMsgHandler->m_nUartArrayDataDownCnt);
			break;
		}
		
	}
	printf("\n");
	printf("m_nMapParity(Total 0x43) : %d\n", m_pMsgQueue->m_nMapParity);

	Main_TagArrayVal_CheckParity(Temp );
	Main_Check_TagArrayPassFail(iTemp, iTemp2);

	memcpy(m_pMsgHandler->m_pu16MsgDataAcknowledge, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 
		sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge)/sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[0]));

/*	printf("Final m_ArrayDataAcknowledge\n");
	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		printf("%x ", m_pMsgHandler->m_pu16MsgDataAcknowledge[i]);
	}
	printf("\n");
	
	for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		printf("%x[%d] ", m_pMsgHandler->m_pu16MsgDataAcknowledge[i], m_pMsgHandler->Map_u16AcknowParityCheck[m_pMsgHandler->m_pu16MsgDataAcknowledge[i]] );
	}
	printf("\n");*/
	
	return 0;
}

int Main_TagArrayVal_CheckParity(int Temp)
{
	VectorSocket<WORD> ArrayUtil;

	printf("\nMain_TagArrayVal_CheckParity() %d\n", sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge)/sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[0]));

	for(WORD i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
		if(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i] != m_pSocket->m_TagNumber[i]) {
			printf("%d : %d [%d, %x]\n",i, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i], 
								m_pSocket->m_TagNumber[i], m_pSocket->m_TagNumber[i] );
			
			Main_InsertArray(i, 0, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge);
		//	printf("insert value[%d] : %d %d\n",i, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i], m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i+1]);
		}
	}
	printf("\n\n");
	/*printf("Complete Data Sort && Arrange !!! : ");
	for(int j=0; j<(int)m_pMsgHandler->m_nUartArrayDataDownCnt; j++) {
		printf("[%d] ",m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[j]);
	}
	printf("\n");*/

	return 1;
}

int Main_Check_TagArrayPassFail(int* iTemp, int* iTemp2)
{
	int icnt =0, cnt =0;
	int j = 0;
	int AckFail_Redown =0;
	while(icnt <=m_pMsgHandler->m_nUartArrayDataDownCnt) {
		if( m_pSocket->m_TagNumber[cnt] != (m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[icnt]) ) {

			m_pMsgHandler->Map_u16AcknowParityCheck.insert({m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[icnt], FAIL});
			j++;
			AckFail_Redown = 1;
		//	printf(" [%d]: %x\n", icnt, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[icnt]);
		}
		else if( m_pSocket->m_TagNumber[cnt] == (m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[icnt]) ) {
			m_pMsgHandler->Map_u16AcknowParityCheck.insert({m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[icnt], PASS});
		
		}
		icnt++;
		cnt++;
	
	/*	if(cnt+1 > m_pMsgHandler->m_nUartArrayDataDownCnt) {
			m_pMsgQueue->Redown =1;
			m_pMsgQueue->m_DataAckCnt = m_pMsgHandler->m_nUartArrayDataDownCnt;
			printf("Redown :%d\n", m_pMsgQueue->Redown);
			break;
		}*/
	}
	*iTemp = j;
	*iTemp2 = AckFail_Redown;

	return 1;
}

int Main_TagAck_Arrange()
{
	printf("Main_TagAck_Arrange()\n");
	WORD Tag =0, Tag2 =0;
//	for(int i=0; i<m_pMsgQueue->setTagAckNumber.size(); i++) {
//		for(int j=0; j<m_pMsgQueue->setTagNumber.size(); j++) {
	if(m_pMsgQueue->setTagAckNumber.size() == 0)
		return 1;
	
	for(auto itr2=m_pMsgQueue->setTagAckNumber.begin(); itr2 != m_pMsgQueue->setTagAckNumber.end(); itr2++) {
		for(auto itr=m_pMsgQueue->setTagNumber.begin(); itr != m_pMsgQueue->setTagNumber.end(); itr++) {
			if(*itr2 == *itr) {
				printf("erase %d\n", *itr);
				Tag = *itr;
				m_pMsgQueue->setTagNumber.erase(Tag);
				mapTagDirectSet[Tag] = DISABLE;

				Tag2 = ByteToWord(m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount][MSG_DADDRONE], 	//Current Send Tag ID
							m_pMsgHandler->m_UartArrayDataDownMsg[m_pMsgHandler->m_nDataDownCount][MSG_DADDRZERO] );

				if(Tag2 == Tag) {
					printf("Tag2:[%d] = Tag:[%d\n", Tag2, Tag);
					m_pMsgHandler->m_iTagDirectDownCnt =0;
					m_pMsgHandler->m_nDataDownCount++;
				}
			}
		}
	}
	
	if(m_pMsgQueue->setTagNumber.empty()) {
		printf("setTagNumber.empty()\n");
		m_pMsgQueue->setTagNumber.clear();		
		m_pMsgQueue->setTagAckNumber.clear();
		mapTagDirectSet.clear();
	
		m_pMsgHandler->iSmallDataDown =0;
		m_pMsgHandler->m_iTagDirectDown =0;
		Main_Service_Stop();
		return 0;
	}
	return 1;
}
int Main_Socket_Init()
{
	int Ret =0;
	PRE_DEFINE::S_PACKET	GetInforPacket;

	m_MainComport->Create_Uart_thread(Main_thread[0]);
	m_pMsgHandler->ServiceIdle();

	while(1) {
		if(m_pMsgQueue->m_bReadEnd_UartMessage) {
			printf("m_pMsgQueue->m_bReadEnd_UartMessage %d\n", m_pMsgQueue->m_bReadEnd_UartMessage);
			if(GetUartMsg(&GetInforPacket)) {
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

int UartToSocket_TagAssociation()
	{
	printf("UartToSocket_TagAssociation\n");
	while(!m_pMsgQueue->m_Queue.empty()) {
		m_pSocketHandle->TagData(m_pMsgQueue->m_Queue);
		m_pMsgQueue->m_Queue.pop();	
		m_pMsgQueue->m_nSendTagCount--;
	//	printf("m_pMsgQueue->m_nSendTagCount : %d\n", m_pMsgQueue->m_nSendTagCount );
	}
	//m_pSocketHandle->SendMessage(TAG_ASSOCIATION, m_GetInforPacket);
	return 1;
}

int UartToSocket_Service_cfm()
{
	PRE_DEFINE::S_PACKET	GetInforPacket;

	printf("UartToSocket_Service_cfm()\n");
	/*if(m_pMsgQueue->m_nSendTagCount > 0) {
		m_pSocketHandle->SetMsg_StartCfm_Remalloc(1);
	}*/
	while(1) {
		if(GetUartMsg(&GetInforPacket)) {
			if(GetInforPacket.header.type == SERVICESTART_CONFIRM) {
		 		m_pSocketHandle->SendMessage(SERVICESTART_CONFIRM, GetInforPacket);
				m_pMsgQueue->m_bReadEnd_UartMessage =0;
				break;
			}
		}
	}

	return 1;
}

int ServiceStart_Cfm()
{
	int Ret =0;
	PRE_DEFINE::S_PACKET	GetInforPacket;
	//m_pMsgQueue->m_Uart_ServiceStart_TagAssociation_InitFlag =1;
	while(1) {
		if(m_pMsgQueue->m_bReadEnd_UartMessage && GetUartMsg(&GetInforPacket)) {			
			if(GetInforPacket.header.type == SERVICESTART_CONFIRM) {				
				m_pSocketHandle->SendMessage(SERVICESTART_CONFIRM, GetInforPacket);	
				m_pSocketHandle->SetBeconCount(&BEACON_MAX);
				m_pMsgHandler->SetBeacon(BEACON_MAX);
				m_pMsgQueue->m_bReadEnd_UartMessage =0;
				Ret =1;
				break;
			}
		}
	}

	return Ret;
}

int Socket_Registration_Req()
{
	PRE_DEFINE::S_PACKET	GetInforPacket;

	m_pSocketHandle->SendMessage(REGISTRATION_REQUEST, GetInforPacket);

	return 1;
}

int Socket_Connect_Req()
{
	PRE_DEFINE::S_PACKET	GetInforPacket;

	m_pSocketHandle->SendMessage(CONNECT_REQUEST, GetInforPacket);
	//printf("main_Socket_Connect_Req(), bWorkingThread : %d\n", m_pSocket->bWorkingThread);

	return 1;
}
int Socket_AliveCheck()
{
	PRE_DEFINE::S_PACKET	GetInforPacket;

	m_pSocketHandle->SendMessage(CONNECT_ALIVE_CHECK, GetInforPacket);
	printf("main_Socket_AliveCheck, bWorkingThread : %d, m_iSocketArive : %d\n", m_pSocket->bWorkingThread, m_pSocketHandle->m_iSocketArive);

	return 1;
}

int GetUartMsg(PRE_DEFINE::S_PACKET* Getpacket)
{
	std::vector<BYTE> vec;
	int size =0;
	int ipos =0;
	int loopEnd =0;

	
	size = m_pMsgQueue->m_vcemsg.size();
	if(size <= 0) {
		return 0;
	}
	
	vec =m_pMsgQueue->m_vcemsg;	
	//m_pMsgQueue->m_vcemsg.clear();


	while(!loopEnd) {
		switch(ipos)
		{
		case 0:
			if(vec[MSG_STX] != STX) {
//				printf("\nSTX error %x\n", vec[MSG_STX]);
				return 0;
			}
			else {
				Getpacket->header.stx = vec[MSG_STX];
	//			printf("STX : [%x] ", Getpacket.header.stx);
				ipos++;
			}
			break;
		case 1:
			Getpacket->header.panID = ByteToWord(vec[MSG_PANIDONE], vec[MSG_PANIDZERO]);
	//		printf("panID [%x], ", Getpacket.header.panID);
			ipos = ipos +2;
			break;
		case 3:
			Getpacket->header.dAddr = ByteToWord(vec[MSG_DADDRONE], vec[MSG_DADDRZERO]);
	//		printf("Daddr[%x] , ",Getpacket.header.dAddr);
			ipos = ipos +2;
			break;
		case 5:
			Getpacket->header.sAddr = ByteToWord(vec[MSG_SADDRONE], vec[MSG_SADDRZERO]);
	//		printf("Saddr[%x], ", Getpacket.header.sAddr);
			ipos = ipos +2;
			break;
		case 7:
			Getpacket->header.type = vec[MSGTYPE];
	//		printf("MsgType[%x], ", Getpacket.header.type);
			ipos++;
			break;
		case 8:
			Getpacket->header.length = ByteToWord(vec[MSG_LENGTHONE], vec[MSG_LENGTHZERO]);
	//		printf("DataLen[%x], ", Getpacket.header.length);
			ipos = ipos +2;
			break;
		case 10:
			Getpacket->pu8Data = new BYTE[Getpacket->header.length];
			for(int i =0; i< Getpacket->header.length; i++) {
				Getpacket->pu8Data[i] = vec[MSG_LENGTHONE+1+i];
	//			printf("pu8Data [%x] ", Getpacket.pu8Data[i]);
			}
			ipos = ipos + (int)Getpacket->header.length;
			break;
		default:
			if(!nlsChecksum) {
				Getpacket->tail.checksum = vec[ipos];
	//			printf("checksum Val :%x ", Getpacket.tail.checksum);
				nlsChecksum =1;
				ipos++;
				break;
			}
			else {
				Getpacket->tail.ext[0] = vec[ipos];
				Getpacket->tail.ext[1] = vec[++ipos];
				Getpacket->tail.ext[2] = vec[++ipos];

				if((Getpacket->tail.ext[0] != 0xA5) || (Getpacket->tail.ext[1] != 0x5A) || (Getpacket->tail.ext[2] != 0x7E)) {
	//				printf("etx error return 0\n");
					return 0;
				}
				else {
	//				printf("etx : %x %x %x\n", Getpacket.tail.ext[0], Getpacket.tail.ext[1], Getpacket.tail.ext[2]);
					loopEnd =1;
					nlsChecksum =0;
					Getpacket->header.DataLength = vec.size();
					break;
				}
			}

		}		//END SWITCH
	}		//END WHILE
	//m_GetInforPacket = Getpacket;

	if(Getpacket->header.type == REGISTRATION_CONFIRM) {
		m_pSocketHandle->GetServerID(Getpacket->header.sAddr);
	//	m_pSocket->m_pSocMsgqueue->GetServerID(Getpacket.header.sAddr);
	}
	if(Getpacket->header.type == GATEWAYID_RESPONSE) {
		m_pSocketHandle->GetPanID(Getpacket->header.panID);
	//	m_pSocket->m_pSocMsgqueue->GetPanID(Getpacket.header.panID);
	}

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

int Set_WaitTimer(timer_t *timerID, int expireMS, int intervalMS)
{

#if 1
	struct sigevent te;
	struct itimerspec its;
	struct sigaction sa;
	int sigNo = SIGRTMIN;

	/* Set up signal handler. */
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = PrintfHello;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sigNo, &sa, NULL) == -1) {
		perror("sigaction");
	}

	/* Set and enable alarm */
	te.sigev_notify = SIGEV_SIGNAL;
	te.sigev_signo = sigNo;
	te.sigev_value.sival_ptr = timerID;
	timer_create(CLOCK_REALTIME, &te, timerID);

	if(!intervalMS) {
		its.it_interval.tv_sec = expireMS;
		its.it_interval.tv_nsec = 0;
		its.it_value.tv_sec = expireMS;
		its.it_value.tv_nsec = 0;//expireMS * 1000;
	}
	else {
		its.it_interval.tv_sec = 0;
		its.it_interval.tv_nsec = 0;
		its.it_value.tv_sec = 0;
		its.it_value.tv_nsec = expireMS * 1000000;
	}
	if (timer_settime(*timerID, 0, &its, NULL) == -1 ) {
		printf("Fail to timer_settimer()\n");
	}
	

#endif
	return 1;
}

void PrintfHello(int sig, siginfo_t* si, void* uc)
{
	timer_t* tidp;

	tidp = (void**)(si->si_value.sival_ptr);

	if(*tidp == firstTimerID) {
		timer_delete(firstTimerID);
		printf("Status Check :%d , %d, %d \n", m_pSocket->m_iSocketReceiveQueue
									, m_pSocket->m_iBypassSocketToUart, m_pSocket->m_iSocketReceiveEnd);
		bSocketAlive = 0;
		printf("Server Connect Timeout : %d\n", bSocketAlive);
		m_pMsgQueue->m_ServerDisconnect =1;
	}
	else if(*tidp == DataDownTimerID) {
		firstTimerFlag =0;
		timer_delete(DataDownTimerID);
	
		printf("Kill Timer DataDownTimerID\n");
		m_pMsgHandler->m_nDataDownCount++;
		m_pMsgHandler->m_DataCnt++;
		m_pMsgHandler->UartPacket_DataDownStart(nBeaconValue);
	}
	else if (*tidp == DataIndecateTimerID){
		SecondTimerFlag =0;		
		timer_delete(DataIndecateTimerID);
	
		printf("Kill Timer DataIndecateTimerID\n");
		m_pMsgHandler->m_nDataDownCount--;
		m_pMsgHandler->UartPacket_DataIndicateStart(nBeaconValue);

	}
}

int TagAssociation_Init()
{
	while (1) {
		if(Main_ServiceStart_TagAssociation_Init() == 0) {
			th_delay(1000);
			th_delay(1000);
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
		th_delay(1000);
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
#if 0
void InsertArray(int idx, BYTE sz, BYTE* ar)
{
	BYTE arr[4096];
	memcpy(arr, ar, 4096);
	int size = (sizeof(arr)/sizeof(*arr));
	memmove(ar+idx+1, ar+idx, size-idx+1);
	ar[idx] = sz;
	printf("Insert %x\n", ar[idx]);
}

void deleteArray(int idx, int size, BYTE* ar)
{
	memmove(ar+idx, ar+idx+1, size-idx);
}

void AppendArray(BYTE sz, int size1, BYTE* ar)
{
	InsertArray(size1, sz, ar);
}

int GetSizeArray (BYTE* ar)
{
	int ret =0;
	BYTE arr[4096];
	memcpy(arr, ar, 4096);
	int size = (sizeof(arr)/sizeof(*arr));
	
	for(int i=0; i< size; i++) {
		if(ar[i] > 0) {
			ret++;
		}
	}
	return ret;
}

void PrintArray(BYTE* ar, int size)
{
	for(int i=0; i< size; i++) {
	//	if(ar[i] > 0)		
			printf("%x ", ar[i]);
	}	
	printf("\n\n");
}
#endif
bool ZeroCompare(int a, int b)
{
	if( (a != 0) && ( b !=0) )
		return a < b;
	else
		return 0;
}


BYTE ar[1024] = {0x31, 0x21, 0x22, 0x32, 0x33, 0x51};

int main(int argc, char *argv[])
{
	installSignal(SIGSEGV);
	pthread_mutex_init(&Main_mutex, NULL);
	pthread_mutex_init(&Main_Uartmutex, NULL);
	pthread_mutex_init(&Main_Tagmutex, NULL);
	setpriority(PRIO_PROCESS, getpid(), -10);

/*	VectorSocket<BYTE> pMm;

	int size, size2;
	
	size = sizeof(ar)/sizeof(*ar);
	printf("size : %d\n", size);
	pMm.PrintArray(ar, pMm.GetSizeArray(ar));

	size2 = pMm.GetSizeArray(ar);
	printf("size2 : %d\n", size2);
	sort(ar, ar+size2, ZeroCompare);
	pMm.PrintArray(ar, size2);

	pMm.InsertArray(2, 0xff, ar);
	pMm.PrintArray(ar, pMm.GetSizeArray(ar));

	pMm.deleteArray(0, size,ar);
	size2 = pMm.GetSizeArray(ar);
	printf("size2 : %d\n", size2);
	pMm.PrintArray(ar, pMm.GetSizeArray(ar));

	pMm.AppendArray(0x0e, pMm.GetSizeArray(ar), ar);
	pMm.PrintArray(ar, pMm.GetSizeArray(ar));

	sort(ar, ar+pMm.GetSizeArray(ar), ZeroCompare);
	pMm.PrintArray(ar, pMm.GetSizeArray(ar));
	*/

	struct rlimit rlim,rlim2;
	int ret =0;
	getrlimit(RLIMIT_STACK, &rlim);
	printf("CurrentStack Size : [%ld] Max Current Stack Size : [%ld]\n", rlim.rlim_cur, rlim.rlim_max);
	rlim.rlim_cur = (1024 * 1024 * 300);
	rlim.rlim_max = (1024 * 1024 * 300);
	ret = setrlimit(RLIMIT_STACK, &rlim);
	printf("CurrentStack Size : %d [%ld] Max Current Stack Size : [%ld]\n", ret, rlim.rlim_cur, rlim.rlim_max);
	
	getrlimit(RLIMIT_DATA, &rlim2);
	printf("Current data Size : [%ld] Max Current Stack Size : [%ld]\n", rlim2.rlim_cur, rlim2.rlim_max);
	th_delay(40);
	th_delay(50);
	
	m_MainComport->uart_init();
	m_MainComport->SetMutex(Main_Uartmutex);
	
	Set_WaitTimer(&firstTimerID, 30, 0);

	Main_Socket_Init();
	th_delay(100);
	th_delay(200);
	TagAssociation_Init();	

 	while(1)
	{
		Main_ByPass_SocketToUart();
		Main_ByPass_UartToSocket();

		if( (m_pSocket->bWorkingThread == 0) || (bSocketAlive == 0) ) {
			printf("delete Timer\n");
			timer_delete(firstTimerID);
			ServerReConn();
			bSocketAlive = 1;			
			Set_WaitTimer(&firstTimerID, 30, 0);
			TagAssociation_Init();
			printf("Main TagData\n");
			while(!m_pMsgQueue->m_Queue.empty()) {
				m_pSocketHandle->TagData(m_pMsgQueue->m_Queue);
				m_pMsgQueue->m_Queue.pop();	
				m_pMsgQueue->m_nSendTagCount--;
				printf("m_pMsgQueue->m_nSendTagCount : %d\n", m_pMsgQueue->m_nSendTagCount );
			}
		}
		usleep(100);
	}

	return 0;
}

void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext) {
	void * array[50];
	void * caller_address;
	char ** messages;
	int size, i;
	sig_ucontext_t * uc;

	uc = (sig_ucontext_t *) ucontext;

	/* Get the address at the time the signal was raised */
	caller_address = (void *) uc->uc_mcontext.arm_pc;  // RIP: x86_64 specific     arm_pc: ARM

	fprintf(stderr, "\n");

	if (sig_num == SIGSEGV)
	printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr,
	       (void *) caller_address);
	else
	printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

	size = backtrace(array, 50);
	/* overwrite sigaction with caller's address */
	array[1] = caller_address;
	messages = backtrace_symbols(array, size);

	/* skip first stack frame (points here) */
	for (i = 1; i < size && messages != NULL; ++i) {
	printf("[bt]: (%d) %s\n", i, messages[i]);
	}
	free(messages);

	exit(EXIT_FAILURE);
}

void installSignal(int __sig) {
	struct sigaction sigact;
	sigact.sa_sigaction = crit_err_hdlr;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;
	if (sigaction(__sig, &sigact, (struct sigaction *) NULL) != 0) {
		fprintf(stderr, "error setting signal handler for %d (%s)\n", __sig, strsignal(__sig));
		exit(EXIT_FAILURE);
	}
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

