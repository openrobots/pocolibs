/***
 *** Entree/Sorties interactives VxWorks a travers X
 ***
 *** Matthieu Herrb - Juin 1991
 ***
 *** Definitions communes
 ***/

/***
 *** Copyright (C) 1991 LAAS/CNRS 
 ***
 *** $Id$
 ***/

#define PORT 1236

#ifdef __STDC__
int xes_set_host(char *host);
int xes_init(char *host);
void xes_set_title(char  *fmt, ...);
int xes_get_host(void);
#endif
