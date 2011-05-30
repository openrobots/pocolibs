/*
 * Copyright (c) yyyy CNRS/LAAS
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
#include <sys/un.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <portLib.h>
#include <errnoLib.h>
#include <h2logLib.h>

static int die = 0;

static void
usage(void)
{
	fprintf(stderr, "usage: h2logd [-o outfile] [-s socket]\n");
	exit(2);
}

static void
sigHandler(int sig)
{
	die++;
}

/**
 ** Create socket on the server side
 **/
int
h2logdInit(char *path)
{
	struct sockaddr_un log_addr; /* AF_UNIX socket */
	int fd;

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1) {
		errnoSet(errno);
		return -1;
	}
#ifdef __BSD__
	log_addr.sun_len = sizeof(struct sockaddr_un);
#endif
	log_addr.sun_family = AF_UNIX;
	snprintf(log_addr.sun_path, sizeof(log_addr.sun_path), "%s", path);

	unlink(log_addr.sun_path);
	if (bind(fd, (struct sockaddr *)&log_addr, sizeof(log_addr)) == -1) {
		close(fd);
		errnoSet(errno);
		return -1;
	}


	if (chmod(log_addr.sun_path, 0666) == -1) {
		errnoSet(errno);
		return -1;
	}

	return fd;
	    
}

int
main(int argc, char *argv[])
{
	int ch, fd, rc = 0;
	ssize_t nread;
	FILE *out = NULL;
	char path[MAXPATHLEN];
	char buf[256];
	char *myPath = NULL;
	struct pollfd pfd;

	while ((ch = getopt(argc, argv, "o:s:")) != -1) {
		switch (ch) {
		case 'o':
			out = fopen(optarg, "w");
			if (out == NULL)
				err(2, "%s", optarg);

			break;
		case 's':
			myPath = optarg;
			break;
		default:
			usage();
		}
	} /* while */

	if (out == NULL)
		out = stdout;
	
	if (myPath == NULL) {
		if (h2logGetPath(path, sizeof(path)) == ERROR) {
			fprintf(stderr, "h2logd: can't find log path\n");
			exit(2);
		}
	} else 
		snprintf(path, sizeof(path), "%s", myPath);

	fd = h2logdInit(path);
	if (fd == -1) {
		fprintf(stderr, "h2logd: %s\n", 
		    h2getErrMsg(errnoGet(), buf, sizeof(buf)));
		exit(2);
	}
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);
	signal(SIGQUIT, sigHandler);

	pfd.fd = fd;
	pfd.events = POLLIN;

	while (1) {
		if (die)
			break;
		/* We need to use poll() to catch signals */ 
		switch (poll(&pfd, 1, -1)) {
		case 0:
			continue;
		case -1:
			if (errno != EINTR) 
				fprintf(stderr, "h2logd: poll: %s\n",
				    strerror(errno));
			continue;
		}
		if ((pfd.revents & POLLIN) != 0) {
			nread = recv(fd, buf, sizeof(buf), 0);
			if (nread == -1) {
				if (errno == EINTR)
					continue;
				fprintf(stderr, "h2logd: recv: %s\n", 
				    strerror(errno));
				rc = 2;
				break;
			}
			buf[nread] = '\0';
			
			fprintf(out, "%s\n", buf);
		}
	}
	close(fd);
	printf("removing %s\n", path);
	unlink(path);
	exit(rc);
}
