/*
** SOCKET.C
**
** Written by Steven Grimm (koreth@ebay.sun.com) on 11-26-87
** Please distribute widely, but leave my name here.
**
** Various black-box routines for socket manipulation, so you don't have to
** remember all the structure elements.
*/

/***
 *** Copyright (C) 1991 LAAS/CNRS 
 ***/

static char *rcsid = "$Id$\tLAAS/CNRS";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef UNIX
#include <netdb.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

extern int errno;

/*
** serversock()
**
** Creates an internet socket, binds it to an address, and prepares it for
** subsequent accept() calls by calling listen().
**
** Input: port number desired, or 0 for a random one
** Output: file descriptor of socket, or a negative error
*/
int serversock(port)
int port;
{
    int	sock, x;
    struct	sockaddr_in server;
    int reuseSocket = 1;
    struct linger sockLinger;
    
    sock = socket(AF_INET, SOCK_STREAM, 6);
    if (sock < 0)
	return -errno;
    
   /*
     * Options du socket
     */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&reuseSocket, sizeof(reuseSocket)) < 0) {
        perror("xes: setsockopt REUSEADDR");
        return(-1);
    }
 
    sockLinger.l_onoff = 0;
    sockLinger.l_linger = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER,
                   (char *)&sockLinger, sizeof(sockLinger)) < 0) {
        perror("xes: setsockopt NO LINGER");
        return(-1);
    }
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    
    x = bind(sock, (struct sockaddr *)&server, sizeof(server));
    if (x < 0) {
	perror("xes: bind");
	close(sock);
	return(-1);
    }
    
    listen(sock, 5);
    
    return sock;
}
