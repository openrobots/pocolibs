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

#include "config.h"
__RCSID("$LAAS$");

#ifndef HAVE_VFSCANF
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <values.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


#define NCHARS	(1 << BITSPERBYTE)

static int mnumber(int stow, int type, int len, int size, 
		   FILE *iop, va_list *listp);
static int mstring(int stow, int type, int len, char *chartab, 
		   FILE *iop, va_list *listp);
static const char *setup(const char *fmt, char *chartab);

/*
 * fonction d'analyse du stream a l'aide du format indique
 *
 */
int
vfscanf(FILE *iop, const char *fmt, va_list va_alist)
{
	char chartab[NCHARS];
	int ch;
	int nmatch = 0, len, inchar, stow, size;

	/*******************************************************
	 * Main loop: reads format to determine a pattern,
	 *		and then goes to read input stream
	 *		in attempt to match the pattern.
	 *******************************************************/
	for( ; ; ) {
		if((ch = *fmt++) == '\0')
			return(nmatch); /* end of format */
		if(isspace(ch)) {
			while(isspace(inchar = getc(iop)))
				;
			if(ungetc(inchar, iop) != EOF)
				continue;
			break;
		}
		if(ch != '%' || (ch = *fmt++) == '%') {
			if((inchar = getc(iop)) == ch)
				continue;
			if(ungetc(inchar, iop) != EOF)
				return(nmatch); /* failed to match input */
			break;
		}
		if(ch == '*') {
			stow = 0;
			ch = *fmt++;
		} else
			stow = 1;

		for(len = 0; isdigit(ch); ch = *fmt++)
			len = len * 10 + ch - '0';
		if(len == 0)
			len = INT_MAX;

		if((size = ch) == 'l' || size == 'h')
			ch = *fmt++;
		if (ch == '\0' ||
		    (ch == '[' && (fmt = setup(fmt, chartab)) == NULL))
			return(EOF); /* unexpected end of format */
		if(isupper(ch)) { /* no longer documented */
			size = 'l';
			ch = tolower(ch);
		}
		if(ch != 'c' && ch != '[') {
		    while(isspace(inchar = getc(iop)))
#if 0
			if ((inchar == '\n')) {
			    ungetc(inchar, iop);
			    return(nmatch);
			}
#else
		    ;
#endif
		    if(ungetc(inchar, iop) == EOF)
			break;
		}
		if((size = (ch == 'c' || ch == 's' || ch == '[') ?
		    mstring(stow, ch, len, chartab, iop,  &va_alist) :
		    mnumber(stow, ch, len, size, iop, &va_alist)) != 0)
			nmatch += stow;
		if(va_alist == NULL) /* end of input */
			break;
		if(size == 0)
			return(nmatch); /* failed to match input */
	}
	return(nmatch != 0 ? nmatch : EOF); /* end of input */
}

/***************************************************************
 * Functions to read the input stream in an attempt to match incoming
 * data to the current pattern from the main loop of _doscan().
 ***************************************************************/
static int
mnumber(stow, type, len, size, iop, listp)
int stow, type, len, size;
FILE *iop;
va_list *listp;
{
	char numbuf[64];
	char *np = numbuf;
	int c, base;
	int digitseen = 0, dotseen = 0, expseen = 0, floater = 0, negflg = 0;
	long lcval = 0;

	switch(type) {
	case 'e':
	case 'f':
	case 'g':
		floater++;
	case 'd':
	case 'u':
		base = 10;
		break;
	case 'o':
		base = 8;
		break;
	case 'x':
		base = 16;
		break;
	default:
		return(0); /* unrecognized conversion character */
	}
	switch(c = getc(iop)) {
	case '-':
		negflg++;
		if(type == 'u') 	/* DAG -- unsigned `-' check */
			break;
	case '+': /* fall-through */
		len--;
		c = getc(iop);
	}
	if(!negflg || type != 'u')	/* DAG -- unsigned `-' check */
	    /* DAG -- added safety check on `np' */
	    for( ; --len >= 0 && np < &numbuf[63]; *np++ = c, c = getc(iop)) {
		if(isdigit(c) || (base == 16 && isxdigit(c))) {
			int digit = c - (isdigit(c) ? '0' :
			    isupper(c) ? 'A' - 10 : 'a' - 10);
			if(digit >= base)
				break;
			if(stow && !floater)
				lcval = base * lcval + digit;
			digitseen++;
			continue;
		}
		if(!floater)
			break;
		if(c == '.' && !dotseen++)
			continue;
		if((c == 'e' || c == 'E') && digitseen && !expseen++) {
			*np++ = c;
			c = getc(iop);
			if(isdigit(c) || c == '+' || c == '-')
				continue;
		}
		break;
	    }
	if(stow && digitseen) {
		if(floater) {
			double dval;
	
			*np = '\0';
			dval = atof(numbuf);
			if(negflg)
				dval = -dval;
			if(size == 'l')
				*va_arg(*listp, double *) = dval;
			else
				*va_arg(*listp, float *) = (float)dval;
		} else {
			/* suppress possible overflow on 2's-comp negation */
			if(negflg && lcval != HIBITL)
				lcval = -lcval;
			if(size == 'l')
				*va_arg(*listp, long *) = lcval;
			else if(size == 'h')
				*va_arg(*listp, short *) = (short)lcval;
			else
				*va_arg(*listp, int *) = (int)lcval;
		}
	}
	if(ungetc(c, iop) == EOF)
		*listp = NULL; /* end of input */
	return(digitseen); /* successful match if non-zero */
}

static int
mstring(stow, type, len, chartab, iop, listp)
int stow, type, len;
char *chartab;
FILE *iop;
va_list *listp;
{
	int ch;
	char *ptr;
	char *start;

	start = ptr = stow ? va_arg(*listp, char *) : NULL;
	if(type == 'c' && len == INT_MAX)
		len = 1;
	while((ch = getc(iop)) != EOF &&
	    !((type == 's' && isspace(ch)) || (type == '[' && chartab[ch]))) {
		if(stow)
			*ptr = ch;
		ptr++;
		if(--len <= 0)
			break;
	}
	if(ch == EOF || (len > 0 && ungetc(ch, iop) == EOF))
		*listp = NULL; /* end of input */
	if(ptr == start)
		return(0); /* no match */
	if(stow && type != 'c')
		*ptr = '\0';
	return(1); /* successful match */
}

static const char *
setup(const char *fmt, char *chartab)
{
	int b, c, d, t = 0;

	if(*fmt == '^') {
		t++;
		fmt++;
	}
	(void)memset(chartab, !t, NCHARS);
	if((c = *fmt) == ']' || c == '-') { /* first char is special */
		chartab[c] = t;
		fmt++;
	}
	while((c = *fmt++) != ']') {
		if(c == '\0')
			return(NULL); /* unexpected end of format */
		if(c == '-' && (d = *fmt) != ']' && (b = fmt[-2]) < d) {
			(void)memset(&chartab[b], t, d - b + 1);
			fmt++;
		} else
			chartab[c] = t;
	}
	return(fmt);
}


#endif
