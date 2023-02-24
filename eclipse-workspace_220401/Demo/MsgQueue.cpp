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
int nDatalength =0;
int nDataDown =0;
BYTE m_u8SendData[1024];

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
	m_DataAckCnt=0;
	m_bReadEnd_UartMessage = 0;
	m_bUartCommuniFlag = 0;
	m_bUartTagAssociFlag =0;
	m_nMapParity =0;
	Redown =0;
	Redown2 =0;
	m_nSendTagCount =0;
	m_Uart_ServiceStart_TagAssociation_InitFlag =0;
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
	memset(m_u8SendData, 0, 1024);
	memset(u8Data, 0, 1024);
	memcpy(u8Data, b, 1024);

	printf("PutByte()\n");

	if(u8Data[MSG_STX] == STX) {
		if(u8Data[MSGTYPE] == DATA_ACKNOWLEDGEMENT) {
			if(u8Data[MSG_ACKNOWLEDGE_STATUS] == PAYLOAD_STATUS_SUCCESS) {	
				Cnt = m_nMapParity;
				for(int i=0; i< Cnt; i++) {
					if( !Redown ) {
						if(m_pu16MsgQueueArrayDataAcknowledge[i] == (ByteToWord(u8Data[MSG_SADDRONE], u8Data[MSG_SADDRZERO])) ) {
							printf("m_nMapParity Overlap Parity 0x%x\n", m_pu16MsgQueueArrayDataAcknowledge[i]);
							return 0;
						}
					}
					else {
						if(m_pu16MsgQueueArrayDataAcknowledge[i] == (ByteToWord(u8Data[MSG_SADDRONE], u8Data[MSG_SADDRZERO])) ) {
							printf("m_pu16MsgQueueArrayDataAcknowledge, Overlap Parity 0x%x \n", m_pu16MsgQueueArrayDataAcknowledge[i]);
							return 0;
						}
					}
				}
				size = 4096; 
				wordPanID = ByteToWord(u8Data[MSG_SADDRONE], u8Data[MSG_SADDRZERO]);
				printf("m_nMapParity :%d, wordPanID : %d, size : %d\n", m_nMapParity, wordPanID, size);
				
				int size2 =GetSizeArray1(m_Test);
//				PrintArray1(m_Test, size2);
				AppendArray1(wordPanID, m_nMapParity, m_Test);
//				m_ArrayUtil.InsertArray(m_nMapParity, wordPanID, m_Test);
//				size2 =GetSizeArray1(m_Test);
//				PrintArray1(m_Test, size2);

				sort(m_Test, m_Test+size);
				int j =0;
				for(int i=0; i<=4096; i++) {
					if(m_Test[i] > 0) {
						m_pu16MsgQueueArrayDataAcknowledge[j] = m_Test[i];
						printf("[%d] ", m_pu16MsgQueueArrayDataAcknowledge[j]);
						j++;
					}
				}
				printf("\n");

				printf("DataAck TagID : ");
				printf("%x\n",(BYTE)(ByteToWord(u8Data[MSG_SADDRONE], u8Data[MSG_SADDRZERO])) );
				

			/*	if(Redown) {
					printf("MsgQueue Redown m_nMapParity : %d \n", m_nMapParity);
					m_ArrayUtil.InsertArray(m_nMapParity, wordPanID, m_Test);
					sort(m_pu16MsgQueueArrayDataAcknowledge, m_pu16MsgQueueArrayDataAcknowledge+size, ZeroCompare2);
					
					for(int i=0; i<m_nMapParity; i++) {
						if(m_Test[i] > 0) {
							m_pu16MsgQueueArrayDataAcknowledge[i] = m_Test[i];
							printf("[%d] ", m_pu16MsgQueueArrayDataAcknowledge[i]);
							}				
						printf("\n");
					//	DataSort();					
					}
				}*/
				m_nMapParity++;				
				printf("m_nMapParity : %d\n", m_nMapParity);

			}
			m_GetSocket->Send_Message(u8Data, len);
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
			//	m_MsgQueueTagData.push_back(m_MsgQueueDataAssocation);
				m_MsgQueueDataAssocation.clear();
				m_nSendTagCount++;
				printf("m_nSendTagCount : %d\n", m_nSendTagCount );
				m_bReadEnd_UartMessage =1;
				return 1;
			}
			else if((u8Data[MSGTYPE] == COORDINATOR_RESET_CONFIRM) || (u8Data[MSGTYPE] == TAG_INFOR_UPDATE) || (u8Data[MSGTYPE] == TAG_INFOR_UPDATE_ACK)
					|| (u8Data[MSGTYPE] == TAG_ALARM_INDICATION) || (u8Data[MSGTYPE] == MULTI_GATEWAY_SCAN_CONFIRM) || (u8Data[MSGTYPE] == MULTI_GATEWAY_SCAN_RESPONESE)
					|| (u8Data[MSGTYPE] == TAG_LOWBATT_ALARM_INDICATION) )
			{

				if(u8Data[MSGTYPE] == COORDINATOR_RESET_CONFIRM)	{
					memcpy(m_u8SendData, u8Data, len);
					m_bUartCommuniFlag = 0;
					m_bReadEnd_UartMessage=1;
					printf("m_bUartCommuniFlag %d\n", m_bUartCommuniFlag);
				}
				m_GetSocket->Send_Message(u8Data, len);
			}
			else {
				if(!m_bUartCommuniFlag && (u8Data[MSGTYPE] == BSN_START_ACK)) {
					
				
					m_vcemsg.MsgPacket.u8MsgType = u8Data[MSGTYPE];
					m_vcemsg.MsgPacket.data[0] = u8Data[MSG_DATA];
				//	printf("BSN_START_ACK msgtype : %x, becondata : %x \n", m_vcemsg.MsgPacket.u8MsgType, m_vcemsg.MsgPacket.data[0]);
					m_bUartCommuniFlag = 1;				
					return 1;
				}
				else if( (u8Data[MSGTYPE] == DATAINDICATION_ACK) || (u8Data[MSGTYPE] == DOWNLOAD_START_ACK) ) {
					m_vcemsg.MsgPacket.u8MsgType = u8Data[MSGTYPE];
					if(u8Data[MSGTYPE] == DOWNLOAD_START_ACK) {
						m_vcemsg.MsgPacket.Saddr[0] = u8Data[MSG_SADDRZERO];
						m_vcemsg.MsgPacket.Saddr[1] = u8Data[MSG_SADDRONE];
					}
					else if(u8Data[MSGTYPE] == DATAINDICATION_ACK) {
						m_vcemsg.MsgPacket.Saddr[0] = u8Data[MSG_SADDRZERO];
						m_vcemsg.MsgPacket.Saddr[1] = u8Data[MSG_SADDRONE];
						m_vcemsg.MsgPacket.status = u8Data[MSG_CFM_DATAINDICATE_STATUS];
					}
					m_bUartCommuniFlag = 1;
				}
			}
		}
		else if( (u8Data[MSGTYPE] == 0) && (u8Data[MSGTYPE] == BSN_DATA_END_ACK) ) {
			Redown =0;
			return 1;
		}
		else {
			memcpy(m_u8SendData, u8Data, len);
			for(int i =0; i< m_vcemsg.m_UartMsg_vec.size(); i++) {
				m_vcemsg.m_UartMsg_vec.remove(i);
			}
			printf("m_vcemsg.m_UartMsg_vec.Push_back\n");
			for(int i=0; i< len; i++) {
				m_vcemsg.m_UartMsg_vec.push_back(u8Data[i]);
				printf("[%x] ", m_vcemsg.m_UartMsg_vec[i]);
			}
			printf("\n");
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

void MsgQueue::GetDataDown(int cnt)
{
	nDataDown = cnt;
}

WORD MsgQueue::ByteToWord(BYTE puData, BYTE puData1)
{
	WORD p16Tempdata_HIGH, p16Tempdata_LOW;

	p16Tempdata_HIGH = (puData << 8);
	p16Tempdata_LOW = puData1;

	return p16Tempdata_HIGH|p16Tempdata_LOW;
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


