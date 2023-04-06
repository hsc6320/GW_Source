/*
 * pch.h
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */

#ifndef PCH_H_
#define PCH_H_


#include <iostream>
#include <ostream>
#include <list>
#include <map>
#include <stdio.h>
#include <signal.h>
#include <string>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <termios.h>
#include <sys/select.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <ctime>
#include <signal.h>
#include <queue>
#include <algorithm>
#include <execinfo.h>
#include <errno.h>

//#include <linux/time.h>
//#include <linux/kernel.h>

#include <netdb.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <net/if.h>
#include <arpa/inet.h>


#include "Msg_Define.h"
#include "PreDefine.h"
#include "Vector_queue.h"
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>

#define DEBUG_LEVEL_	3

#define PASS 	1
#define FAIL 	0

//#define BEACON_MAX		63	//63	//63	//31	//9

#define BUILD_UINT16(loByte, hiByte)	((unsigned int )(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#ifdef  DEBUG_LEVEL_
#define dp(n, fmt, args...)	if (DEBUG_LEVEL_ <= n) fprintf(stderr, "%s:%d,", fmt, __FILE__, __LINE__, ## args)
#define dp0(n, fmt)		if (DEBUG_LEVEL_ <= n) fprintf(stderr, "%s:%d,"fmt, __FILE__, __LINE__)
#define _dp(n, fmt, args...)	if (DEBUG_LEVEL_ <= n) fprintf(stderr, " "fmt, ## args)
#else	/* DEBUG_LEVEL_ */
#define dp(n, fmt, args...)
#define dp0(n, fmt)
#define _dp(n, fmt, args...)
#endif	/* DEBUG_LEVEL_ */
void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext);
void installSignal(int __sig);



#endif /* PCH_H_ */
