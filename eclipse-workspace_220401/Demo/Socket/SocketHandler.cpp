/*
 * SocketHandler.cpp
 *
 *  Created on: 2021. 12. 6.
 *      Author: hong
 */

#include "SocketHandler.h"

Socket* pSocket;
MsgQueue* pUartQueue;
int nServiceStart_Confirm =0, beacon_count =0;
std::queue<std::vector<BYTE>> vTagData;

SocketHandler::SocketHandler()
{
	m_iSocketArive =1;
	m_nTagDataCount =0;
}

SocketHandler::~SocketHandler() {
	// TODO Auto-generated destructor stub
}

int SocketHandler::SendMessage(int msg, PRE_DEFINE::S_PACKET packet)
{
	int msgtype =0;
	//uint8_t pu8data[15] = {0xaa, 0, 0xfc, 0x01, 0, 0x01, 0, 0x05, 0x01, 0, 0, 0x04, 0xa5, 0x5a, 0x7e};
	//printf("SocketHandler SendMessage\n");
	msgtype = msg;

	switch(msgtype)
	{
	case REGISTRATION_REQUEST:
		printf("Socket SendMsg : REGISTRATION_REQUEST\n");
		Registration_Request();
		break;
	case CONNECT_REQUEST:
		printf("Socket SendMsg : CONNECT_REQUEST\n");
		Connect_Request();
		break;
	case DOWNLOAD_START_ACK:
		printf("Socket SendMsg : DOWNLOAD_START_ACK\n");
		break;
	case TAG_ASSOCIATION:
		printf("Socket SendMsg : TAG_ASSOCIATION\n");
		break;
	case CONNECT_ALIVE_CHECK:
	//	printf("Socket SendMsg : CONNECT_ALIVE_CHECK\n");
		m_iSocketArive = GateWay_Status_Check();
		break;
	case SERVICESTART_CONFIRM:
		printf("Socket SendMsg : SERVICESTART_CONFIRM\n");
		SendSocket_Data(packet);
		break;
	default :
		printf("Socket SendMsg : default : 0x%x\n", packet.header.type);
	//	SendSocket_Data(packet);
		break;
	}

	return 1;
}


int SocketHandler::SendSocket_Data(PRE_DEFINE::S_PACKET packet)
{
	BYTE pu8data[1024] = {0, };
	BYTE u8Checksum;
	int iBufcnt =0;

	pu8data[iBufcnt] = packet.header.stx;
	printf("ByPass() %x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.panID << 8;//packet.PanID << 8;
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.panID >> 8;
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.dAddr; //ServerID;
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.dAddr >> 8;
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = (BYTE)packet.header.sAddr;//0x01;		//gateway ID
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.sAddr >> 8;//0xfb;		//gateway ID
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.type; //SERVICESTART_CONFIRM;	//msg type
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.length;//0x02;	//Data Length
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.header.length >> 8;
	printf("%x ", pu8data[iBufcnt]);
	for(int i=0; i< (int)packet.header.length; i++) {
		if(i==0) {
			if( nServiceStart_Confirm ) {
				pu8data[++iBufcnt] = 0x06;
			}
			else {
				pu8data[++iBufcnt] = packet.pu8Data[i];//0x01;
			}
			printf("%x ", pu8data[iBufcnt]);
		}
		else {
			pu8data[++iBufcnt] = packet.pu8Data[i];
			beacon_count = packet.pu8Data[i];
			beacon_count--;
			//pu8data[++iBufcnt] = packet.pu8Data[i];//0x01;
			printf("(bEACON COUNT : %x ", beacon_count);
		}		
	}

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;
	printf("%x ", pu8data[iBufcnt]);

	pu8data[++iBufcnt] = packet.tail.ext[0];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.tail.ext[1];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = packet.tail.ext[2];
	printf("%x\n", pu8data[iBufcnt]);

	packet.header.DataLength = iBufcnt+1;
	pSocket->Send_Message(pu8data, packet.header.DataLength);

	return 1;
}

int SocketHandler::DownLoad_Start_Ack()
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)packet.PanID;
	pu8data[++iBufcnt] = packet.PanID >> 8;
	pu8data[++iBufcnt] = packet.ServerID;
	pu8data[++iBufcnt] = packet.ServerID >> 8;
	pu8data[++iBufcnt] = (BYTE)packet.GateWayID;		//gateway ID
	pu8data[++iBufcnt] = packet.GateWayID >> 8;		//gateway ID
	pu8data[++iBufcnt] = DOWNLOAD_START_ACK;	//msg type
	pu8data[++iBufcnt] = 0x0a;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0x01;

	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = packet.ext[0];
	pu8data[++iBufcnt] = packet.ext[1];
	pu8data[++iBufcnt] = packet.ext[2];

	pSocket->Send_Message(pu8data, iBufcnt+1);

	return 1;
}


int SocketHandler::Connect_Request()
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;
	printf("Server iD :%x\n", packet.ServerID);
	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)packet.PanID;
	pu8data[++iBufcnt] = packet.PanID >> 8;
	pu8data[++iBufcnt] = 0x06;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = (BYTE)packet.GateWayID;		//gateway ID
	pu8data[++iBufcnt] = packet.GateWayID >> 8;		//gateway ID
	pu8data[++iBufcnt] = CONNECT_REQUEST;	//msg type
	pu8data[++iBufcnt] = 0x0a;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0x01;

	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = packet.ext[0];
	pu8data[++iBufcnt] = packet.ext[1];
	pu8data[++iBufcnt] = packet.ext[2];

	pSocket->Send_Message(pu8data, iBufcnt+1);
	pSocket->m_pSocMsgqueue->GetServerID(packet.ServerID);

	return 1;
}


int SocketHandler::Registration_Request()
{
	uint8_t pu8data[25];
	BYTE u8Checksum;
	int iBufcnt =0, ret =0;

	packet.GateWayID = packet.PanID;

	printf("packet.GateWayID : %x\n", packet.GateWayID);
	printf("packet.GateWayID : %x\n", (BYTE)packet.GateWayID);

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)packet.PanID;
	pu8data[++iBufcnt] = packet.PanID>> 8;
	pu8data[++iBufcnt] = 0x06;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = (BYTE)packet.GateWayID << 8;		//gateway ID
	pu8data[++iBufcnt] = packet.GateWayID >> 8;		//gateway ID
	
	pu8data[++iBufcnt] = REGISTRATION_REQUEST;	//msg type
	pu8data[++iBufcnt] = 0x0d;
	pu8data[++iBufcnt] = 0;
	
	pu8data[++iBufcnt] = 1;
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[0];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[1];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[3];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[4];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[6];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[7];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[9];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[10];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[12];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[13];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[15];
	pu8data[++iBufcnt] = pSocket->m_szMac_addr[16];

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = packet.ext[0];
	pu8data[++iBufcnt] = packet.ext[1];
	pu8data[++iBufcnt] = packet.ext[2];

	ret = pSocket->Send_Message(pu8data, iBufcnt+1);
	
	return ret;
	
}


int SocketHandler::GateWay_Status_Check()
{
	uint8_t pu8data[25];
	BYTE u8Checksum;
	int iBufcnt =0, ret =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)packet.PanID;
	pu8data[++iBufcnt] = packet.PanID>> 8;
	pu8data[++iBufcnt] = packet.ServerID;
	pu8data[++iBufcnt] = packet.ServerID >> 8;
	pu8data[++iBufcnt] = (BYTE)packet.GateWayID << 8;		//gateway ID
	pu8data[++iBufcnt] = packet.GateWayID >> 8;		//gateway ID
	
	pu8data[++iBufcnt] = CONNECT_ALIVE_CHECK;	//msg type
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; 

	pu8data[++iBufcnt] = packet.ext[0];
	pu8data[++iBufcnt] = packet.ext[1];
	pu8data[++iBufcnt] = packet.ext[2];

	ret = pSocket->Send_Message(pu8data, iBufcnt+1);
	
	return ret;
	
}


int SocketHandler::Server_BSN_Stop_Packet()
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;
	printf("Server_BSN_Stop_Packet()\n");
	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = (BYTE)packet.PanID;
	pu8data[++iBufcnt] = packet.PanID>> 8;
	pu8data[++iBufcnt] = (BYTE)packet.GateWayID << 8;		//gateway ID
	pu8data[++iBufcnt] = packet.GateWayID >> 8;		//gateway ID
	pu8data[++iBufcnt] = packet.ServerID;
	pu8data[++iBufcnt] = packet.ServerID >> 8;
	pu8data[++iBufcnt] = BSN_DATA_END_REQ;	//msg type
	pu8data[++iBufcnt] = 0x01;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = packet.ext[0];
	pu8data[++iBufcnt] = packet.ext[1];
	pu8data[++iBufcnt] = packet.ext[2];

	pSocket->Send_Message(pu8data, iBufcnt+1);

	return 1;
}


WORD SocketHandler::ByteToWord(BYTE puData, BYTE puData1)
{
	WORD p16Tempdata_HIGH, p16Tempdata_LOW;

	p16Tempdata_HIGH = (puData << 8);
	p16Tempdata_LOW = puData1;

	return p16Tempdata_HIGH|p16Tempdata_LOW;

}

void SocketHandler::GetPanID(WORD panid)
{
	packet.PanID = panid;
}

void SocketHandler::GetServerID(WORD severid)
{
	//packet.ServerID = severid;
	packet.ServerID = 0x06;

	printf("GetServerID %x\n", packet.ServerID);
}

void SocketHandler::SetMsg_StartCfm_Remalloc(int OnOff)
{
	if(OnOff)
		nServiceStart_Confirm =1;
	else {
		nServiceStart_Confirm =0;
		m_nTagDataCount = 0; 
		pUartQueue->m_nSendTagCount = 0;
	}
//	printf("SetMsg_StartCfm_Remalloc(%d)\n", nServiceStart_Confirm);
}

void SocketHandler::TagData(std::queue<std::vector<BYTE>> que)
{
	BYTE pu8data[1024];
	int iBufcnt =0;
	std::vector<std::vector<BYTE>> vec;

	vTagData = que;
	/*
	for(int k=m_nTagDataCount; k<pUartQueue->m_nSendTagCount; k++) {
		printf("m_nTagDataCount : %d, m_nSendTagCount :%d\n", m_nTagDataCount, pUartQueue->m_nSendTagCount);
		for(int i=0; i<(int)vTagData[k].size(); i++) {
			printf("%x ", vTagData[k][i]);
			pu8data[iBufcnt] = vTagData[k][i];
			iBufcnt++;
		}
		printf("\n");

		if(pSocket->Send_Message(pu8data, iBufcnt) > 0 ) {
			printf("Socket Write Sucess\n");
		}
		else
			printf("Socket Re-Write\n");

		iBufcnt =0;
		memset(pu8data, 0, 1024);
		m_nTagDataCount++;
	
	}
	*/
	vec.push_back(vTagData.front());
	for(int i=0; i<(int)vec.size(); i++) {
		for(int k=0; k<(int)vec[i].size(); k++) {
	//		printf("%x ", vec[i][k]);
			pu8data[iBufcnt] = vec[i][k];
			iBufcnt++;
		}
	}
	printf("\n");

	if(pSocket->Send_Message(pu8data, iBufcnt) > 0 ) {	
		vTagData.pop();
	}
	else
		printf("Socket Write Fail\n");

	iBufcnt =0;
	memset(pu8data, 0, 1024);
	
}

BYTE SocketHandler::GetChecksum(BYTE* puData, int len)
{
	BYTE sum =0;

	for(int i=1; i<= len; i++) {
	//	printf("%x ", puData[i]);
		sum += puData[i];
	}
	printf("(check nsum : %x) ", sum);

	return sum;
}

void SocketHandler::SetBeconCount(int* cnt)
{
	*cnt = beacon_count;
}

void SocketHandler::SetMsgQueueHwnd(MsgQueue* soc)
{
	pUartQueue = soc;
}


void SocketHandler::SetSocketHwnd(Socket* soc)
{
	pSocket = soc;
}

