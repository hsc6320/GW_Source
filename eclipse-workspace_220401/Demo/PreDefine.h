/*
 * PreDefine.h
 *
 *  Created on: 2021. 12. 3.
 *      Author: hong
 */

#ifndef PREDEFINE_H_
#define PREDEFINE_H_

#include "pch.h"


/********************************************************************************************************************
 *
 * Message Format
 STX		PANID		DADDR		SADDR		M Type		Length		Data		checksum		ETX
 1BYTE		2BYTE		2BYTE		2BYTE		1BYTE		2BYTE					 1BYTE

************************************************************************************************************************/

namespace PRE_DEFINE {

#pragma pack(push, 1)
	typedef struct _S_HEADER {
		BYTE	stx;
		WORD	panID;
		WORD	dAddr;
		WORD	sAddr;
		BYTE	type;
		WORD	length;
		int		DataLength;
		_S_HEADER() {stx = 0xAA;}
	}S_HEADER, *PS_HEADER;

	typedef struct _S_TAIL {
		BYTE	checksum;
		BYTE	ext[3];
	}S_TAIL, *PS_TAIL;


	typedef struct _PACKET {
		S_HEADER		header;
		S_TAIL			tail;
		BYTE* 			pu8Data;
	}S_PACKET, *PS_PACKET;
#pragma pack(pop)
}


using namespace PRE_DEFINE;

#endif /* PREDEFINE_H_ */
