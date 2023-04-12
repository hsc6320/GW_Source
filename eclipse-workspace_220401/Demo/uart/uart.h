/*
 * uart.h
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */

#ifndef UART_UART_H_
#define UART_UART_H_


#include "../pch.h"
#include "../MsgQueue.h"

typedef struct _uart_ctx_t
{
	pthread_mutex_t mutex;
	int fd;
	struct termios oldtio, newtio;
}uart_ctx_t;


class UartComThread
{

public:
	UartComThread();
	virtual ~UartComThread();

	int m_uartd;

	void SetMutex(pthread_mutex_t mutex);
	int uart_init();
	int uart_SetTimer();
	int Uart_Open(const char *dev, int baud_rate);
	int Uart_Close(int uartd);
	int Open_fd(const char *dev);
	int Ready_to_Read(int uartd, int timeoutms);
	void deleteArray(int idx, int size, BYTE* ar);
	int Ready_to_Write(int uartd, int timeoutms);
	int Uart_Read(int uartd, uint8_t* data, int size);
	int Uart_Write(int uartd, uint8_t* data, int size);
	int Uart_Config(uart_ctx_t* ctx, int baudrate);
	int Uart_fd_Select(int fd, int timeout_ms, int rw);
	void Exit_Uart_Thread();
	int Create_Uart_thread(pthread_t thread);
	void SetMsgQueue(MsgQueue* msg);
	void th_delay(int millsec);
	void InsertArray(int idx, BYTE sz, BYTE* ar);
	void AppendArray(BYTE sz, int idx, BYTE* ar);
	void Uart_deleteArray(int idx, int size, BYTE* ar);
	BYTE Uart_GetChecksum(BYTE* puData, int len);
	void SetUartComm(UartComThread* msg);
};
#endif /* UART_UART_H_ */
