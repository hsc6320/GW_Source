/*
 * MsgHandler.h
 *
 *  Created on: 2021. 11. 30.
 *      Author: hong
 */

#ifndef MSGHANDLER_H_
#define MSGHANDLER_H_

#include "pch.h"
#include "uart/uart.h"
//#include "Vector_queue.h"

typedef struct _HandlerPacket {
	WORD PanID;
	WORD ServerID;
	WORD GateWayID;
	WORD Length;
	BYTE* Data;
	BYTE ext[3] = {0xA5, 0x5A, 0x7E};
}Common_Packet;

class MsgHandler {

public:
	MsgHandler();
	virtual ~MsgHandler();

	std::vector<BYTE> m_UartReDataDownMsg;

	std::vector<std::vector<BYTE>> m_UartArrayDataDownMsg;
	std::vector<std::vector<BYTE>> m_UartArrayDataIndicateMsg;

	std::vector<std::vector<BYTE>> m_UartArrayReDataDownMsg;
	std::vector<std::vector<BYTE>> m_UartArrayReDataIndicateMsg;
	std::map<std::vector<BYTE>, int > Map_dataParityCheck;
	std::map<WORD, int > Map_u16AcknowParityCheck;
	WORD m_pu16MsgDataAcknowledge[4096];

	Common_Packet	CommonPacket;
	int Map_AcknowCnt,Map_AcknowCnt2;
	int m_nDataDownCount, m_nDataIndiCount;
	int m_DataCnt;
	int m_DataFlag;
	int DataSendFail_RedownCnt;
	int Beacon_Max;
	int iSmallDataDown, m_iTagDirectDown, m_iTagDirectDownCnt, m_iTagDirectDownCnt2;
	UartComThread* m_pCommUart;
	int m_nUartArrayDataDownCnt, m_nUartArrayDataIndicateCnt;
	int m_nUartArrayReDataIndicateCnt;
	int m_nDataSendFail_SuccessCnt;
	void SetHandle(UartComThread* msg);
	void SetSocketArray(std::vector<std::vector<BYTE>> DataDownmsg, std::vector<std::vector<BYTE>> DataIndimsg);
	void ServiceIdle();
	int ServiceStart();
	int GetTagNumber(int temp);
	int DataFlag_Initialize(int beacon);
	void BypassSocketToUart(BYTE* p8Data, int DataLen, int msgtype);
	void BypassUartToSocket();
	BYTE GetChecksum(BYTE* puData, int len);
	void GetPanID(WORD panid);
	void GetServerID(WORD severid);
	void SetBeacon(int a);
	int BSN_Start_Packet(/*BYTE* u8data*/);
	int BSN_Stop_Packet();
	int UartPacket_DataDownStart(BYTE u8data);
	int UartPacket_ReDataDownStart(BYTE u8data);
	int UartPacket_DataIndicateStart(BYTE u8data);
	int UartPacket_ReDataIndicateStart(BYTE u8data);
	int UartPacket_ReDataAcknowledge_DownStart(BYTE u8data);
	int UartPacket_ReDataAcknowledge_DataIndicateStart(BYTE u8data);
	int Send_BeaconData(BYTE ibeaconvalue);
	WORD ByteToWord(BYTE puData, BYTE puData1);
	int bClear();
};

namespace SENDPACKET {
#pragma pack(push, 1)
	typedef struct _HEADER {
		BYTE	stx;
		WORD	panID;
		WORD	dAddr;
		WORD	sAddr;
		BYTE	type;
		WORD	length;
		_HEADER() {stx = 0xAA;}
	}SEND_HEADER, *PS_SOCKET_HEADER;

	typedef struct _TAIL {
		BYTE	checksum;
		BYTE	ext[3];
		void Endsignal() {ext[0] = 0xA5; ext[1] = 0x5A; ext[2] = 0x7E; }
	}SEND_TAIL, *PS_SOCKET_TAIL;

	typedef struct _SOCKET_PACKET {
		SEND_HEADER		header;
		SEND_TAIL			tail;
		BYTE* 			pu8Data;
	}SOCKET_PACKET, *PS_SOCKET_PACKET;

#pragma pack(pop)
}
using namespace SENDPACKET;

#endif /* MSGHANDLER_H_ */

