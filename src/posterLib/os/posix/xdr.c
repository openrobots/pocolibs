/*
 * Copyright (c) 2004 CNRS/LAAS
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

/* $LAAS$ */
/*	$NetBSD: xdr.c,v 1.27 2003/07/26 19:24:50 salo Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <inttypes.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

int
xdr_uint64_t(XDR *xdrs, uint64_t *ullp)
{
	unsigned long ul[2];

        switch (xdrs->x_op) {
        case XDR_ENCODE:
                ul[0] = (unsigned long)(*ullp >> 32) & 0xffffffff;
                ul[1] = (unsigned long)(*ullp) & 0xffffffff;
                if (XDR_PUTLONG(xdrs, (long *)&ul[0]) == FALSE)
                        return (FALSE);
                return (XDR_PUTLONG(xdrs, (long *)&ul[1]));
        case XDR_DECODE:
                if (XDR_GETLONG(xdrs, (long *)&ul[0]) == FALSE)
                        return (FALSE);
                if (XDR_GETLONG(xdrs, (long *)&ul[1]) == FALSE)
                        return (FALSE);
                *ullp = (uint64_t)
                    (((uint64_t)ul[0] << 32) | ((uint64_t)ul[1]));
                return (TRUE);
        case XDR_FREE:
                return (TRUE);
        }
        /* NOTREACHED */
        return (FALSE);
}	
