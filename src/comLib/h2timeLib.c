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


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "portLib.h"
#include "h2timeLib.h"

static H2TIMESPEC h2TimeSpec0;
static void h2timespec_substract(H2TIMESPEC *,
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
    struct timespec ts;

#ifdef __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;
#else
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
	return ERROR;
#endif

    h2timeFromTimespec(pTimeStr, &ts);

    return(OK);
} /* h2timeGet */

STATUS
h2GetTimeSpec(H2TIMESPEC *pTs)
{
	struct timespec ts;
#ifdef __MACH__
	clock_serv_t cclock;
	mach_timespec_t mts;
	
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts.tv_sec = mts.tv_sec;
	ts.tv_nsec = mts.tv_nsec;
#else
	if (clock_gettime(CLOCK_REALTIME, &ts) < 0)
	    return ERROR;
#endif
	if (pTs != NULL) {
		pTs->tv_sec = ts.tv_sec;
		pTs->tv_nsec = ts.tv_nsec;
	}
	return OK;
}


/*----------------------------------------------------------------------*/
void
h2timeFromTimespec(H2TIME* pTimeStr, const H2TIMESPEC *ts)
{
	struct tm tm;
	H2TIMESPEC diff;
	unsigned long rate = sysClkRateGet();

	h2timespec_substract(&diff, ts, &h2TimeSpec0);
	gmtime_r(&ts->tv_sec, &tm);

	pTimeStr->ntick  = diff.tv_sec*rate
	    + (unsigned long long)diff.tv_nsec*rate/1000000000ULL;
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
	h2timespec_substract(&res, &ts, pOldTime);
	*pNmsec = 1000*res.tv_sec + res.tv_nsec / 1000000;
	return OK;
}

/*----------------------------------------------------------------------*/

static void
h2timespec_substract(H2TIMESPEC *result,
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
	h2TimeSpec0.tv_sec = h2TimeSpec0.tv_sec - h2TimeSpec0.tv_sec % 86400;
	h2TimeSpec0.tv_nsec = 0;
} /* h2timeInit */

/*----------------------------------------------------------------------*/
