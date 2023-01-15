/*
 * SocketHandler.h
 *
 *  Created on: 2021. 12. 6.
 *      Author: hong
 */

#ifndef SOCKET_SOCKETHANDLER_H_
#define SOCKET_SOCKETHANDLER_H_


#include "../pch.h"
#include "Socket.h"
#include "../MsgQueue.h"

typedef struct _Packet {
	WORD PanID;
	WORD ServerID;
	WORD GateWayID;
	WORD Length;
	BYTE* Data;
	BYTE ext[3] = {0xA5, 0x5A, 0x7E};
}Get_Packet;

class SocketHandler {
public:
	SocketHandler();
	virtual ~SocketHandler();
	int m_nTagDataCount;
	Get_Packet		packet;

	BYTE GetChecksum(BYTE* puData, int len);
	void SetSocketHwnd(Socket* soc);
	void SetMsgQueueHwnd(MsgQueue* soc);
	int SendMessage(int msg, PRE_DEFINE::S_PACKET packet);
	void GetPanID(WORD panid);
	void GetServerID(WORD severid);
	int Registration_Request();
	WORD ByteToWord(BYTE puData, BYTE puData1);
	int Connect_Request();
	int DownLoad_Start_Ack();
	int SendSocket_Data(PRE_DEFINE::S_PACKET packet);
	void SetMsg_StartCfm_Remalloc();
	void TagData(std::vector<std::vector<BYTE>> vec);
};

#endif /* SOCKET_SOCKETHANDLER_H_ */
