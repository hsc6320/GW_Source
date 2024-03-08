/*
 * Socket.h
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */

#ifndef SOCKET_SOCKET_H_
#define SOCKET_SOCKET_H_

#include "../pch.h"
#include "sys/socket.h"
#include "sys/types.h"
#include <arpa/inet.h>
#include "netinet/in.h"
#include "SocketMsgQueue.h"
#include <vector>
using std:: string;


typedef struct _sig_ucontext2 {
  unsigned long uc_flags;
  struct ucontext *uc_link;
  stack_t uc_stack;
  struct sigcontext uc_mcontext;
  sigset_t uc_sigmask;
} sig_ucontext_t2;


class Socket {
public:
	Socket();
	virtual ~Socket();
	int m_serv_sock;
	

	pthread_mutex_t Socket_mutex;
	pthread_t p_thread;

	std::vector<uint8_t> m_SocketMsg_vec;
	std::vector<std::vector<BYTE>> m_SocketArrayDataDownMsg;
	std::vector<std::vector<BYTE>> m_SocketArrayDataIndicateMsg;

	std::vector<BYTE> m_SocketQueue_vec;

	Socket_MsgQueue* m_pSocMsgqueue;
	BYTE **OneData1;
	BYTE **TwoData1;

	int m_iSocketReceiveEnd, m_iBypassSocketToUart, m_iSocketReceiveQueue, m_iStatusAlive;
	int m_iWorkingAlive;
	int m_nSocketArrayDataDownCnt, m_nSocketArrayDataIndicateCnt;
	BYTE* m_p8uData;
	BYTE m_p8uSendData[4096];
	int bWorkingThread;
	int m_nServerMessge_End;
	int m_ReceiveData_len;
	int m_Main_ServiceStart_TagAssociation_InitFlag;
	WORD m_TagNumber[4096];
	
	::string m_IP_String, m_Mac_String;
	char m_szMac_addr[20];

	void Convert_mac(const char* data, char* cvrt_str, int sz);
	void SetMutex(pthread_mutex_t mutex, int fd);
	int IP_Address_Init();
	int Socket_fd_Select(int fd, int timeout_ms);
	int Ready_to_Read(int uartd, int timeoutms);
	int Socket_Init();
	void Create_Socket_Thread(pthread_t thread, int strucData);
	int Send_Function();
	int TagSend_Function(std::vector<std::vector<BYTE>> msg);
	int TagSend_Message(std::vector<std::vector<BYTE>> msg);
	int Send_Message(BYTE* msg, int len);
	int Read_Message(BYTE* msg);
	void SetMsgHwnd(Socket* soc);
	void Exit_Socket_Thread();
	bool GetSocketMsg(BYTE* p8udata, int Len);
	WORD ByteToWord(BYTE puData, BYTE puData1);
	BYTE Socket_GetChecksum(BYTE* puData, int len);
	void th_delay(int millsec);
	void deleteArray(int idx, int size, BYTE* ar);
};

#endif /* SOCKET_SOCKET_H_ */

