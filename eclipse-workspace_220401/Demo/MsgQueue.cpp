/*
 * MsgQueue.cpp
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */

#include "MsgQueue.h"
#include <queue>

using namespace std;

int m_uiPos =0;
int nLengthCnt =0;
int nTagidCnt =0;
int nDataDown =0;
BYTE m_u8SendData[1024];
WORD u16TagNumberData[4096];

std::vector<uint8_t*> vec;

msgform* m_pMsg;
Socket* m_GetSocket;

template <typename T>
int GetSizeArray (T* ar)
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

bool ZeroCompare2(WORD a, WORD b)
{
//	if( (a != 0) && ( b !=0) )
		return a < b;
//	else
//		return 0;
}
bool ZeroCompare3(WORD a, WORD b)
{
	if( (a != 0) && ( b !=0) )
		return a < b;
	else
		return 0;
}


MsgQueue:: MsgQueue(void)
{
	m_bReadEnd_UartMessage = 0;
	m_bUartCommuniFlag = 0;
	m_bUartTagAssociFlag =0;
	m_nMapParity =0;
	m_nSendTagCount =0;
	m_Uart_ServiceStart_TagAssociation_InitFlag =0;
	memset(m_Test, 0, sizeof(WORD)*4096);
	memset(m_pu16MsgQueueArrayDataAcknowledge, 0, sizeof(WORD)*4096);
	memset(u16TagNumberData, 0, sizeof(WORD)*4096);	
	m_pMsg = NULL;
}

MsgQueue::~MsgQueue(void)
{
}

bool MsgQueue::PutByte(uint8_t* b, int len)
{
	BYTE u8Data[1024];
	int size =0, Cnt =0;
	WORD wordPanID =0;
	std::set<WORD>::iterator iterSet;
	memset(m_u8SendData, 0, 1024);
	memset(u8Data, 0, 1024);
	memcpy(u8Data, b, 1024);

	std::set<WORD>::iterator iter;

	if(u8Data[MSG_STX] == STX) {
		if(u8Data[MSGTYPE] == DATA_ACKNOWLEDGEMENT) {
			if(u8Data[MSG_ACKNOWLEDGE_STATUS] == PAYLOAD_STATUS_SUCCESS) {	
				memcpy(m_u8SendData, u8Data, len);
				wordPanID = ByteToWord(u8Data[MSG_SADDRONE], u8Data[MSG_SADDRZERO]);
				
				iterSet = setTagNumber.find(wordPanID);
		//		iter = setTagAckNumber.find(wordPanID);
				if(iterSet != setTagNumber.end()) {
					printf("---- Exist ");
					printf("setTagNumber : %d----\n", *iterSet);  
				}
			/*	else {
					printf("----Not exist ----\n");
					if(iter != setTagAckNumber.end()) {
						printf("----BUT Exist ");
						printf("setAckNumber : %d----\n", *iter);  	
						return 1;
					}
				}*/
				
				if(!setTagNumber.empty() && (iterSet != setTagNumber.end()) ) {
					iter = setTagAckNumber.find(wordPanID);
					if(iter != setTagAckNumber.end()) {
						printf("TagAck overlap : %d, %x \n");
					}
					else {
						setTagAckNumber.insert(wordPanID);

						printf("----DataAck TagID : ");
						printf("%x, %d------\n", wordPanID, wordPanID);
					
						m_nDirectMapParity++;
						printf("0x43 Count : %d\n", m_nDirectMapParity);
					}
					return 1;
				}
				
				Cnt = m_nMapParity;
				for(int i=0; i<=nDataDown; i++ ) {
					if(u16TagNumberData[i] == wordPanID ) {
						break;
					}
					if(i == nDataDown ) {
						printf("0x43 && TagNumber (%x)not exist  return \n", wordPanID);
						return 1;
					}
				}
				for(int i=0; i<= Cnt; i++) {
					if(m_pu16MsgQueueArrayDataAcknowledge[i] == wordPanID) {
						printf("m_pu16MsgQueueArrayDataAcknowledge, Overlap Parity 0x%x \n", m_pu16MsgQueueArrayDataAcknowledge[i]);
						return 1;
					}
				}
				size = 4096;

				m_Test[m_nMapParity] = wordPanID;
				sort(m_Test, m_Test+size);

				int j =0;
				for(int i=0; i<4096; i++) {
					if(m_Test[i] > 0) {
						m_pu16MsgQueueArrayDataAcknowledge[j] = m_Test[i];
//						printf("%d ",m_pu16MsgQueueArrayDataAcknowledge[j]);
						j++;
					}
				}
				printf("\n");

				printf("DataAck TagID : ");
				printf("%x\n",(ByteToWord(u8Data[MSG_SADDRONE], u8Data[MSG_SADDRZERO])));
			
				m_nMapParity++;				
				printf("0x43 Count : %d\n", m_nMapParity);
			/*	
				iter = setTagAckNumber.find(wordPanID);
				if(iter != setTagAckNumber.end()) {
					printf("TagAck overlap\n");
				}
				else {
					setTagNumber.erase(wordPanID);
					setTagAckNumber.insert(wordPanID);
					printf("TagAck setTagAckNumber : %x\n", wordPanID);
				}*/

			}
			m_GetSocket->Send_Message(u8Data, len);
			m_bReadEnd_UartMessage =1;
		}
		else if((u8Data[MSGTYPE] == TAG_ASSOCIATION) || (u8Data[MSGTYPE] == DOWNLOAD_START_ACK)
				|| (u8Data[MSGTYPE] == DATAINDICATION_ACK) || (u8Data[MSGTYPE] == TAG_INFOR_UPDATE)
				|| (u8Data[MSGTYPE] == BSN_START_ACK)
				|| (u8Data[MSGTYPE] == BSN_DATA_END_ACK)
				|| (u8Data[MSGTYPE] == COORDINATOR_RESET_CONFIRM)
				|| (u8Data[MSGTYPE] == TAG_INFOR_UPDATE_ACK)
				|| (u8Data[MSGTYPE] == TAG_ALARM_INDICATION)
				|| (u8Data[MSGTYPE] == MULTI_GATEWAY_SCAN_CONFIRM)
				|| (u8Data[MSGTYPE] == MULTI_GATEWAY_SCAN_RESPONESE)
				|| (u8Data[MSGTYPE] == TAG_LOWBATT_ALARM_INDICATION)
				/*|| (u8Data[MSGTYPE] == SERVICESTART_CONFIRM)*/)
		{

			if(u8Data[MSGTYPE] == TAG_ASSOCIATION) {
				m_MsgQueueDataAssocation.clear();
				memcpy(m_u8SendData, u8Data, len);
				for(int i=0; i<len; i++) {
					m_MsgQueueDataAssocation.push_back(u8Data[i]);
					printf("%x ", m_MsgQueueDataAssocation[i]);
				}
				printf("\n");
				m_Queue.push(m_MsgQueueDataAssocation);
				
			/*	if(u8Data[MSG_ASSOCIATION_STATUS] == PAYLOAD_STATUS_SUCCESS) {
					m_MsgTempTagIDAssociation[nTagidCnt] = ByteToWord(m_MsgQueueDataAssocation.at(MSG_SADDRONE), m_MsgQueueDataAssocation.at(MSG_SADDRZERO)) ;
					printf("Association TAG ID : %d\n", m_MsgTempTagIDAssociation[nTagidCnt]);
					
					nTagidCnt++;
				}*/
				m_MsgQueueDataAssocation.clear();
				m_nSendTagCount++;				
				
			//	printf("m_nSendTagCount : %d\n", m_nSendTagCount );
			/*	while(1) {
					if(!m_bReadEnd_UartMessage)
						break;
				}*/
				m_bReadEnd_UartMessage =1;
				return 1;
			}
			else if((u8Data[MSGTYPE] == COORDINATOR_RESET_CONFIRM) || (u8Data[MSGTYPE] == TAG_INFOR_UPDATE) || (u8Data[MSGTYPE] == TAG_INFOR_UPDATE_ACK)
					|| (u8Data[MSGTYPE] == TAG_ALARM_INDICATION) || (u8Data[MSGTYPE] == MULTI_GATEWAY_SCAN_CONFIRM) || (u8Data[MSGTYPE] == MULTI_GATEWAY_SCAN_RESPONESE)
					|| (u8Data[MSGTYPE] == TAG_LOWBATT_ALARM_INDICATION) )
			{

				if(u8Data[MSGTYPE] == COORDINATOR_RESET_CONFIRM) {
					memcpy(m_u8SendData, u8Data, len);
					m_bUartCommuniFlag = 0;
					m_bReadEnd_UartMessage=1;
					printf("m_bUartCommuniFlag %d\n", m_bUartCommuniFlag);
				}
				m_GetSocket->Send_Message(u8Data, len);
			}
			else {
				m_vcemsg.clear();
				for(int i=0; i<len; i++) {
					m_vcemsg.push_back(u8Data[i]);
			//		printf("[%x] ", m_vcemsg.at(i));
				}
				if(!m_bUartCommuniFlag && (u8Data[MSGTYPE] == BSN_START_ACK)) {					
		/*			m_vcemsg[MSGTYPE] = u8Data[MSGTYPE];
					m_vcemsg[MSG_BSN_DATA] = u8Data[MSG_DATA];
				*/
		/*			printf("BSN_START ACK :%x \n", m_vcemsg[MSGTYPE] );
					printf("MSGTYPE  :%x \n", m_vcemsg[MSGTYPE] );
					printf("MSG_BSN_DATA  :%x \n", m_vcemsg[MSG_BSN_DATA] );*/
					m_bUartCommuniFlag = 1;
					return 1;
				}
				else if( (u8Data[MSGTYPE] == DATAINDICATION_ACK) || (u8Data[MSGTYPE] == DOWNLOAD_START_ACK) ) {
					//m_vcemsg[MSGTYPE] = u8Data[MSGTYPE];
			//		printf("m_vcemsg[MSGTYPE] : %x \n", m_vcemsg[MSGTYPE]);
					if(u8Data[MSGTYPE] == DOWNLOAD_START_ACK) {
						m_vcemsg[MSG_SADDRZERO] = u8Data[MSG_SADDRZERO];
						m_vcemsg[MSG_SADDRONE] = u8Data[MSG_SADDRONE];
					}
					else if(u8Data[MSGTYPE] == DATAINDICATION_ACK) {
						m_vcemsg[MSG_SADDRZERO] = u8Data[MSG_SADDRZERO];
						m_vcemsg[MSG_SADDRONE] = u8Data[MSG_SADDRONE];
						m_vcemsg[MSG_CFM_DATAINDICATE_STATUS] = u8Data[MSG_CFM_DATAINDICATE_STATUS];
					}
					m_bUartCommuniFlag = 1;
				}
			}
		}
		else if( (u8Data[MSGTYPE] == 0) && (u8Data[MSGTYPE] == BSN_DATA_END_ACK) ) {
			return 1;
		}
		else {
			memcpy(m_u8SendData, u8Data, len);
			if(m_vcemsg.size() > 0) {
				m_vcemsg.clear();
			}
		//	printf("m_vcemsg.m_UartMsg_vec.Push_back\n");
			for(int i=0; i< len; i++) {
				m_vcemsg.push_back(u8Data[i]);
			//	printf("[%x] ", m_vcemsg[i]);
			}
		//	printf("\n");
			m_bReadEnd_UartMessage =1;
		}
	}

	return 1;
}

int MsgQueue::DataSort()
{
	
	printf("DataSort()\n");
	printf("Before Sort  m_ArrayDataAcknowledge.size %d\n", (int)m_ArrayDataAcknowledge.size());
	for(int j=0; j<(int)m_ArrayDataAcknowledge.size(); j++) {
		for(int i=0; i<(int)m_ArrayDataAcknowledge[j].size(); i++) {
			printf("[%x] ", m_ArrayDataAcknowledge[j].at(i));
		}
	}
	printf("\n");
	sort(m_ArrayDataAcknowledge.begin(), m_ArrayDataAcknowledge.end());
	m_ArrayDataAcknowledge.erase(unique(m_ArrayDataAcknowledge.begin(), m_ArrayDataAcknowledge.end()),
																			m_ArrayDataAcknowledge.end()); 	//Delete overlap
	//printf("Sort , Delete 0 and Overlap after\n");
	for(int j=0; j<nDataDown; j++) {
		for(int i=0; i<(int)m_ArrayDataAcknowledge[j].size(); i++) {
			if(m_ArrayDataAcknowledge[j].at(i) == 0) {
			//	printf("Erase %x[%d] ", m_ArrayDataAcknowledge[j].at(i), j);
				m_ArrayDataAcknowledge.erase(m_ArrayDataAcknowledge.begin()+j, m_ArrayDataAcknowledge.begin()+j+1);
			}
		}
	}
	sort(m_ArrayDataAcknowledge.begin(), m_ArrayDataAcknowledge.end());

	printf("Final Sort after m_ArrayDataAcknowledge.size %d\n", (int)m_ArrayDataAcknowledge.size());
	for(int j=0; j<nDataDown; j++) {
		for(int i=0; i<(int)m_ArrayDataAcknowledge[j].size(); i++) {
			printf("[%x] ", m_ArrayDataAcknowledge[j].at(i));
		}
		
	}
	printf("\n");

	return 1;
}

void MsgQueue::GetDataDown(int cnt, WORD* TagNumber)
{
	nDataDown = cnt;
	memset(u16TagNumberData, 0, sizeof(WORD)*4096);
	memcpy(u16TagNumberData, TagNumber, sizeof(WORD)*cnt);
}

WORD MsgQueue::ByteToWord(BYTE puData, BYTE puData1)
{
	WORD p16Tempdata_HIGH, p16Tempdata_LOW;

	p16Tempdata_HIGH = (puData << 8);
	p16Tempdata_LOW = puData1;

	return p16Tempdata_HIGH|p16Tempdata_LOW;
}

void MsgQueue::th_delay(int millsec)
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


void MsgQueue::GetSocket(Socket* soc)
{
	m_GetSocket = soc;
}


void MsgQueue::deleteArray1(int idx, int size, WORD* ar)
{
	memmove(ar+idx, ar+idx+1, size-idx);
}

void MsgQueue::AppendArray1(WORD sz, int size1, WORD* ar)
{
	InsertArray1(size1, sz, ar);
}

int MsgQueue::GetSizeArray1 (WORD* ar)
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
void MsgQueue::InsertArray1(int idx, WORD sz, WORD* ar)
{
	WORD arr[4096];
	memcpy(arr, ar, 4096);
	int size = (sizeof(arr)/sizeof(*arr));
	memmove(ar+idx+1, ar+idx, size-idx+1);
	ar[idx] = sz;
/*	for(int i=0; i<4096; i++) {
		printf("%x ", ar[i]);
	}*/
	printf("Insert [%d]%x, size : %d\n", idx, ar[idx], size);
}

void MsgQueue::PrintArray1(WORD* ar, int size)
{
	WORD arr[4096];
	memcpy(arr, ar, 4096);
	for(int i=0; i<= size; i++) {
	//	if(ar[i] > 0)		
			printf("%x ", arr[i]);
	}	
	printf("\n\n");
}


