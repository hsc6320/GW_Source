/*
 * SocketMsgQueue.h
 *
 *  Created on: 2021. 12. 9.
 *      Author: hong
 */

#ifndef SOCKET_SOCKETMSGQUEUE_H_
#define SOCKET_SOCKETMSGQUEUE_H_

#include "../pch.h"
#include "../Vector_SocketQueue.h"
#include "../Msg_Define.h"
#include <vector>

typedef struct _PacketQueue {
	WORD PanID;
	WORD ServerID;
	WORD GateWayID;
	WORD Length;
	BYTE* Data;
	BYTE ext[3] = {0xA5, 0x5A, 0x7E};
}Common_QueuePacket;

class Socket_MsgQueue {

public:
	Socket_MsgQueue();
	virtual ~Socket_MsgQueue();
	//VectorSocket<uint8_t> m_SocketMsg_vec;
	Common_QueuePacket	QuquePacket;

	int GetSocketMsg(std::vector<uint8_t> vec, int len);
	void BSN_MSG_ACK(BYTE* puData);
	void BSN_MSG_END_ACK(BYTE* puData);
	void Tag_Direct_Update_Ack(BYTE* puData);
	void DownLoad_MSG_Start_ACK(BYTE* puData);
	void DataIndication_MSG_Start_ACK(BYTE* puData);
	void GetPanID(WORD panid);
	void GetServerID(WORD severid);
	BYTE GetChecksum(BYTE* puData, int len);
	WORD ByteToWord(BYTE puData, BYTE puData1);

};

#endif /* SOCKET_SOCKETMSGQUEUE_H_ */
