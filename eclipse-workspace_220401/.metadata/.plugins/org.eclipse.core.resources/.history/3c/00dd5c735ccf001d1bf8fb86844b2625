/*
 * socket.cpp
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */


#include "Socket.h"
#include "../uart/uart.h"
#include "../Vector_SocketQueue.h"


#define EPOLL_SIZE 20

void *Recieve_Function(void* rcvDt);

Socket* m_pSoc;


int nDataLen=0;
int serv_sockfd;
int efd;
struct epoll_event	ev, *events;

extern UartComThread* m_MainComport;
extern MsgQueue* m_pMsgQueue;

std::vector<std::vector<BYTE>> vecTagData;

BYTE OneData[4096][2048] = { {0,}, {0,} };
BYTE TwoData[4096][2048] = { {0,}, {0,} };



int vectorCnt = 0;

Socket::Socket()
{
	p_thread = NULL;
	m_p8uData = NULL;
	m_p8uSendData = NULL;
	
	m_nServerMessge_End =0;
	m_nSocketArrayDataDownCnt =0;
	m_nSocketArrayDataIndicateCnt =0;
	m_iBypassSocketToUart =0;
	m_ReceiveData_len =0;
	m_iSocketReceiveQueue =0;
	m_Main_ServiceStart_TagAssociation_InitFlag = 0;
	m_iSocketReceiveEnd =0;
	bWorkingThread =0;
	m_SocketArrayDataDownMsg.reserve(5000);
	m_SocketArrayDataIndicateMsg.reserve(5000);
	memset(m_TagNumber, 0, sizeof(WORD)*4096);
	OneData1 = (BYTE**) malloc(sizeof(BYTE*) * 4096);
	TwoData1 = (BYTE**) malloc(sizeof(BYTE*) * 4096);

	m_serv_sock = 0;
	m_pSocMsgqueue = NULL;

}
Socket::~Socket()
{
	delete m_pSocMsgqueue;
}


int Socket::Socket_Init(/*int argc, char *argv[]*/)
{
	//socket_ctx_t* ctx = NULL;

	struct ifconf	ifc;
	struct ifreq	ifr[256];
	struct ifreq	ifrMac;
	struct sockaddr_in serv_addr;

	struct sockaddr*	pAddr(NULL);

	size_t	nNIC;
	int nRet =0, retEpoll = 0;
	int fd2, fd3, n;

	char buf[13];
	char buf2[5];
	char Mac_addr[20];	

	if(m_pSocMsgqueue == NULL)
		m_pSocMsgqueue = new Socket_MsgQueue;

		
	if(efd != 0) {
		retEpoll = epoll_ctl(efd, EPOLL_CTL_DEL, m_serv_sock/*events[0].data.fd*/, events);
		switch(retEpoll)
		{
		case 0:
			printf("epoll_ctl delete Success\n");
		case EBADF:
			printf("EBADF\n");
			break;
		case EEXIST:
			printf("EEXIST\n");
			break;
		case EINVAL:
			printf("EINVAL\n");
			break;
		case ELOOP:
			printf("ELOOP\n");
			break;
		case ENOENT:
			printf("ENOENT\n");
			break;
		case ESRCH:
			printf("ESRCH\n");
			break;
		case ENOMEM:
			printf("ENOMEM\n");
			break;
		case ENOSPC:
			printf("ENOSPC\n");
			break;
		case EPERM:
			printf("EPERM\n");
			break;
		default:
			printf("EPOLL CTL DEFAULT: %d\n", retEpoll);
			break;
		}
		free(events);
		efd =0;
		printf("Re-Connect efd : %d\n", efd);
	}
	if(m_serv_sock != 0) {
		if(close(m_serv_sock) == -1)
			printf("close(m_serv_sock) error \n");

		printf("Re-Connect m_serv_sock : %d\n", m_serv_sock);
		m_serv_sock = 0;
	}


	events = (struct epoll_event *)malloc(sizeof(*events) * EPOLL_SIZE);
	if( (efd = epoll_create(100)) < 0) {
		printf("epoll error\n");
		return -1;
	}

//	ctx = (socket_ctx_t*)malloc(sizeof(socket_ctx_t));

	m_serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	//ctx->fd = socket(PF_INET, SOCK_STREAM,  0);

	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_ifcu.ifcu_req = ifr;

	if(m_serv_sock == -1) {
		printf("Socket Error\n");
	}
	else {
		printf("\nSocket Val : %d\n", m_serv_sock);
	}

	nRet = ioctl(m_serv_sock, SIOCGIFCONF, &ifc);
	if(nRet == -1) printf("Get ip error\n");

	nNIC = ifc.ifc_len / sizeof(struct ifreq);

	for(size_t i =0; i< nNIC; i++) {
		if(PF_INET == ifc.ifc_ifcu.ifcu_req[i].ifr_ifru.ifru_addr.sa_family)
		{
			pAddr = (&ifc.ifc_ifcu.ifcu_req[i].ifr_ifru.ifru_addr);
		}
	}

	for(int i=0; i<15; i++) {
		if(pAddr->sa_data[i] != NULL) {
			m_IP_String += std::to_string(pAddr->sa_data[i]);
			m_IP_String.append(" ");
		}
	}

	printf("-------------MY IP ADDRESS : ");
	printf("MY IP %s ", m_IP_String.c_str());
	printf("---------------------\n");

	fd2 = open("ip", O_RDONLY);
	if(fd2 < 0) {
		printf("IP file open error\n");
		exit(1);
	}
	n = read(fd2, buf, 1024);
	printf("SERVER IP : ");
	for(int i=0; i<n; i++) {
		printf("%c",buf[i]);
	}

	strcpy(ifrMac.ifr_name, "eth0");
	if(ioctl(m_serv_sock, SIOCGIFHWADDR, &ifrMac)<0) {
		dp(4, "ioctl() - get mac");
	}
	Convert_mac( ether_ntoa((struct ether_addr *)(ifrMac.ifr_hwaddr.sa_data)), Mac_addr, sizeof(Mac_addr) -1 );
	strcpy(m_szMac_addr, Mac_addr);
	::string str = "";
	for(int i=0; i<20; i++) {
		str += m_szMac_addr[i];
		printf("%c[%d] ", m_szMac_addr[i], i);
	}
	printf("\n");
	
	m_Mac_String = str;
	printf("Mac Addrss : %s\n",m_Mac_String.c_str());
	
	fd3 = open("port", O_RDONLY);
	if(fd3 < 0) {
		printf("PORT file open error : %d\n", fd3);
		exit(1);
	}
	n = read(fd3, buf2, 1024);
	printf("PORT : ");
	for(int i=0; i<n; i++) {
		printf("%c",buf2[i]);
	}

	if(close(fd3) == -1)
		printf("Failed to Close file fd3\n");
	if(close(fd2) == -1)
		printf("Failed to Close file fd2\n");

#define PORT atoi(buf2)

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(buf);

	serv_addr.sin_port = htons(PORT/*atoi(argv[2])*/);

	while(1) {
		if(connect(m_serv_sock , (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
			printf("Failed Connection, Try Re-Connect\n");
			//exit(0);
		}
		else {
			printf("Success Connection\n");
			break;
		}
	}

	printf("m_serv_sock : %d,\n", m_serv_sock);

	printf("Socket Init\n");

	return (int)m_serv_sock;
#if 0
	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_ifcu.ifcu_req = ifr;

	if(ctx->fd == -1) {
		printf("Socket Error\n");
	}
	else {
		printf("Socket Val : %d\n", ctx->fd);
	}

	nRet = ioctl(ctx->fd, SIOCGIFCONF, &ifc);
	if(nRet == -1) printf("Get ip error\n");

	nNIC = ifc.ifc_len / sizeof(struct ifreq);

	for(size_t i =0; i< nNIC; i++) {
		if(PF_INET == ifc.ifc_ifcu.ifcu_req[i].ifr_ifru.ifru_addr.sa_family)
		{
			pAddr = (&ifc.ifc_ifcu.ifcu_req[i].ifr_ifru.ifru_addr);
		}
	}

	for(int i=0; i<15; i++) {
		if(pAddr->sa_data[i] != NULL) {
		//	printf("%d ", pAddr->sa_data[i]);
			m_IP_String += std::to_string(pAddr->sa_data[i]);
			m_IP_String.append(" ");
		//	m_IP_String.append(i, pAddr->sa_data[i]);
		//	m_IP_String += pAddr->sa_data[i];
		}
	}

	printf("-------------MY IP ADDRESS : ");
	printf("MY IP %s ", m_IP_String.c_str());
	printf("---------------------\n");

	fd2 = open("ip", O_RDONLY);
	if(fd2 < 0) {
		printf("IP file open error\n");
		exit(1);
	}
	n = read(fd2, buf, 1024);
	printf("SERVER IP : ");
	for(int i=0; i<n; i++) {
		printf("%c",buf[i]);
	}

	fd3 = open("port", O_RDONLY);
	if(fd3 < 0) {
		printf("PORT file open error\n");
		exit(1);
	}
	n = read(fd3, buf2, 1024);
	printf("PORT : ");
	for(int i=0; i<n; i++) {
		printf("%c",buf2[i]);
	}

#define PORT atoi(buf2)

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(buf);

	serv_addr.sin_port = htons(PORT/*atoi(argv[2])*/);

	if(connect(ctx->fd , (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
		printf("Connect Error\n");
		exit(0);
	}
	m_serv_sock = (int)ctx;
	printf("serv_sockfd : %d, %d\n", ctx->fd , (int)ctx);

	printf("Socket Init\n");

	return (int)ctx;
#endif

}

void Socket::Convert_mac(const char* data, char* cvrt_str, int sz)
{
	char buf[128] = {0,};
     char t_buf[8];
     char *stp = strtok( (char *)data , ":" );
     int temp=0;

     do
     {
          memset( t_buf, 0, sizeof(t_buf) );
          sscanf( stp, "%x", &temp );
          snprintf( t_buf, sizeof(t_buf)-1, "%02X", temp );
          strncat( buf, t_buf, sizeof(buf)-1 );
          strncat( buf, ":", sizeof(buf)-1 );
     } while( (stp = strtok( NULL , ":" )) != NULL );

     buf[strlen(buf) -1] = '\0';
     strncpy( cvrt_str, buf, sz );
	
}

void Socket::Create_Socket_Thread(pthread_t thread, int strucData)
{

	int status;
	printf("Create_Socket_Thread() m_serv_sock : %d\n", m_serv_sock);
	//socket_ctx_t* ctx = (socket_ctx_t *)strucData;
	p_thread = thread;

	if(pthread_create(&p_thread, NULL, Recieve_Function,(void* )m_serv_sock) < 0) {
		printf("thread Create Error\n");
		pthread_join(p_thread, (void**)&status);
		exit(0);
	}
	else {
		bWorkingThread = 1;
	}
	//printf("Create_Socket_Thread sock %d %d %d\n\n", ctx->fd, (int)ctx, m_serv_sock);
	printf("Create_Socket_Thread sock %d \n\n", m_serv_sock);
}

void Socket::Exit_Socket_Thread()
{
	int retEpoll =0;
	void *retval;
	int ret =0;

	if(m_serv_sock != 0) {
		if(close(m_serv_sock) == -1)
			printf("Exit_Socket_Thread close(m_serv_sock) error : %d\n", m_serv_sock);
		m_serv_sock = 0;
	}
	retEpoll = epoll_ctl(efd, EPOLL_CTL_DEL, m_serv_sock/*events[0].data.fd*/, events);
	switch(retEpoll)
	{
	case 0:
		printf("epoll_ctl delete Success\n");
	case EBADF:
		printf("EBADF\n");
		break;
	case EEXIST:
		printf("EEXIST\n");
		break;
	case EINVAL:
		printf("EINVAL\n");
		break;
	case ELOOP:
		printf("ELOOP\n");
		break;
	case ENOENT:
		printf("ENOENT\n");
		break;
	case ESRCH:
		printf("ESRCH\n");
		break;
	case ENOMEM:
		printf("ENOMEM\n");
		break;
	case ENOSPC:
		printf("ENOSPC\n");
		break;
	case EPERM:
		printf("EPERM\n");
		break;
	default:
		printf("EPOLL CTL DEFAULT\n");
		break;
	}
	ret = pthread_join(p_thread, &retval);

	printf("Socket Exit Thread , ");
	if(retval == PTHREAD_CANCELED)
		printf("The thread was canceled - \n");
	else
		printf("Returned value %d \n", (int)retval);

	switch(ret)
	{
	case 0:
		printf("The thread joined successfully\n");
		break;
	case EDEADLK:
		printf("Deadlock detected\n");
		break;
	case EINVAL:
		printf("The thread is not joinable\n");
		break;
	case ESRCH:
		printf("No thread with given ID is found\n");
		break;
	default:
		printf("Error occurred when joining the thread\n");
		break;
	}
	printf("Exit_Socket_Thread() \n");
}

int Socket::Send_Message(BYTE* msg, int len)
{
	int ret =0;
	m_p8uSendData = new BYTE[len+1];
	memcpy(m_p8uSendData, msg, len+1);
	nDataLen = len;

	ret = Send_Function();

	return ret;
}

int Socket::Send_Function()
{
	//socket_ctx_t* ctx = (socket_ctx_t *)m_serv_sock;
	int ret =0;
	BYTE p8Data[1024];

	memset(p8Data, 0, 1024);
	memcpy(p8Data, m_p8uSendData, 1024);

	printf("socket Send_Function() ");
	for(int i=0; i < nDataLen; i++) {
		printf("%x ", m_p8uSendData[i]);
	}
	delete[] m_p8uSendData;
	m_p8uSendData = NULL;

	ret = write(m_serv_sock,p8Data,nDataLen);
	//ret = write(m_serv_sock,&vecTagData,sizeof(vecTagData));

	if(ret == 0)
		printf("SendFail\n");
	else 
		printf(" ret :%d <--SERVER\n\n", ret);

	return ret;
}

int Socket::Read_Message(BYTE* msg)
{
	//socket_ctx_t* ctx = (socket_ctx_t *)m_serv_sock;
	//int str_len = read(ctx->fd,msg,sizeof(BYTE)*1024);
	int str_len = read(events[0].data.fd,msg,sizeof(BYTE)*1024);

	return str_len;
}

void *Recieve_Function(void* rcvDt)
{
	Socket* pSoc = NULL;
	int Socketd = 0;//pSoc->m_serv_sock;
	pSoc = (Socket* )m_pSoc;

	time_t ct;
	struct tm tm;
	int str_len, restBufCnt =0, nChecksumFlag =0;
	WORD CheckSumIndex =0;
	BYTE u8CheckSum =0;
	BYTE u8data[1024] = {0, };
	BYTE u8data2[1024] = {0, };

	
	
	printf("Recieve_Function() Socketd : %d, m_serv_sock : %d\n", Socketd, pSoc->m_serv_sock);

	//socket_ctx_t* ctx = (socket_ctx_t *)pSoc->m_serv_sock;
	while(1)
	{
		if(pSoc->Ready_to_Read(Socketd,10)) {
			pthread_mutex_lock(&pSoc->Socket_mutex);
			
			pSoc->m_nServerMessge_End =1;
			str_len = pSoc->Read_Message(u8data/*pSoc->m_p8uData*/);
			/*for(int i=0; i<=str_len; i++) {
				printf("[%x] ",u8data[i]);
			}
			printf("\n");*/
			ct = time(NULL);
			tm = *localtime(&ct);
			int TempCnt =0, TempCnt2 =0;
			printf("\n*********Socket Read Thread ******%d-%d-%d:%d:%d***** %d\n", tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,str_len);
			if(str_len > 0) {				
				pSoc->m_ReceiveData_len = str_len;
				while(1) {
					u8data2[TempCnt2] = u8data[TempCnt];					
					if(TempCnt2 > MSG_LENGTHONE) {

						CheckSumIndex = pSoc->ByteToWord(u8data[MSG_LENGTHONE], u8data[MSG_LENGTHZERO]);
					//	printf("CheckSumIndex : %d u8data2(CheckSumIndex) : %x\n",CheckSumIndex, u8data2[CheckSumIndex]);
						CheckSumIndex = CheckSumIndex+ MSG_HEADER;					

						if(TempCnt2 > CheckSumIndex) {						
							u8CheckSum = pSoc->Socket_GetChecksum(u8data2 , CheckSumIndex);
					//		printf("u8CheckSum : %d, u8data[CheckSumIndex]) : %d\n", u8CheckSum, u8data2[CheckSumIndex]);

							if(u8CheckSum != u8data[CheckSumIndex]) {
								printf("%x != %x\n", u8CheckSum, u8data[CheckSumIndex]);
								printf("CheckSumIndex : %d ###str_len :  %d\n",CheckSumIndex, str_len);
								if(str_len >= 14) {
									for(int j=0; j<=CheckSumIndex+MSG_TAIL; j++) {
										pSoc->deleteArray(0, 1024, u8data);
									}								
									str_len = str_len - (CheckSumIndex+MSG_TAIL);
								}
								/*for(int j=0; j<str_len; j++) {
									printf("%x ", u8data[j]);
								}
								printf("\n");*/
								printf("str_len :  %d\n", str_len);
								printf("STX/ETX ERROR\n");
								memset(u8data2, 0, 1024);
								TempCnt=0;
								TempCnt2=0;
								if(str_len > 1) {
									printf("Continue\n");
									continue;
								}
								else {
									printf("Checksum break\n");
									break;
								}
							}
						/*	else {
								printf("%x == %x\n", u8CheckSum, u8data[CheckSumIndex]);
							}*/
						}
					}
					if( (u8data2[MSG_STX] == STX) && (u8data2[TempCnt2-1] == 0x7e) && (u8data2[TempCnt2-2] == 0x5a) && (u8data2[TempCnt2-3] == 0xa5) ) {						
						if(pSoc->GetSocketMsg(u8data2, TempCnt) ) {
							if(TempCnt == str_len) {
								printf("%d == %d\n", TempCnt, str_len);
								memset(u8data, 0, 1024);
								memset(u8data2, 0, 1024);
								break;
							}
							else if(TempCnt < str_len) {
								for(int j=0; j<TempCnt; j++) {
									pSoc->deleteArray(0, 1024, u8data);
								}
								str_len = str_len - TempCnt;
								printf("After delete, Rest Array, str_len : %d\n", str_len);
								for(int j=0; j<=str_len; j++) {
									printf("<%x> ", u8data[j]);
								}
								printf("\n");
								restBufCnt=1;
								memset(u8data2, 0, 1024);
							}
						}
					}
					
					if( (!restBufCnt) && (TempCnt == str_len) ) {
						printf("&&&&&&&&&&&&& %d %d\n", TempCnt, str_len);
						
						memset(u8data, 0, 1024);
						memset(u8data2, 0, 1024);
						break;
					}
					if(restBufCnt) {
						printf("restBufCnt On\n");
						restBufCnt =0;
						TempCnt=0;
						TempCnt2=0;
					}
					else {
						TempCnt++;
						TempCnt2++;
					}
				}
			}
			else if(str_len <= 0) {
				printf("Disconnect Server\n");
				pSoc->bWorkingThread = 0;
				pSoc->m_nServerMessge_End =0;
				pthread_mutex_unlock(&pSoc->Socket_mutex);
				break;
			}
			memset(u8data, 0, 1024);
			memset(u8data2, 0, 1024);
			pSoc->m_nServerMessge_End =0;
			pthread_mutex_unlock(&pSoc->Socket_mutex);
		}
		usleep(10);
	}

	return 0;
}

bool Socket::GetSocketMsg(BYTE* p8udata, int Len)
{
	int DataLen =0;
	std::vector<BYTE> Vecc;
	DataLen = Len;
	VectorSocket<BYTE> pMm;	
	int index =0;


	printf("GetSocketMsg()\n");

	if((p8udata[MSG_STX] == STX) && (p8udata[DataLen-1] == 0x7e) && (p8udata[DataLen-2] == 0x5a) && (p8udata[DataLen-3] == 0xa5))
	{

		if(p8udata[MSGTYPE] != 0x01 &&
			p8udata[MSGTYPE] != 0x02 &&
			p8udata[MSGTYPE] != 0x03 &&
			p8udata[MSGTYPE]!= 0x04 &&
			p8udata[MSGTYPE] != 0x06 &&
			p8udata[MSGTYPE] != 0x10 &&
			p8udata[MSGTYPE] != SERVICESTART_REQUEST &&
			p8udata[MSGTYPE] != COORDINATOR_RESET_REQ &&
			p8udata[MSGTYPE] != 0x11 &&
			p8udata[MSGTYPE] != TAG_INFOR_UPDATE_REQ &&
		    p8udata[MSGTYPE] != MULTI_GATEWAY_SCAN_REQ &&
		    p8udata[MSGTYPE] != POWEROFF_REQ &&
		    p8udata[MSGTYPE] != DISPLAY_ENABLE_REQ )
		{
			//printf("Msg VAL : %x %x %x %x %x\n", p8udata[MSG_STX], p8udata[MSGTYPE], p8udata[DataLen-3], p8udata[DataLen-2], p8udata[DataLen-1]);
			if(p8udata[MSGTYPE] == BSN_START) {
				m_nServerMessge_End =1;
				m_pSocMsgqueue->BSN_MSG_ACK(p8udata);
				for(int i=0; i<15; i++) {
					printf("%x ", p8udata[i]);
				}
				printf("\n");				
				m_nSocketArrayDataDownCnt =0;
				m_nSocketArrayDataIndicateCnt =0;		
				Send_Message(p8udata, 15);
				m_nServerMessge_End =0;
				return 1;
			}
			else if (p8udata[MSGTYPE] == CONNECT_SOCKET_ALIVE_CHECK) {
				for(int i=0; i<Len; i++) {
					printf("%x ", p8udata[i]);
				}
				printf("\n");
			}
			else if(p8udata[MSGTYPE] == BSN_DATA_END_REQ) {
				int n;
				m_SocketQueue_vec.clear();
				m_SocketQueue_vec.shrink_to_fit();
				
				for(int i=0; i<m_nSocketArrayDataDownCnt; i++) {
					for(int j =1; j<512; j++) {
					//	printf("%x ", OneData[i][j]);
						if( (OneData[i][j-3] == 0xa5) && (OneData[i][j-2] == 0x5a) && (OneData[i][j-1] == 0x7e) ) {
							index = j;
						//	printf("Index : %d\n", index);
							break;
						}
					}
					
				//	printf("OneData[%d] size : %d\n", i, index);
					for(int j=0; j<index; j++) {
						m_SocketQueue_vec.push_back(OneData[i][j]);
					}
				//	printf("\n");

				/*	for(int i=0; i<(int)m_SocketQueue_vec.size(); i++) {
						printf("%x ", m_SocketQueue_vec.at(i));
					}
					printf("\n");*/
					m_SocketArrayDataDownMsg.push_back(m_SocketQueue_vec);
					m_SocketQueue_vec.clear();
					m_SocketQueue_vec.shrink_to_fit();
				}
				
				m_SocketQueue_vec.clear();
				m_SocketQueue_vec.shrink_to_fit();
				for(int i=0; i<m_nSocketArrayDataDownCnt; i++) {
					for(int j =1; j<512; j++) {
						if( (TwoData[i][j-3] == 0xa5) && (TwoData[i][j-2] == 0x5a) && (TwoData[i][j-1] == 0x7e) ) {
							index = j;
					//		printf("\nIndex : %d\n", index);
							break;
						}
					}
					printf("\n");
					
				//	printf("TwoData[%d] size : %d\n", i, index);
					for(int j=0; j<index; j++) {
						m_SocketQueue_vec.push_back(TwoData[i][j]);
					}
				//	free(TwoData1[i]);
				/*	for(int i=0; i<(int)m_SocketQueue_vec.size(); i++) {
						printf("%x ", m_SocketQueue_vec.at(i));
					}
					printf("\n");*/
					m_SocketArrayDataIndicateMsg.push_back(m_SocketQueue_vec);
					m_SocketQueue_vec.clear();
					m_SocketQueue_vec.shrink_to_fit();
				}

				m_pSocMsgqueue->BSN_MSG_END_ACK(p8udata);
				for(int i=0; i<15; i++) {
					printf("%x ", p8udata[i]);
				}
				printf("\n");
				Send_Message(p8udata, 15);
				m_iSocketReceiveQueue =1;
			}
			else if((p8udata[MSGTYPE] == DOWNLOAD_START_REQ) || (p8udata[MSGTYPE] == DATAINDICATION_REQ)) {

				for(int i=0; i<Len; i++) {
					printf("%x ", p8udata[i]);
				}
				printf("\n");

				m_SocketQueue_vec.clear();
				m_SocketQueue_vec.shrink_to_fit();
				for(int i=0; i< DataLen; i++) {
					m_SocketQueue_vec.push_back(p8udata[i]);					
				}

				if(m_SocketQueue_vec[MSGTYPE] == DOWNLOAD_START_REQ) {
					
					memset(OneData[m_nSocketArrayDataDownCnt], 0, 2048);
					for(int i=0; i< DataLen; i++) {
						OneData[m_nSocketArrayDataDownCnt][i] = p8udata[i];
					}
					
				//	OneData1[m_nSocketArrayDataDownCnt] = (BYTE*)malloc(sizeof(BYTE) * DataLen);
				//	for(int i = 0; i<DataLen; i++) {
				//		OneData1[m_nSocketArrayDataDownCnt][i] = OneData[m_nSocketArrayDataDownCnt][i];
					//	printf("[%x] ", OneData1[m_nSocketArrayDataDownCnt][i]);
				//	}
					//printf("\n");
					
					m_pSocMsgqueue->DownLoad_MSG_Start_ACK(p8udata);
					Send_Message(p8udata, 15);
					printf("\nm_nSocketArrayDataDown Size : %d\n", m_nSocketArrayDataDownCnt);
					m_nSocketArrayDataDownCnt++;
				}
				else if(m_SocketQueue_vec[MSGTYPE] == DATAINDICATION_REQ) {
					
					memset(TwoData[m_nSocketArrayDataIndicateCnt], 0, 2048);
					for(int i=0; i<DataLen; i++) {
						TwoData[m_nSocketArrayDataIndicateCnt][i] = p8udata[i];
					}
					m_TagNumber[m_nSocketArrayDataIndicateCnt] = (int)ByteToWord(TwoData[m_nSocketArrayDataIndicateCnt][MSG_DADDRONE], TwoData[m_nSocketArrayDataIndicateCnt][MSG_DADDRZERO]);				
					printf("Tag Number[%d] : %d\n", m_nSocketArrayDataIndicateCnt, m_TagNumber[m_nSocketArrayDataIndicateCnt]);
			//		TwoData1[m_nSocketArrayDataIndicateCnt] = (BYTE*)malloc(sizeof(BYTE) * DataLen);
			//		for(int i = 0; i<DataLen; i++) {
			//			TwoData1[m_nSocketArrayDataIndicateCnt][i] = TwoData[m_nSocketArrayDataIndicateCnt][i];
					//	printf("[%x] ", OneData1[m_nSocketArrayDataIndicateCnt][i]);
			//		}
					//printf("\n");
					m_pSocMsgqueue->DataIndication_MSG_Start_ACK(p8udata);
					Send_Message(p8udata, 16);
					printf("\nm_nSocketArrayDataIndicate Size : %d\n", m_nSocketArrayDataIndicateCnt);
					m_nSocketArrayDataIndicateCnt++;
				}
			}
		}
		else if ( (p8udata[MSGTYPE] == COORDINATOR_RESET_REQ) || (p8udata[MSGTYPE] == TAG_INFOR_UPDATE_REQ) || (p8udata[MSGTYPE] == MULTI_GATEWAY_SCAN_REQ)
			 || (p8udata[MSGTYPE] == POWEROFF_REQ) ||(p8udata[MSGTYPE] == DISPLAY_ENABLE_REQ)  ) {
			m_SocketMsg_vec.clear();
			printf("Socket queue Read : ");
			for(int i=0; i< DataLen; i++) {
				m_SocketMsg_vec.push_back(p8udata[i]);
				printf("%x ", m_SocketMsg_vec[i]);
			}
			printf("\n");

			if((m_SocketMsg_vec[DataLen-1] == 0x7e) &&
				(m_SocketMsg_vec[DataLen-2] == 0x5a) &&
				(m_SocketMsg_vec[DataLen-3] == 0xa5)) {
				if(m_p8uData != NULL) {
					m_p8uData = NULL;
				}
				m_p8uData = new BYTE[DataLen];
				memcpy(m_p8uData, p8udata, DataLen);

				m_iSocketReceiveEnd =1;
				m_iBypassSocketToUart = 1;
			}
		}
		else if(m_Main_ServiceStart_TagAssociation_InitFlag) {
			m_SocketMsg_vec.clear();
			printf("m_Main_ServiceStart_TagAssociation_InitFlag Socket queue Read : ");
			for(int i=0; i< DataLen; i++) {
				m_SocketMsg_vec.push_back(p8udata[i]);
				printf("%x ", m_SocketMsg_vec[i]);
			}
			printf("\n");

			if((m_SocketMsg_vec[DataLen-1] == 0x7e) &&
				(m_SocketMsg_vec[DataLen-2] == 0x5a) &&
				(m_SocketMsg_vec[DataLen-3] == 0xa5)) {
				if(m_p8uData != NULL) {
					m_p8uData = NULL;
				}

				m_p8uData = new BYTE[DataLen];
				memcpy(m_p8uData, p8udata, DataLen);

				for(int i=0; i<DataLen; i++) {
					printf("%x ", m_p8uData[i]);
				}
				printf("\n");
				m_iSocketReceiveEnd =1;

				if((m_SocketMsg_vec[MSGTYPE] == SERVICESTART_REQUEST) || (m_SocketMsg_vec[MSGTYPE] == COORDINATOR_RESET_CONFIRM))
				{
					m_iBypassSocketToUart = 1;
					printf("Socket Receive End,\n\n");
				}
			}
		}
		else if(p8udata[MSGTYPE] == SERVICESTART_REQUEST) {
			m_SocketMsg_vec.clear();
			printf("Socket queue Read : ");
			for(int i=0; i< DataLen; i++) {
				m_SocketMsg_vec.push_back(p8udata[i]);
				printf("%x ", m_SocketMsg_vec[i]);
			}
			printf("\n");

			if((m_SocketMsg_vec[DataLen-1] == 0x7e) &&
				(m_SocketMsg_vec[DataLen-2] == 0x5a) &&
				(m_SocketMsg_vec[DataLen-3] == 0xa5)) {
				if(m_p8uData != NULL) {
					m_p8uData = NULL;
				}

				m_p8uData = new BYTE[DataLen];
				memcpy(m_p8uData, p8udata, DataLen);

				m_iSocketReceiveEnd =1;
				m_iBypassSocketToUart = 1;
			}
		}

	}
	printf("Socket GetSocketMsg ReceiveData_len %d\n", Len);

	return 1;
}

int Socket::Ready_to_Read(int uartd, int timeoutms)
{

//	printf("Ready_to_Read m_serv_sock : %d\n", m_serv_sock);
	pthread_mutex_lock(&Socket_mutex);
	if(Socket_fd_Select(m_serv_sock, timeoutms) == m_serv_sock) {
	//	printf("Ready_to_Read return 1\n");
		pthread_mutex_unlock(&Socket_mutex);
		return 1;
	}
	pthread_mutex_unlock(&Socket_mutex);
	return 0;

}

void Socket::deleteArray(int idx, int size, BYTE* ar)
{
	memmove(ar+idx, ar+idx+1, size-idx);
}

int Socket::Socket_fd_Select(int fd, int timeout_ms)
{
	int n;
	//printf("Socket_fd_Select()\n");
	ev.events = EPOLLIN;
	ev.data.fd = m_serv_sock;
	epoll_ctl(efd, EPOLL_CTL_ADD, m_serv_sock, &ev);
//	printf("Socket_fd_Select() epoll_ctl\n");

	n = epoll_wait(efd, events, EPOLL_SIZE, 10);
//	printf("Socket_fd_Select() epoll_wait : %d\n", n);
	if( n== -1) {
		printf("epoll error\n");
		return -1;
	}

	for(int i=0; i<n; i++) {
	//	printf("epoll N : %d\n", events[i].data.fd );
		if(events[i].data.fd == m_serv_sock) {
			return m_serv_sock;
		}
	}

	return -1;

#if 0

	fd_set	io_fds;
	int retval =0;
	struct timeval timeout;

	if(fd<=0) return -1;
	if(timeout_ms < 0) timeout_ms =0;

	FD_ZERO(&io_fds);
	printf("FD_ZERO\n");
	FD_SET(STDIN_FILENO, &io_fds);
	//FD_SET(fd, &io_fds);
	printf("FD_SET : %d\n", fd);

	timeout.tv_sec =0;
	timeout.tv_usec = 1000*timeout_ms;

	retval = select(fd+1, &io_fds, 0,0,&timeout);

	if(retval == -1) {
		return -1;
	}
	else if(retval == 0) {
		printf("Socket_fd_Select timeout\n");
		return -1;
	}

	if(FD_ISSET(fd,&io_fds)) {
		printf("FD_ISSET : %d\n", fd);
		return fd;
	}

	return -1;
#endif
}




void Socket::th_Socket_delay(int millsec)
{
	double time;
	double timedelay = millsec;
	struct timeval start1 = {};
	struct timeval end1 = {};

	clock_t end = timedelay* 1000;
	clock_t start = clock();

	//printf("th_dealy %.2f msec\n", timedelay*2);
	gettimeofday(&start1 , NULL);
	while(clock()-start < end) {;}
	gettimeofday(&end1 , NULL);
	time = end1.tv_sec + end1.tv_usec / 1000000.0 - start1.tv_sec - start1.tv_usec / 1000000.0;
	printf("%.2f sec\n", time);
}


void Socket::SetMutex(pthread_mutex_t mutex, int fd)
{
	//Socket_mutex = mutex;
	Socket_mutex = mutex;
	printf("SetMutex()\n");
}

BYTE Socket::Socket_GetChecksum(BYTE* puData, int len)
{
	BYTE sum =0;

	for(int i=1; i< len; i++) {
	//	printf("%x ", puData[i]);
		sum += puData[i];
	}
//	printf("(check nsum : %x) ", sum);

	return sum;
}

void Socket::th_delay(int millsec)
{
	double time;
	double timedelay = millsec;
	struct timeval start1 = {};
	struct timeval end1 = {};

	clock_t end = timedelay* 1000;
	clock_t start = clock();

	//printf("th_dealy %.2f msec\n", timedelay*2);
	gettimeofday(&start1 , NULL);
	while(clock()-start < end) {;}
	gettimeofday(&end1 , NULL);
	time = end1.tv_sec + end1.tv_usec / 1000000.0 - start1.tv_sec - start1.tv_usec / 1000000.0;
	printf("%.2f sec\n", time);
}


WORD Socket::ByteToWord(BYTE puData, BYTE puData1)
{
	WORD p16Tempdata_HIGH, p16Tempdata_LOW;

	p16Tempdata_HIGH = (puData << 8);
	p16Tempdata_LOW = puData1;

	return p16Tempdata_HIGH|p16Tempdata_LOW;
}
void crit_err_hdlr2(int sig_num, siginfo_t * info, void * ucontext)
{
  void * array[50];
  void * caller_address;
  char ** messages;
  int size, i;
  sig_ucontext_t2 *uc;

  uc = (sig_ucontext_t2 *) ucontext;

  /* Get the address at the time the signal was raised */
  caller_address = (void *) uc->uc_mcontext.arm_pc;  // RIP: x86_64 specific     arm_pc: ARM

  fprintf(stderr, "\n");

  if (sig_num == SIGSEGV)
	printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr,
		   (void *) caller_address);
  else
	printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

  size = backtrace(array, 50);
  /* overwrite sigaction with caller's address */
  array[1] = caller_address;
  messages = backtrace_symbols(array, size);

  /* skip first stack frame (points here) */
  for (i = 1; i < size && messages != NULL; ++i) {
	printf("[bt]: (%d) %s\n", i, messages[i]);
  }
  free(messages);

  exit(EXIT_FAILURE);
}

void Socket::installSignal2(int __sig)
{
  struct sigaction sigact;
  sigact.sa_sigaction = crit_err_hdlr2;
  sigact.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(__sig, &sigact, (struct sigaction *) NULL) != 0) {
	  fprintf(stderr, "error setting signal handler for %d (%s)\n", __sig, strsignal(__sig));
  }
	exit(EXIT_FAILURE);
}
/*
void Socket::SetMsgQueue(MsgQueue* msgqueue)
{
	m_pQueueMsg = msgqueue;
}
*/
void Socket::SetSocketQueue(Socket_MsgQueue* socketqueue)
{
	m_pSocMsgqueue = socketqueue;
}

void Socket::SetMsgHwnd(Socket* soc)
{
	m_pSoc = soc;
}
