#
# Copyright (c) 2004 
#      Autonomous Systems Lab, Swiss Federal Institute of Technology.
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

VPATH=		$(srcdir):$(srcdir)/os/posix

SRCS= \
	posterLib.c		\
	localPosterLib.c	\
	remotePosterLib.c

SRCS_RPC= \
	posters_clnt.c \
	posters_xdr.c

HDR_RPC=	posters.h

# because it's not yet possible to make a remote real-time version
MOD_CPPFLAGS+= -DPOSTERLIB_ONLY_LOCAL

MODSRCS+= \
	posterLib.c 			\
	localPosterLib.c		\
	$(MODVPATH)/module.c