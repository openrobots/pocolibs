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

CPPFLAGS += -DROOTROBOTS=\"$(ROOTROBOTS)\" -DTARGET=\"$(TARGET)\" -I..

SRCS = \
	commonStructLib.c	\
	csLib.c			\
	gcomLib.c		\
	h2devLib.c		\
	h2evnLib.c		\
	h2initGlob.c		\
	h2mathLib.c		\
	h2rngLib.c		\
	h2semLib.c		\
	h2timeLib.c		\
	h2timerLib.c		\
	h2timerTest.c		\
	mboxEssay.c		\
	mboxLib.c		\
	smMemLib.c		\
	smObjLib.c		\
	vfscanf.c		\
	xesClient.c		\
	xesStdio.c		\
	h2endianness.c

OBJS = $(SRCS:%.c=$(TARGET)/%.o)

SHLIB_MAJOR = 2
SHLIB_MINOR = 1

BASENAME = libcomLib

LIB = $(TARGET)/$(BASENAME).$(SHLIBREV)

all: $(LIB)

$(LIB): $(OBJS)
	$(SHLIBLD) $(SHLIBLDFLAGS) -o $@ $(OBJS) \
	$(PORTLIB)  $(ERRORLIB) $(LIBM)

$(TARGET)/%.o: %.c
	$(CC) -c $(SHLIBCFLAGS) -o $@ $(CPPFLAGS) $(CFLAGS) $<

depend $(TARGET)/DEPENDANCES:: $(TARGET)

depend $(TARGET)/DEPENDANCES:: $(SRCS)
	$(MKDEP) -c$(CC) -o$(TARGET)/DEPENDANCES -d'$$(TARGET)' \
	-sH2UNIX=$(H2UNIX) \
	$(CPPFLAGS) $?

$(TARGET):
	-mkdir -p $@

clean: 
	rm -f $(OBJS) $(LIB)

install:
	$(MKINSTALLDIRS) $(LIBDIR)
	$(INSTALL) $(LIB) $(LIBDIR)
	(cd $(LIBDIR); rm -f $(BASENAME).$(SHLIBSUF) ; \
	 $(LN) $(BASENAME).$(SHLIBREV) $(BASENAME).$(SHLIBSUF))

include $(TARGET)/DEPENDANCES
