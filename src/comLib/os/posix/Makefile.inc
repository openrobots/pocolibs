# -*- Makefile -*-
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

# --- posix-specific configuration --------------------------------------

SRCS+= \
	commonStructLib.c	\
	csLib.c			\
	gcomLib.c		\
	h2devLib.c		\
	h2devOsLib.c		\
	h2endianness.c		\
	h2evnLib.c		\
	h2initGlob.c		\
	h2logLib.c 		\
	h2mathLib.c		\
	h2rngLib.c		\
	h2semLib.c		\
	h2timeLib.c		\
	h2timerLib.c		\
	h2timerTest.c           \
	mboxLib.c		\
	mboxEssay.c             \
	smMemLib.c		\
	smMemOsLib.c		\
	smObjLib.c
