#ifndef __dbgmsg_h
#define __dbgmsg_h

#include <stdio.h>
#include <string.h>
#include <errno.h>

#if DEBUG_LOG_FILE_SUPPORT
  	#include "utils.h"
	#define LOG_FILE 		1
#else
	#define LOG_FILE 		0
	void log_printf(const char *format, ...);	// dummy
#endif

#if APP_PTHREADED
	#include <pthread.h>
	#define DBG_MUTUAL	1			// Mutual operations
#else 
	#define DBG_MUTUAL	0
#endif


#ifndef DEBUG_MODULE_NAME
	#define DEBUG_MODULE_NAME ""
#endif

void hexstr (const void *dump, size_t len);
void hexdump (const void *dump, size_t len, size_t offset);
void print_arr(char *msg, unsigned char *buf, int buflen);
void set_log_level(unsigned char new_lvl);
unsigned char get_log_level(void);

void lock_dbgbuf_mutex(void);
void unlock_dbgbuf_mutex(void);

#define __dbgmsgstr_len 1024
extern char __dbgmsgstr[__dbgmsgstr_len];

// dont use this macros directly!
#define __dbgmsg(__dbgarg...) do { \
	printf (__dbgarg); \
	fflush (stdout); \
} while (0)

#if DEBUG_MSG
  	#define dbgmsg __dbgmsg
  	#define dbghex(dump, len) hexstr(dump, len)
	#define dbghd(dump, len) hexdump(dump, len, 0)
	#define dbghd_o(dump, len, off) hexdump(dump, len, off)
#else /* DEBUG_MSG */
	#define dbgmsg(arg...)
	#define dbghex(dump, len)
	#define dbghd(dump, len)
	#define dbghd_o(dump, len, off)
#endif /* DEBUG_MSG */

#define _RED     "\x1b[31m"
#define _GREEN   "\x1b[32m"
#define _YELLOW  "\x1b[33m"
#define _BLUE    "\x1b[34m"
#define _MAGENTA "\x1b[35m"
#define _CYAN    "\x1b[36m"
#define _LGRAY   "\x1b[90m"
#define _RESET   "\x1b[0m"
#define _C(e) e?_RED:_GREEN

#define resmsg(assertion, args...) do \
{ \
	snprintf(__dbgmsgstr, __dbgmsgstr_len, ## args); \
	if (!(assertion)) \
	{ \
	snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), _GREEN "OK\r\n" _RESET); \
	} \
	else \
	{ \
	snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), _RED "FAIL\r\n" _RESET); \
	} \
	__dbgmsg (__dbgmsgstr); \
} while(0);

#define errmsg(args...)  do \
{ \
	strcpy(__dbgmsgstr, DEBUG_MODULE_NAME " "); \
	snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ## args);\
	if (__dbgmsgstr[strlen (__dbgmsgstr)-1] == '\n') \
	{ \
		__dbgmsgstr[strlen (__dbgmsgstr)] = 0; \
		__dbgmsgstr[strlen (__dbgmsgstr)-1] = 0; \
	} \
	snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), _RED " [in %s()]\n" _RESET, __func__);\
	__dbgmsg(__dbgmsgstr); \
} while(0);

#define perrmsg(args...) do \
{ \
	strcpy(__dbgmsgstr, DEBUG_MODULE_NAME " "); \
	snprintf (&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), "[ Error (%d) ] %s[%s()]%s ", errno, errno?_RED:_GREEN, __func__, _RESET); \
	snprintf (&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ## str); \
	if (__dbgmsgstr[strlen (__dbgmsgstr)-1] == '\n') \
	{ \
		__dbgmsgstr[strlen (__dbgmsgstr)] = 0; \
		__dbgmsgstr[strlen (__dbgmsgstr)-1] = 0; \
	} \
	snprintf (&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ": %s\n", strerror(errno)); \
	__dbgmsg(__dbgmsgstr); \
} while(0);



// Log levels print (7bit active)
#define MSG_NO				(0x00)
#define MSG_DEBUG			(0x01)
#define MSG_VERBOSE			(0x02)
#define MSG_TRACE			(0x03)
// Flag to save message in file
#define MSG_TO_FILE			(0x80)

#define dbgmsg_v(loglvl, str...)	do{ \
										lock_dbgbuf_mutex(); \
										strcpy(__dbgmsgstr, DEBUG_MODULE_NAME); \
										snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ## str); \
										if( ((loglvl) != MSG_TO_FILE) && (get_log_level() >= ((loglvl) & 0x7F)) ) dbgmsg(__dbgmsgstr); \
										if( LOG_FILE && ((loglvl) & MSG_TO_FILE) ) log_printf(__dbgmsgstr); \
										unlock_dbgbuf_mutex(); \
									}while(0)

#define errmsg_v(loglvl, str...)	do{ \
										lock_dbgbuf_mutex(); \
										strcpy(__dbgmsgstr, DEBUG_MODULE_NAME); \
										snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ## str); \
										if (__dbgmsgstr[strlen (__dbgmsgstr)-1] == '\n') \
										{ \
											__dbgmsgstr[strlen (__dbgmsgstr)] = 0; \
											__dbgmsgstr[strlen (__dbgmsgstr)-1] = 0; \
										} \
										snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), _RED " [%s() %s:%u]\n" _RESET , __func__, __FILE__, __LINE__);\
										if( ((loglvl) != MSG_TO_FILE) && (get_log_level() >= ((loglvl) & 0x7F)) ) __dbgmsg(__dbgmsgstr); \
										if( LOG_FILE && ((loglvl) & MSG_TO_FILE) ) log_printf(__dbgmsgstr); \
										unlock_dbgbuf_mutex(); \
									}while(0)	

#define perrmsg_v(loglvl, str...)	do{ \
										lock_dbgbuf_mutex(); \
										strcpy(__dbgmsgstr, DEBUG_MODULE_NAME); \
										snprintf (__dbgmsgstr, __dbgmsgstr_len, "[ Error (%d) ] %s[%s()]%s", errno, errno? _RED : _GREEN, __func__, _RESET); \
										snprintf (&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ## str); \
										if (__dbgmsgstr[strlen (__dbgmsgstr)-1] == '\n') \
										{ \
											__dbgmsgstr[strlen (__dbgmsgstr)] = 0; \
											__dbgmsgstr[strlen (__dbgmsgstr)-1] = 0; \
										} \
										snprintf (&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ": %s\n", strerror(errno)); \
										if( ((loglvl) != MSG_TO_FILE) && (get_log_level() >= ((loglvl) & 0x7F)) ) __dbgmsg(__dbgmsgstr); \
										if( LOG_FILE && ((loglvl) & MSG_TO_FILE) ) log_printf(__dbgmsgstr); \
										unlock_dbgbuf_mutex(); \
									}while(0)





// RTOS mutual debug print. Use inside tasks!
#define m_dbgmsg(args...) do{ \
	xSemaphoreTake(USARTDebugMutex, portMAX_DELAY); \
	dbgmsg(args); \
	xSemaphoreGive(USARTDebugMutex); \
} while(0);

#define m_errmsg(args...) do{ \
	xSemaphoreTake(USARTDebugMutex, portMAX_DELAY); \
	errmsg(args); \
	xSemaphoreGive(USARTDebugMutex); \
} while(0);

#endif
