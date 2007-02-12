/*
 * Copyright (c) 2006 CNRS/LAAS
 *
 * Permission to use, copy, modify, and distribute this software for any
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
#ident "$Id$"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>

#include <portLib.h>
#include <posterLib.h>

#include "remotePosterLibPriv.h"


static HOST_CLIENT_KEY *hostClientList = NULL;
static pthread_mutex_t clientKeyMutex = PTHREAD_MUTEX_INITIALIZER;

static void clientDestroy(void *);

/*----------------------------------------------------------------------*/
/* 
 * Create or retrieve a key for a given host
 */
int
clientKeyFind(const char *hostname, pthread_key_t *key)
{
	HOST_CLIENT_KEY *hc;

	pthread_mutex_lock(&clientKeyMutex);
	for (hc = hostClientList; hc != NULL; hc = hc->next) {
		if (strcmp(hostname, hc->hostname) == 0) {
			/* found */
			*key = hc->key;
			pthread_mutex_unlock(&clientKeyMutex);
			return 0;
		}
	}
	/* not found */
	hc = (HOST_CLIENT_KEY *)malloc(sizeof(HOST_CLIENT_KEY));
	if (hc == NULL) {
		pthread_mutex_unlock(&clientKeyMutex);
		return -1;
	}
	if (pthread_key_create(&hc->key, clientDestroy) == -1) {
		pthread_mutex_unlock(&clientKeyMutex);
		return -1;
	}
	hc->hostname = strdup(hostname);
	hc->next = hostClientList;
	hostClientList = hc;
	pthread_mutex_unlock(&clientKeyMutex);
	*key = hc->key;
	return 0;
}


/*----------------------------------------------------------------------*/
/*
 * Create the thread-specific client-handle, if needed
 */
CLIENT *
clientCreate(pthread_key_t key, const char *hostname)
{
	CLIENT *client;

	client = pthread_getspecific(key);
	if (client == NULL) {
		client = clnt_create(hostname, POSTER_SERV, 
		    POSTER_VERSION, "tcp");
	} else {
		return client;
	}
	if (client == NULL) {
		clnt_pcreateerror(hostname);
		return(NULL);
	}
	pthread_setspecific(key, client);
	return client;
}


/*----------------------------------------------------------------------*/
/*
 * Remove a client from a thread
 */
void
clientRemove(pthread_key_t key)
{
	CLIENT *client;

	client = pthread_getspecific(key);
	if (client != NULL) {
		clnt_destroy(client);
		pthread_setspecific(key, NULL);
	}
}

/*----------------------------------------------------------------------*/

/*
 * Destroy a thread-specific client handle upon thread termination 
 */
static void
clientDestroy(void *clnt)
{
	clnt_destroy((CLIENT *)clnt);
}

