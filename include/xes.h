/* $LAAS$ */
/*
 * Copyright (c) 1991, 2003 CNRS/LAAS
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
 *** Entree/Sorties interactives VxWorks a travers X
 ***
 *** Matthieu Herrb - Juin 1991
 *** Modification en janvier 1999: ajout de xesStdio
 ***
 *** Definitions communes
 ***/

#ifndef XES_H
#define XES_H

#ifdef __cplusplus
extern "C" {
#endif

#define PORT 1236

#ifdef __STDC__
extern int xes_set_host(char *host);
extern int xes_init(char *host);
extern void xes_set_title(char  *fmt, ...);
extern int xes_get_host(void);

extern STATUS xesStdioInit ( void );
extern STATUS xesStdioSet ( int fd );
extern FILE *xesStdinOfThread(void);
extern FILE *xesStdoutOfThread(void);
extern int xesVPrintf ( const char *fmt, va_list args);
extern int xesPrintf ( const char *fmt, ... );
extern int xesScanf ( const char *fmt, ... );
extern int h2scanf ( const char *fmt, void *value );
#endif

/* 
 * Redefinitions des fonctions d'E/S standard 
 */
#ifndef XES_STDIO_C

#ifdef putchar
#undef putchar
#endif
#define putchar(c) putc(c, xesStdoutOfThread())

#ifdef getchar
#undef getchar
#endif
#define getchar() getc(xesStdinOfThread())

#ifdef printf
#undef printf
#endif
#define printf xesPrintf

#ifdef vprintf
#undef vprintf
#endif
#define vprintf xesVPrintf

#ifdef scanf
#undef scanf
#endif
#define scanf xesScanf

#ifdef stdin
#undef stdin
#endif
#define stdin (xesStdinOfThread())

#ifdef stdout
#undef stdout
#endif
#define stdout (xesStdoutOfThread())

#ifdef stderr
#undef stderr
#endif
#define stderr (xesStdoutOfThread())

#endif /* XES_STDIO_C */
#ifdef __cplusplus
};
#endif

#endif
