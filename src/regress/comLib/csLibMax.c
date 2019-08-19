/*
 * Copyright (c) 2018 CNRS/LAAS
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
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "pocolibs-config.h"

#include <sys/types.h>
#include "portLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "gcomLib.h"
#include "h2devLib.h"
#include "h2evnLib.h"
#include "smMemLib.h"
#include "h2initGlob.h"
#include "csLib.h"

#define MAX_SERV (4)

struct rq {
	int a;
};

struct rp {
	int b;
};

#define MY_TASK_DEV (taskGetUserData(0))

#define BUF_SIZE(size) \
   (size + 4 - (size & 3) + sizeof(LETTER_HDR) + 8)

#define  MBOX_RQST_SIZE       \
	(BUF_SIZE(sizeof(struct rq)) * SERV_NMAX_RQST_ID)

#define  MBOX_REPLY_SIZE   \
	(BUF_SIZE(sizeof(struct rp)) * CLIENT_NMAX_RQST_ID)

pthread_barrier_t barrier;
int result = 0;

int
func(SERV_ID si, int rqstId)
{
	struct rq q;
	struct rp p;

	logMsg("func called\n");
	csServRqstParamsGet(si, rqstId, (char *)&q, sizeof(struct rq), NULL);
	logMsg("got %d\n", q.a);
	p.b = q.a;
	csServReplySend(si, 0, FINAL_REPLY, OK, (char *)&p, sizeof(struct rp), NULL);
	return 0;
}


void *
server(void *arg)

{
	SERV_ID si;
	char name[20];
	char errMsg[80];
	int instance = (int)(long)arg;

	logMsg("server %d %lx\n", instance, taskIdSelf());
	snprintf(name, sizeof(name), "csTest%02d", instance);
	if (csMboxInit(name, MBOX_RQST_SIZE, 0) != OK) {
		logMsg("server csMboxInit failed %s\n",
		    h2getErrMsg(errnoGet(), errMsg, sizeof(errMsg)));
		return NULL;
	}
	logMsg("server dev %d\n", MY_TASK_DEV);
	if (csServInit(sizeof(struct rq), sizeof(struct rp), &si) != OK) {
		logMsg("csServInit failed\n");
		result = 2;
		goto done;
	}
	if (csServFuncInstall(si, 0, func) != OK) {
		logMsg("csServFuncInstall failed\n");
		result = 2;
		goto done;
	}
	logMsg("server %d ready\n", instance);
#if 0
	if (h2evnSusp(0) != TRUE) {
		logMsg("h2evnSusp failed\n");
		result = 2;
		goto done;
	}
#endif
	if (csMboxWait(WAIT_FOREVER, RCV_MBOX) == ERROR) {

		logMsg("csMboxWait failed\n");
		result = 2;
		goto done;
	}
	logMsg("server %d got request\n", instance);
	if (csServRqstExec(si) != OK) {
		logMsg("csServRqstExec failed %s\n",
		    h2getErrMsg(errnoGet(), errMsg, sizeof(errMsg)));
		result = 2;
		goto done;
	}
	if (csServEnd(si) != OK) {
		logMsg("csServEnd failed\n");
		result = 2;
	}
done:
	logMsg("server ending %d %d\n", instance, result);
	if (csMboxEnd() != OK) {
		logMsg("csMboxEnd failed\n");
		result = 2;
	}
	return NULL;
}

void *
client(void *arg)
{
	char errMsg[80];
	char cname[20];
	CLIENT_ID cid;
	struct rq q;
	struct rp p;
	int rqstid;
	int i = (int)(long)arg;

	logMsg("client %d %lx\n", i, taskIdSelf());
	snprintf(cname, 20, "client%0d", i);
	if (csMboxInit(cname, 0, MBOX_REPLY_SIZE) != OK) {
		logMsg("client csMoxInit failed %d %s\n",
		    i, h2getErrMsg(errnoGet(), errMsg, sizeof(errMsg)));
		result = 2;
		goto done;
	}
	logMsg("client dev %d\n", MY_TASK_DEV);
	snprintf(cname, 20, "csTest%02d", i);
	if (csClientInit(cname, sizeof(struct rq), 0,

		sizeof(struct rp), &cid) != OK) {
		logMsg("csClientInit %d failed %s\n", i,
		    h2getErrMsg(errnoGet(), errMsg, sizeof(errMsg)));
		result = 2;
		goto done;
	}
	if (csClientRqstSend(cid, 0, (char *)&q, sizeof(struct rq),
		NULL, FALSE, 0, 0, &rqstid) == ERROR) {
		logMsg("client %d sending request fails %s\n", i,
		    h2getErrMsg(errnoGet(), errMsg, sizeof(errMsg)));
		result = 2;
		goto done;
	}
	logMsg("client %d request sent\n", i);
	if (csClientReplyRcv(cid, 0, BLOCK_ON_FINAL_REPLY,
	    NULL, 0, NULL,
		(char *)&p, sizeof(struct rp), NULL) == ERROR) {
		logMsg("client %d receiving reply fails %s\n", i,
		    h2getErrMsg(errnoGet(), errMsg, sizeof(errMsg)));
		result = 2;
		goto done;
	}
	if (csClientEnd(cid) != OK) {
		logMsg("csClientEnd %d failed\n", i);
		result = 2;
		goto done;
	}
done:
	logMsg("client ending %d %d\n", i, result);
	pthread_barrier_wait(&barrier);
	return NULL;
}

int
pocoregress_init(void)
{
	int i;
	long pid[MAX_SERV], cpid[MAX_SERV];
	char name[32];

	logMsg("csLibMax test started\n");

	for (i = 0; i < MAX_SERV; i ++) {
		snprintf(name, sizeof(name), "tServ%04d", i);
		pid[i] = taskSpawn2(name, 100, 0, 65536,
		    server, (void *)(intptr_t)i);
		if (pid[i] == ERROR)
			goto done;
	}
	logMsg("servers launched - waiting a bit\n");
	taskDelay(200);
	logMsg("done\n");
	pthread_barrier_init(&barrier, NULL, MAX_SERV + 1);
	for (i = 0; i < MAX_SERV; i++) {
		snprintf(name, sizeof(name), "tClient%04d", i);
		cpid[i] = taskSpawn2(name, 110, 0, 65536,
		    client, (void *)(intptr_t)i);
		if (cpid[i] == ERROR)
			goto done;
	}
	pthread_barrier_wait(&barrier);
done:
	return result;
}
