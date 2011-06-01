/*
 * Copyright (c) 2011 CNRS/LAAS
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <errno.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <portLib.h>
#include <errnoLib.h>
#include <h2devLib.h>
#include <h2logLib.h>

static int h2logFd = -1;
static int h2logConnected = FALSE;
static char h2logPath[MAXPATHLEN] = { '\0' };

STATUS
h2logGetPath(char *path, size_t len)
{
	char *home;
	struct utsname uts;

	home = getenv("HOME");

	if (home == NULL) {
		errnoSet(S_h2devLib_BAD_HOME_DIR);
		return ERROR;
	}
	if (uname(&uts) == -1) {
		errnoSet(errno);
		return ERROR;
	}
	/* Check the length of the string */
	if (strlen(home) + strlen(uts.nodename) +
	    strlen(H2LOG_SOCKET_NAME) + 3 > len) {
		errnoSet(S_h2devLib_BAD_HOME_DIR);
		return ERROR;
	}
	snprintf(path, len, "%s/%s-%s", home, H2LOG_SOCKET_NAME, uts.nodename);
	return OK;
}


/**
 ** Init log lib
 **/
static STATUS
h2logConnect(const char *path)
{
	struct sockaddr_un log_addr; /* AF_UNIX socket */

	if (path == NULL) {
		/* init log functions */
		if (h2logPath[0] == '\0' && 
		    h2logGetPath(h2logPath, sizeof(h2logPath)) == ERROR)
			return ERROR;
	} else
		snprintf(h2logPath, sizeof(h2logPath), "%s", path);

#ifdef __BSD__
	log_addr.sun_len = sizeof(struct sockaddr_un);
#endif
	log_addr.sun_family = AF_UNIX;
	snprintf(log_addr.sun_path, sizeof(log_addr.sun_path), 
	    "%s", h2logPath);
	if (connect(h2logFd, (struct sockaddr *)&log_addr, 
		sizeof(log_addr)) == -1) {
		errnoSet(errno);
		h2logConnected = FALSE;
		return ERROR;
	}
	h2logConnected = TRUE;
	return OK;
}

STATUS
h2logMsgInit(const char *path)
{
	/* Create socket */
	h2logFd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (h2logFd == -1) {
		errnoSet(errno);
		return ERROR;
	}
	/* failure to connect is tolerated */
	(void)h2logConnect(path);
	return OK;
}

/**
 ** Log a message
 **/
void
_h2logMsg(const char *func, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	h2logMsgv(func, fmt, ap);
	va_end(ap);
}

/**
 ** Log a message, va_list version
 **/
void
h2logMsgv(const char *func, const char *fmt, va_list ap)
{
	char buf[H2LOG_MSG_LENGTH];
	struct timeval tv;
	size_t len;
	ssize_t error;

	if (h2logFd == -1 && h2logMsgInit(NULL) == ERROR)
		return;
	if (h2logConnected == FALSE && h2logConnect(NULL) == ERROR)
		return;

	gettimeofday(&tv, NULL);
	len = snprintf(buf, sizeof(buf), "[%9" PRIdMAX ".%06" PRIdMAX "] ", 
	    (intmax_t)tv.tv_sec, (intmax_t)tv.tv_usec);
	len += snprintf(buf+len, sizeof(buf) - len , "%s: ", func);
	len += vsnprintf(buf+len, sizeof(buf) - len, fmt, ap);
	error = send(h2logFd, buf, len, MSG_DONTWAIT);

	if (error == -1) {
		close(h2logFd);
		h2logFd = -1;
		h2logConnected = -1;
	}
}
