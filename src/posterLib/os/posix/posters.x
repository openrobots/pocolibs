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
 *** Serveur de posters pour Unix
 *** base' sur RPC
 ***
 *** De'finition du protocole RPC
 ***/

enum POSTER_STATUS {
    POSTER_OK,
    POSTER_ERROR
    };

struct POSTER_FIND_RESULT {
    int status;
    int id;
    int length;
    int endianness;
};

struct POSTER_CREATE_PAR {
    string name<256>;
    int length;
    int endianness;
};

struct POSTER_CREATE_RESULT {
    int status;
    int id;
};

struct POSTER_WRITE_PAR {
    int id;
    int offset;
    int length;
    char data<>;
};

struct POSTER_READ_PAR {
    int id;
    int offset;
    int length;
};

struct POSTER_READ_RESULT {
    int status;
    char data<>;
};

struct POSTER_IOCTL_PAR {
    int id;
    int cmd;
};

struct POSTER_IOCTL_RESULT {
    int status;
    unsigned long ntick;
    unsigned short msec;
    unsigned short sec;
    unsigned short minute;
    unsigned short hour;
    unsigned short day;
    unsigned short date;
    unsigned short month;
    unsigned short year;
};
	

program POSTER_SERV {
    version POSTER_VERSION {
	POSTER_FIND_RESULT poster_find(string) = 1;
	POSTER_CREATE_RESULT poster_create(POSTER_CREATE_PAR) = 2;
	int poster_write(POSTER_WRITE_PAR) = 3;
	POSTER_READ_RESULT poster_read(POSTER_READ_PAR) = 4;
	int poster_delete(int) = 5;
	POSTER_IOCTL_RESULT poster_ioctl(POSTER_IOCTL_PAR) = 6;
     } = 1;
} = 600000001;

    
