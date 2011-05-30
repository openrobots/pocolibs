#ifndef _H2LOGLIB_H_
#define _H2LOGLIB_H_

#define H2LOG_SOCKET_NAME ".h2log"
#define H2LOG_MSG_LENGTH 256

extern STATUS h2logGetPath(char *, size_t);
extern STATUS h2logMsgInit(const char *);
extern void _h2logMsg(const char *, const char *, ...);
extern void h2logMsgv(const char *, const char *, va_list);

#define h2logMsg(fmt, ...) _h2logMsg(__func__, fmt, __VA_ARGS__)

#endif
