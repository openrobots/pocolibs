/***
 *** Entree/Sorties interactives VxWorks a travers X
 ***
 *** Matthieu Herrb - Juin 1991
 *** Portage a Solaris 2 - Avril 1994
 ***
 *** Module serveur X
 ***/

/***
 *** Copyright (C) 1991,1997 LAAS/CNRS 
 ***/

#ident "$Id$"

/*
 * Base' sur: 
 *
 * slavewindow.c
 *
 * Copyright Richard Tobin / AIAI 1990.
 * May be freely distributed if this notice remains intact.
 *    
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#ifndef linux
#include <sys/filio.h>
#endif
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#if defined(USE_STREAMS) || defined(USE_SYSV_PTY)
#include <sys/stream.h>
#include <sys/stropts.h>
#endif
#include <X11/Xlib.h>

#include "xes.h"
#include "rngLib.h"
#include "version.h"

/* IRIX n'a pas vfork() */
#ifdef __sgi__
#define vfork fork
#endif

/*----------------------------------------------------------------------*/

/**
 ** Variables globales 
 **/
 
int serv_socket_fd;
int *win;			/* tableau des fenetres */
int *open_socket_fd;		/* tableau des clients */
int *win_pid;			/* tableau des pid des xterms */
RNG_ID *win_rng;		/* tableau des ring buffers des xterms */
RNG_ID *sock_rng;		/* tableau des rings buffers des clients */
int tablesize;

int read_junk = 0;		/* flag pour lecture WinId xterm */

/*----------------------------------------------------------------------*/

/**
 ** Prototypes
 **/

int copyin(int in, RNG_ID out);
void copyout(RNG_ID in, int out);
int create_window(int *win_fd, int *win_pid);
void set_term_modes(int to);

extern int serversock(int port);

/*----------------------------------------------------------------------*/

/**
 ** traitement des signaux
 **/
void
sigHandler(int sig)
{
    unlink(PID_FILE);
    exit(0);
}

void
pipeHandler(int sig)
{
    printf("Sigpipe\n");
}

/*----------------------------------------------------------------------*/

/**
 ** Le programme principal:
 **
 ** Cree un socket, attend une connexion puis cree le xterm esclave
 **/

int
main(int argc, char *argv[])
{
    struct sockaddr *open_socket;
    int i, length, newconn, pid;
    int stat, child_stat;
    fd_set ibits, obits, ebits;
    char buf[500];
    struct timeval timeout;
    FILE *pid_file;
#ifdef USE_RLIMIT
    struct rlimit rl;
#endif
    Display *dpy;
    XEvent event;


    /* 
     * Connexion au serveur X
     */
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
	fprintf(stderr, "xes: cannot open display\n");
	exit(1);
    }
    /*
     * Verifie si /tmp/xes-pid existe
     */
    if (access(PID_FILE, F_OK | R_OK) == 0) {
	fprintf(stderr, "xes: %s existe\nUtilisez kill_xes\n", PID_FILE);
	exit(1);
    }
    
    /*
     * Lancement en backgound
     */
    if ((pid = fork()) != 0) {
	printf("xes_server version %s.%s\n"
	       "Copyright (C) LAAS/CNRS 1991-2000\n",
	       major_version, minor_version);
	/* Creation du fichier xes-pid */
	pid_file = fopen(PID_FILE, "w");
	if (pid_file == NULL) {
	    perror("xes: create /tmp/xes-pid");
	    kill(pid, SIGINT);
	    exit(1);
	}
	fprintf(pid_file, "%d\n", pid);
	fclose(pid_file);
	exit(0);
    }
    /* 
     * Detache le serveur du terminal
     */
#if !defined(SVR4) && !defined(linux)
    i = open("/dev/tty", 2);
    if (i >= 0) {
	(void) ioctl(i, TIOCNOTTY, (char *) 0);
	(void) close(i);
    }
#else
    setsid();
#endif

    /*
     * installe handler signaux pour fin propre
     */
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, sigHandler);
    signal(SIGQUIT, sigHandler);
    signal(SIGTERM, sigHandler);

    /*
     * Creation du service
     */
    serv_socket_fd = serversock(PORT);
    if (serv_socket_fd < 0) {
	exit(-1);
    }
#ifndef USE_RLIMIT
    tablesize = getdtablesize();
#else
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
	perror("getrlimit");
	exit(1);
    }
    tablesize = rl.rlim_cur;
#endif
    win = (int *)malloc(tablesize * sizeof(int));
    open_socket_fd = (int *)malloc(tablesize * sizeof(int));
    open_socket = (struct sockaddr *)malloc(tablesize *
					    sizeof(struct sockaddr));
    win_pid = (int *)malloc(tablesize * sizeof(int));
    win_rng = (RNG_ID *)malloc(tablesize * sizeof(RNG_ID));
    sock_rng = (RNG_ID *)malloc(tablesize * sizeof(RNG_ID));
    
    if (win == NULL || open_socket_fd == NULL || open_socket == NULL
	|| win_pid == NULL || win_rng == NULL || sock_rng == NULL) {
	fprintf(stderr, "xes: pas assez de memoire\n");
	exit(-1);
    }
    for (i = 0; i < tablesize; i++) {
	win[i] = -1;
	open_socket_fd[i] = -1;
	win_pid[i] = -1;
	win_rng[i] = NULL;
	sock_rng[i] = NULL;
    }

    /*
     * boucle d'attente d'evenements
     */
    
    while (1) {
	/* Creation du masque pour select */
	FD_ZERO(&ibits);
	FD_ZERO(&obits);
	FD_ZERO(&ebits);
	for (i = 0; i < tablesize;  i++) {
	    if (open_socket_fd[i] >=0 && win[i] >= 0) {
		FD_SET(win[i], &ibits);
		FD_SET(open_socket_fd[i], &ibits);
		FD_SET(win[i], &ebits);
		FD_SET(open_socket_fd[i], &ebits);
	    }
	    if (win_rng[i] != NULL && !rngIsEmpty(win_rng[i])) {
		FD_SET(win[i], &obits);
	    }
	    if (sock_rng[i] != NULL && !rngIsEmpty(sock_rng[i])) {
		FD_SET(open_socket_fd[i], &obits);
	    }
	}
	FD_SET(serv_socket_fd, &ibits);
	FD_SET(serv_socket_fd, &ebits);

	/* traite les evenements X */
	FD_SET(ConnectionNumber(dpy), &ebits);
	FD_SET(ConnectionNumber(dpy), &ibits);
	
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
	
	/* Effectue les wait sur les fils eventuellement morts */
	while ((pid = waitpid(-1, &child_stat, WNOHANG)) > 0) {
	    for (i = 0; i < tablesize; i++) {
		if (win_pid[i] == pid) {
		    rngDelete(win_rng[i]);
		    close(win[i]);
		    close(open_socket_fd[i]);
		    win_pid[i] = win[i] = open_socket_fd[i] = -1;
		}
	    } /* for */
	} /* while */

	/* Attente d'un evenement  d'ES */
	stat = select(tablesize, &ibits, &obits, &ebits,
		      &timeout);

	/* Depuis 4.1.1, select est interruptible */
	if (stat == -1 && errno == EINTR)
	    continue;
	
	if(stat == -1)	{
	    perror("xes: select");
	    continue;
	}
	
	/*
	 * Evenements X
	 */
	if (FD_ISSET(ConnectionNumber(dpy), &ibits)) {
	    /* Connexion ferm�e */
	    if (QLength(dpy) == 0) {
		unlink(PID_FILE);
		exit(0);
	    }
	    /* Traitement des �venements (?) */
	    for (i = 0; i < QLength(dpy); i++) {
		XNextEvent(dpy, &event);
		fprintf(stderr, "xes: XEvent: %d\n", event.type);
	    }
	}
	if (FD_ISSET(ConnectionNumber(dpy), &ebits)) {
	    fprintf(stderr, "Erreur X\n");
	    unlink(PID_FILE);
	    exit (0);
	}
	    
	/*
	 * Demande de connexion
	 */
	if (FD_ISSET(serv_socket_fd, &ibits)) {
	    /* Recherche d'une connexion libre */
	    newconn = -1;
	    for (i = 0; i < tablesize; i++)
		if (open_socket_fd[i] == -1) {
		    newconn = i;
		    break;
		}
	    if (newconn == -1) {
		fprintf(stderr, "xes: peut pas accepter nouvelle connexion\n");
		continue;
	    }

	    /* Creation des Ring Buffer associe' */
	    win_rng[newconn] = rngCreate(RNG_SIZE);
	    if (win_rng[newconn] == NULL) {
		perror("xes: cannot allocate ring buffer");
		continue;
	    }
	    sock_rng[newconn] = rngCreate(RNG_SIZE);
	    if (sock_rng[newconn] == NULL) {
		perror("xes: cannot allocate ring buffer");
		continue;
	    }

	    /* creation de la connexion avec le client */
	    length = sizeof(struct sockaddr); 
	    open_socket_fd[newconn] = accept(serv_socket_fd,
					     &(open_socket[newconn]), &length);
	    if (open_socket_fd[newconn] == -1) {
		perror("xes: accept");
		continue;
	    }

	    /* Passage en mode non-bloquant de ce fd */
	    if (fcntl(open_socket_fd[newconn], F_SETFL, O_NONBLOCK) < 0) {
		perror("xes: set socket flags");
	    }
	    
	    /* creation de la fenetre d'ES */
	    if (create_window(&win[newconn], &win_pid[newconn]) < 0) {
		perror("xes: create_window");
		continue;
	    }
	}

	/* Erreur sur le socket serveur */
	if (FD_ISSET(serv_socket_fd, &ebits)) {
	    printf("xes: erreur sur serv_soket_fd\n");
	}

	/*
	 * Teste le traffic entre les clients et les fenetres
	 */
	for (i = 0; i < tablesize; i++) {
	    
	    if ((win[i] >=0 && FD_ISSET(win[i], &ebits)) 
		|| (open_socket_fd[i] >= 0 
		    && FD_ISSET(open_socket_fd[i], &ebits))) {
		close(win[i]);
		close(open_socket_fd[i]);
		win[i] = open_socket_fd[i] = -1;
	    }

	    if (win[i] >=0 && FD_ISSET(win[i], &ibits))	{
#ifdef DEBUG
		printf("win[%d] -> socket\n", i);
#endif
		if (copyin(win[i], sock_rng[i]) == EOF) {
		    buf[0] = '\004';
		    write(open_socket_fd[i], buf, 1);
		    close(win[i]);
		    close(open_socket_fd[i]);
		    kill(win_pid[i], SIGTERM);
		    win[i] = open_socket_fd[i] = win_pid[i] = -1;
		    rngDelete(win_rng[i]);
		    win_rng[i] = NULL;
		    rngDelete(sock_rng[i]);
		    sock_rng[i] = NULL;
		}
	    }

	    if (open_socket_fd[i] >= 0 
		&& FD_ISSET(open_socket_fd[i], &ibits)) {
#ifdef DEBUG
		printf("socket -> win[%d]\n", i);
#endif
		if (copyin(open_socket_fd[i], win_rng[i]) == EOF) {
		    close(win[i]);
		    close(open_socket_fd[i]);
		    win[i] = open_socket_fd[i] = -1;
		    rngDelete(win_rng[i]);
		    win_rng[i] = NULL;
		    rngDelete(sock_rng[i]);
		    sock_rng[i] = NULL;
		}
	    }

	    if (win[i] >=0 && FD_ISSET(win[i], &obits))	{
		copyout(win_rng[i], win[i]);
	    }

	    if (open_socket_fd[i] >= 0 
		&& FD_ISSET(open_socket_fd[i], &obits)) {
		copyout(sock_rng[i], open_socket_fd[i]);
	    }

	} /* for */

    } /* while */
    return 0;
}

/*----------------------------------------------------------------------*/

/**
 ** Copie des caracte`res d'un file descriptor sur un autre
 **/

int 
copyin(int in, RNG_ID out)
{
    static char buf[BUFFER_SIZE];
    int m, n, nlus = 0;
    
#ifndef USE_STREAMS
    if ((m = ioctl(in, FIONREAD, &n)) < 0) {
	perror("xes: ioctl FIONREAD");
    }
#else
    if ((m = ioctl(in, I_NREAD, &n)) < 0) {
	perror("ioctl I_NREAD");
	return(EOF);
    }
#endif
    
    if (n == 0)
	return(EOF);

    while (n > 0) {
	if ((nlus = read(in, buf, n <= BUFFER_SIZE ? n : BUFFER_SIZE)) <= 0) {
	    return(EOF);
	}
#ifdef DEBUG
	printf("buf[0] = %c (0x%d)\n", buf[0], buf[0]);
#endif
	if (rngBufPut(out, buf, nlus) < nlus) {
	    fprintf(stderr, "xes: ring buffer overflow\n");
	} 
	n -= nlus;

    }
    return(n<0 ? EOF : 0);
}

/*----------------------------------------------------------------------*/

void 
copyout(RNG_ID in, int out) 
{
    static char buf[BUFFER_SIZE];
    int ntot, n;
    
    ntot = rngNBytes(in);
    while (ntot > 0) {
	
	n = (ntot <= BUFFER_SIZE ? ntot : BUFFER_SIZE);
	rngBufSpy(in, buf, n);
	
	if ((n = write(out, buf, n)) <= 0) {
	    if (errno == EWOULDBLOCK || errno == EAGAIN) {
		/* rien ecrit - tant pis on reessaye + tard */
		return;
	    } else {
		perror("xes: write");
		return;
	    }
	} else {
	    /* Ecriture totale ou partielle */
	    ntot -= n;
	    rngBufSkip(in, n);
	}
    } /* while */
} /* copyout */

/*----------------------------------------------------------------------*/

/**
 ** Creation d'un Xterm esclave
 **/
int 
create_window(int *win_fd, int *win_pid)
{
    int  master, slave, pid;
    struct termios termioStr;
    static char buf[100];
    char *master_path, *slave_path;

#ifndef USE_SYSV_PTY
    int i;

    master_path = strdup(MASTER);
    slave_path = strdup(SLAVE);
    
    for(i = 0; i <= ('s'-'p') * 16; i++) {
	
        master_path[strlen(MASTER)-2] = i / 16 + 'p';
        master_path[strlen(MASTER)-1] = "0123456789abcdef"[i & 15];
        master = open(master_path,O_RDWR);
        if(master >= 0)
	    break;
    }

    if(master < 0) {
	fprintf(stderr, "xes: can't get a pty\n");
	exit(1);
    }

    slave_path[strlen(SLAVE)-2] = master_path[strlen(MASTER)-2];
    slave_path[strlen(SLAVE)-1] = master_path[strlen(MASTER)-1];

#else /* USE_SYSV_PTY */
    
    master = open("/dev/ptmx", O_RDWR);
    if (master < 0) {
	perror("xes: open /dev/ptmx");
    }
    if (grantpt(master) < 0) {
	perror("xes: grantpt");
    }
    if (unlockpt(master) < 0) {
	perror("xes: unlockpt");
    }
    slave_path = ptsname(master);
    if (slave_path == NULL) {
	perror("xes: ptsname");
    }

#endif /* USE_SYSV_PTY */
    
    sprintf(buf, "-S%s%d", &slave_path[strlen(slave_path)-2], master);

    if((pid = vfork()) == 0) {

#if !defined(SVR4) && !defined(linux)
	setpgrp(0, getpid());
#else
	setsid();
#endif
	signal(SIGPIPE, pipeHandler);
	execlp("xterm", "xterm(xes)", buf, 0);
	fprintf(stderr, "xterm exec failed\n");
	exit(1);
    }
    if (pid == -1) {
	perror("xes: vfork");
    }

    close(master);

    slave = open(slave_path, O_RDWR | O_NONBLOCK, 0);
    if (slave < 0) {
	fprintf(stderr, "xes: open slave %s: %s", 
		slave_path, strerror(errno));
	return -1;
    }

#ifdef USE_SYSV_PTY
    if (ioctl(slave, I_PUSH, "ptem") < 0) {
	perror("xes: push ptem");
	return -1;
    }
    if (ioctl(slave, I_PUSH, "ldterm") < 0) {
	perror("xes: push ldterm");
	return -1;
    }
#endif

    /* Passe en mode no echo pour recevoir le junk */
    if (tcgetattr(slave, &termioStr) < 0) {
	perror("xes: tcgetattr");
    }
    termioStr.c_lflag &= ~ECHO;
    if (tcsetattr(slave, TCSANOW, &termioStr) < 0) {
	perror("xes: tcsetattr");
    }
    /* Repasse en mode bloquant */
    if (fcntl(slave, F_SETFL, 0) < 0) {
	perror("xes: set blocking io");
    }
    (void)read(slave, buf, sizeof(buf)); /* suck xterm junk before echo on */
    
    if (fcntl(slave, F_SETFL, O_NONBLOCK) < 0) {
	perror("xes: set non-blocking io");
    }

#ifdef linux
    (void)signal(SIGPIPE, pipeHandler);
#endif

    set_term_modes(slave);

    *win_fd = slave;
    *win_pid = pid;
    return(0);

} /* create_window */

/*----------------------------------------------------------------------*/

/**
 ** definit le mode par defaut du terminal
 **/
void set_term_modes(to)
int to;
{
    int err = 0;
    struct termios termioStr;

    termioStr.c_iflag = BRKINT | ICRNL | IXON | IMAXBEL;
#ifdef TAB3
    termioStr.c_oflag = OPOST | ONLCR | TAB3;
#else
    termioStr.c_oflag = OPOST | ONLCR;
#endif
    termioStr.c_cflag = B9600 | CS8 | CREAD;
    termioStr.c_lflag = ISIG | ICANON | IEXTEN | ECHO | ECHOK | ECHOE 
	| ECHOKE | ECHOCTL;
    memset(termioStr.c_cc, 0, sizeof(termioStr.c_cc));
    termioStr.c_cc[VINTR] = 0x03;
    termioStr.c_cc[VQUIT] =  0x1c; 
    termioStr.c_cc[VERASE] = 0x7f; 
    termioStr.c_cc[VKILL] = 0x15;
    termioStr.c_cc[VEOF] = 0x04;

    err = tcsetattr(to, TCSANOW, &termioStr);

    if(err != 0)
	perror("can't set terminal modes");

} /* set_term_modes */

/*----------------------------------------------------------------------*/


