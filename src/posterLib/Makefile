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
TOP = ..
include $(TOP)/Init.make

CPPFLAGS += -I$(TARGET) -I..


SRCS = \
	posterLib.c \
	localPosterLib.c \
	remotePosterLib.c

SRCS_RPC = \
	$(TARGET)/posters_clnt.c \
	$(TARGET)/posters_xdr.c

OBJS = $(SRCS:%.c=$(TARGET)/%.o) $(SRCS_RPC:$(TARGET)/%.c=$(TARGET)/%.o)

SHLIB_MAJOR = 2
SHLIB_MINOR = 1

BASENAME = libposterLib

LIB = $(TARGET)/$(BASENAME).$(SHLIBREV)

all: $(LIB)

$(LIB): $(OBJS)
	$(SHLIBLD) $(SHLIBLDFLAGS) -o $@ $(OBJS) \
	$(COMLIB) $(PORTLIB) $(ERRORLIB)

#----------------------------------------------------------------------
# RPC specific rules

$(TARGET)/posters_clnt.c: posters.x
	rm -f $@
	$(RPCGEN) -l  -o $@ posters.x

$(TARGET)/posters_xdr.c: posters.x
	rm -f $@
	$(RPCGEN) -c -o $@ posters.x

$(TARGET)/posters.h: posters.x
	rm -f $@
	$(RPCGEN) -h -o $@ posters.x

#----------------------------------------------------------------------
# Regles generiques

$(TARGET)/%.o: %.c
	$(CC) -c $(SHLIBCFLAGS) -o $@ $(CPPFLAGS) $(CFLAGS) $<

$(TARGET)/%.o: $(TARGET)/%.c
	$(CC) -c -o $@ $(SHLIBCFLAGS) $(CPPFLAGS) $(CFLAGS) $<

depend $(TARGET)/DEPENDANCES:: $(TARGET)

depend $(TARGET)/DEPENDANCES:: $(TARGET)/posters.h $(SRCS)  $(LIB_SRCS_RPC)
	$(MKDEP) -c$(CC) -o$(TARGET)/DEPENDANCES -d'$$(TARGET)' \
	-sH2UNIX=$(H2UNIX) \
	$(CPPFLAGS) $?

$(TARGET):
	-mkdir -p $@

clean: 
	rm -f $(OBJS) $(LIB) $(SRCS_RPC) $(TARGET)/posters.h

install:
	$(MKINSTALLDIRS) $(LIBDIR)
	$(INSTALL) $(LIB) $(LIBDIR)
	(cd $(LIBDIR); rm -f $(BASENAME).$(SHLIBSUF); \
         $(LN) $(BASENAME).$(SHLIBREV) $(BASENAME).$(SHLIBSUF))

include $(TARGET)/DEPENDANCES

.SUFFIXES = .x 
