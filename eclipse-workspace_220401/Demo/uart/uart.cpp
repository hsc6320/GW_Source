/*
 * uart.cpp
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */

#include "uart.h"
#include <thread>
#include <sstream>

#define BUF_MAX 512

using namespace std;
int fd =0;
int nToTalLen = 0, nToTalLen2 =0;
pthread_t mthreads;

MsgQueue* pMsgQueue;

UartComThread* mpComm;
uint8_t buf[BUF_MAX];

pthread_mutex_t mutex2;

UartComThread::UartComThread()
{
	m_uartd = 0;
	pMsgQueue = NULL;
}

UartComThread::~UartComThread()
{
}

#define reset_buffer() memset(rx,0,sizeof(char)*1024)


static void *uart_Rx_Thread(void *param)
{
   UartComThread* pComm = NULL;
   pComm = (UartComThread* )mpComm;
	int uartd = (int)param;
	int len =0, underflowcnt =0;;
	int ChecksumError =0;
	uart_ctx_t* ctx = (uart_ctx_t *)uartd;
	BYTE* rx = (uint8_t *)malloc(sizeof(uint8_t)*1024);
	uint8_t* rx2 = (uint8_t *)malloc(sizeof(uint8_t)*1024);
	uint8_t* rx_sto = (uint8_t *)malloc(sizeof(uint8_t)*1024);

	while(1) {
		if(pComm->Ready_to_Read(uartd,0)) {
			
			ChecksumError =0;
			pthread_mutex_lock(&ctx->mutex);
			len = pComm->Uart_Read(uartd, rx2, 1024);
			pthread_mutex_unlock(&ctx->mutex);

			if( (rx2[0] == 0) && (len == 1) ) {
				printf("rx[%d] : %x, len : %d continue \n", rx2[0], len);
				continue;
			}
			else if( (len == 1) && (rx2[0] == 0xaa) ) {
				nToTalLen += len;
				for(int i=nToTalLen2; i<nToTalLen; i++) {
					pComm->AppendArray( rx2[i], i, rx);
				}
				nToTalLen2 = 1;
				continue;
			}
			
			nToTalLen += len;
			int restBufCnt =0;
			for(int i=nToTalLen2; i<=nToTalLen; i++) {
				pComm->AppendArray( rx2[restBufCnt], i, rx);
				restBufCnt++;
			}

			printf("\n***************uart_Rx_Thread uart **********%d*********\n", nToTalLen);
			int i=0;
			while(1) {
	 			for( i =0; i<= nToTalLen; i++) {
					if( (rx[0] == STX) && (rx[i-3] == 0xa5) && (rx[i-2] == 0x5a) && (rx[i-1] == 0x7e) ) {					
						underflowcnt =0;
						if(rx[i-4] != pComm->Uart_GetChecksum(rx ,i)) {
							printf("Uart Checksum Error \n");
							for(int k=0; k<i; k++) {
								printf("%x ", rx[k]);
							}
							printf("\n");
							ChecksumError =1;
							memset(rx, 0, 1024);
							break;
						}
						if(pMsgQueue->PutByte(rx, i) != 1) {
							printf("putbyte return 0\n");
						}
						else {
							break;
						}
					}
					else if( i == nToTalLen) {
						nToTalLen2 = i;
						underflowcnt =1;
					}
	 			}
				
				if(ChecksumError)
					break;
				if(underflowcnt) {
					break;
				}
				
				if(i < nToTalLen ) {
					int tempCnt =i;
					printf("%d < %d\n", i, nToTalLen);
					
					for(int j=0; j<nToTalLen; j++) {
						rx_sto[j] = rx[tempCnt];
						printf("%x ", rx_sto[j]);
						tempCnt++;
					}
					printf("\n");
					memset(rx, 0, 1024);
					memcpy(rx, rx_sto, 1024);
					
					printf("Uart_deleteArray  nToTalLen : %d\n", nToTalLen);
					int restCnt =0;
					nToTalLen = nToTalLen - i;
					while(restCnt<nToTalLen) {
						printf("%x ", rx[restCnt]);
						restCnt++;
					}
					printf("\n");
	
					continue;
				}
				else if(i == nToTalLen) {
				//	printf("%d == %d\n", i, nToTalLen);
					nToTalLen =0;
					nToTalLen2 =0;
					break;
				}
			}
			if(underflowcnt) {
			//	printf(" underflowcnt Continue \n");
				memset(rx2, 0, 1024);
				continue;
			}

			reset_buffer();
			memset(rx2, 0, sizeof(char)*1024);
			memset(rx_sto, 0, sizeof(char)*1024);
			nToTalLen =0;
		}
		usleep(10);
	}
	free(rx_sto);
	free(rx);

	return 0;
}
void UartComThread::deleteArray(int idx, int size, BYTE* ar)
{
	memmove(ar+idx, ar+idx+1, size-idx);
}


void UartComThread::Exit_Uart_Thread()
{
	void *status;
	int ait, ret =0;

	ret = pthread_join(mthreads,&status);
	wait(&ait);

	printf("Uart Exit Thread , ");

	if(status == PTHREAD_CANCELED)
		printf("The thread was canceled - \n");
	else
		printf("Returned value %d - \n", (int)status);

	switch(ret) {
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


	//pthread_exit(NULL);
	printf("Exit_Uart_Thread() : [%d] \n", (int)status);

}

void timer_handler(int signum)
{
    printf("Uart Read expired \t");

}

int UartComThread::uart_SetTimer()
{
	struct sigaction sa;
	struct itimerval timer;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &timer_handler;
	sigaction(SIGVTALRM, &sa, NULL);

	//timer.it_interval.tv_sec = 0;
	//timer.it_interval.tv_usec = 0;

	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;

	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	return 1;
}
int UartComThread::uart_init()
{
	m_uartd = Uart_Open("/dev/ttyS1", 230400);

	if(m_uartd <= 0)
		return 0;
	return 1;
}

int UartComThread::Create_Uart_thread(pthread_t thread)
{
	int rc=0, status;

	mthreads = thread;
	rc = pthread_create(&mthreads, NULL, uart_Rx_Thread, (void *)m_uartd);

	printf("uart_thread init m_uartd, %d rc %d\n", m_uartd, rc);
	wait(&status);
	if (rc) {
		printf("Error:unable to create thread, %d\n", rc);
		exit(-1);
	}
	return 1;
}

int UartComThread::Uart_Open(const char *dev, int baud_rate)
{
	uart_ctx_t* ctx = NULL;
	if(!dev)	return 0;
	pthread_mutex_init(&mutex2, NULL);
	ctx = (uart_ctx_t*)malloc(sizeof(uart_ctx_t));

	if(ctx) {
		printf("Uart_Open baud_rate :%d\n", baud_rate);
		memset(ctx, 0, sizeof(uart_ctx_t));

		while(1) {
			if(ctx->fd = Open_fd(dev))
				break;

			printf("Uart_Open Fail ctx->fd : %d\n", ctx->fd);
			th_delay(3000);
		}
		printf("Uart_Open ctx->fd : %d\n", ctx->fd);
		if(ctx->fd <= 0) {
			printf("Uart_Close\n");
			Uart_Close((int)ctx);
			return 0;
		}
		Uart_Config(ctx, baud_rate);
	}
	return (int)ctx;

}

int UartComThread::Uart_Close(int uartd)
{
	void* result;
	int status;
	uart_ctx_t *ctx = (uart_ctx_t *)uartd;

	if(ctx) {
		if(ctx->fd > 0) {
			tcsetattr(ctx->fd, TCSANOW, &ctx->oldtio);
			close(ctx->fd);
			ctx->fd =0;
		}
		free(ctx);
	}
	pthread_join(mthreads, &result);
	wait(&status);
	printf("pthread_join UART result : %d\n", (int)result);

	return 1;
}

int UartComThread::Uart_Config(uart_ctx_t* ctx, int baudrate)
{
	if(!ctx || (ctx->fd <= 0)) return -1;
	tcgetattr(ctx->fd, &ctx->oldtio);		//save current port setting.

	//set new port Setting
	switch(baudrate)
	{
	case 9600:
		ctx->newtio.c_cflag = B9600;
		break;
	case 115200:
		ctx->newtio.c_cflag = B115200;
		break;
	case 230400:
		ctx->newtio.c_cflag = B230400;
		break;
	case  460800:
		ctx->newtio.c_cflag = B460800;
		break;
	case 921600:
		ctx->newtio.c_cflag = B921600;
		break;
	}

	ctx->newtio.c_cflag |= (CS8 | CLOCAL | CREAD);
	ctx->newtio.c_iflag = IGNPAR /*| ICRNL*/;
	ctx->newtio.c_oflag = 0;
	ctx->newtio.c_lflag = ~(ICANON | ECHO | ECHOE | ISIG);

	tcflush(ctx->fd, TCIFLUSH);
	tcsetattr(ctx->fd, TCSANOW, &ctx->newtio);
	return 0;

}

int UartComThread::Open_fd(const char *dev)
{
	int fd =0;
	if(!dev) return -1;

	 fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	 printf("Open_fd %d\n", fd);
	if (fd < 0) {
		return 0;
	}

	return fd;
}

int UartComThread::Ready_to_Read(int uartd, int timeoutms)
{
	uart_ctx_t* ctx = (uart_ctx_t *)uartd;

	if(!ctx) return -1;

	//printf("Ready_to_Read ctx->fd : %d\n", ctx->fd);
	pthread_mutex_lock(&ctx->mutex);
	if(Uart_fd_Select(ctx->fd, timeoutms,0) == ctx->fd) {
	//	printf("Ready_to_Read return 1\n");
		pthread_mutex_unlock(&ctx->mutex);
		return 1;
	}
	pthread_mutex_unlock(&ctx->mutex);
	return 0;

}

int UartComThread::Ready_to_Write(int uartd, int timeoutms)
{
	uart_ctx_t* ctx = (uart_ctx_t *)uartd;

	if(!ctx) return -1;

	pthread_mutex_lock(&ctx->mutex);
	if(Uart_fd_Select(ctx->fd, timeoutms, 1) == ctx->fd) {
		pthread_mutex_unlock(&ctx->mutex);
		return 1;
	}
	pthread_mutex_unlock(&ctx->mutex);
	return 0;

}

int UartComThread::Uart_Read(int uartd, uint8_t* data, int size)
{
	int readlen =0;
	uart_ctx_t* ctx = (uart_ctx_t *)uartd;

	if(!ctx || ctx->fd <= 0 || !data || size <= 0) return 0;
	readlen = read(ctx->fd, data, size*sizeof(uint8_t));

	return readlen;
}

int UartComThread::Uart_Write(int uartd, uint8_t* data, int size)
{
	int wrotelen =0;
	int failcnt =0;
	uart_ctx_t* ctx = (uart_ctx_t *)uartd;
	if(!ctx || ctx->fd <= 0 || !data || size <= 0) return 0;

	printf("Uart");
	pthread_mutex_lock(&ctx->mutex);
	while(wrotelen < size && failcnt < 5) {
		int ret  =0;
		int towritelen = size - wrotelen;
		uint8_t* ptr = data + wrotelen;
			ret = write(ctx->fd, ptr, towritelen*sizeof(uint8_t));
			if(ret > 0) {
				printf("_Write ");
		//		for(int i=0; i<size; i++) {
		//			printf("%x ", ptr[i]);
		//		}
				printf(" End\n");
				wrotelen += ret;
			}
			else {
				printf("failcnt : %d", failcnt);
				failcnt++;
			}
	}
	pthread_mutex_unlock(&ctx->mutex);

	return wrotelen;

}

int UartComThread::Uart_fd_Select(int fd, int timeout_ms, int rw)
{
	fd_set	io_fds;
	int retval =0;
	struct timeval timeout;

	if(fd<=0) return -1;
	if(timeout_ms < 0) timeout_ms =0;

	FD_ZERO(&io_fds);
	//FD_SET(STDIN_FILENO, &io_fds);
	FD_SET(fd, &io_fds);

	timeout.tv_sec =0;
	timeout.tv_usec = 1000*timeout_ms;

	if(rw)
		retval = select(fd+1, 0, &io_fds,0,&timeout);		//write
	else
		retval = select(fd+1, &io_fds, 0,0,&timeout);		//read

	if(retval == -1) {
		return -1;
	}
	else if(retval == 0) {
		//printf("Uart_fd_Select timeout\n");
	}

	if(FD_ISSET(fd,&io_fds)) {
		return fd;
	}

	return -1;

}

BYTE UartComThread::Uart_GetChecksum(BYTE* puData, int len)
{
	BYTE sum =0;

	for(int i=1; i< len-4; i++) {
		//printf("%x ", puData[i]);
		sum += puData[i];
	}
	//printf("(check nsum : %x) ", sum);

	return sum;
}

void UartComThread::th_delay(int millsec)
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


void UartComThread::InsertArray(int idx, BYTE sz, BYTE* ar)
{
	BYTE arr[1024];
	memcpy(arr, ar, 1024);
	int size = (sizeof(arr)/sizeof(*arr));
	memmove(ar+idx+1, ar+idx, size-idx+1);
	ar[idx] = sz;
	//printf("Insert [%d]%x, size : %d\n", idx, ar[idx], size);
}

void UartComThread::AppendArray(BYTE sz, int idx, BYTE* ar)
{
	InsertArray(idx, sz, ar);
}

void UartComThread::Uart_deleteArray(int idx, int size, BYTE* ar)
{
	memmove(ar+idx, ar+idx+1, size-idx);
}


void UartComThread::SetMutex(pthread_mutex_t mutex)
{
	uart_ctx_t *ctx = (uart_ctx_t *)m_uartd;
	ctx->mutex = mutex;
}

void UartComThread::SetMsgQueue(MsgQueue* msg)
{
	pMsgQueue = msg;
}

void UartComThread::SetUartComm(UartComThread* msg)
{
	mpComm = msg;
}



