/*
 * Copyright (c) 1990, 2003 CNRS/LAAS
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
 *** Fonction d'initialisation globale pour un process qui veut utiliser
 *** comLib
 ***
 *** ticksPerSec -> frequence de l'horloge systeme
 ***                0 -> pas d'horloge systeme
 ***
 ***/

#include "config.h"
__RCSID("$LAAS$");

#include "portLib.h"
#include "h2devLib.h"
#include "h2timerLib.h"
#include "smMemLib.h"
#include "xes.h"

#include <stdio.h>

/**
 ** Initialisation de comLib dans un processus Unix 
 **/

STATUS
h2initGlob(int ticksPerSec)
{
    /* Initialise l'os */
    if (osInit(ticksPerSec) == ERROR) {
	return ERROR;
    }
    /* Initialise les E/S */
    if (xesStdioInit() == ERROR) {
        return ERROR;
    }
    /* Attache aux devices h2 */
    if (h2devAttach() == ERROR) {
	printf("Error: could not find h2 devices\n"
		"Did you execute `h2 init' ?\n");
	return ERROR;
    }
    /* Attache la memoire partagee */
    if (smMemAttach() == ERROR) {
	printf("Error: could not attach shared memory\n");
	return ERROR;
    }
		
    /* Initialise les timers h2 */
    if (ticksPerSec != 0) {
	if (h2timerInit() == ERROR) {
	    return ERROR;
	}
    }
    printf("Hilare2 execution environment version 1.1\n"
	   "Copyright (c) 1999-2003 LAAS/CNRS\n");

    return OK;
}
