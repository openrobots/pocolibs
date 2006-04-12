/*
 * Copyright (c) 2006 CNRS/LAAS
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
#ident "$Id$"

#include <stdio.h>
#include <unistd.h>

#include <portLib.h>
#include <h2initGlob.h>
#include <posterLib.h>

struct testPoster {
	int a;
	int b;
};

#define TEST_POSTER_NAME "deletePosterTest"
#define TEST_POSTER_SIZE sizeof(struct testPoster)

int 
main(int argc, char *argv[])
{
	POSTER_ID p1, p2, p3;

	if (h2initGlob(0) == ERROR) {
		h2perror("h2initGlob");
		exit(2);
	}
	if (posterCreate(TEST_POSTER_NAME, TEST_POSTER_SIZE, &p1) == ERROR){
		h2perror("posterCreate");
		exit(2);
	}
	if (posterFind(TEST_POSTER_NAME, &p2) == ERROR) {
		h2perror("posterFind");
		exit(2);
	}
	if (posterDelete(p1) == ERROR) {
		h2perror("posterDelete");
		exit(2);
	}
	if (posterFind(TEST_POSTER_NAME, &p3) == OK) {
		fprintf(stderr, "posterFind on a closed poster succeded!\n");
		exit(3);
	}
	exit(0);
}

