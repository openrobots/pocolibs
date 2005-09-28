/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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

/***
 *** Entrees/sorties interactives VxWorks sous X
 ***
 *** Matthieu Herrb - Juin 1991
 ***
 *** Module client Unix et VxWorks
 ***/

/*----------------------------------------------------------------------

  Mode d'emploi:

  xes_set_host(<machine>) definit le nom de la machine sur laquelle tourne
                          xes_server. C'est l'environnement Unix sur cette
			  machine (.Xdefaut, variable DISPLAY) qui definit
			  les caracte'ristiques des xterms.

  xes_init(<machine> | NULL) initialise une fenetre d'entre'e/sortie (xterm) 
                          sur <machine>. Si on spe'cifie NULL, la machine 
			  de'finie par xes_set_host est utilise'e.

----------------------------------------------------------------------*/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

#include "portLib.h"

#include "xes.h"

static char *xes_host = "localhost";	/* le nom du serveur xes */

/*----------------------------------------------------------------------*/

/**
 ** clientsock()
 **
 ** Returns a connected client socket.
 **
 ** Input: host name and port number to connect to
 ** Output: file descriptor of CONNECTED socket, or a negative error (-9999
 **         if the hostname was bad).
 **
 ** Written by Steven Grimm (koreth@ebay.sun.com) on 11-26-87
 ** Please distribute widely, but leave my name here.
 **/

static int 
clientsock(char *host, int port)
{
    int	sock;
    struct sockaddr_in server;
    struct hostent *hp;
    
    memset((char *)&server, sizeof(server), 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (isdigit(host[0]))
	server.sin_addr.s_addr = inet_addr(host);
    else {
	hp = gethostbyname(host);
	if (hp == NULL)
	    return -9999;
	memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    }
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
	return -errno;
    
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
	close(sock);
	return -errno;
    }
    
    return sock;
}

/*----------------------------------------------------------------------*/

/**
 ** La fonction pour definir la machine hote de xes
 **/
int 
xes_set_host(char *host)
{
    struct hostent *hp = gethostbyname(host);

    if (hp == NULL) {
	xes_host = NULL;
	return(-1);
    }
    xes_host = (char *)malloc(strlen(host) + 1);
    if (xes_host == NULL) {
      return(-1);
    }
    strcpy(xes_host, host);
    return(0);
}

/*----------------------------------------------------------------------*/

int
xes_get_host(void)
{
    if (xes_host != NULL) {
	printf("xes host: %s\n", xes_host);
	return(0);
    } else {
	printf("no xes host defined.\n");
	return(-1);
    }
} /* xes_get_host */

/*----------------------------------------------------------------------*/

/**
 **  La fonction d'initialisation du service xes: ouvre une connexion
 **  avec le serveur et redirige stdin et stdout dessus.
 **
 ** argument: host: soit un nom de serveur, soit NULL pour utiliser le
 **                  serveur par defaut.
 ** retourne le file descriptor du socket ou -1 si erreur
 **/
int 
xes_init(char *host)
{
    int fd;

    if (host == NULL)
	host = xes_host;

    if (host == NULL) {
	fprintf(stderr, "xes_init: pas de serveur defini\n");
	return(-1);
    }
    
    /* Ouverture de la conexion */
    fd = clientsock(host, PORT);

    if (fd < 0) {
	perror("xes_init: clientsock");
	return(-1);
    }

    /* Utilise ce file descriptor comme E/S standard dans ce thread */
    if (xesStdioSet(fd) == ERROR) {
	return -1;
    }
    
    return(fd);
}

/*----------------------------------------------------------------------*/

/**
 ** Cette fonction permet de changer le titre d'un xterm 
 ** gere par xes
 ** arguments comme printf()
 **/
void
xes_set_title(char  *fmt, ...)
{
    va_list ap;
    
    if (fmt == NULL) {
	return;
    }
    va_start(ap, fmt);
    
    printf("\x1b]2;");
    vprintf(fmt, ap);
    printf("\x7");
    
}
/*----------------------------------------------------------------------*/

/***
 *** Un petit programme de test...
 *** (Definir TEST lors de la compilation)
 ***/

#ifdef TEST

main(int argc, char *argv[])
{
    static char buf[80];

    char *host;

    if (argc > 1)
	host = argv[1];
    else {
	fprintf(stderr, "syntaxe: %s <host>\n", argv[0]);
	exit(1);
    }
    
    if (xes_init(host) < 0) {
	exit (1);
    }
    while (1) {
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	
	printf("test> ");
	
	gets(buf);
	printf("-> %s\n", buf);
    }

    exit(0);
}
#endif /* TEST */
