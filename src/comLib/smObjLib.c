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

/***
 *** Fonctions d'emulation de smObjLib de VxMP
 ***
 *** Remarque: applicables seulement pour la memoire partagee
 *** 
 *** L'identificateur global est l'offset par rapport a 
 *** l'adresse de base du shared memory segment 
 ***/

#include "pocolibs-config.h"
__RCSID("$LAAS$");

#include "portLib.h"

#include <stdio.h>

#include "smObjLib.h"
#include "smMemLib.h"

/*----------------------------------------------------------------------*/

/*
 * Passage d'un identificateur global (offset) a une adresse locale 
 */
void *
smObjGlobalToLocal(void *globalAdrs)
{
    if (globalAdrs == NULL) {
	/* NULL est le meme en local et en global */
	return NULL;
    }
    return (void *)((char *)globalAdrs + smMemBase());
}

/*----------------------------------------------------------------------*/

/*
 * Passage d'une adresse locale a l'adresse globale (offset)
 */
void *
smObjLocalToGlobal(void *localAdrs)
{
    if (localAdrs == NULL) {
	/* NULL est le meme en local et en global */
	return NULL;
    }
    return (void *)((char *)localAdrs - smMemBase());
}

