/*
 * Msg_Define.h
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */

#ifndef MSG_DEFINE_H_
#define MSG_DEFINE_H_

typedef uint8_t	BYTE;
typedef uint16_t WORD;


#define MSG_HEADER		10
#define MSG_TAIL		3
#define STX	0xAA
#define MSG_STX 		0
#define MSG_PANIDZERO 			1
#define MSG_PANIDONE 			2
#define MSG_DADDRZERO			3
#define MSG_DADDRONE				4
#define MSG_SADDRZERO			5
#define MSG_SADDRONE				6
#define MSGTYPE					7
#define MSG_LENGTHZERO			8
#define MSG_LENGTHONE			9
#define MSG_DATA					10
#define MSG_ACKNOWLEDGE_STATUS		10
#define MSG_TAG_DIRECT_CHANGE_STATUS		10
#define MSG_CFM_DATAINDICATE_STATUS		11
#define MSG_BSN_DATA				10
#define MSG_ASSOCIATION_STATUS 	19


#define REGISTRATION_REQUEST				0x01
#define REGISTRATION_CONFIRM				0x02
#define CONNECT_REQUEST						0x03
#define CONNECT_CONFIRM						0x04
#define SERVICESTART_REQUEST  				0x05
#define SERVICESTART_CONFIRM 				0x06
#define SERVICESTOP_REQUEST					0x07
#define SERVICESTOP_CONFIRM					0x08
#define GATEWAYID_REQUEST					0x10
#define GATEWAYID_RESPONSE					0x11
#define TAG_ASSOCIATION						0x21
#define DOWNLOAD_START_REQ					0x31
#define DOWNLOAD_START_ACK					0x32
#define DATAINDICATION_REQ					0x41
#define DATAINDICATION_ACK					0x42
#define DATA_ACKNOWLEDGEMENT				0x43
#define TAG_INFOR_UPDATE						0x44
#define TAG_INFOR_UPDATE_REQ				0x45
#define TAG_INFOR_UPDATE_ACK				0x46
#define TAG_DIRECT_UPDATE_REQ				0x51
#define TAG_DIRECT_UPDATE_ACK				0x52
#define COORDINATOR_RESET_REQ				0x60
#define COORDINATOR_RESET_CONFIRM			0x61
#define BSN_START								0x71
#define BSN_START_ACK						0x72
#define BSN_DATA_END_REQ						0x73
#define BSN_DATA_END_ACK						0x74
#define TAG_ALARM_INDICATION				0x80
#define TAG_LOWBATT_ALARM_INDICATION		0x82
#define TAG_DIRECT_CHANGE_INDICATION		0x84
#define TAG_POWEROFF_INDICATION				0x86

#define CHECKSUM_ERR							0x90
#define BARCODE_REQ							0x91
#define MULTI_GATEWAY_SCAN_REQ				0xA0
#define MULTI_GATEWAY_SCAN_CONFIRM			0xA1
#define MULTI_GATEWAY_SCAN_RESPONESE		0xA2
#define CONNECT_ALIVE_CHECK					0x0D
#define CONNECT_SOCKET_ALIVE_CHECK			0x0E
#define POWEROFF_REQ						0xE0
#define POWEROFF_ACK						0xE1
#define DISPLAY_ENABLE_REQ					0xF0
#define DISPLAY_ENABLE_ACK					0xF1





#define PAYLOAD_STATUS_SUCCESS		0x01
#define PAYLOAD_STATUS_FAIL			0x02

#define ENABLE						0x00
#define DISABLE						0x01


enum E_MAC_ADDRESS : BYTE {
	E_MAC_ADDRESS1 = 10,
	E_MAC_ADDRESS2,
	E_MAC_ADDRESS3,
	E_MAC_ADDRESS4,
	E_MAC_ADDRESS5,
	E_MAC_ADDRESS6,
	E_MAC_ADDRESS7,
	E_MAC_ADDRESS8,
};

#endif /* MSG_DEFINE_H_ */
