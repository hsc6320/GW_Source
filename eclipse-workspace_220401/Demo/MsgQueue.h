/*
 * MsgQueue.h
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */


#ifndef MSGQUEUE_H_
#define MSGQUEUE_H_

#include "pch.h"
#include "Vector_queue.h"
#include "Vector_SocketQueue.h"
#include <sys/resource.h>
#include "Socket/Socket.h"
#include <sched.h>
#include <vector>


typedef struct _msgform {
	uint8_t stx;
	uint8_t PanId[2];
	uint8_t Daddr[2];
	uint8_t Saddr[2];
	uint8_t u8MsgType;
	BYTE    Length[2];
	BYTE status;
	uint8_t data[10];
	uint8_t Checksum;
	uint8_t ext[3] = { 0xA5, 0x5A, 0x7E };
} msgform;


typedef struct _msgvecform {
	Vector<BYTE> m_UartMsg_vec;
	//std::vector<BYTE> m_UartMsg_vec;
	int iMsgExcep;
	msgform MsgPacket;
}vecmsgform;



class MsgQueue
{
public :
	MsgQueue(void);
	~MsgQueue(void);
public:

	std::queue<BYTE> que;
	std::vector<BYTE> m_vcemsg;
	
	int m_bReadEnd_UartMessage;
	int m_bUartCommuniFlag, m_bUartTagAssociFlag;
	int m_Uart_ServiceStart_TagAssociation_InitFlag;
	int m_nMapParity;
	int m_nSendTagCount;

	std::queue<std::vector<BYTE>> m_Queue;
	std::vector<BYTE> m_MsgQueueDataAssocation;
	std::set<WORD> setTagNumber, setTagAckNumber;
	
	BYTE m_u8SendData[1024];
	WORD m_pu16MsgQueueArrayDataAcknowledge[4096];
	WORD m_Test[4096];

	void th_delay(int millsec);
	void GetSocket(Socket* soc);
	int uart_DelaySend();
	WORD ByteToWord(BYTE puData, BYTE puData1);
	bool PutByte(uint8_t* b, int len);
	void GetDataDown(int cnt, WORD* TagNumber);
	int GetSizeArray1(WORD* ar);
	void AppendArray1(WORD sz, int size1, WORD* ar);
	void PrintArray1(WORD* ar, int size);
	void InsertArray1(int idx, WORD sz, WORD* ar);
	void deleteArray1(int idx, int size, WORD* ar);

};
#endif /* MSGQUEUE_H_ */
