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

int nTagNumber =0, existTagNumber =0;
int BEACON_MAX_COUNT=0;
int tempBeacon =0;


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
	m_nDataSendFail_SuccessCnt =0;
	DataSendFail_RedownCnt =0;
	m_nUartArrayDataIndicateCnt =0;
	m_nUartArrayReDataIndicateCnt =0;
	m_nThreadUartArrayDataDownCnt=0;
	m_nThreadUartArrayDataIndecateCnt =0;
	m_UartArrayDataIndicateMsg.reserve(5000);
	m_UartArrayDataDownMsg.reserve(5000);

	m_UartArrayThreadDataDownMsg.reserve(5000);
	m_UartArrayThreadDataIndecateMsg.reserve(5000);
	
}

MsgHandler::~MsgHandler()
{
	// TODO Auto-generated destructor stub
}
int MsgHandler::bClear()
{
	DataSendFail_RedownCnt =0;
	m_nDataIndiCount =0;
	m_nDataDownCount =0;	
	m_UartArrayDataDownMsg.clear();
	m_UartArrayDataIndicateMsg.clear();

	m_nThreadDataDownCount =0; 
	m_nThreadDataIndiCount =0;
	m_UartArrayThreadDataDownMsg.clear();
	m_UartArrayThreadDataIndecateMsg.clear();
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

int MsgHandler::UartPacket_ThreadDataIndicateStart()
{
	BYTE pu8data[1024];
	int iBufcnt =0, dataLength =0;

	printf("UartPacket_ThreadDataIndicateStart()  \n");

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;
	m_nThreadDataIndiCount = m_nThreadDataDownCount;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//msg type
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//Data Length
		
	/* Tag Raw Data */
	dataLength = (int)ByteToWord(pu8data[MSG_LENGTHONE], pu8data[MSG_LENGTHZERO]);
	for(int i=0; i<dataLength; i++) {
		pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];
	}

	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt]; 		//Check Sum

	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//etx0
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//etx1
	pu8data[++iBufcnt] = m_UartArrayThreadDataIndecateMsg[m_nThreadDataIndiCount][++nTagidCOunt];		//etx2

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);

	return 1;
}

int MsgHandler::UartPacket_ThreadDataDownStart()
{
	BYTE pu8data[1024];
	int iBufcnt =0;

	printf("UartPacket_ThreadDataDownStart() \n");

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;

	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//PanID
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//panID
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//TagID
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//TagID
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//msg type
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Total Data Length
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Total Data Length

	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Fragment Number
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//Flash Number


	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt]; 		//Check Sum

	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//etx0
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//etx1
	pu8data[++iBufcnt] = m_UartArrayThreadDataDownMsg[m_nThreadDataDownCount][++nTagidCOunt];		//etx2

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);

	return 1;
}

int MsgHandler::UartPacket_DataIndicateStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	BYTE pu8data[1024];
	int dataLength =0;
	int iBufcnt =0, ibeaconvalue =0;

	printf("\nDataIndicateStart() \n");

	nTagidCOunt =0;
	memset(pu8data, 0, 1024);

	ibeaconvalue = (int)u8data;
	printf("BEACON VAL : HEX[%x], DEC[%d] Tag : ", ibeaconvalue, ibeaconvalue);

	m_nDataIndiCount = m_nDataDownCount;

	if(m_nDataIndiCount >= m_nUartArrayDataIndicateCnt) {
		printf("return 0\n");
		return 0;
	}

	pu8data[iBufcnt] = STX;
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
//	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
	printf("%x ", pu8data[iBufcnt]);
	pu8data[++iBufcnt] = m_UartArrayDataIndicateMsg[m_nDataIndiCount][++nTagidCOunt];
	printf("%x ", pu8data[iBufcnt]);
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

	printf("\n");
	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);


	return 1;
}

int MsgHandler::UartPacket_DataDownStart(BYTE u8data)
{
	SENDPACKET::SOCKET_PACKET sendPacket;
	int ibeaconvalue =0;
	BYTE pu8data[1024];
	int iBufcnt =0;

	printf("DataDownStart()  %d \n", m_nUartArrayDataDownCnt);

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;
	ibeaconvalue = (int)u8data;

	if(Send_BeaconData(ibeaconvalue) == 0) {		
	//	printf("UartPacket_DataDownStart() return 0\n");
		return 0;
	}
	if(m_nDataDownCount >= m_nUartArrayDataDownCnt)  {
		printf("UartPacket_DataDownStart() m_nDataDownCount : %d , m_nUartArrayDataDownCnt : %d return 0\n", m_nDataDownCount, m_nUartArrayDataDownCnt);
		return 0;
	}
	pu8data[iBufcnt] = STX;
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//PanID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//panID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//TagID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//TagID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Server ID
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//msg type
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Total Data Length

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Fragment Number
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Flash Number


	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt]; 		//Check Sum

	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx0
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx1
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//etx2

	m_pCommUart->Uart_Write(m_pCommUart->m_uartd, pu8data, iBufcnt+1);

	return 1;
}

int MsgHandler::Send_BeaconData(BYTE ibeaconvalue)
{
	BYTE beaconcnt =0;
		
	while(BEACON_MAX_COUNT >= (int)beaconcnt) {
		
		if(ibeaconvalue == beaconcnt) {
		//	printf("Send_BeaconData () ibeaconvalue :DEC[%d], HEX[%x]\n", ibeaconvalue , ibeaconvalue);
			if((int)ibeaconvalue == BEACON_MAX_COUNT) {				
				if( ((m_nDataDownCount / 16) != 0) && (m_DataCnt > 15) ) {
					printf("m_nDataDownCount / 16 return0\n");
					return 0;
				}
				else {					
					if(m_DataCnt > 14) {
						printf("BEACON_MAX_COUNT m_DataCnt %d > 15\n", m_DataCnt);
						return 0;
					}
					else if(m_DataCnt == 0) {						
						printf("BEACON_MAX_COUNT m_DataCnt %d \n", m_DataCnt);
						GetTagNumber(0);
						if( 15 >= nTagNumber) {
							existTagNumber = nTagNumber;
							m_DataFlag =1;
							m_DataCnt++;
						}
						else {
							return 0;
						}
						
						break;
					}
					else {
						printf("BEACON_MAX_COUNT m_DataCnt %d m_nDataDownCount :%d\n", m_DataCnt, m_nDataDownCount);
						if(m_nDataDownCount >= m_nUartArrayDataDownCnt) {
							printf("m_nDataDownCount[%d] == m_nUartArrayDataDownCnt[%d]\n", m_nDataDownCount, m_nUartArrayDataDownCnt);
							m_nDataDownCount =0;
							return 0;
						}
						nTagNumber = (int)ByteToWord(m_UartArrayDataDownMsg[m_nDataDownCount].at(MSG_DADDRONE) ,m_UartArrayDataDownMsg[m_nDataDownCount].at(MSG_DADDRZERO) );
						
						if (nTagNumber == existTagNumber) {
							printf("TagNumber %d == existTagNumber %d \n", nTagNumber, existTagNumber);
							return 0;
						}
						else if (nTagNumber>15) return 0;
						
						existTagNumber= nTagNumber;
						if(!m_DataFlag  && (m_nDataDownCount > 0)) {
							m_DataFlag =1;
							m_DataCnt =0;
						}
						m_DataCnt++;
						break;
					}

				}
			}
			else {				
				if( ((nTagNumber/16) != (int)(beaconcnt)+1) && (m_DataFlag) ) {
					return 0;
				}
				else {
					if(m_DataCnt >15) {
						return 0;
					}
					else if(m_DataCnt == 0) {
						int temp3 =0;
						tempBeacon =(16*((int)(beaconcnt)+1))-1;
					
						GetTagNumber(tempBeacon);
						if( ( tempBeacon+1 > nTagNumber) || (tempBeacon+16 < nTagNumber) ) {
							printf("Beacon_TagNumber : %d, nTagNumber : %d return 0 \n", tempBeacon+1, nTagNumber);
							return 0;
						}
						
						if(m_nDataDownCount >= m_nUartArrayDataDownCnt) {
							printf("m_nDataDownCount == m_nUartArrayDataDownCnt return 0\n");
							m_nDataDownCount =0;
							return 0;
						}
						m_DataFlag =1;
						temp3 = tempBeacon+1+15;
						m_DataCnt = 16-(temp3 - nTagNumber); 
						existTagNumber = nTagNumber;
						printf(" m_DataCnt : %d , existTagNumber : %d %x, nTagNumber : %d %x\n",m_DataCnt,existTagNumber,existTagNumber, nTagNumber, nTagNumber);
						break;
			
					}
					else {
						if(m_nDataDownCount >= m_nUartArrayDataDownCnt) {
							printf("//---m_nDataDownCount >= m_nUartArrayDataDownCnt return 0\n");
							m_nDataDownCount =0;
							return 0;
						}
						nTagNumber = (int)ByteToWord(m_UartArrayDataDownMsg[m_nDataDownCount].at(MSG_DADDRONE) ,m_UartArrayDataDownMsg[m_nDataDownCount].at(MSG_DADDRZERO) );
						if( (m_nUartArrayDataDownCnt == m_nDataDownCount) || (nTagNumber == existTagNumber) ) {
							printf("m_nDataDownCount %d = m_nUartArrayDataDownCnt : %d\n", m_nDataDownCount+1, m_nUartArrayDataDownCnt);
							printf("nTagNumber %d = existTagNumber : %d\n", nTagNumber, existTagNumber);
							return 0;
						}
						if(!m_DataFlag  && (m_nDataDownCount > 0)) {
							m_DataFlag =1;
							m_DataCnt =0;
						}
						
						
						if( ( tempBeacon+1 > nTagNumber) || (tempBeacon+16 < nTagNumber) ) {
							printf("////tempBeacon+1 : %d, nTagNumber : %d return 0 \n", tempBeacon+1, nTagNumber);
							return 0;
						}
						existTagNumber = nTagNumber;
						m_DataCnt++;
						printf("  nTagNumber : %x\n", nTagNumber);
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
	int iBufcnt =0, temp =0;
	int beaconcnt =0;

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;
	ibeaconvalue = (int)u8data;
	if(DataSendFail_RedownCnt <= m_nDataSendFail_SuccessCnt) {
		return 0;
	}	
	printf("UartPacket_ReDataDownStart ()\n");
	while(BEACON_MAX_COUNT >= beaconcnt) {
		if(ibeaconvalue == beaconcnt) {
			if(ibeaconvalue == BEACON_MAX_COUNT) {
				printf("E_BEACON_BSN_NUMBER MAX %d\n", ibeaconvalue);
				
				for(int i=0; i< 15; i++) {
					temp = (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRONE) | (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO);					
					if( (i==15) || (m_nUartArrayDataDownCnt-1 < i) || (temp >15)) {
						printf("%d == 15 m_nUartArrayDataDownCnt %d temp : %d return 0\n", i, m_nUartArrayDataDownCnt, temp);
						return 0;
					}
					else if(Map_dataParityCheck[m_UartArrayDataDownMsg[i]] != PASS) {
						m_nDataDownCount = (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRONE) | (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO);
						m_nDataDownCount--;
						
					//	printf("\nReDown E_BEACON_BSN_NUMBER6:\n");
						m_nDataSendFail_SuccessCnt++;
						break;
					}
					if(i==14) {
				//		printf("i == %d\n", i);
						return 0;
					}
				}
			}
			else {
				printf("E_BEACON_BSN_NUMBER%d[%d]\n", ibeaconvalue, beaconcnt);
				int j = (16 * (beaconcnt+1))-1;
				int k=0;
				for(k=0; k<m_nUartArrayDataDownCnt; k++) {
					nTagNumber = (int)ByteToWord(m_UartArrayDataDownMsg[k].at(MSG_DADDRONE) ,m_UartArrayDataDownMsg[k].at(MSG_DADDRZERO) );
					if( (j+1 <= nTagNumber) && (j+1+15 >= nTagNumber) ) {
						if(Map_dataParityCheck[m_UartArrayDataDownMsg[k]] != PASS) {
							nTagNumber = (int)ByteToWord(m_UartArrayDataDownMsg[k].at(MSG_DADDRONE) ,m_UartArrayDataDownMsg[k].at(MSG_DADDRZERO) );
							printf("j+1 : %d, nTagNumber : %d break \n", j+1, nTagNumber);
							m_nDataDownCount = k;
							break;
						}
					}
					if (k == m_nUartArrayDataDownCnt-1) {
						printf(" k(%d)== m_nUartArrayDataDownCnt (%d)return 0\n", k, m_nUartArrayDataDownCnt-1);
						return 0;
					}
				}
				if( ( j+1 > nTagNumber) || (j+1+15 < nTagNumber) ) {
					printf("temp+1 : %d, nTagNumber : %d return 0 \n", j+1, nTagNumber);
					return 0;
				}
#if 0

				for(int i=j; i<j+16; j++) {
					if(Map_dataParityCheck[m_UartArrayDataDownMsg[k]] != PASS) {
						m_nDataDownCount = ByteToWord(m_UartArrayDataDownMsg[i][MSG_DADDRONE], m_UartArrayDataDownMsg[i][MSG_DADDRZERO]);
						m_nDataDownCount--;
						printf("************TAG ID : %d\n", m_nDataDownCount);

						m_nDataSendFail_SuccessCnt++;
						break;
					}
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				for(int i=j; i< j+16; i++) {
					temp = (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRONE) | (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO);
					if( (i==(j+16)) || (temp < i) ) {
						printf("%d != 15 temp : %d return \n", i, temp);
						return 0;
					}
					else if(Map_dataParityCheck[m_UartArrayDataDownMsg[i]] != PASS) {
						m_nDataDownCount = ByteToWord(m_UartArrayDataDownMsg[i][MSG_DADDRONE], m_UartArrayDataDownMsg[i][MSG_DADDRZERO]);
						//	(int)m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO) | (int)m_UartArrayDataDownMsg[i].at(MSG_DADDRONE);
						m_nDataDownCount--;
						printf("************TAG ID : %d\n", m_nDataDownCount);

						m_nDataSendFail_SuccessCnt++;
						break;
					}
					if(i== (j+15)) {
						return 0;
					}
				}
#endif
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
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Flash Number


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

	printf("UartPacket_ReDataIndicateStart() %d of %d \n",  m_nDataIndiCount, m_nUartArrayDataIndicateCnt);

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
	if(DataSendFail_RedownCnt <= m_nDataSendFail_SuccessCnt) {
		printf("DataSendFail_RedownCnt : %d <= m_nDataSendFail_SuccessCnt :%d\n", DataSendFail_RedownCnt,  m_nDataSendFail_SuccessCnt);
		return 0;
	}
	
	while(BEACON_MAX_COUNT >= beaconcnt) {
		if(ibeaconvalue == beaconcnt) {
			printf("ReDownStart() BEACON : DEC[%d], HEX[%x]\n", ibeaconvalue , ibeaconvalue);
			if(ibeaconvalue == BEACON_MAX_COUNT) {

				if(!Map_AcknowOverlap) {
					GetTagNumber(0);
					Map_AcknowCnt = m_nDataDownCount;
				}	
				else {					
					m_nDataDownCount++;
					Map_AcknowCnt = m_nDataDownCount;
					printf("Map_AcknowOverlap : %d, nTagNumber : %d\n", Map_AcknowOverlap, nTagNumber);
				}
				
				if (15 < nTagNumber) {
					Map_AcknowOverlap =0;
					printf("nTagNumber : %d return 0 \n", nTagNumber);
					return 0;
				}
				else {
					Map_AcknowOverlap =1;
				}
				
				if(m_nDataDownCount >= m_nUartArrayDataDownCnt) {
					m_nDataDownCount =0;
					return 0;
				}
				
				
				if(Map_AcknowCnt == 15) {
					Map_AcknowOverlap =0;
					printf("Map_AcknowOverlap : %d, nTagNumber : %d\n", Map_AcknowOverlap, Map_AcknowCnt);
					return 0;
				}
				while(Map_AcknowCnt < 15) {			
				//	printf(" Total Send data %d, Map_AcknowCnt : %d\n", m_nUartArrayDataDownCnt, Map_AcknowCnt);
					if(Map_u16AcknowParityCheck[m_pu16MsgDataAcknowledge[Map_AcknowCnt]] == FAIL) {
			
						m_nDataDownCount = Map_AcknowCnt;

				//		printf("DataDown Tag Number [%x] : %d\n", Map_AcknowCnt+1, Map_u16AcknowParityCheck[m_pu16MsgDataAcknowledge[Map_AcknowCnt]]);
						printf("\nReDown BEACON_NUMBER %d TagNumber : %d, %x\n", ibeaconvalue, m_pu16MsgDataAcknowledge[Map_AcknowCnt-1]+1, m_pu16MsgDataAcknowledge[Map_AcknowCnt-1]+1);

						m_nDataSendFail_SuccessCnt++;
						Map_AcknowCnt++;

						beaconcnt = BEACON_MAX_COUNT;
						break;
					}
					else if(Map_u16AcknowParityCheck[m_pu16MsgDataAcknowledge[Map_AcknowCnt]] == PASS) {
					//	Map_AcknowOverlap =0;
					}
					if(Map_AcknowCnt>=14) {
						Map_AcknowOverlap =0;
						printf("Map_AcknowOverlap : %d, Map_AcknowCnt == %d return 0\n", Map_AcknowOverlap, Map_AcknowCnt);
						return 0;
					}
					Map_AcknowCnt++;
				}
			}
			else {
				int j = (16 * (beaconcnt+1))-1;
				int jend = j+16;
			
				if(!Map_AcknowOverlap2) {
					GetTagNumber(j);
					Map_AcknowCnt2 = m_nDataDownCount;
				}
				else {					
					m_nDataDownCount++;
					Map_AcknowCnt2 = m_nDataDownCount;
					printf("Map_AcknowOverlap2 : %d, nTagNumber : %d\n", Map_AcknowOverlap, nTagNumber);
				}
				
				if( ( j+1 > nTagNumber) || (j+16 < nTagNumber) ) {
					printf("j+1 : %d, nTagNumber : %d return 0 \n", j+1, nTagNumber);
					return 0;
				}
				else {
					Map_AcknowOverlap2=1;
				}
				
				if(m_nDataDownCount >= m_nUartArrayDataDownCnt) {
					printf("m_nDataDownCount >= m_nUartArrayDataDownCnt return 0\n");
					m_nDataDownCount =0;
					return 0;
				}
				
				if(Map_AcknowCnt2 == jend) {
					Map_AcknowOverlap2 =0;
					printf("Map_AcknowOverlap2 : %d, nTagNumber : %d\n", Map_AcknowOverlap, nTagNumber);
					return 0;
				}
				while(Map_AcknowCnt2 < jend) {
					if(Map_u16AcknowParityCheck[m_pu16MsgDataAcknowledge[Map_AcknowCnt2]] != PASS) {					

						if( ((j+1) > (int)m_pu16MsgDataAcknowledge[Map_AcknowCnt2-1]) ||((j+16 )< (int)m_pu16MsgDataAcknowledge[Map_AcknowCnt2-1]) )
							return 0;

						m_nDataDownCount = Map_AcknowCnt2;

						printf("\nReDown E_BEACON_BSN_NUMBER %d m_nDataDownCount : %d, TagNumber : %d, %x\n", ibeaconvalue, m_nDataDownCount, m_pu16MsgDataAcknowledge[Map_AcknowCnt2-1]+1, m_pu16MsgDataAcknowledge[Map_AcknowCnt2-1]+1);

						
						m_nDataSendFail_SuccessCnt++;
						Map_AcknowCnt2++;

						beaconcnt = BEACON_MAX_COUNT;
						break;
					}
					
					if(Map_AcknowCnt2 >= jend-1) {
						Map_AcknowOverlap2 =0;
						printf("Map_AcknowOverlap2 : %d\n", Map_AcknowOverlap);
						return 0;
					}
					Map_AcknowCnt2++;
				}
			}
		}

		beaconcnt++;
	}

	if (m_nUartArrayDataDownCnt-1 < m_nDataDownCount)  {
	//	printf("%d < %d return 000\n", m_nUartArrayDataDownCnt-1, m_nDataDownCount);
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
	pu8data[++iBufcnt] = m_UartArrayDataDownMsg[m_nDataDownCount][++nTagidCOunt];		//Flash Number


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

	printf("REDataIndicateStart()\n");

	memset(pu8data, 0, 1024);
	nTagidCOunt =0;

	m_nDataIndiCount = m_nDataDownCount;

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
	case POWEROFF_REQ:
		printf("POWEROFF_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	case DISPLAY_ENABLE_REQ:
		printf("DISPLAY_ENABLE_REQ UART_WRITE\n");
		m_pCommUart->Uart_Write(m_pCommUart->m_uartd, p8Data, DataLen);
		break;
	}
	//printf("BypassSocketToUart() End\n");
}

void MsgHandler::SetSocketArray(std::vector<std::vector<BYTE>> DataDownmsg, std::vector<std::vector<BYTE>> DataIndimsg)
{	
	m_UartArrayDataIndicateMsg.reserve(5000);
	m_UartArrayDataDownMsg.reserve(5000);
	
	m_UartArrayDataDownMsg = DataDownmsg;
	m_UartArrayDataIndicateMsg = DataIndimsg;
//	printf("SetSocketArray() Current m_UartArrayDataDownMsg Size : %d, m_UartArrayDataIndicateMsg Size : %d\n", m_UartArrayDataDownMsg.size(), m_UartArrayDataIndicateMsg.size());
	m_UartArrayDataDownMsg.shrink_to_fit();
	m_UartArrayDataIndicateMsg.shrink_to_fit();
	
	m_nUartArrayDataDownCnt = (int)DataDownmsg.size();
	m_nUartArrayDataIndicateCnt = (int)DataIndimsg.size();

	printf("Beacon DataDown Count : %d\n", m_nUartArrayDataDownCnt);	
}

int MsgHandler::DataFlag_Initialize(int beacon)
{
	
	m_DataFlag =0;
	m_DataCnt =0;
	Map_AcknowCnt =0;
	Map_AcknowCnt2 = (16 * ((int)(beacon)+1))-1;
	Map_AcknowOverlap =0;
	Map_AcknowOverlap2 =0;

	return 1;
}

int MsgHandler::GetTagNumber(int temp)
{
	int nTemptemp =0;
	std::set<WORD>::iterator iter;

	if(temp == 0) 
		nTemptemp = temp+15;
	else
		nTemptemp = temp+16;
 
	for(int i=0; i<m_nUartArrayDataDownCnt; i++) {
		nTagNumber = (int)ByteToWord(m_UartArrayDataDownMsg[i].at(MSG_DADDRONE) ,m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO) );
		if( (temp+1 <= nTagNumber) && (nTemptemp >= nTagNumber)) {
			nTagNumber = (int)ByteToWord(m_UartArrayDataDownMsg[i].at(MSG_DADDRONE) ,m_UartArrayDataDownMsg[i].at(MSG_DADDRZERO) );
		
			if(existTagNumber == nTagNumber)
				continue;
			else {				
				m_nDataDownCount = i;
				printf("TagNumber : %d, m_nDataDownCount : %d break \n",  nTagNumber, m_nDataDownCount);			
				break;
			}
			
		}							
	}

	return 1;

}
void MsgHandler::SetBeacon(int a)
{
	BEACON_MAX_COUNT = a;
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
