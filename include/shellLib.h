/* $LAAS$ */
/*
 * Copyright (c) 1999, 2003 CNRS/LAAS
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

#ifndef _SHELLLIB_H
#define _SHELLLIB_H

#include <stdio.h>

extern STATUS executeCmd ( char *cmdbuf, int clientSock, int clientError, 
			  int recur );

extern int get_args ( int line_length, char *line, char **argv );
extern void getstr ( int soc, char *buf, int cnt, char *errmesg );
extern void parse_args(int firstArg, int argc, char *argv[], int argType[]);
extern void shellMainLoop(FILE *in, FILE *out, FILE *err, char *prompt);
#endif
