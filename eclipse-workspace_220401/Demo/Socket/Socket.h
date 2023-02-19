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

typedef struct _socket_ctx_t
{
	pthread_mutex_t Socket_mutex;
	int fd;
}socket_ctx_t;

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

	pthread_t p_thread;

	std::vector<uint8_t> m_SocketMsg_vec;
	std::vector<std::vector<BYTE>> m_SocketArrayDataDownMsg;
	std::vector<std::vector<BYTE>> m_SocketArrayDataIndicateMsg;
//	Vector<Vector<BYTE>> m_SocketArrayDataDownMsg;
//	Vector<Vector<BYTE>> m_SocketArrayDataIndicateMsg;

//	Vector<BYTE> m_SocketQueue_vec;

	std::vector<BYTE> m_SocketQueue_vec;

	Socket_MsgQueue* m_pSocMsgqueue;
	BYTE **OneData1;
	BYTE **TwoData1;

	int m_iSocketReceiveEnd, m_iBypassSocketToUart, m_iSocketReceiveQueue;
	int m_nSocketArrayDataDownCnt, m_nSocketArrayDataIndicateCnt;
	BYTE* m_p8uData;
	BYTE* m_p8uSendData;
	pthread_mutex_t Socket_mutex;
	int bWorkingThread;
	int m_nServerMessge_End;
	int m_ReceiveData_len;
	int m_Main_ServiceStart_TagAssociation_InitFlag;
	::string m_IP_String;

	void SetMutex(pthread_mutex_t mutex, int fd);
	int Socket_fd_Select(int fd, int timeout_ms);
	int Ready_to_Read(int uartd, int timeoutms);
	void SetSocketQueue(Socket_MsgQueue* socketqueue);
	int Socket_Init(/*int argc, char *argv[]*/);
	void Create_Socket_Thread(pthread_t thread, int strucData);
	int Send_Function();
	int Send_Message(BYTE* msg, int len);
	int Read_Message(BYTE* msg);
	void SetMsgHwnd(Socket* soc);
	void Exit_Socket_Thread();
	void th_Socket_delay(int millsec);
	bool GetSocketMsg(BYTE* p8udata, int Len);
	WORD ByteToWord(BYTE puData, BYTE puData1);
	//void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext);
	void installSignal2(int __sig);
};

#endif /* SOCKET_SOCKET_H_ */

