#
# Copyright (c) 2003 CNRS/LAAS
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
#
# $LAAS$
#

TOP = ..
include $(TOP)/Init.make

CPPFLAGS += -I..

SRCS = \
	errnoLib.c 		\
	osInit.c		\
	semLib.c		\
	shellLib.c		\
	snprintf.c		\
	symLib.c		\
	sysLib.c		\
	taskLib.c		\
	tickLib.c		\
	wdLib.c

OBJS = $(SRCS:%.c=$(TARGET)/%.o)

SHLIB_MAJOR=2
SHLIB_MINOR=0

BASENAME = libportLib

LIB = $(TARGET)/$(BASENAME).$(SHLIBREV)

all: $(LIB)

$(LIB): $(OBJS)
	$(SHLIBLD) $(SHLIBLDFLAGS) -o $@ $(OBJS)

$(TARGET)/%.o: %.c
	$(CC) -c $(SHLIBCFLAGS) -o $@ $(CFLAGS) $(CPPFLAGS) $<

depend $(TARGET)/DEPENDANCES:: $(TARGET)

depend $(TARGET)/DEPENDANCES:: $(SRCS)
	$(MKDEP) -c$(CC) -o$(TARGET)/DEPENDANCES -d $(TARGET) \
	-sH2UNIX=$(H2UNIX) \
	$(CPPFLAGS) $?

$(TARGET):
	-mkdir -p $(TARGET)

clean: 
	rm -f $(OBJS) $(LIB)

install:
	$(MKINSTALLDIRS) $(LIBDIR)
	$(INSTALL) $(LIB) $(LIBDIR)
	(cd $(LIBDIR); rm -f $(BASENAME).$(SHLIBSUF); \
	 $(LN) $(BASENAME).$(SHLIBREV) $(BASENAME).$(SHLIBSUF))

include $(TARGET)/DEPENDANCES
