/*
 * SocketMsgQueue.cpp
 *
 *  Created on: 2021. 12. 9.
 *      Author: hong
 */

#include "SocketMsgQueue.h"

BYTE ext[3] = {0xA5, 0x5A, 0x7E};

Socket_MsgQueue::Socket_MsgQueue() {
}

Socket_MsgQueue::~Socket_MsgQueue() {
	// TODO Auto-generated destructor stub
}

//int Socket_MsgQueue::GetSocketMsg(uint8_t* p8Data, int len)
int Socket_MsgQueue::GetSocketMsg(std::vector<uint8_t> vec, int len)
{
	int msgtype = vec[MSGTYPE];
	int retVal =0;

	printf("\nGetSocketQueue msgtype: %x ", msgtype);

	printf("\n");
	switch(msgtype)
	{
	case REGISTRATION_CONFIRM:
		QuquePacket.ServerID = ByteToWord(vec[MSG_SADDRONE], vec[MSG_SADDRZERO]);
		break;
	case SERVICESTART_REQUEST:
		retVal = SERVICESTART_REQUEST;
		break;
	default:
		retVal = msgtype;
		break;
	}

	return retVal;
}

void Socket_MsgQueue::DataIndication_MSG_Start_ACK(BYTE* puData)
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)QuquePacket.PanID;
	pu8data[++iBufcnt] = QuquePacket.PanID >> 8;
	pu8data[++iBufcnt] = QuquePacket.ServerID;
	pu8data[++iBufcnt] = QuquePacket.ServerID >> 8;
	pu8data[++iBufcnt] = puData[MSG_DADDRZERO];		//TAG ID
	pu8data[++iBufcnt] = puData[MSG_DADDRONE];		//TAG ID

	pu8data[++iBufcnt] = DATAINDICATION_ACK;	//msg type
	pu8data[++iBufcnt] = 0x02;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0x01;			//status
	pu8data[++iBufcnt] = 0x01;			//Received sequence number

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = QuquePacket.ext[0];
	pu8data[++iBufcnt] = QuquePacket.ext[1];
	pu8data[++iBufcnt] = QuquePacket.ext[2];

	memset(puData, 0, iBufcnt);
	memcpy(puData, pu8data, iBufcnt+1);
}


void Socket_MsgQueue::DownLoad_MSG_Start_ACK(BYTE* puData)
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)QuquePacket.PanID;
	pu8data[++iBufcnt] = QuquePacket.PanID >> 8;
	pu8data[++iBufcnt] = QuquePacket.ServerID;
	pu8data[++iBufcnt] = QuquePacket.ServerID >> 8;
	pu8data[++iBufcnt] = puData[MSG_DADDRZERO];		//TAG ID
	pu8data[++iBufcnt] = puData[MSG_DADDRONE];		//TAG ID

	pu8data[++iBufcnt] = DOWNLOAD_START_ACK;	//msg type
	pu8data[++iBufcnt] = 0x01;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0x01;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = QuquePacket.ext[0];
	pu8data[++iBufcnt] = QuquePacket.ext[1];
	pu8data[++iBufcnt] = QuquePacket.ext[2];

	memset(puData, 0, iBufcnt);
	memcpy(puData, pu8data, iBufcnt+1);
}

void Socket_MsgQueue::BSN_MSG_END_ACK(BYTE* puData)
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)QuquePacket.PanID;
	pu8data[++iBufcnt] = QuquePacket.PanID >> 8;
	pu8data[++iBufcnt] = QuquePacket.ServerID;
	pu8data[++iBufcnt] = QuquePacket.ServerID >> 8;
	pu8data[++iBufcnt] = (BYTE)QuquePacket.GateWayID;		//gateway ID
	pu8data[++iBufcnt] = QuquePacket.GateWayID >> 8;		//gateway ID

	pu8data[++iBufcnt] = BSN_DATA_END_ACK; //msg type
	pu8data[++iBufcnt] = 0x01;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0x00;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = QuquePacket.ext[0];
	pu8data[++iBufcnt] = QuquePacket.ext[1];
	pu8data[++iBufcnt] = QuquePacket.ext[2];


	memcpy(puData, pu8data, iBufcnt);
}

void Socket_MsgQueue::BSN_MSG_ACK(BYTE* puData)
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)QuquePacket.PanID;
	pu8data[++iBufcnt] = QuquePacket.PanID >> 8;
	pu8data[++iBufcnt] = (BYTE)QuquePacket.GateWayID;		//gateway ID
	pu8data[++iBufcnt] = QuquePacket.GateWayID >> 8;		//gateway ID
	pu8data[++iBufcnt] = QuquePacket.ServerID;
	pu8data[++iBufcnt] = QuquePacket.ServerID >> 8;

	pu8data[++iBufcnt] = BSN_START_ACK;	//msg type
	pu8data[++iBufcnt] = 0x01;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0x00;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = QuquePacket.ext[0];
	pu8data[++iBufcnt] = QuquePacket.ext[1];
	pu8data[++iBufcnt] = QuquePacket.ext[2];

	memcpy(puData, pu8data, iBufcnt);

}

void Socket_MsgQueue::GetPanID(WORD panid)
{
	QuquePacket.PanID = panid;
	QuquePacket.GateWayID = panid;
}

void Socket_MsgQueue::GetServerID(WORD severid)
{
	QuquePacket.ServerID = severid;
	printf("GetServerID %x\n", QuquePacket.ServerID);
}

BYTE Socket_MsgQueue::GetChecksum(BYTE* puData, int len)
{
	BYTE sum =0;

	for(int i=1; i<= len; i++) {
	//	printf("%x ", puData[i]);
		sum += puData[i];
	}
	//printf("(check nsum : %x) ", sum);

	return sum;
}

WORD Socket_MsgQueue::ByteToWord(BYTE puData, BYTE puData1)
{
	WORD p16Tempdata_HIGH, p16Tempdata_LOW;

	p16Tempdata_HIGH = (puData << 8);
	p16Tempdata_LOW = puData1;

	return p16Tempdata_HIGH|p16Tempdata_LOW;

}



