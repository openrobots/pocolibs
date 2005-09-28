/* $LAAS$ */
/*
 * Copyright (c) 1999, 2003 CNRS/LAAS
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

#ifndef _SYMLIB_H
#define _SYMLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* -- STRUCTURES ----------------------------------------------- */

typedef void *SYMTAB_ID;
extern SYMTAB_ID sysSymTbl;

/* Types de symboles */
typedef int SYM_TYPE;

/* -- CONSTANTS ------------------------------------------------ */

#define SYM_UNDF        0x0     /* undefined */
#define SYM_LOCAL       0x0     /* local */
#define SYM_GLOBAL      0x1     /* global (external) (ORed) */
#define SYM_ABS         0x2     /* absolute */
#define SYM_TEXT        0x4     /* text */
#define SYM_DATA        0x6     /* data */
#define SYM_BSS         0x8     /* bss */

/* -- ERRORS CODES ----------------------------------------------- */

#include "h2errorLib.h"
#define M_symLib        28
#define S_symLib_SYMBOL_NOT_FOUND       H2_ENCODE_ERR(M_symLib, 1)
#define S_symLib_NOT_IMPLEMENTED	H2_ENCODE_ERR(M_symLib, 2)

#define SYM_LIB_H2_ERR_MSGS { \
    {"SYMBOL_NOT_FOUND",  H2_DECODE_ERR(S_symLib_SYMBOL_NOT_FOUND)},  \
    {"NOT_IMPLEMENTED",	  H2_DECODE_ERR(S_symLib_NOT_IMPLEMENTED)},  \
}

/* -- PROTOTYPES ----------------------------------------------- */
int  symRecordH2ErrMsgs(void);
STATUS symLibInit(void);
STATUS symFindByName(SYMTAB_ID symTblId, char *name, char **pValue, 
		     SYM_TYPE *pType);
STATUS symFindByValue(SYMTAB_ID symTldId, char *value, char *name, 
		      int *pValue, SYM_TYPE *pType);

#ifdef __cplusplus
};
#endif

#endif
