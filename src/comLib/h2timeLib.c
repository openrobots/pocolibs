/*
 * Copyright (c) 1990, 2003-2004 CNRS/LAAS
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

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include "portLib.h"

#if defined(__RTAI__) && defined(__KERNEL__)
# include <linux/time.h>
#else
# include <stdio.h>
# include <time.h>
# include <sys/time.h>
# include <sys/types.h>
#endif

#include "h2timeLib.h"

/*----------------------------------------------------------------------*/

/**
 **  h2timeGet  -  Lire la valeur actuelle de l'horloge
 **
 **  Description : 
 **  Lit la valeur actuelle de l'horloge et la met dans la structure
 **  fournit par l'utlisateur.
 **
 **  Retourne : OK ou ERROR
 **/

STATUS
h2timeGet(H2TIME *pTimeStr)
{
    struct timeval tv;

#if defined(__RTAI__) && defined(__KERNEL__)
    long tmp;
    do_gettimeofday(&tv);
#else
    struct tm *tmp;
    gettimeofday(&tv, NULL);
#endif

    pTimeStr->ntick = tv.tv_sec * NTICKS_PER_SEC + tv.tv_usec / TICK_US;
    pTimeStr->msec = tv.tv_usec / 1000;

#if defined(__RTAI__) && defined(__KERNEL__)
    tmp = tv.tv_sec;
    pTimeStr->sec = tmp % 60;
    tmp /= 60;
    pTimeStr->minute = tmp % 60;
    tmp /= 60;
    pTimeStr->hour = tmp % 24;

    /* XXX I'll work on that later */
    pTimeStr->day = 1;
    pTimeStr->date = 1;
    pTimeStr->month = 1;
    pTimeStr->year = 2004;
#else
    tmp = localtime((time_t *)&tv.tv_sec);

    pTimeStr->sec = tmp->tm_sec;
    pTimeStr->minute = tmp->tm_min;
    pTimeStr->hour = tmp->tm_hour;
    pTimeStr->day = tmp->tm_wday == 0 ? 7 : tmp->tm_wday;
    pTimeStr->date = tmp->tm_mday;
    pTimeStr->month = tmp->tm_mon + 1;
    pTimeStr->year = tmp->tm_year;
#endif

    /* free(tmp); */
    
    return(OK);
} /* h2timeGet */

/*----------------------------------------------------------------------*/

STATUS
h2timeSet(H2TIME *pTimeStr)
{

    return(ERROR);
} /* h2timeSet */

/*----------------------------------------------------------------------*/

STATUS
h2timeAdj(H2TIME *pTimeStr)
{

    return(ERROR);
} /* h2timeAdj */

/*----------------------------------------------------------------------*/

/**
 **  h2timeShow  - Envoyer la date vers la sortie standard
 **
 **  Description:
 **  Envoie la date vers la sortie standard.
 **
 **  Retourne : Rien
 **/
void
h2timeShow(void)
{
    H2TIME strTime;
    static char *dayStr [] = {
	"lundi", "mardi", "mercredi", "jeudi",
	"vendredi", "samedi", "dimanche"};

    /* Demander la lecture de la date */
    if (h2timeGet (&strTime) != OK) {
	logMsg ("Probleme de lecture de la date!\n");
	return;
    }
    
    /* Envoyer la date vers la console */
    logMsg ("\nDate: %02d-%02d-%02d, %s, %02dh:%02dmin:%02ds\n\n", 
	    strTime.date, strTime.month, strTime.year, 
	    dayStr[strTime.day - 1], strTime.hour, strTime.minute, 
	    strTime.sec);
} /* h2timeShow */

/*----------------------------------------------------------------------*/

/**
 **  h2timeInterval  -  Intervalle de temps entre deux lectures horloge
 **
 **  Description: 
 **  Donne l'ecart en milli-secondes entre une ancienne lecture de l'horloge
 **  et sa valeur actuelle. 
 **
 **  Retourne : OK ou ERROR
 **/

STATUS
h2timeInterval (H2TIME *pOldTime, unsigned long *pNmsec)
{
    H2TIME strTime;		/* Ou` mettre la valeur actuelle */
    unsigned long ntick1, ntick2;	/* Nombre de ticks */
 
    /* Lire l'horloge */
    if (h2timeGet (&strTime) != OK)
	return (ERROR);
    
    /* Charger les compteurs de ticks */
    ntick2 = strTime.ntick;
    ntick1 = pOldTime->ntick;
    
    /* Retourner l'intervalle de temps */
    *pNmsec = 1000 * ((ntick2 >= ntick1) ? (ntick2 - ntick1)
		   : (((unsigned long) ~0 - ntick1) + 1 + ntick2))
			/NTICKS_PER_SEC;
    return (OK);
} /* h2timeInterval */

/*----------------------------------------------------------------------*/

void
h2timeInit(void)
{

} /* h2timeInit */

/*----------------------------------------------------------------------*/
