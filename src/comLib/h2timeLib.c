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


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include "portLib.h"
#include "h2timeLib.h"

static H2TIMESPEC h2TimeSpec0;
static void h2timespec_subtract(H2TIMESPEC *, 
    const H2TIMESPEC *, const H2TIMESPEC *);

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

    gettimeofday(&tv, NULL);

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

	gettimeofday(&tv, NULL);
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

    if (day < 0) {
	    day = 365 + day;
	    year -= 1;
    }
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
void
h2timeFromTimespec(H2TIME* pTimeStr, const H2TIMESPEC *ts)
{
	struct tm tm;
	H2TIMESPEC diff;
	unsigned long rate = sysClkRateGet();

	h2timespec_subtract(&diff, ts, &h2TimeSpec0);
	gmtime_r(&ts->tv_sec, &tm);

	pTimeStr->ntick  = diff.tv_sec*rate + (unsigned long long)diff.tv_nsec*rate/1000000000ULL;
	printf("xxx %lu:%09lu\n", diff.tv_sec, diff.tv_nsec);
	pTimeStr->msec   = diff.tv_nsec / 1000000;
	pTimeStr->sec    = tm.tm_sec;
	pTimeStr->minute = tm.tm_min;
	pTimeStr->hour   = tm.tm_hour;
	pTimeStr->day    = tm.tm_wday == 0 ? 7 : tm.tm_wday;
	pTimeStr->date   = tm.tm_mday;
	pTimeStr->month  = tm.tm_mon + 1;
	pTimeStr->year   = tm.tm_year;
}

/*----------------------------------------------------------------------*/
void
timespecFromH2time(H2TIMESPEC *ts, const H2TIME *pTimeStr)
{
	struct tm tm;

	tm.tm_sec = pTimeStr->sec;
	tm.tm_min = pTimeStr->minute;
	tm.tm_hour = pTimeStr->hour;
	tm.tm_wday = pTimeStr->day;
	tm.tm_mday = pTimeStr->date;
	tm.tm_mon = pTimeStr->month - 1;
	tm.tm_year = pTimeStr->year;

	ts->tv_sec = mktime(&tm);
	ts->tv_nsec = pTimeStr->msec*1000000;
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

/**
 ** Returns the difference in mili-seconds between current time and 
 ** an older H2TIMESPEC value
 **/

STATUS
h2timespecInterval(const H2TIMESPEC *pOldTime, unsigned long *pNmsec)
{
	H2TIMESPEC ts, res;

	if (h2GetTimeSpec(&ts) != OK)
		return (ERROR);
	res.tv_sec = ts.tv_sec - pOldTime->tv_sec;
	res.tv_nsec = ts.tv_nsec - pOldTime->tv_nsec;
	if (res.tv_nsec < 0) {
		res.tv_sec--;
		res.tv_nsec += 1000000000;
	}
	*pNmsec = 1000*res.tv_sec + res.tv_nsec / 1000000;
	return OK;
}

/*----------------------------------------------------------------------*/

static void
h2timespec_subtract(H2TIMESPEC *result, 
    const H2TIMESPEC *x, const H2TIMESPEC *y)
{
	H2TIMESPEC yy;

	memcpy(&yy, y, sizeof(H2TIMESPEC));

	/* Carry */
	if (x->tv_nsec < y->tv_nsec) {
		long sec = (y->tv_nsec - x->tv_nsec) / 1000000000 + 1;
		yy.tv_nsec -= 1000000000 * sec;
		yy.tv_sec += sec;
	}
	if (x->tv_nsec - y->tv_nsec > 1000000000) {
		int sec = (x->tv_nsec - y->tv_nsec) / 1000000000;
		yy.tv_nsec += 1000000000 * sec;
		yy.tv_sec -= sec;
	}
	
	result->tv_sec = x->tv_sec - yy.tv_sec;
	result->tv_nsec = x->tv_nsec - yy.tv_nsec;
}

/*----------------------------------------------------------------------*/

void
h2timeInit(void)
{
	h2GetTimeSpec(&h2TimeSpec0);
} /* h2timeInit */

/*----------------------------------------------------------------------*/
