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
#include "config.h"
__RCSID("$LAAS$");

/***
 *** Emulate the symbol table handling functions from VxWorks
 ***/

#include <stdio.h>
#ifndef __DARWIN__
#include <dlfcn.h>
#else
#include <mach-o/dyld.h>
#endif

#include "portLib.h"
#include "errnoLib.h"
#include "symLib.h"

SYMTAB_ID sysSymTbl = NULL;

/*----------------------------------------------------------------------*/

STATUS
symLibInit(void)
{
    /* Chargement de la table des symboles du process */
#ifndef __DARWIN__
    sysSymTbl = dlopen(NULL, RTLD_LAZY);
    if (sysSymTbl == NULL) {
	fprintf(stderr, "Erreur dlopen() %s\n", dlerror());
	return ERROR;
    }
#endif
    return OK;
}

/*----------------------------------------------------------------------*/

STATUS
symFindByName(SYMTAB_ID symTblId, char *name, char **pValue,
	      SYM_TYPE *pType)
{
#ifndef __DARWIN__
    void *addr;

    addr = dlsym(symTblId, name);

    if (addr == NULL) {
	errnoSet(S_symLib_SYMBOL_NOT_FOUND);
	return ERROR;
    }
    *pValue = (char *)addr;
    *pType = SYM_GLOBAL;
#else
    NSSymbol *nssym = NULL;

    if (NSIsSymbolNameDefined(name)) {
	    nssym = NSLookupAndBindSymbol(name);
    } 
    if (nssym == NULL) {
	    errnoSet(S_symLib_SYMBOL_NOT_FOUND);
	    return ERROR;
    } 
    *pValue = NSAddressOfSymbol(nssym);
    *pType = SYM_GLOBAL;
#endif
    return OK;
}

/*----------------------------------------------------------------------*/

STATUS
symFindByValue(SYMTAB_ID symTldId, char *value, char *name, 
	       int *pValue, SYM_TYPE *pType)
{
    return ERROR;
}
