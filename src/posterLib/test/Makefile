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
SUBDIRS = \
	basic \
#	endian

all: unix

install:
	@echo "nothing to install for now"

unix:
	@for i in $(SUBDIRS) ; do \
		echo "" ; \
		echo "-- Make in $$i for unix" ; \
		(cd $$i; $(MAKE) -f Makefile.unix) || exit 2; \
	done


all clean depend:
	@for i in $(SUBDIRS) ; do \
		echo "" ; \
		echo "-- Make $@ in $$i for unix" ; \
		(cd $$i; $(MAKE) $@ -f Makefile.unix) || exit 2; \
\
	done


vxworks:
	@for i in $(SUBDIRS) ; do \
		echo "" ; \
		echo "-- Make $@ in $$i for m68k-vxworks" ; \
		(cd $$i; $(MAKE) $@ -f Makefile.vxworks TARGET=m68k-vxworks) || exit 2; \
\
		echo "" ; \
		echo "-- Make $@ in $$i for ppc-vxworks" ; \
		(cd $$i; $(MAKE) $@ -f Makefile.vxworks TARGET=ppc-vxworks) || exit 2; \
	done
