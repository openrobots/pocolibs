#
# Copyright (c) 1998, 2003 CNRS/LAAS
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
# $LAAS$
#
include ../Init.make

CPPFLAGS += -I..

# CPPFLAGS += -DRSHD_DEBUG

PROG = $(TARGET)/shell
SRCS = \
	config.c	\
	main.c

OBJS = $(SRCS:%.c=$(TARGET)/%.o)

all: $(PROG)
pure: $(PROG).pure

$(PROG): $(OBJS) $(LIBH2) $(LIBOS)
	$(CC) -o $(PROG) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) 

$(PROG).pure: $(OBJS) $(LIBH2) $(LIBOS)
	purify $(CC) -o $(PROG).pure $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

$(TARGET)/%.o: %.c
	$(CC) -c -o$@ $(CPPFLAGS) $(CFLAGS) $<

depend $(TARGET)/DEPENDANCES: $(SRCS)
	-mkdir -p $(TARGET)
	$(MKDEP) -c$(CC) -o$(TARGET)/DEPENDANCES -d$(TARGET) \
	-sH2UNIX=$(H2UNIX) $(CPPFLAGS) $?

clean:
	rm -f $(OBJS) $(PROG)

install:
	$(INSTALL) $(PROG) $(BINDIR)

include $(TARGET)/DEPENDANCES
