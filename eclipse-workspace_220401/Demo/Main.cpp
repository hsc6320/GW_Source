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
#include <signal.h>


using namespace std;

Socket*	m_pSocket;
MsgHandler* m_pMsgHandler;
UartComThread* m_MainComport;
MsgQueue* m_pMsgQueue;
SocketHandler*	m_pSocketHandle;

typedef WORD TAG;


typedef struct TagThread_Cond {
	
	pthread_cond_t cond;
	pthread_cond_t condWaiting;
	pthread_mutex_t Main_Tagmutex;

	int condition =0;
	int condwait =0;
	int condReadyFlag =0;

}Cond_elt_t;

typedef struct TagThread {
	pthread_t TagDownload_Thread[4096];
	std::map<TAG, pthread_t> TagDownload_Thread_ThreadVal;
	std::map<int, TAG> mapThread_TagNumber;
	std::map<pthread_t, int> mapThread_TagDirectCount;
	std::map<TAG, int> mapTag_DataDownCnt; 
	std::map<TAG,int> mapTagNumber_WriteFlag;
	
	unsigned int nThreadCount =0, ThreadBusy = 0;
	
}Tag_Thread_t;

typedef struct TagDownloadThread {
	int iReturnAck =0;
	int tempCnt =0;
}TagDownloadThread_t;
	
typedef struct info {
	std::vector<std::vector<BYTE>> Tagnum;
}info;


Cond_elt_t ep;
Tag_Thread_t Tag_Thread;
TagDownloadThread_t TagDownThread;


pthread_t Main_thread[2];
pthread_mutex_t Main_mutex;
pthread_mutex_t Main_Uartmutex;
timer_t firstTimerID;
timer_t DataDownTimerID;
timer_t DataIndecateTimerID;
int firstTimerFlag =0, SecondTimerFlag=0;

PRE_DEFINE::S_PACKET	m_GetInforPacket;


template <typename T>
std::vector<uint8_t> vec;

std::vector<info> v;

std::map<WORD, int> mapTagDirectSet;

WORD CurrentDataDownTag =0;
std::set<WORD>::iterator iterSet;
std::set<WORD> nDirectDownTagNumber;

int nDataDownTagCount =0;
int nlsChecksum =0;
int socket_fd =0;
int nBeaconCnt =0, nTempBeaconCnt =0, nTemp2BeaconCnt = 0, nTemp1BeaconCnt=0;
BYTE nBeaconValue =0;
int bReDownloadFlag =0, bDataAckFlag =0, AckFail_Redown =0;
int BEACON_MAX =0;
int bSocketAlive =1;
WORD setTagNumber[4096];
	

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
int Main_ByPass_Command();
int Main_ByPass_Command2();
int Main_ByPass_Command3();
int Main_DeleteThread(WORD Tg);
int Main_Socket_Alive();

int Set_WaitTimer(timer_t *timerID, int expireMS, int intervalMS);
void PrintfHello(int sig, siginfo_t* si, void* uc);
void th_delay(int millsec);
int ServerReConn();
int TagAssociation_Init();
void Main_Service_Stop();
int Main_TagSort_Arrange(int* iTemp, int* iTemp2);
int Main_TagSort_Arrange2(int* iTemp, int* iTemp2);

void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext);
void installSignal(int __sig);
//bool Main_Cmp(const info &a);
bool Main_Cmp(const std::vector<BYTE> &a, const std::vector<BYTE> &b);
bool Main_ArrayCmp(const    WORD &a, const WORD &b);

int Main_TagArrayVal_CheckParity(int Temp);
int Main_Check_TagArrayPassFail(int* iTemp, int* iTemp2);
void Main_InsertArray(int idx, WORD sz, WORD* ar);
void Main_Init_CondThread();

typedef struct _sig_ucontext {
  unsigned long uc_flags;
  struct ucontext *uc_link;
  stack_t uc_stack;
  struct sigcontext uc_mcontext;
  sigset_t uc_sigmask;
} sig_ucontext_t;



void Main_deleteArray(int idx, int size, WORD* ar)
{
	memmove(ar+idx, ar+idx+1, size-idx);
}


int Main_GetSizeArray (WORD* ar)
{
	int ret =0;
	BYTE arr[BUF_MAX];
	memcpy(arr, ar, BUF_MAX);
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
	WORD arr[BUF_MAX];
	memcpy(arr, ar, BUF_MAX);
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
	WORD arr[BUF_MAX];
	memcpy(arr, ar, BUF_MAX);
	for(int i=0; i<= size; i++) {
	//	if(ar[i] > 0)		
			printf("%x ", arr[i]);
	}	
	printf("\n\n");
}

int Main_DeleteThread(WORD Tg)
{
	WORD Tag =Tg;
	std::set<WORD>::iterator iterTag;
	std::set<WORD>::iterator iter;

				
	iter = m_pMsgQueue->setTagNumber.find(Tag);
	if(iter != m_pMsgQueue->setTagNumber.end()) {
		if(Tag_Thread.TagDownload_Thread_ThreadVal.find(Tag) != Tag_Thread.TagDownload_Thread_ThreadVal.end()) {			
			printf("TAG_DIRECT_CHANGE_INDICATION DISABLE Tag : %d\n", Tag); 
			m_pMsgQueue->setTagNumber.erase(Tag);
	
			if(Tag_Thread.TagDownload_Thread_ThreadVal[Tag] == 0) {
				printf("------------------------------Main_DeleteThread() Tag[%d] return 0------------------------\n", Tag);
				return 0;
			}
			mapTagDirectSet[Tag] = DISABLE;
			pthread_detach(Tag_Thread.TagDownload_Thread_ThreadVal[Tag]);
			printf(" pthread_detach, Tag	: %d, %lu \n", Tag, Tag_Thread.TagDownload_Thread_ThreadVal[Tag]);

			Tag_Thread.nThreadCount--;
			int temp;
			for(auto it=Tag_Thread.mapThread_TagNumber.begin(); it != Tag_Thread.mapThread_TagNumber.end(); it++) {
				if(it->second == Tag) {
				//	printf(" 1. KEY : %d  Value : %d\n", (int)it->first, (int)it->second);
					temp = it->first;
				}
			}
			Tag_Thread.mapThread_TagNumber.erase(temp); 					
			
			m_pMsgHandler->m_UartArrayThreadDataDownMsg.erase(m_pMsgHandler->m_UartArrayThreadDataDownMsg.begin() + Tag_Thread.mapTag_DataDownCnt[Tag]);
			m_pMsgHandler->m_UartArrayThreadDataIndecateMsg.erase(m_pMsgHandler->m_UartArrayThreadDataIndecateMsg.begin() + Tag_Thread.mapTag_DataDownCnt[Tag]);
			Tag_Thread.mapTag_DataDownCnt.erase(Tag);

			iterTag = m_pMsgQueue->setTagNumber.find(Tag);
			if(iterTag != m_pMsgQueue->setTagNumber.end()) {
				m_pMsgQueue->setTagNumber.erase(Tag);
			}
			
			for(int i=0; i<m_pMsgHandler->m_nThreadUartArrayDataDownCnt; i++) {
				if(setTagNumber[i] == Tag) {
					Main_deleteArray(i, BUF_MAX, setTagNumber);
					break;
				}
			}
			m_pMsgHandler->m_nThreadUartArrayDataDownCnt--;
			m_pMsgHandler->m_nThreadUartArrayDataIndecateCnt--; 			
			

			WORD tempWORD[4096] = {0, };
			int nWORDcnt =0, nTotalDataCount =0;
			printf("setTagNumber size : %d, m_nThreadUartArrayDataDownCnt size :%d \n", sizeof(setTagNumber)/sizeof(setTagNumber[0]), m_pMsgHandler->m_nThreadUartArrayDataDownCnt);
			for (auto it = m_pMsgQueue->setTagNumber.begin(); it != m_pMsgQueue->setTagNumber.end(); it++) {
				tempWORD[nWORDcnt] = *it;
				for(int j=0; j < sizeof(setTagNumber)/sizeof(setTagNumber[0]); j++) {
					if(setTagNumber[j] == tempWORD[nWORDcnt] ) {
						printf("tempWORD[%d] : %d\n", nWORDcnt, tempWORD[nWORDcnt]);
						nWORDcnt++;
						break;
					}
				}							
				if(nWORDcnt == m_pMsgQueue->setTagNumber.size())
					break;
			}
			printf("\n");
		
			for(int i=0; i<m_pMsgHandler->m_nThreadUartArrayDataDownCnt; i++) {
				Tag_Thread.mapTag_DataDownCnt[tempWORD[i]] = i;
				printf("Tag : %d DataDownCnt : %d\n", tempWORD[i], Tag_Thread.mapTag_DataDownCnt[tempWORD[i]]);
			}						

			if(Tag_Thread.mapThread_TagNumber.size() == 0) {
				Tag_Thread.ThreadBusy =0;
			}
			nTotalDataCount = m_pMsgHandler->m_nUartArrayDataDownCnt + m_pMsgHandler->m_nThreadUartArrayDataDownCnt;
			if(Tag_Thread.ThreadBusy == 0) {
				printf("ThreadBusy :%d, m_nUartArrayDataDownCnt : %d\n", Tag_Thread.ThreadBusy, m_pMsgHandler->m_nThreadUartArrayDataDownCnt );
				mapTagDirectSet.clear();

				if(m_pMsgQueue->setTagNumber.empty()) {
					m_pMsgQueue->setTagNumber.clear();
				}
			}
			if(( Tag_Thread.ThreadBusy == 0) &&(m_pMsgHandler->m_nUartArrayDataDownCnt == 0) ) {
				Main_Service_Stop();							
			}
		}
		else {
			m_pMsgQueue->setTagNumber.erase(Tag);
			
			mapTagDirectSet[Tag] = DISABLE;
		}
		printf("Tag Erase : [%d] \n", Tag);
	}
	else 
		return 0;

	return 1;
}
void* Main_TagDownload_Thread(void *arg) 
{
	pthread_t iTagthreadNum = pthread_self();
	int mapVal =0;
	int id = (int)arg;
	WORD Tagkey =0;
	std::set<WORD>::iterator iter;
	printf(" Main_TagDownload_Thread() Tag thread Number : %d , iTagthreadID: %lu \n",  id, (int)iTagthreadNum);

	if(Tag_Thread.mapThread_TagDirectCount.find(iTagthreadNum) ==Tag_Thread.mapThread_TagDirectCount.end() ) {
		Tag_Thread.mapThread_TagDirectCount[iTagthreadNum] = 3;
	}

	for(auto it=Tag_Thread.mapThread_TagNumber.begin(); it != Tag_Thread.mapThread_TagNumber.end(); it++) {
		if(it->first == id) {
	//		printf("Thread Tag KEY : %d  Value : %d\n", (int)it->first, (int)it->second);
			Tagkey = it->second;
		}
	}	

	while(1){
		if(!ep.condReadyFlag) {
			ep.condwait++;
		//	printf("ep.condwait++ : %d\n",ep.condwait++);
			do {
				pthread_cond_signal(&ep.condWaiting);				
				pthread_cond_wait(&ep.cond, &ep.Main_Tagmutex);
			} while(ep.condReadyFlag == 0);
			ep.condwait--;
		}
		iter = m_pMsgQueue->setTagNumber.find(Tagkey);
		if(mapTagDirectSet[Tagkey] != ENABLE) {	
			printf("mapTagDirectSet DISABLE [%d]\n", Tagkey);
			pthread_mutex_unlock(&ep.Main_Tagmutex);		
			break;
		}
		else if(iter == m_pMsgQueue->setTagNumber.end()) {
			printf("------------------------------------------iter[%d] == m_pMsgQueue->setTagNumber.end()-----------------------------\n", *iter);
			return 0;
		}
		
		for(auto it=mapTagDirectSet.begin(); it != mapTagDirectSet.end(); it++) {
			if(Tag_Thread.TagDownload_Thread_ThreadVal[it->first] != iTagthreadNum ) {
				if(it->second == DISABLE) {					
					if(Main_DeleteThread(it->first)) {
						printf("-----------------------Remote Delete Thread\n---------------------------------------------------");
					}
					break;
				}
			}
		}
		
		if(m_pMsgQueue->m_bUartCommuniFlag) {
			m_pMsgQueue->m_bUartCommuniFlag =0;			
			switch((int)m_pMsgQueue->m_vcemsg.at(MSGTYPE)) 
			{
			case DOWNLOAD_START_ACK:
		/*		if(firstTimerFlag) {
					firstTimerFlag =0;
					timer_delete(DataDownTimerID);
				}
				
				CurrentDataDownTag = ByteToWord(m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRONE]
						, m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRZERO]);
		*/
				if(Tag_Thread.TagDownload_Thread_ThreadVal[Tagkey] != iTagthreadNum ) {
					m_pMsgQueue->m_bUartCommuniFlag =1;		
					break;
				}
				printf("Tagkey : %d\n", Tagkey);
				if(ByteToWord(m_pMsgQueue->m_vcemsg.at(MSG_SADDRONE), m_pMsgQueue->m_vcemsg.at(MSG_SADDRZERO)) !=
					ByteToWord(m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount].at(MSG_DADDRONE), 
								m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount].at(MSG_DADDRZERO)) )
				{
					printf("Parity Fail\n");						
				}				

				if (m_pMsgHandler->UartPacket_ThreadDataIndicateStart() ) {
				//	Set_WaitTimer(&DataIndecateTimerID, 100, 1);
				//	SecondTimerFlag = 1;
				}
				break; 
	
			case DATAINDICATION_ACK:
		/*		if(SecondTimerFlag) {
					SecondTimerFlag =0;
					timer_delete(DataIndecateTimerID);
				}*/
				printf("[%lu]DATAINDICATION_ACK, TagKey : %d\n", iTagthreadNum, Tagkey);
				
				if (Tag_Thread.nThreadCount <= m_pMsgHandler->m_nThreadDataDownCount) {
					m_pMsgHandler->m_nThreadDataDownCount =0;
					CurrentDataDownTag = ByteToWord(m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRONE]
											, m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRZERO]);
				}
				/*else {
					for(int i=0; i<(int)Tag_Thread.nThreadCount; i++) {
						if(Tag_Thread.mapTagNumber_WriteFlag[setTagNumber[i]] == 0) {
							m_pMsgHandler->m_nThreadDataDownCount = Tag_Thread.mapTag_DataDownCnt[setTagNumber[i]];
						}
					}
					CurrentDataDownTag = ByteToWord(m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRONE]
											, m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRZERO]);					
				}*/
				printf("CurrentDataDownTag :%d\n", CurrentDataDownTag);
				
				if(Tag_Thread.TagDownload_Thread_ThreadVal[CurrentDataDownTag] != iTagthreadNum ) {
					m_pMsgQueue->m_bUartCommuniFlag =1;		
					break;
				}
				else if(Tag_Thread.mapTagNumber_WriteFlag[CurrentDataDownTag] == 1) {
					break;
				}

				if(m_pMsgQueue->m_vcemsg.at(MSG_CFM_DATAINDICATE_STATUS) != 0x01) {
					printf("Data FAIL %d\n ", m_pMsgQueue->m_vcemsg.at(MSG_CFM_DATAINDICATE_STATUS));
					break;
				}
				
				if( m_pMsgHandler->UartPacket_ThreadDataDownStart() ) {
				//	Set_WaitTimer(&DataDownTimerID, 100, 1);
				//	firstTimerFlag =1;
					Tag_Thread.mapTagNumber_WriteFlag[CurrentDataDownTag] =1;					
				}
				break;
			case BSN_START_ACK:	
				int TagSetReviewCnt =0, temp =0;				
				printf("Tag thread Number : %d , iTagthreadID: %lu \n", id, (int)iTagthreadNum);
			
				if(!TagDownThread.iReturnAck) {
					for(int i=0; i<(int)Tag_Thread.nThreadCount; i++) {
						if(Tag_Thread.mapTagNumber_WriteFlag[setTagNumber[i]] == 1) {
							TagSetReviewCnt++;
						}
					}
					if(TagSetReviewCnt == (int)Tag_Thread.nThreadCount) {
						for(int i=0; i<(int)Tag_Thread.nThreadCount; i++) {
							Tag_Thread.mapTagNumber_WriteFlag[setTagNumber[i]] = 0;
						}
						m_pMsgHandler->m_nThreadDataDownCount =0;
					}
					else if(TagSetReviewCnt< (int)Tag_Thread.nThreadCount) {
						temp =1;
					}
				}
				else if(TagDownThread.tempCnt > 0) {
					if(Tag_Thread.TagDownload_Thread_ThreadVal[setTagNumber[TagDownThread.tempCnt]] != iTagthreadNum ) {
						m_pMsgQueue->m_bUartCommuniFlag =1;
						TagDownThread.iReturnAck =1;
				//		printf("iRetrunAck : %d\n", TagDownThread.iReturnAck);
						break;
					}
				}

				if(temp) {
					for(int i=0; i<(int)Tag_Thread.nThreadCount; i++) {
						if(Tag_Thread.mapTagNumber_WriteFlag[setTagNumber[i]] == 0) {
							TagDownThread.tempCnt = i;
					//		printf("m_TagNumber :%d, tempCnt[%d] = %d\n", setTagNumber[i], TagDownThread.tempCnt);
							break;
						}
					}

					if(Tag_Thread.TagDownload_Thread_ThreadVal[setTagNumber[TagDownThread.tempCnt]] != iTagthreadNum ) {
						m_pMsgQueue->m_bUartCommuniFlag =1;
						TagDownThread.iReturnAck =1;
				//		printf("iReturnAck : %d\n", TagDownThread.iReturnAck);
						break;
					}
				}

				mapVal = Tag_Thread.mapThread_TagDirectCount[iTagthreadNum];
				mapVal++;
				Tag_Thread.mapThread_TagDirectCount[iTagthreadNum] = mapVal;
				printf("[%d, %lu] : %d,  %d \n", ep.condition,iTagthreadNum, Tag_Thread.mapThread_TagDirectCount[iTagthreadNum] , Tagkey);
				
				if(Tag_Thread.mapThread_TagDirectCount[iTagthreadNum]%4 == 0) {
					m_pMsgHandler->m_nThreadDataDownCount = Tag_Thread.mapTag_DataDownCnt[Tagkey];
					printf("Tag :%d %d, Tag_Thread.mapTag_DataDownCnt :%d\n ", Tagkey, CurrentDataDownTag, Tag_Thread.mapTag_DataDownCnt[Tagkey]);

					if(m_pMsgHandler->m_UartArrayThreadDataDownMsg.size() < m_pMsgHandler->m_nThreadDataDownCount ) {
						printf("m_UartArrayThreadDataDownMsg.size()  : %d m_nThreadDataDownCount  :%d\n", m_pMsgHandler->m_UartArrayThreadDataDownMsg.size(), m_pMsgHandler->m_nThreadDataDownCount );
					}

					CurrentDataDownTag = ByteToWord(m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRONE]
						, m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRZERO]);
				
					if( m_pMsgHandler->UartPacket_ThreadDataDownStart( ) ) {
				//		Set_WaitTimer(&DataDownTimerID, 100, 1);
				//		firstTimerFlag = 1;
						Tag_Thread.mapTagNumber_WriteFlag[Tagkey] =1;
						TagDownThread.iReturnAck =0;
						TagDownThread.tempCnt =0;
					}
				}
				else if(Tag_Thread.mapThread_TagDirectCount[iTagthreadNum] >= 23) {		//sent 5 times 					
					
					CurrentDataDownTag = ByteToWord(m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRONE]
						, m_pMsgHandler->m_UartArrayThreadDataDownMsg[m_pMsgHandler->m_nThreadDataDownCount][MSG_DADDRZERO]);
					printf("[%d, %lu] : %d,  %d : %d \n", ep.condition,iTagthreadNum, Tag_Thread.mapThread_TagDirectCount[iTagthreadNum] , Tagkey, CurrentDataDownTag);

					if (mapTagDirectSet[CurrentDataDownTag] == DISABLE) break;
					else mapTagDirectSet[CurrentDataDownTag] = DISABLE;
					
					WORD tempWord =0;
					m_pMsgHandler->m_nThreadDataDownCount++;
					if(m_pMsgHandler->m_nThreadDataDownCount >= Tag_Thread.nThreadCount) {
						printf("m_pMsgHandler->m_nThreadDataDownCount(%d) >= Tag_Thread.nThreadCount(%d)\n", m_pMsgHandler->m_nThreadDataDownCount , Tag_Thread.nThreadCount);						

						for(int i=0; i<(int)Tag_Thread.nThreadCount; i++) {
							if(Tag_Thread.mapTagNumber_WriteFlag[setTagNumber[i]] == 0) {
								tempWord = setTagNumber[i];
								m_pMsgHandler->m_nThreadDataDownCount = Tag_Thread.mapTag_DataDownCnt[setTagNumber[i]];
								printf("setTagNumber[%d] :%d, m_nThreadDataDownCount : %d\n", i, setTagNumber[i], m_pMsgHandler->m_nThreadDataDownCount);
								break;								
							}
							else if(i == (int)Tag_Thread.nThreadCount-1) {
								pthread_mutex_unlock(&ep.Main_Tagmutex);
								Main_Service_Stop();			
								return 0;
							}
						}
					}
					if(tempWord == CurrentDataDownTag) {
						if( (m_pMsgHandler->m_nThreadUartArrayDataDownCnt == 1) && (setTagNumber[0] == CurrentDataDownTag) ) {
							printf("m_nThreadUartArrayDataDownCnt : %d tempWord:%d == CurrentDataDownTag:%d\n", m_pMsgHandler->m_nThreadUartArrayDataDownCnt, tempWord, CurrentDataDownTag);							
							pthread_mutex_unlock(&ep.Main_Tagmutex);
							Main_Service_Stop();			
							return 0;
						}
						pthread_mutex_unlock(&ep.Main_Tagmutex);
						printf("-----------------------------------------------------------tempWord:%d == CurrentDataDownTag:%d--------------------------------------------\n", tempWord, CurrentDataDownTag);
						return 0;
					}
					if( m_pMsgHandler->UartPacket_ThreadDataDownStart() ) {
						Tag_Thread.mapTagNumber_WriteFlag[CurrentDataDownTag] =1;
						TagDownThread.iReturnAck =0;
						TagDownThread.tempCnt =0;
					}
					
				}
				break;
			}
		}
		pthread_mutex_unlock(&ep.Main_Tagmutex);

		usleep(50);
	}
	return 0;
}

int Main_Socket_Alive()
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
	/*	printf("Main_ByPass_Command() : ");
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

	return 0;
}

int Main_ByPass_Command2()
{
	int msg = 0;
	PRE_DEFINE::S_PACKET GetInforPacket;
	WORD Tag =0;
	std::set<WORD>::iterator iterTag;
	std::set<WORD>::iterator iter;

	if(m_pMsgQueue->m_bReadEnd_UartMessage) {
	//	printf("Main_ByPass_Command2\n");
		m_pMsgQueue->m_bReadEnd_UartMessage =0;
		msg = (int)m_pMsgQueue->m_u8SendData[MSGTYPE];

		switch(msg)
		{
		case DATA_ACKNOWLEDGEMENT:
			Tag = ByteToWord(m_pMsgQueue->m_u8SendData[MSG_SADDRONE], m_pMsgQueue->m_u8SendData[MSG_SADDRZERO] );	
			printf("Tag MSG_ACKNOWLEDGE : [%d] \n", Tag);		
			Main_DeleteThread(Tag);
			break;
		case COORDINATOR_RESET_CONFIRM:
			nBeaconCnt = 0;
			Main_Init_CondThread();
			
			m_pMsgQueue->m_nSendTagCount =0;
			m_pSocket->m_SocketArrayDataDownMsg.clear();
			m_pSocket->m_SocketArrayDataIndicateMsg.clear();
			m_pMsgQueue->m_nMapParity =0;
			m_pMsgQueue->m_nDirectMapParity =0;
			m_pMsgHandler->bClear();
			m_pSocketHandle->SetMsg_StartCfm_Remalloc(0);
			m_pSocketHandle->m_nTagDataCount =0;
			m_pMsgHandler->BSN_Stop_Packet();

			m_pMsgQueue->m_ArrayDataAcknowledge.clear();
			memset (m_pSocket->m_TagNumber, 0, sizeof(WORD)*BUF_MAX);
			memset(m_pMsgQueue->m_Test, 0, sizeof(WORD)*BUF_MAX);
			memset(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 0, sizeof(WORD)*BUF_MAX);
			memset(m_pMsgHandler->m_pu16MsgDataAcknowledge, 0, sizeof(WORD)*BUF_MAX);
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
				printf("TAG_DIRECT_CHANGE_INDICATION ENABLE\n");
				Tag = ByteToWord(m_pMsgQueue->m_u8SendData[MSG_SADDRONE], m_pMsgQueue->m_u8SendData[MSG_SADDRZERO] );
				iter = m_pMsgQueue->setTagNumber.find(Tag);
				if(iter != m_pMsgQueue->setTagNumber.end()) {
					printf("Tag overlap\n");
				}
				else {
					printf("Tag Insert : [%d] \n", Tag);
					m_pMsgQueue->setTagNumber.insert(Tag);
					for(auto itr : m_pMsgQueue->setTagNumber) {
						if(itr == Tag) {
							mapTagDirectSet[Tag] = ENABLE;
							printf("mapTagDirectSet.insert({%d, ENABLE})\n", Tag);
						}
					}
				}
			}
			else {
				Tag = ByteToWord(m_pMsgQueue->m_u8SendData[MSG_SADDRONE], m_pMsgQueue->m_u8SendData[MSG_SADDRZERO] );
				Main_DeleteThread(Tag);								
			}
			break;
		case TAG_ASSOCIATION:
			if(m_pMsgQueue->m_nSendTagCount > 0) {
				m_pSocketHandle->SetMsg_StartCfm_Remalloc(1);
			}
			Tag = ByteToWord(m_pMsgQueue->m_u8SendData[MSG_SADDRONE], m_pMsgQueue->m_u8SendData[MSG_SADDRZERO] );
			iterTag = nDirectDownTagNumber.find(Tag);
			if(iterTag != nDirectDownTagNumber.end()) {
				printf("Tag Exitst %d\n", *iterTag);
			}
			else {
				nDirectDownTagNumber.insert(Tag);
				mapTagDirectSet.insert({Tag, DISABLE});
				printf("Tag Map Init\n");
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
				if(m_pSocket->m_iSocketReceiveEnd && m_pSocket->m_iBypassSocketToUart) {
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
				Main_ByPass_Command();
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


int Main_ByPass_Command3()
{	
	WORD TagNumber[4096];
	int nTotalDataCount =0;
	std::map<int, int> mapSocketTag;
	std::vector<std::vector<BYTE>> m_SocketArraySortDataDownMsg, m_SocketArraySortDataIndicateMsg;

	int nChoiceNumber =0;
	memset(TagNumber, 0, sizeof(WORD)*4096);	
	
	if(m_pSocket->m_iSocketReceiveQueue) {
		m_pSocket->m_iSocketReceiveQueue =0;
		Main_Init_CondThread();
		memset(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 0, sizeof(WORD)*BUF_MAX);
		memset(m_pMsgHandler->m_pu16MsgDataAcknowledge, 0, sizeof(WORD)*BUF_MAX);
		memset(setTagNumber, 0, sizeof(WORD)*4096);

		memcpy(TagNumber, m_pSocket->m_TagNumber, sizeof(m_pSocket->m_TagNumber)/sizeof(m_pSocket->m_TagNumber[0]));
		
		m_pMsgQueue->GetDataDown(m_pSocket->m_nSocketArrayDataDownCnt, TagNumber);
			
		std::set<WORD>::iterator iter;
		WORD TempTag =0;
		int nSocketArrayMsgSize = (int)m_pSocket->m_SocketArrayDataDownMsg.size();
		if(m_pMsgQueue->setTagNumber.size() >0) {
			for(int i=0; i<nSocketArrayMsgSize; i++) {
				TempTag = ByteToWord(m_pSocket->m_SocketArrayDataDownMsg[i][MSG_DADDRONE]
							, m_pSocket->m_SocketArrayDataDownMsg[i][MSG_DADDRZERO]);
				iter = m_pMsgQueue->setTagNumber.find(TempTag);
				if(iter != m_pMsgQueue->setTagNumber.end()) {
					m_pMsgHandler->m_UartArrayThreadDataDownMsg.push_back(m_pSocket->m_SocketArrayDataDownMsg[i]);
					m_pMsgHandler->m_UartArrayThreadDataIndecateMsg.push_back(m_pSocket->m_SocketArrayDataIndicateMsg[i]);

					Main_deleteArray(i, BUF_MAX, m_pSocket->m_TagNumber);
				}
				else {
					printf("Not Find Tag : %d\n", TempTag);
					/*for(int i=0; i<m_pSocket->m_SocketArrayDataDownMsg.size(); i++) {
						for(int j=0; j<m_pSocket->m_SocketArrayDataDownMsg[i].size(); j++) {
							printf("%x ", m_pSocket->m_SocketArrayDataDownMsg[i][j]);
						}
						printf("\n");
					}
					printf("\n");*/
					
					m_SocketArraySortDataDownMsg.push_back(m_pSocket->m_SocketArrayDataDownMsg[i]);
					m_SocketArraySortDataIndicateMsg.push_back(m_pSocket->m_SocketArrayDataIndicateMsg[i]);

					for(int i=0; i<m_SocketArraySortDataDownMsg.size(); i++) {
						for(int j=0; j<m_SocketArraySortDataDownMsg[i].size(); j++) {
							printf("%x ", m_SocketArraySortDataDownMsg[i][j]);
						}
						printf("\n");
					}
				}
			}

			m_pSocket->m_SocketArrayDataDownMsg.clear();
			m_pSocket->m_SocketArrayDataIndicateMsg.clear();

			for(int i=0; i<m_SocketArraySortDataDownMsg.size(); i++) {
				m_pSocket->m_SocketArrayDataDownMsg.resize(m_SocketArraySortDataDownMsg.size(), std::vector<BYTE>(m_SocketArraySortDataDownMsg[i].size()) );
				m_pSocket->m_SocketArrayDataIndicateMsg.resize(m_SocketArraySortDataIndicateMsg.size(), std::vector<BYTE>(m_SocketArraySortDataIndicateMsg[i].size()) );
			}


			std::copy(m_SocketArraySortDataDownMsg.begin(), m_SocketArraySortDataDownMsg.end() , m_pSocket->m_SocketArrayDataDownMsg.begin());
			std::copy(m_SocketArraySortDataIndicateMsg.begin(), m_SocketArraySortDataIndicateMsg.end() , m_pSocket->m_SocketArrayDataIndicateMsg.begin());
			
			printf("m_SocketArrayDataDownMsg Size : %d\n", m_pSocket->m_SocketArrayDataDownMsg.size());
		
			m_pMsgHandler->m_nThreadUartArrayDataDownCnt = m_pMsgHandler->m_UartArrayThreadDataDownMsg.size();
			m_pMsgHandler->m_nThreadUartArrayDataIndecateCnt = m_pMsgHandler->m_UartArrayThreadDataIndecateMsg.size();
			printf("m_nThreadUartArrayDataDownCnt :%d\n\n", m_pMsgHandler->m_nThreadUartArrayDataDownCnt);
			
		}
		m_pMsgHandler->SetSocketArray(m_pSocket->m_SocketArrayDataDownMsg, m_pSocket->m_SocketArrayDataIndicateMsg);

		nTotalDataCount = m_pMsgHandler->m_nUartArrayDataDownCnt + m_pMsgHandler->m_nThreadUartArrayDataDownCnt;		
		
		printf("nDirectDownTagNumber.size()  : %d m_pMsgHandler->m_nUartArrayDataDownCnt : %d\n", (int)nDirectDownTagNumber.size(), m_pMsgHandler->m_nUartArrayDataDownCnt);
		if((int)nDirectDownTagNumber.size() == (int)m_pMsgHandler->m_nUartArrayDataDownCnt ) {
			printf("Map All Initialise\n");
			for (auto it = mapTagDirectSet.begin(); it != mapTagDirectSet.end(); it++) {
				it->second = DISABLE; 
			}
		}
		int rc =0, status =0, j=0;
		if(m_pMsgQueue->setTagNumber.size() > 0) {
			for(int i=0; i<nTotalDataCount; i++ ) {
				for(auto itrTag :m_pMsgQueue->setTagNumber) {
					if(TagNumber[i] == itrTag) {
						setTagNumber[j] = TagNumber[i];
					//	printf("setTagNumber[%d] : %d\n", j, setTagNumber[j]);
						Tag_Thread.nThreadCount++;
						j++;
					}
				}
			}
		}
		printf("nThreadCount : %d\n", Tag_Thread.nThreadCount);
		
		if(Tag_Thread.nThreadCount > 0) {
			for(int i=0; i<(int)Tag_Thread.nThreadCount; i++ ) {
				
				rc = pthread_create(&Tag_Thread.TagDownload_Thread[i], NULL, Main_TagDownload_Thread, (void *)i );
				wait(&status);
				Tag_Thread.mapThread_TagNumber[i] = setTagNumber[i];
				Tag_Thread.TagDownload_Thread_ThreadVal[setTagNumber[i]] = Tag_Thread.TagDownload_Thread[i];
				printf("Tag :%d , pthread ID : %lu\n", setTagNumber[i], Tag_Thread.TagDownload_Thread_ThreadVal[setTagNumber[i]]);
				
				Tag_Thread.mapTag_DataDownCnt[setTagNumber[i]] = i;
		//		printf("Tag : %d, DataDownCnt : %d\n", setTagNumber[i], Tag_Thread.mapTag_DataDownCnt[setTagNumber[i]]);

				Tag_Thread.mapTagNumber_WriteFlag[setTagNumber[i]] =0;


				if (rc) {
					printf("Error:unable to create thread[%d], %d\n", i, rc);
				}
				ep.condition++;
			}
			
			th_delay(10);
			Tag_Thread.ThreadBusy =1;
			while(ep.condwait < ep.condition) {
				pthread_cond_wait(&ep.condWaiting, &ep.Main_Tagmutex);
			}
			if(ep.condwait != 0) {
				ep.condReadyFlag =1;
				printf("pthread_cond_broadcast(&ep.cond)\n");
				pthread_cond_broadcast(&ep.cond);
			}
			
		}			
	
		nDataDownTagCount = m_pMsgHandler->m_nUartArrayDataDownCnt;
		printf(" nDataDownTagCount : %d , setTagNumber : %d \n", nDataDownTagCount, m_pMsgQueue->setTagNumber.size());

		m_pMsgHandler->BSN_Start_Packet();
		
	}

	return 1;
}

int Main_ByPass_Command()
{	
	WORD TagNumber[4096];
	memset(TagNumber, 0, sizeof(WORD)*4096);
	
	if((!Tag_Thread.ThreadBusy)&& (m_pMsgQueue->m_bUartCommuniFlag) ) {
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
			if(!bReDownloadFlag && !bDataAckFlag) {
				if (m_pMsgHandler->UartPacket_DataIndicateStart(nBeaconValue) ) {
					Set_WaitTimer(&DataIndecateTimerID, 100, 1);
					SecondTimerFlag = 1;
				}
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
			if(!m_pMsgHandler->m_nUartArrayDataDownCnt ||( m_pSocket->m_Main_ServiceStart_TagAssociation_InitFlag && m_pMsgQueue->m_Uart_ServiceStart_TagAssociation_InitFlag) )
				return 1;			
	
			if(firstTimerFlag) {
				firstTimerFlag =0;
				timer_delete(DataDownTimerID);
			}
			nBeaconValue = (BYTE)m_pMsgQueue->m_vcemsg.at(MSG_BSN_DATA);
			m_pMsgHandler->DataFlag_Initialize(nBeaconValue);

			if((int)m_pMsgHandler->m_nUartArrayDataDownCnt <= m_pMsgQueue->m_nMapParity) {
				printf("Main_Service_Stop()\n");
				printf("nDataDownTagCount :%d, m_nMapParity :%d\n", nDataDownTagCount, m_pMsgQueue->m_nMapParity);
				Main_Service_Stop();
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
				int j =0;

				if(nDataDownTagCount < 0 )	nDataDownTagCount =0;
				
				m_pMsgHandler->DataSendFail_RedownCnt =0;
				m_pMsgHandler->m_nDataSendFail_SuccessCnt =0;				

				nTempBeaconCnt++;
				printf("nTempBeaconCnt : %d\n", nTempBeaconCnt);
 

				if(nTempBeaconCnt >= 3/*BEACON_MAX/2*/) {
					bReDownloadFlag =0;
					printf("bDataAckFlag : %d, m_nUartArrayDataDownCnt : %d, m_nMapParity : %d\n", bDataAckFlag, nDataDownTagCount, m_pMsgQueue->m_nMapParity);

					if(!bDataAckFlag && (nDataDownTagCount > m_pMsgQueue->m_nMapParity) ) {
						Main_TagSort_Arrange(&j, &AckFail_Redown);
					}
					else if(bDataAckFlag && (nDataDownTagCount > m_pMsgQueue->m_nMapParity) ) {						
						Main_TagSort_Arrange2(&j, &AckFail_Redown);
					}
					m_pMsgHandler->DataSendFail_RedownCnt = j;
					printf("\nMain Fail tag count: %d,  AckFail_Redown : %d\n", j, AckFail_Redown);						
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
					if( (nDataDownTagCount <= m_pMsgQueue->m_nMapParity) && (m_pMsgQueue->setTagNumber.size()==0) ){
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
	m_pMsgQueue->m_nDirectMapParity =0;
	m_pMsgQueue->m_ArrayDataAcknowledge.clear();
	m_pMsgQueue->setTagAckNumber.clear();
	m_pMsgHandler->m_DataFlag =0;
	m_pMsgHandler->m_DataCnt =0;
	memset (m_pSocket->m_TagNumber, 0, sizeof(WORD)*BUF_MAX);
	memset(m_pMsgQueue->m_Test, 0, sizeof(WORD)*BUF_MAX);
	memset(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge, 0, sizeof(WORD)*BUF_MAX);
	memset(m_pMsgHandler->m_pu16MsgDataAcknowledge, 0, sizeof(WORD)*BUF_MAX);
	
	m_pMsgHandler->Map_dataParityCheck.erase(m_pMsgHandler->Map_dataParityCheck.begin(), m_pMsgHandler->Map_dataParityCheck.end());
	m_pMsgHandler->Map_u16AcknowParityCheck.erase(m_pMsgHandler->Map_u16AcknowParityCheck.begin(), m_pMsgHandler->Map_u16AcknowParityCheck.end());
	
	m_pMsgHandler->bClear();
	memset(setTagNumber, 0, sizeof(WORD)*4096);
	bReDownloadFlag = 0;
	bDataAckFlag =0;
	nTemp1BeaconCnt =0;
	nTempBeaconCnt =0;
	nTemp2BeaconCnt =0;
	Main_Init_CondThread();
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
	for(int i=0; i<nDataDownTagCount; i++) {
	//	printf("[%d] %d ", i, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i]);
		if(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i] == 0) {
			Main_deleteArray(i, BUF_MAX, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge);
	//		printf("Main_deleteArray[%d] %d ", i, m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i]);

			if(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i] == 0 ) {
		//		printf("i-- \n");
				i--;
			}
		}
		j++;
		if(j ==nDataDownTagCount) {
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
	std::set<WORD>::iterator iter;
	//int nDataDownTagCount = m_pMsgHandler->m_nUartArrayDataDownCnt-m_pMsgQueue->setTagNumber.size();

	printf("\nMain_TagArrayVal_CheckParity() %d\n", sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge)/sizeof(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[0]));
	
	for(WORD i=0; i<nDataDownTagCount; i++) {
		if(m_pMsgQueue->m_pu16MsgQueueArrayDataAcknowledge[i] != m_pSocket->m_TagNumber[i]) {
			
		//	if(mapTagDirectSet[m_pSocket->m_TagNumber[i]] == ENABLE)
		//		continue;
			iter = m_pMsgQueue->setTagAckNumber.find(m_pSocket->m_TagNumber[i]);
			if(iter != m_pMsgQueue->setTagAckNumber.end())
				return 1;
			
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
	//int nDataDownTagCount = m_pMsgHandler->m_nUartArrayDataDownCnt-m_pMsgQueue->setTagNumber.size();
	
	while(icnt <=nDataDownTagCount/*m_pMsgHandler->m_nUartArrayDataDownCnt*/) {
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

void Main_Init_CondThread()
{
	pthread_cond_init(&ep.cond, NULL);
	Tag_Thread.ThreadBusy = 0;
	Tag_Thread.nThreadCount =0;
	Tag_Thread.mapThread_TagNumber.clear();
	Tag_Thread.mapThread_TagDirectCount.clear();	
	Tag_Thread.TagDownload_Thread_ThreadVal.clear();
	Tag_Thread.mapTagNumber_WriteFlag.clear();
	Tag_Thread.mapTag_DataDownCnt.clear();
	
	ep.condition = 0;
	ep.condwait =0;
	ep.condReadyFlag =0;
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



bool Main_Cmp(const std::vector<BYTE> &a, const std::vector<BYTE> &b)
{
	int nTagNum =0, nTagNum2 =0;
	int temp =0, temp2 =0;
	std::set<WORD>::iterator iter, iter2;

//	nTagNum = ByteToWord(a[i].at(MSG_DADDRONE) ,a[i].at(MSG_DADDRZERO));
	nTagNum = ByteToWord(a[MSG_DADDRONE] ,a[MSG_DADDRZERO]);
	nTagNum2 = ByteToWord(b[MSG_DADDRONE] ,b[MSG_DADDRZERO]);


	iter = m_pMsgQueue->setTagNumber.find(nTagNum);
	iter2 = m_pMsgQueue->setTagNumber.find(nTagNum2);
	if( (iter != m_pMsgQueue->setTagNumber.end()) || iter2 != m_pMsgQueue->setTagNumber.end() ) {
		if ( (*iter2 == (WORD)nTagNum2) || (*iter == (WORD)nTagNum) ) {
			printf("aaaa %d %d\n", *iter, *iter2);
			for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
				printf("%d ", ByteToWord(a[MSG_DADDRONE] ,a[MSG_DADDRZERO]));
			}
			printf("\n");
			return true;
		}		
	}
	else {
		if(nTagNum < nTagNum2) {
			printf("bbb %d < %d\n", nTagNum, nTagNum2);
			for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
				printf("%d ", ByteToWord(a[MSG_DADDRONE] ,a[MSG_DADDRZERO]));
			}
			printf("\n");
			return true;
		}
		else {			
			printf("%d > %d\n", nTagNum, nTagNum2);
			for(int i=0; i<m_pMsgHandler->m_nUartArrayDataDownCnt; i++) {
				for(int j=0; j<m_pMsgHandler->m_UartArrayDataDownMsg[i].size(); j++) {
					printf("%x ", m_pMsgHandler->m_UartArrayDataDownMsg[i][j]);
				}
				printf("\n");
			}
			printf("\n");
			return false;
		}
	}
	return true;
}


bool Main_ArrayCmp(const    WORD &a, const WORD &b)
{
	int temp =0, temp2 =0;
	std::set<WORD>::iterator iter, iter2;
	//m_pSocket->m_TagNumber;
	
	iter = m_pMsgQueue->setTagNumber.find(a);
	iter2 = m_pMsgQueue->setTagNumber.find(b);
	if( (iter != m_pMsgQueue->setTagNumber.end()) || iter2 != m_pMsgQueue->setTagNumber.end() ) {
		if ( (*iter == a) || (*iter2 == b) ) {
			return true;
		}
		else {
			if((int)a < (int)b) return true;
			else return false;	
		}
		return true;
	}
	else {
		if((int)a< (int)b) return true;
		else return false;
	}
	
	return true;

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
	printf("delay %.2f sec\n", time);
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
	time_t ct;
	struct tm tm;

	tidp = (void**)(si->si_value.sival_ptr);

	if(*tidp == firstTimerID) {
		timer_delete(firstTimerID);
		printf("Status Check :%d , %d, %d \n", m_pSocket->m_iSocketReceiveQueue
									, m_pSocket->m_iBypassSocketToUart, m_pSocket->m_iSocketReceiveEnd);
		bSocketAlive = 0;
		printf("Server Connect Timeout : %d\n", bSocketAlive);

		if( (m_pSocket->bWorkingThread == 0) || (bSocketAlive == 0) ) {
			if(!m_pSocket->bWorkingThread)
				printf("//-------m_pSocket->bWorkingThread  false\n");
			else if(!bSocketAlive) {
				ct = time(NULL);
				tm = *localtime(&ct);
				printf("\n**********%d-%d-%d:%d:%d***** \n", tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				printf("//------bSocketAlive  false\n");
			}
			
			printf("delete Timer \n");
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
	}
	else if(*tidp == DataDownTimerID) {
		firstTimerFlag =0;
		timer_delete(DataDownTimerID);
	
		printf("Kill Timer DataDownTimerID\n");
		if(Tag_Thread.ThreadBusy) {
			for(int i=0; i<(int)Tag_Thread.nThreadCount; i++) {
				if(Tag_Thread.mapTagNumber_WriteFlag[setTagNumber[i]] == 0) {
					m_pMsgHandler->m_nThreadDataDownCount = Tag_Thread.mapTag_DataDownCnt[setTagNumber[i]];
					m_pMsgHandler->UartPacket_ThreadDataDownStart();
					break;
				}
			}			
	
		}
		else
			m_pMsgHandler->UartPacket_DataDownStart(nBeaconValue);

	}
	else if (*tidp == DataIndecateTimerID){
		SecondTimerFlag =0;		
		timer_delete(DataIndecateTimerID);
	
		printf("Kill Timer DataIndecateTimerID\n");
		if(Tag_Thread.ThreadBusy) {				
			m_pMsgHandler->UartPacket_ThreadDataIndicateStart();
		}
		else {
			m_pMsgHandler->m_nDataDownCount--;
			m_pMsgHandler->UartPacket_DataIndicateStart(nBeaconValue);
		}

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
	BYTE arr[BUF_MAX];
	memcpy(arr, ar, BUF_MAX);
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
	BYTE arr[BUF_MAX];
	memcpy(arr, ar, BUF_MAX);
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
	pthread_mutex_init(&ep.Main_Tagmutex, NULL);
	
	/*for(int i=0; i<4096;i++) {
		pthread_cond_init(&ep.cond[i], NULL);
	}*/
	pthread_cond_init(&ep.cond, NULL);
	pthread_cond_init(&ep.condWaiting, NULL);
	
	setpriority(PRIO_PROCESS, getpid(), -10);


	time_t ct;
	struct tm tm;	
#if 0
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
#endif 

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
	
	//pthread_cond_wait(&ep.cond, &ep.Main_Tagmutex);
	//pthread_cond_signal(&ep.cond);
	
	Set_WaitTimer(&firstTimerID, 30, 0);

	Main_Socket_Init();
	th_delay(100);
	th_delay(200);
	printf("busy : %d m_bUartCommuniFlag : %d \n", Tag_Thread.ThreadBusy,  m_pMsgQueue->m_bUartCommuniFlag);
	TagAssociation_Init();

	//pthread_mutex_init(&ep.Main_Tagmutex, NULL);

 	while(1)
	{
		Main_Socket_Alive();
		Main_ByPass_Command2();

		if(!Tag_Thread.ThreadBusy) {
			Main_ByPass_Command3();
			Main_ByPass_Command();
		}

		if( (m_pSocket->bWorkingThread == 0) || (bSocketAlive == 0) ) {
			if(!m_pSocket->bWorkingThread)
				printf("m_pSocket->bWorkingThread  false\n");
			else if(!bSocketAlive) {
				ct = time(NULL);
				tm = *localtime(&ct);
				printf("\n**********%d-%d-%d:%d:%d***** \n", tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
				printf("bSocketAlive  false\n");
			}
			
			printf("delete Timer \n");
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

