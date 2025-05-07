/*
 * Copyright (c) 1997, 2003,2025 CNRS/LAAS
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

/*----------------------------------------------------------------------------
Modifications:
    06 Feb, 1992    jwm     Customized original UCBerkely code for VxWorks
                                  including elements from VxWorks User's Group
                                  archives version of vxrsh
    03 Mar, 1992    jwm     Wrote VxWorks shell emulation code
    07 Apr, 1992    jwm     Modified calls to to open() & ld() to handle
                                  VxWorks version dependencies using the
                                  compile variable "VXWORKS_MAJOR_VERSION"
    11 Jan, 1995    mh      Removed VxWorks 4 support.
                            Added script execution.
    15 Apr, 1996    mh      Added variable setting.			   

Written by:         John Monroe         SPARTA, Inc.        (703) 448-0210
Modified by:        Matthieu Herrb      LAAS/CNRS           matthieu@laas.fr
-----------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "portLib.h"
#include "symLib.h"
#include "errnoLib.h"
#include "taskLib.h"
#include "shellLib.h"

typedef uintptr_t (*shellFunc)(char *arg1, char *arg2,
    char *arg3, char *arg4, char *arg5, char *arg6,
    char *arg7, char *arg8, char *arg9, char *arg10);
typedef void *(*spFunc)(void *arg);

static STATUS findSymbol(char *name, shellFunc *value);
static void printValue(uintptr_t value);
static void printSymbol(char *name, void *value);

#define UNKNOWN                 0       /* arg type unknown */
#define	FUNCTION		1	/* function in symbol table */
#define	HEX_VALUE		2	/* '0x' or '0X' string of digits */
#define	DEC_VALUE		3	/* string of decimal digits */
#define VARIABLE                4	/* variable in symbol table */


/**
 ** This Local Shell emulates the VxWorks shell and can run as a 2nd shell.
 ** Its purpose is to execute the 'rsh' command without going through
 ** the normal shell (which may be logged into at the momemt).
 **/

STATUS 
executeCmd(char *cmdbuf,		/* client command line */
    int clientSock,			/* client stdIn & stdOut sockets */
    int clientError, 
    int recur)				/* number of recursive calls */
{
	char *argv[16];			/* function + up to 15 args */
	int argType[16];		/* types of arguments 
					 * = FUNCTION, HEX_VALUE, or
					 * DEC_VALUE */
	int argc;			/* number of arguments passed by
					   * 'rsh' */
	shellFunc symStartAddr = NULL;	/* start addr Major fctn (sp, ld,
					 * etc.) */
	spFunc fctnStartAddr = NULL;	/* addr Minor fctn (task being
					 * spawned) */
	STATUS statusReturn = OK;	/* OK/ERROR status returned by
					 * function */
	int i, val = 0;			/* miscellaneous temporary variables */
	
	fflush(stdout);
	fflush(stderr);
#ifdef RSHD_DEBUG
	printf("Command Line Sent by rsh: %s\n", cmdbuf);
#endif
	/* Parse the command line sent by host 'rsh' task */
	for (i = 0; i < 16; i++) {
		argv[i] = NULL;
		argType[i] = UNKNOWN;
	}
	argc = get_args(strlen(cmdbuf), cmdbuf, argv);
	
	if (argc > 0) {
#ifdef RSHD_DEBUG
		printf("(rshd): %d args sent by 'rsh'\n", argc);
		for (i = 0; i < argc; i++)
			printf("  %d - %s\n", i, argv[i]);
#endif
		
		if (strncmp(argv[0], "#", 1) == 0 || 
		    strncmp(argv[0], "/*", 2) == 0) {
			/* Comments */
			return(OK);
		} else if (strcmp(argv[0], "<") == 0) {
			/* Script execution */
			FILE *script;
			char buf[128];
			
			if (argc != 2) {
				printf("<: need an argument\n");
				fflush(stdout);
				statusReturn = ERROR;
				goto returnLabel;
			}
			script = fopen(argv[1], "r");
			if (script == NULL) {
				printf("can't open '%s'\n", argv[1]);
				printf("  errno = 0x%x ", errnoGet());
				fflush(stdout);
				statusReturn = ERROR;
				goto returnLabel;
			}
			statusReturn = OK;
			while (!feof(script)) {
				if (fgets(buf, 128, script) == NULL) {
					break;
				}
				if ((i = strlen(buf)) > 0) {
					buf[i - 1] = '\0';
				}
				/* XXX should also replace \ sequences 
				   in strings */
				printf("%s\n", buf);
				fflush(stdout);
				if (executeCmd(buf, clientSock, clientError,
					recur+1) == ERROR) {
					statusReturn = ERROR;
					break;
				}
			} /* while */
			goto returnLabel;
		}
		
		
		/* Handle "special" cases first, then the "general" cases */
		
		if (strcmp(argv[0], "p") == 0) {
			/* print variable */
			parse_args(1, argc, argv, argType);
			switch (argType[1]) {
			case 0:
				if (findSymbol(argv[1], &symStartAddr) != OK) {
					fprintf(stderr, 
					    "symbol not found: %s\n", argv[1]);
				} else {
					printSymbol(argv[1], symStartAddr);
				}
				break;
			case HEX_VALUE:
			case DEC_VALUE:
				printValue((uintptr_t)argv[1]);
				break;
			default:
				puts(argv[1]);
			} /* switch */
		} else if (strcmp(argv[0], "sp") == 0) {
			/* task Spawn */
			if (findSymbol(argv[1], (shellFunc *)&fctnStartAddr) != OK) {
				fprintf(stderr, "symbol not found: %s\n", 
				    argv[1]);
			} else {
				parse_args(2, argc, argv, argType);
				if (argType[2] == 0 && argv[2] != NULL) {
					if (findSymbol(argv[2], 
						&symStartAddr) != OK) {
						fprintf(stderr, 
						    "symbol not found: %s\n", 
						    argv[2]);
						symStartAddr = NULL;
					} 
					argv[2] = (char *)symStartAddr;
				} 
				if (taskSpawn2(NULL, 50, 0, 10000, 
					fctnStartAddr,
					(void *)argv[2]) == ERROR) {
					fprintf(stderr, 
					    "Error creating process\n");
					statusReturn = ERROR;
				}
			}
			
		} else {
			/* All Other VxWorks Functions */
			if (findSymbol(argv[0], &symStartAddr) != OK) {
				fprintf(stderr, "not found: %s\n", argv[0]);
				statusReturn = ERROR;
			} else {
#ifdef RSHD_DEBUG
				int j;
				
				printf("Normal Fctn: [%s]\n", argv[0]);
				printf("  No. Args = %d\n", argc);
				
				if (argc > 1) {
					for (j = 0; j < argc; j++) {
						if ((strlen(argv[j]) > 2) &&
						    (strlen(argv[j]) < 16)) {
							printf("    arg(%d) = [ %s ],   len= %d bytes\n",
							    j, argv[j], 
							    strlen(argv[j]));
						} else {
							printf("    arg(%d) = 0x%x = %d  (value)\n",
							    j, (int) argv[j], (int) argv[j]);
						}
					}
				}
#endif
				
				parse_args(0, argc, argv, argType);
				val = symStartAddr(argv[1], 
						 argv[2], argv[3], argv[4],
						 argv[5], argv[6], argv[7], 
						 argv[8], argv[9], argv[10]);
				printValue(val);
				fflush(stdout);
			}
			
		} /* All Other VxWorks Functions */
		
		
	} /* at least 1 argument */
 returnLabel:
	return (statusReturn);
	
} /* executeCmd() */

/**
 ** See if 1st argument is an entry in the symbol table	
 ** Accept either 'arg' or '_arg' as 1st argument. 
 **/

static STATUS
findSymbol(char *name, shellFunc *value)
{
	char *pAdrs;
	SYM_TYPE  symType;
	char underscoreBuf[64];		  /* to create '_sym' form of 
					   * command */
	
	if (symFindByName(sysSymTbl, name, &pAdrs, &symType) != OK) {
		snprintf(underscoreBuf, sizeof(underscoreBuf), 
		    "_%s", name); /* add '_' to arg */
		if (symFindByName(sysSymTbl, underscoreBuf, &pAdrs, 
			&symType) != OK) {
			return(ERROR);
		}
	}
	if (value != NULL) {
		*value = (shellFunc)pAdrs;
	}
	return(OK);
	
} /* findSymbol */

/**
 ** Display a value like the VxWorks shell does
 **/
static void
printValue(uintptr_t value)
{
	
	printf("value = %lu = 0x%lx", value, value);
	if (value > ' ' && value < '~' ) {
		printf(" = '%c'", (int)value);
	}
	putchar('\n');
	fflush(stdout);
}

/**
 ** Display a symbol and its value like the VxWorks shell does
 **/
static void
printSymbol(char *name, void *value)
{
	printf("%s = %p: ", name, value);
	printValue(*(uintptr_t *)value);
}


/**
 ** Function to parse a NULL-terminated command line
 ** and return the number arguments found.
 **/
int
get_args(int line_length, char *line, char *argv[])
{
	char quote1 = 0x27, quote2 = 0x22; /* single & double quotes */
	char *c;
	int i, j;
	
	if (line_length == 0)
		return (0);		/* NULL string */
	c = line;
	i = 0;
	j = 0;
	while (i < line_length) {
		
		argv[j] = c;		/* set next arg pointer */
		while ((*c != '\0') && ((*c == ' ') || 
			   (*c == '\t') || (*c == ','))) {
			i++;
			c++;		/* skip past blanks, tabs, & commas */
		}
		argv[j] = c;		/* set arg ptr at next Non-Delimiter
					 * character */
		
		if ((*c == '\0') || (i >= line_length))
			return (j);
		
		if ((*c == quote1) || (*c == quote2)) {	
			/* check for quoted strings */
			/* (assumes matched sets)  */
#ifdef RSHD_DEBUG
			printf("Starting Quote Analysis:"
			    " j= %d, i= %d, c= %c, string= %s\n", j, i, *c, c);
#endif
			i++;
			c++;		/* move past leading quote mark */
			argv[j] = c;	/* & start string there */
			while ((*c != '\0') && (*c != quote1) && 
			    (*c != quote2)) {
				/** printf("skip: %c\n", *c); **/
				i++;
				c++;
			}
			if ((*c == '\0') || (i >= line_length))
				return (j);
#ifdef RSHD_DEBUG
			printf("Ending Quote Analysis: j= %d, i= %d, c= %c, string= %s\n",
			    j, i, *c, c);
#endif
			*c = '\0';    /* replace ending quote with terminator */
			i++;
			c++;	  /* move past quoted string & bump arg count */
			j++;
			argv[j] = c;
			while ((*c != '\0') && ((*c == ' ') 
				   || (*c == '\t') || (*c == ','))) {
				*c = '\0'; /* replace with terminator */
				i++;
				c++;
			}
			
			if ((*c == '\0') || (i >= line_length))
				return (j);
			argv[j] = c;	/* set at start of Non-Delimiter */
			
		} else {
			
			while ((*c != '\0') && (*c != ' ') 
			    && (*c != '\t') && (*c != ',')) {
				i++;
				c++;	/* move to end of arg string */
			}
			*c++ = '\0';	/* terminate arg string */
			i++;
			j++;		/* then, bump arg count */
			
		} /* quoted or Non-quoted arg ?? */
		
	} /* while processing entire command line */
	
	return (j);			/* return count of arguments found */
}

/**
 ** Parse_args
 **
 **
 ** Parse each argument as follows:
 **
 ** Convert args starting with '0x' or '0X' to hex values.
 ** Convert args made up of all digits to decimal values.
 ** Duplicate all other args;
 **/
void
parse_args(int firstArg, int argc, char *argv[], int argType[])
{
	int i, j;
	char *argPtr;
	int argLen;
	long intValue;
	BOOL isDecimal;
	unsigned long hexValue;
	
	for (i = firstArg; i < argc; i++) {
		argPtr = argv[i];
		argLen = strlen(argPtr);
		
		/* Convert Hex String to an Integer Values.  */
		/* Must start with '0x' or '0X' to be valid. */
		if (argLen > 2) {    
			
			if ((argPtr[0] == '0') &&
			    ((argPtr[1] == 'x') || (argPtr[1] == 'X'))
			) {
				argType[i] = HEX_VALUE;
				hexValue = strtoul(&argPtr[2], NULL, 16);
				/* replace w/ value */
				argv[i] = (char *) hexValue; 
				continue;	/* & exit 'do while' loop */
			}
		}
		/* If not a hex value, check for a decimal value */
		/* Convert Decimal Strings to equivalent Integer Values */
		/* Must be a digit in the range (0 - 9) */
		
		isDecimal = TRUE;
		for (j = 0; j < argLen; j++) {
			if ((argPtr[j] < '0') || (argPtr[j] > '9')) {
				isDecimal = FALSE;
				break;
			}
		}
		if (isDecimal) {
			intValue = atoi(argPtr);
			argv[i] = (char *) intValue; /* replace with value */
			argType[i] = DEC_VALUE;
			continue;	/* & exit 'do while' loop */
		}
		
		/* Duplicate other args */
		argPtr = malloc(argLen + 1);
		if (argPtr != NULL) {
			strcpy(argPtr, argv[i]);
			argv[i] = argPtr;
		}
		
	} /* for */
}

/*----------------------------------------------------------------------*/

/**
 ** A simple shell main loop
 **/
void
shellMainLoop(FILE *in, FILE *out, FILE *err, char *prompt)
{
	static char buf[1024];

	while (1) {
		fprintf(out, "%s", prompt);
		while (fgets(buf, sizeof(buf), in) == NULL && errno == EINTR);
		
		if (feof(in)) {
			fputc('\n', out);
			break;
		}
		buf[strlen(buf)-1] = '\0';
		executeCmd(buf, fileno(out), fileno(err), 0);
	} /* while */
}
