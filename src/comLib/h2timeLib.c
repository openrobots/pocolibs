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
    do_gettimeofday(&tv);
#else
    gettimeofday(&tv, NULL);
#endif

    h2timeFromTimeval(pTimeStr, &tv);
    /* free(tmp); */
    
    return(OK);
} /* h2timeGet */

STATUS
h2GetTimeSpec(H2TIMESPEC *pTs)
{
#ifdef HAVE_CLOCK_GETTIME
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
		return ERROR;
	if (pTs != NULL) {
		pTs->tv_sec = ts.tv_sec;
		pTs->tv_nsec = ts.tv_nsec;
	}
#else
	struct timeval tv;

#if defined(__RTAI__) && defined(__KERNEL__)
	do_gettimeofday(&tv);
#else
	gettimeofday(&tv, NULL);
#endif
	if (pTs != NULL) {
		pTs->tv_sec = tv.tv_sec;
		pTs->tv_nsec = tv.tv_usec * 1000;
	}
#endif
	return OK;
}

static int days_in_year[] = 
    { 0,                // Janvier
      31,               // Fevrier
      31+28,            // Mars
      2*31+28,          // Avril
      2*31+30+28,       // Mai
      3*31+30+28,       // Juin
      3*31+2*30+28,     // Juillet
      4*31+2*30+28,     // Aout
      5*31+2*30+28,     // Septembre
      5*31+3*30+28,     // Octobre
      6*31+3*30+28,     // Novembre
      6*31+4*30+28,     // Decembre
      7*31+4*30+28
    };
#define is_bisextile_exception(y) ( ((y) % 100 == 0) && ((y) % 400 != 0) )
#define is_bisextile(y)     ( ( !((y) % 4) && !is_bisextile_exception(y) ) ? 1 : 0 )
#define days_since_zero(y)  ( (y) * days_in_year[12] + ((y) / 4) + ((y) / 400) - ((y) / 100) )
#define days_since_epoch(y) ( days_since_zero(y) - days_since_zero(1970) )

void
h2timeFromTimeval(H2TIME* pTimeStr, const struct timeval* tv)
{
    long sec, day, dow, month, year;
    static int day_per_month[] = 
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    pTimeStr->ntick = tv -> tv_sec * NTICKS_PER_SEC + tv -> tv_usec / TICK_US;
    pTimeStr->msec = tv -> tv_usec / 1000;

    sec = tv -> tv_sec;
    day  = sec / (3600 * 24); // day since 1970
    dow = (day + 4) % 7;		/* the 1/1/70 was a Thurday */
    year = (day / 365 + 1970);        // We'll have one year difference each 4*365 years (roughly)
    day  -= days_since_epoch(year);  // day in the year

    day_per_month[1] = is_bisextile(year) ? 29 : 28;
    for (month = 0; month < 12; ++month)
    {
        if (day < day_per_month[month])
            break;
        day -= day_per_month[month];
    }
    // now, day is the day in the month and month the month index

    pTimeStr->sec    = sec % 60;
    pTimeStr->minute = (sec / 60) % 60;
    pTimeStr->hour   = (sec / 3600) % 24;
    pTimeStr->day   = dow;
    pTimeStr->date  = day + 1;
    pTimeStr->month = month + 1;
    pTimeStr->year  = year - 1900;
//#else
//    tmp = localtime((time_t *)&tv -> tv_sec);
//
//    pTimeStr->sec    = tmp->tm_sec;
//    pTimeStr->minute = tmp->tm_min;
//    pTimeStr->hour   = tmp->tm_hour;
//    pTimeStr->day    = tmp->tm_wday == 0 ? 7 : tmp->tm_wday;
//    pTimeStr->date   = tmp->tm_mday;
//    pTimeStr->month  = tmp->tm_mon + 1;
//    pTimeStr->year   = tmp->tm_year;
//#endif
}

void
timevalFromH2time(struct timeval* tv, const H2TIME* pTimeStr)
{
    int year = pTimeStr->year + 1900;
    int days = pTimeStr->date - 1
             + days_in_year[pTimeStr->month - 1]
             + ((pTimeStr->month > 2) ? is_bisextile(year) : 0)
             + days_since_epoch(year);

    tv -> tv_usec = pTimeStr->msec*1000;

    tv -> tv_sec 
        = pTimeStr->sec 
            + pTimeStr->minute*60
            + pTimeStr->hour  *3600
            + days            *24*3600;
}

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
	    "sunday", "monday", "tuesday", "wednesday", "thursday",
	    "friday", "saturday"};
    char buf[50];

    /* Demander la lecture de la date */
    if (h2timeGet (&strTime) != OK) {
	logMsg ("Problem reading date/time!\n");
	return;
    }
    
    /* Envoyer la date vers la console */
    snprintf(buf, sizeof(buf), 
	"\nDate: %02u-%02u-%02u, %s, %02uh:%02umin:%02us\n\n", 
	strTime.month, strTime.date, strTime.year + 1900, 
	dayStr[strTime.day], strTime.hour, strTime.minute, 
	strTime.sec);
    logMsg(buf);
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
