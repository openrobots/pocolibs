#
# Copyright (c) 2004 CNRS/LAAS
# Copyright (c) 2004 EPFL
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

# use the posix version for userspace lib
VPATH=	$(srcdir)/os/posix:$(srcdir)

MODSRCS+= \
	osInit.c		\
	$(MODVPATH)/logLib.c	\
	$(MODVPATH)/semLib.c	\
	$(MODVPATH)/symLib.c	\
	$(MODVPATH)/sysLib.c	\
	$(MODVPATH)/taskLib.c	\
	$(MODVPATH)/tickLib.c	\
	$(MODVPATH)/wdLib.c	\
	$(MODVPATH)/module.c
