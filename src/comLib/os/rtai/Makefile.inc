#
# Copyright (c) 2004 
#      Autonomous Systems Lab, Swiss Federal Institute of Technology.
# Copyright (c) 2004 CNRS/LAAS
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

# --- RTAI-specific configuration ---------------------------------------

VPATH=		$(srcdir)/os/rtai:$(srcdir):$(top_srcdir)/compat
VPATH+=		:$(srcdir)/os/posix

CPPFLAGS+=	$(RTAI_CFLAGS)
MOD_CPPFLAGS+=	-I$(top_srcdir)/compat

# userspace library is for the most part a wrapper around ioctls
SRCS+= \
        commonStructLib.c		\
        csLib.c				\
        gcomLib.c			\
        h2endianness.c			\
        h2evnLib.c			\
        h2initGlob.c			\
        h2mathLib.c			\
        h2rngLib.c			\
	h2timeLib.c			\
        h2timerLib.c			\
	h2timerTest.c			\
	mboxLib.c			\
	mboxEssay.c			\
	ioctlwrappers.c			\
        smMemLib.c			\
	smMemOsLib.c			\
        smObjLib.c			\
	vfscanf.c			\
	xesClient.c			\
	xesStdio.c

# kernel module contains the regular comLib functions, plus device interface
MODSRCS+= \
        commonStructLib.c		\
        csLib.c				\
        gcomLib.c			\
        h2devLib.c			\
        $(MODVPATH)/h2devOsLib.c	\
        h2endianness.c			\
        h2evnLib.c			\
        h2mathLib.c			\
        h2rngLib.c			\
        $(MODVPATH)/h2semLib.c		\
        h2timeLib.c			\
        h2timerLib.c			\
        mboxLib.c			\
        smMemLib.c			\
        $(MODVPATH)/smMemOsLib.c	\
        smObjLib.c			\
	$(MODVPATH)/h2device.c		\
	$(MODVPATH)/module.c		\
	fnmatch.c
