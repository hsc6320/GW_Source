/*
 * MsgHandler.cpp
 *
 *  Created on: 2021. 11. 30.
 *      Author: hong
 */

#include "MsgHandler.h"
#include <cmath>

int nTagidCOunt =0;
int iBeaconFlag =0;
int iBeaconFlag2 =0;
int CheckbsnFlag =0;
int Map_AcknowOverlap =0;
int Map_AcknowOverlap2 =0;

MsgHandler::MsgHandler()
{
	m_pCommUart = NULL;
	Beacon_Max =0;
	Map_AcknowCnt =0;
	Map_AcknowCnt2 =0;
	m_nDataDownCount =0;
	m_nDataIndiCount =0;
	m_nUartArrayDataDownCnt =0;
	m_DataCnt =0;
	m_DataFlag =0;
	m_nDownloadedSuccessCnt =0;
	m_nReDownCnt =0;
	m_nUartArrayDataIndicateCnt =0;
	m_nUartArrayReDataIndicateCnt =0;

	memset(m_GetDownTagID, 0, 512);
}

MsgHandler::~MsgHandler()
{
	// TODO Auto-generated destructor stub
}
int MsgHandler::bClear()
{
	m_nDataIndiCount =0;
	m_nReDownCnt =0;
	m_nDataDownCount =0;
	m_UartArrayDataDownMsg.clear();
	m_UartArrayDataIndicateMsg.clear();
	Map_AcknowParityCheck.clear();
	vMsgDataAcknowledge.clear();
	Map_dataParityCheck.clear();
	return 1;
}

int MsgHandler::ServiceStart()
{
	int ret =0;
	uint8_t pu8data[15] = {0xaa, 0, 0xfc, 0x01, 0, 0x01, 0, 0x05, 0x01, 0, 0, 0x04, 0xa5, 0x5a, 0x7e};
	printf("ServiceStart\n");
	ret = m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, 16);

	return ret;
}

void MsgHandler::ServiceIdle()
{
	uint8_t pu8data[14] = {0xaa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x10, 0, 0, 0x0a, 0xa5, 0x5a, 0x7e};
	printf("ServiceIdle\n");
	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, 14);
}

int MsgHandler::BSN_Start_Packet(/*BYTE* u8data*/)
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;
	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = CommonPacket.PanID << 8;
	pu8data[++iBufcnt] = CommonPacket.PanID >> 8;
	pu8data[++iBufcnt] = CommonPacket.ServerID << 8;
	pu8data[++iBufcnt] = CommonPacket.ServerID >> 8;
	pu8data[++iBufcnt] = CommonPacket.GateWayID << 8;		//gateway ID
	pu8data[++iBufcnt] = CommonPacket.GateWayID >> 8;		//gateway ID
	pu8data[++iBufcnt] = BSN_START;	//msg type
	pu8data[++iBufcnt] = 0x01;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = CommonPacket.ext[0];
	pu8data[++iBufcnt] = CommonPacket.ext[1];
	pu8data[++iBufcnt] = CommonPacket.ext[2];

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);

	return 1;
}

int MsgHandler::BSN_Stop_Packet()
{
	BYTE pu8data[30];
	BYTE u8Checksum;
	int iBufcnt =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = CommonPacket.PanID << 8;
	pu8data[++iBufcnt] = CommonPacket.PanID >> 8;
	pu8data[++iBufcnt] = CommonPacket.GateWayID << 8;		//gateway ID
	pu8data[++iBufcnt] = CommonPacket.GateWayID >> 8;		//gateway ID
	pu8data[++iBufcnt] = CommonPacket.ServerID;
	pu8data[++iBufcnt] = CommonPacket.ServerID >> 8;
	pu8data[++iBufcnt] = BSN_DATA_END_REQ;	//msg type
	pu8data[++iBufcnt] = 0x01;
	pu8data[++iBufcnt] = 0;
	pu8data[++iBufcnt] = 0;

	u8Checksum = GetChecksum(pu8data, iBufcnt);
	pu8data[++iBufcnt] = u8Checksum; //0x07;

	pu8data[++iBufcnt] = CommonPacket.ext[0];
	pu8data[++iBufcnt] = CommonPacket.ext[1];
	pu8data[++iBufcnt] = CommonPacket.ext[2];

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);

	return 1;
}

int MsgHandler::UartPacket_DataIndicateStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	BYTE pu8data[1024];
	int dataLength =0;
	int iBufcnt =0, ibeaconvalue =0;

	printf("\nUartPacket_DataIndicateStart() %d %d \n", m_nUartArrayDataIndicateCnt, m_nDataIndiCount);

	nTagidCOunt =0;
	memset(pu8data, 0, 1024);

	ibeaconvalue = (int)u8data;
	printf("ibeaconvalue : %d\n", ibeaconvalue);

	m_nDataIndiCount = m_nDataDownCount;

	printf("m_nDataIndiCount : %d\n", m_nDataIndiCount);
	if(m_nDataIndiCount >= m_nUartArrayDataIndicateCnt) {
		printf("return 0\n");
		return 0;
	}

	pu8data[iBufcnt] = STX;
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Server ID
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Server ID
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//msg type
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Data Length
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Data Length
	printf("%x ", pu8data[iBufcnt]);
	/* Tag Raw Data */
	dataLength = (int)ByteToWord(pu8data[MSG_LENGTHONE], pu8data[MSG_LENGTHZERO]);
	for(int i=0; i<dataLength; i++) {
		pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
		printf("%x ", pu8data[iBufcnt]);
	}

	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt]; 		//Check Sum
	printf("%x ", pu8data[iBufcnt]);

	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx0
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx1
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx2
	printf("%x \n\n", pu8data[iBufcnt]);

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);


	return 1;
}

int MsgHandler::UartPacket_DataDownStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	int ibeaconvalue =0;
	BYTE pu8data[1024];
	int iBufcnt =0;

	printf("UartPacket_DataDownStart() %d %d\n", m_nUartArrayDataDownCnt, m_nDataDownCount);


	memset(pu8data, 0, 1024);
	nTagidCOunt =0;
	ibeaconvalue = (int)u8data;

	if( (m_nUartArrayDataDownCnt <= 10) && (m_nUartArrayDataDownCnt != m_nDataDownCount) ) {
		if(m_DataCnt >= m_nUartArrayDataDownCnt) {
			m_nUartArrayDataDownCnt = m_DataCnt;
			printf("m_DataCnt : %d return 1\n", m_DataCnt);
			return 1;
		}
		if(m_DataCnt == 0) {
			m_nDataDownCount = 0;
			m_DataFlag =1;
			m_DataCnt++;
		}
		else {
			if(!m_DataFlag  && (m_nDataDownCount > 0)) {
				m_DataFlag =1;
				m_DataCnt =0;
			}
			m_DataCnt++;
			printf("m_DataCnt : [%d] \n", m_DataCnt);
		}
	}
	else if(m_nUartArrayDataDownCnt > 10) {
		if(Send_BeaconData(ibeaconvalue) == 0) {
			printf("Send_BeaconData() return 0\n");
			return 0;
		}
	}


	printf("m_nDataDownCount : %d\n", m_nDataDownCount);
	if(m_nDataDownCount >= m_nUartArrayDataDownCnt) {
		printf("UartPacket_DataDownStart()return 0\n");
		return 0;
	}
	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//msg type
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Fragment Number


	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt]; 		//Check Sum

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx0
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx1
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx2

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);

	return 1;
}

int MsgHandler::Send_BeaconData(int ibeaconvalue)
{
	int beaconcnt =0;

	while(BEACON_MAX >= beaconcnt) {
		if(ibeaconvalue == beaconcnt) {
			if(ibeaconvalue == BEACON_MAX) {
				printf("%d == %d BEACON MAX\n", ibeaconvalue, BEACON_MAX);
				if( ((m_nDataDownCount / 16) != 0) && (m_DataCnt > 15) ) {
					printf("m_nDataDownCount / 16 return0\n");
					return 0;
				}
				else {
					if(m_DataCnt >= 15) {
						printf("m_DataCnt %d > 15\n", m_DataCnt);
						return 0;
					}
					else if(m_DataCnt == 0) {
						m_nDataDownCount = 0;
						m_DataFlag =1;
						m_DataCnt++;
					//	printf("Send_BeaconData() m_nDataDownCount : %d break; \n", m_nDataDownCount);
						break;
					}
					else {
						if(!m_DataFlag  && (m_nDataDownCount > 0)) {
							m_DataFlag =1;
							m_DataCnt =0;
						}
						m_DataCnt++;
						printf("m_DataCnt : [%d] \n", m_DataCnt);
						break;
					}

				}
			}
			else {
				if( ((m_nDataDownCount/16) != beaconcnt+1) && (m_DataFlag) ) {
					printf("m_nDataDownCount / 16 m_DataFlag return0\n");
					return 0;
				}
				else {
					if(m_DataCnt > 15) {
						printf("m_DataCnt %d > 15\n", m_DataCnt);
						return 0;
					}
					else if(m_DataCnt == 0) {
						m_nDataDownCount = (16 * (beaconcnt+1))-1;
						m_DataFlag =1;
						m_DataCnt++;
					//	printf("m_nDataDownCount: %d[beacon : %d] \n", m_nDataDownCount, ibeaconvalue);
						break;
					}
					else {
						if(!m_DataFlag  && (m_nDataDownCount > 0)) {
							m_DataFlag =1;
							m_DataCnt =0;
						}
						m_DataCnt++;
						printf("m_DataCnt : [%d] \n", m_DataCnt);
						break;
					}
				}
			}
		}
		beaconcnt++;
	}

	return 1;
}

int MsgHandler::UartPacket_ReDataDownStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	int ibeaconvalue =0;
	BYTE pu8data[1024];
	int iBufcnt =0;
	int beaconcnt =0;

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;
	ibeaconvalue = (int)u8data;
	printf("UartPacket_ReDataDownStart ()\n");
	if(m_nReDownCnt <= m_nDownloadedSuccessCnt) {
		printf("m_nReDownCnt <= m_nDownloadedSuccessCnt\n");
		return 0;
	}
	while(BEACON_MAX >= beaconcnt) {
		if(ibeaconvalue == beaconcnt) {
			if(ibeaconvalue == BEACON_MAX) {
				printf("E_BEACON_BSN_NUMBER%d\n", ibeaconvalue);
				for(int i=0; i< 15; i++) {
					if( (i==15) || (m_nUartArrayDataDownCnt-1 < i) ) {
						printf("%d == 15 m_nUartArrayDataDownCnt %d return 0\n", i, m_nUartArrayDataDownCnt);
						return 0;
					}
					else if(Map_dataParityCheck[m_UartArrayDataDownMsg[i]] != PASS) {
						m_nDataDownCount = (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO);
						m_nDataDownCount--;

					//	printf("\nReDown E_BEACON_BSN_NUMBER6:\n");
						m_nDownloadedSuccessCnt++;
						break;
					}
					if(i==14) {
				//		printf("i == %d\n", i);
						return 0;
					}
				}
			}
			else {
				printf("E_BEACON_BSN_NUMBER%d\n", ibeaconvalue);
				int j = (16 * (beaconcnt+1))-1;
				for(int i=j; i< j+16; i++) {
					if( (i==(j+16)) || (m_nUartArrayDataDownCnt-1 < i) ) {
						printf("%d == 15 m_nUartArrayDataDownCnt %d return 0\n", i, m_nUartArrayDataDownCnt);
						return 0;
					}
					else if(Map_dataParityCheck[m_UartArrayDataDownMsg[i]] != PASS) {
						m_nDataDownCount = (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO);
						m_nDataDownCount--;

					//	printf("\nReDown E_BEACON_BSN_NUMBER%d:\n", ibeaconvalue);
						m_nDownloadedSuccessCnt++;
						break;
					}
					if(i== (j+15)) {
					//	printf("i == %d\n", i);
						return 0;
					}
				}
			}
		}
		beaconcnt++;
	}

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//msg type
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Fragment Number


	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt]; 		//Check Sum

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx0
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx1
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx2

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);
	return 1;
}

int MsgHandler::UartPacket_ReDataIndicateStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	BYTE pu8data[1024];
	int dataLength =0;
	int iBufcnt =0;

	printf("UartPacket_ReDataIndicateStart() %d %d \n", m_nUartArrayDataIndicateCnt, m_nDataIndiCount);

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;

	m_nDataIndiCount = m_nDataDownCount;
	printf("m_nDataIndiCount : %d\n", m_nDataIndiCount);

	pu8data[iBufcnt] = STX;
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Server ID
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Server ID
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//msg type
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Data Length
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Data Length
//	printf("%x ", pu8data[iBufcnt]);
	/* Tag Raw Data */
	dataLength = (int)ByteToWord(pu8data[MSG_LENGTHONE], pu8data[MSG_LENGTHZERO]);
	for(int i=0; i<dataLength; i++) {
		pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//		printf("%x ", pu8data[iBufcnt]);
	}

	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt]; 		//Check Sum
//	printf("%x ", pu8data[iBufcnt]);

	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx0
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx1
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx2
//	printf("%x \n\n", pu8data[iBufcnt]);

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);


	return 1;
}

int MsgHandler::UartPacket_ReDataAcknowledge_DownStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	int ibeaconvalue =0;
	BYTE pu8data[1024];
	int iBufcnt =0;
	int beaconcnt =0;
	memset(pu8data, 0, 1024);
	nTagidCOunt =0;
	ibeaconvalue = (int)u8data;
	printf("UartPacket_ReDataAcknowledge_DownStart ()\n");
	if(m_nReDownCnt <= m_nDownloadedSuccessCnt) {
		printf("m_nReDownCnt <= m_nDownloadedSuccessCnt\n");
		return 0;
	}

	while(BEACON_MAX >= beaconcnt) {
		if(ibeaconvalue == beaconcnt) {
			if(ibeaconvalue == BEACON_MAX) {
				if(Map_AcknowCnt == 15) {
					Map_AcknowOverlap =0;
					return 0;
				}
				printf("E_BEACON_BSN_NUMBER%d MAX\n", ibeaconvalue);
				while(Map_AcknowCnt < 15) {
					if( (Map_AcknowCnt==15) || (m_nUartArrayDataDownCnt-1 < Map_AcknowCnt) ) {
						Map_AcknowOverlap =0;
						printf("%d == 15 m_nUartArrayDataDownCnt %d return 0\n", Map_AcknowCnt, m_nUartArrayDataDownCnt);
						return 0;
					}
					else if(Map_AcknowParityCheck[vMsgDataAcknowledge[Map_AcknowCnt]] == FAIL) {
						if( (Map_AcknowCnt != 0) && (Map_AcknowOverlap == Map_AcknowCnt) ) { printf("Map_AcknowOverlap : %d Map_AcknowCnt : %d\n", Map_AcknowOverlap, Map_AcknowCnt); return 0; }
				//		m_nDataDownCount = (int)vMsgDataAcknowledge[Map_AcknowCnt].at(0);
				//		m_nDataDownCount--;
						m_nDataDownCount = Map_AcknowCnt;

						printf("vMsgDataAcknowledge[Map_AcknowCnt(%d}].at(0) : %d\n", Map_AcknowCnt, vMsgDataAcknowledge[Map_AcknowCnt].at(0));
						printf("\nReDown E_BEACON_BSN_NUMBER%d: Map_AcknowCnt : %d m_nDataDownCount : %d\n", ibeaconvalue, Map_AcknowCnt, m_nDataDownCount);

						m_nDownloadedSuccessCnt++;
						Map_AcknowOverlap = Map_AcknowCnt;
						Map_AcknowCnt++;

						beaconcnt = BEACON_MAX;
						break;
					}
					else if(Map_AcknowParityCheck[vMsgDataAcknowledge[Map_AcknowCnt]] == PASS) {
						Map_AcknowOverlap =0;
					}
					if(Map_AcknowCnt>=15) {
						Map_AcknowOverlap =0;
					//	printf("i == %d\n", i);
						return 0;
					}
					Map_AcknowCnt++;
				}
			}
			else {
				int j = (16 * (beaconcnt+1))-1;
				int jend = j+16;
				if(Map_AcknowCnt2 == jend) {
					Map_AcknowOverlap2 =0;
					return 0;
				}

			//	printf("E_BEACON_BSN_NUMBER%d\n", ibeaconvalue);
			//	printf(":Map_AcknowCnt2 : %d\n", Map_AcknowCnt2);

				while(Map_AcknowCnt2 < jend) {
				//	printf("Map_AcknowCnt2 : %d\n", Map_AcknowCnt2);
					if( (Map_AcknowCnt2==jend) || (m_nUartArrayDataDownCnt-1 < Map_AcknowCnt2) ) {
						Map_AcknowOverlap2 = 0;
						printf("%d == 15 m_nUartArrayDataDownCnt %d return 0\n", Map_AcknowCnt2, m_nUartArrayDataDownCnt);
						return 0;
					}
					else if(Map_AcknowParityCheck[vMsgDataAcknowledge[Map_AcknowCnt2]] != PASS) {
						if(Map_AcknowOverlap2 == Map_AcknowCnt2) { printf("Map_AcknowOverlap2\n"); return 0; }

					/*	if((int)vMsgDataAcknowledge[Map_AcknowCnt2].at(0) > 0) {
							m_nDataDownCount--;
						}
						else {
							m_nDataDownCount = (int)vMsgDataAcknowledge[Map_AcknowCnt2].at(0);
						}*/

			//			m_nDataDownCount = (int)vMsgDataAcknowledge[Map_AcknowCnt2].at(0);
			//			m_nDataDownCount--;
						m_nDataDownCount = Map_AcknowCnt2;

						printf("\nReDown E_BEACON_BSN_NUMBER%d: Map_AcknowCnt2 %d m_nDataDownCount : %d\n", ibeaconvalue, Map_AcknowCnt2, m_nDataDownCount);
						m_nDownloadedSuccessCnt++;
						Map_AcknowOverlap2 = Map_AcknowCnt2;
						Map_AcknowCnt2++;

						beaconcnt = BEACON_MAX;
						break;
					}
					else if(Map_AcknowParityCheck[vMsgDataAcknowledge[Map_AcknowCnt2]] == PASS) {
						Map_AcknowOverlap2 =0;
					}
					if(Map_AcknowCnt2 >= jend-1) {
						printf("Map_AcknowCnt2== jend-1\n");
						Map_AcknowOverlap2 =0;
						return 0;
					}
					Map_AcknowCnt2++;
				}
			}
		}
		beaconcnt++;
	}

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//msg type
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Fragment Number


	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt]; 		//Check Sum

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx0
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx1
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx2

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);
	return 1;
}

int MsgHandler::UartPacket_ReDataAcknowledge_DataIndicateStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	BYTE pu8data[1024];
	int dataLength =0;
	int iBufcnt =0;

	printf("UartPacket_ReDataAcknowledge_DataIndicateStart() %d %d \n", m_nUartArrayDataIndicateCnt, m_nDataIndiCount);

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;

	m_nDataIndiCount = m_nDataDownCount;
	printf("m_nDataIndiCount : %d\n", m_nDataIndiCount);

	pu8data[iBufcnt] = STX;
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Server ID
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Server ID
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//msg type
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Data Length
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//Data Length
//	printf("%x ", pu8data[iBufcnt]);
	/* Tag Raw Data */
	dataLength = (int)ByteToWord(pu8data[MSG_LENGTHONE], pu8data[MSG_LENGTHZERO]);
	for(int i=0; i<dataLength; i++) {
		pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//		printf("%x ", pu8data[iBufcnt]);
	}

	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt]; 		//Check Sum
//	printf("%x ", pu8data[iBufcnt]);

	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx0
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx1
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];		//etx2
//	printf("%x \n\n", pu8data[iBufcnt]);

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);

	return 1;
}

void MsgHandler::BypassSocketToUart(BYTE* p8Data, int DataLen, int msgtype)
{
	//printf("BypassSocketToUart() msgtype: %x, DataLen: %d \n", msgtype, DataLen);

	switch(msgtype)
	{
	case SERVICESTART_REQUEST:
		printf("SERVICESTART_REQUEST UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case DOWNLOAD_START_REQ:
		printf("DOWNLOAD_START_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case DATAINDICATION_REQ:
		printf("DATAINDICATION_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case TAG_INFOR_UPDATE_REQ:
		printf("TAG_INFOR_UPDATE_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case COORDINATOR_RESET_REQ:
		printf("COORDINATOR_RESET_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case BSN_START:
		printf("BSN_START UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case BSN_DATA_END_REQ:
		printf("BSN_DATA_END_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case TAG_ALARM_INDICATION:
		printf("TAG_ALARM_INDICATION UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case MULTI_GATEWAY_SCAN_REQ:
		printf("MULTI_GATEWAY_SCAN_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case MULTI_GATEWAY_SCAN_CONFIRM:
		printf("MULTI_GATEWAY_SCAN_CONFIRM UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case MULTI_GATEWAY_SCAN_RESPONESE:
		printf("MULTI_GATEWAY_SCAN_RESPONESE UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	}
	//printf("BypassSocketToUart() End\n");
}
void MsgHandler::SetMapValueArray(std::vector<std::vector<BYTE>> vec, int Len)
{
	printf("SetMapValueArray() m_nUartArrayDataDownCnt : %d\n", m_nUartArrayDataDownCnt);
	for(int i=0; i<m_nUartArrayDataDownCnt; i++) {
		for(int j=0; j<(int)vMsgDataAcknowledge[i].size(); j++) {
			printf("%x " , vMsgDataAcknowledge[i].at(j));
		}
	}
	printf("\n");
	//vMsgDataAcknowledge.clear();
}

void MsgHandler::SetSocketArray(std::vector<std::vector<BYTE>> DataDownmsg, std::vector<std::vector<BYTE>> DataIndimsg, int Downcnt, int Indicnt, std::map<std::vector<BYTE>, int > MapAck)
{
	printf("SetSocketArray()\n");
	m_UartArrayDataDownMsg = DataDownmsg;
	m_UartArrayDataIndicateMsg = DataIndimsg;

	m_nUartArrayDataDownCnt = Downcnt;
	m_nUartArrayDataIndicateCnt = Indicnt;
	Map_AcknowParityCheck = MapAck;
/*
	if(ceil(m_nUartArrayDataDownCnt%16) ) {
		BEACON_MAX = m_nUartArrayDataDownCnt/16 +1;
	}
	else {
		Beacon_Max = m_nUartArrayDataDownCnt/16;
	}
*/

#if 1
	printf("m_nUartArrayDataDownCnt : %d\n", m_nUartArrayDataDownCnt);
	for(int i =0; i < m_nUartArrayDataDownCnt; i++) {
		printf("<<<<<DataDown Size : [%d]>>>>>>\n",m_UartArrayDataDownMsg[i].size());
	}
/*
	for(int i =0; i < m_nUartArrayDataIndicateCnt; i++) {
		printf("<<<<<DataIndi Size : [%d]>>>>>>\n\n",m_UartArrayDataIndicateMsg[i].size());
	}

	printf("\nMsgHandler m_nSocketArrayDataDownCnt : %d, m_SocketQueue_vec Size : %d\n", m_nUartArrayDataDownCnt, (int)m_UartArrayDataDownMsg[0].size());
*/
/*	for(int i=0; i < m_nUartArrayDataDownCnt; i++)
	{
		for(int j=0; j<(int)m_UartArrayDataDownMsg[i].size(); j++) {
			printf("%x ", m_UartArrayDataDownMsg[i].at(j));
		}
		printf("\n");
	}

	for(int i=0; i < m_nUartArrayDataIndicateCnt; i++)
	{
		for(int j=0; j<(int)m_UartArrayDataIndicateMsg[i].size(); j++) {
			printf("%x ", m_UartArrayDataIndicateMsg[i].at(j));
		}
		printf("\n");
	}*/
#endif
	BSN_Start_Packet();

}

BYTE MsgHandler::GetChecksum(BYTE* puData, int len)
{
	BYTE sum =0;

	for(int i=1; i< len; i++) {
	//	printf("%x ", puData[i]);
		sum += puData[i];
	}
	//printf("(check nsum : %x) ", sum);

	return sum;
}

void MsgHandler::GetPanID(WORD panid)
{
	CommonPacket.PanID = panid;
	CommonPacket.GateWayID = panid;
}

void MsgHandler::GetServerID(WORD severid)
{
	CommonPacket.ServerID = severid;
}

WORD MsgHandler::ByteToWord(BYTE puData, BYTE puData1)
{
	WORD p16Tempdata_HIGH, p16Tempdata_LOW;

	p16Tempdata_HIGH = (puData << 8);
	p16Tempdata_LOW = puData1;

	return p16Tempdata_HIGH|p16Tempdata_LOW;
}

void MsgHandler::SetHandle(UartComThread* msg)
{
	m_pCommUart = msg;
}
