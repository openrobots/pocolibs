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
#include "pocolibs-config.h"
__RCSID("$LAAS$");

/***
 *** Emulate the symbol table handling functions from VxWorks
 ***/

#include <stdio.h>
#include <dlfcn.h>

#include "portLib.h"
#include "errnoLib.h"
#include "symLib.h"
static const H2_ERROR const symLibH2errMsgs[]   = SYM_LIB_H2_ERR_MSGS;

SYMTAB_ID sysSymTbl = NULL;


/*----------------------------------------------------------------------*/

int
symRecordH2ErrMsgs()
{
    return h2recordErrMsgs("symRecordH2ErrMsg", "symLib", M_symLib, 
			   sizeof(symLibH2errMsgs)/sizeof(H2_ERROR), 
			   symLibH2errMsgs);
}
/*----------------------------------------------------------------------*/

STATUS
symLibInit(void)
{
    /* Load the process's own symbol table */
    sysSymTbl = dlopen(NULL, RTLD_LAZY);
    if (sysSymTbl == NULL) {
	fprintf(stderr, "dlopen() error %s\n", dlerror());
	return ERROR;
    }
    return OK;
}

/*----------------------------------------------------------------------*/

STATUS
symFindByName(SYMTAB_ID symTblId, char *name, char **pValue,
	      SYM_TYPE *pType)
{
    void *addr;

    addr = dlsym(symTblId, name);

    if (addr == NULL) {
	errnoSet(S_symLib_SYMBOL_NOT_FOUND);
	return ERROR;
    }
    *pValue = (char *)addr;
    *pType = SYM_GLOBAL;
    return OK;
}

/*----------------------------------------------------------------------*/

STATUS
symFindByValue(SYMTAB_ID symTldId, char *value, char *name, 
	       int *pValue, SYM_TYPE *pType)
{
	/* Not implemented - and not trivial to implement without 
	   fetching GNU binutils */
	errnoSet(S_symLib_NOT_IMPLEMENTED);
	return ERROR;
}
