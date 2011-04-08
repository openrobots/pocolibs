/*
 * Copyright (c) 1992-2005 CNRS/LAAS
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


/* ----------------------------------------------------------------------
 *
 * h2errorLib -   This library manages the error codes. It provides :
 *
 *                . Macros to encode/decode the error codes :
 *                      H2_ENCODE_ERR/H2_DECODE_ERR
 *                . Functions to record {error-code/error-string} couples :
 *                      h2recordErrMsgs();
 *                . Functions to get the string of a given error code :
 *                      h2perror(), h2getErrMsg()
 *
 * 1/ Errors encoding : 
 * --------------------
 * Every error code must be unique. For that we follow the vxworks like 
 * strategy : 
 * - a unique M_lib number (short) must be attributed to the library 
 *   or module that exports errors,
 * - each error value err (short) is then coded as following : 
 *               S_lib_error = (M_lib << 16 | err)
 *
 * We recommend  to use the H2_ENCODE_ERR macro to encode the error.
 *
 * Example :
 *
 *      #define M_posterLib  		    510
 *
 *      #define S_posterLib_POSTER_CLOSED   H2_ENCODE_ERR(M_posterLib, 0)
 *      #define S_posterLib_NOT_OWNER       H2_ENCODE_ERR(M_posterLib, 1)
 *
 * 2/ Error strings declaration :
 * ------------------------------
 * The error code is not very readable for a human, that's why it is nice 
 * to have a string associated to each error including the name of the  
 * library and the name of the error (eg: "S_posterLib_NOT_OWNER" instead
 * of 33423361 (or even (510,1)).
 *
 * The function h2recordErrMsgs() allows to record all the couples 
 * {error-code/error-string}. 
 *
 * First, each library must export a const array of H2_ERROR structures 
 * of {"ERR_MSG", S_lib_ERR} couples. 
 *
 * Example:
 *
 *      #define POSTER_LIB_H2_ERR_MSGS {				   \
 *        {"POSTER_CLOSED",   H2_DECODE_ERR(S_posterLib_POSTER_CLOSED)},   \
 *        {"NOT_OWNER",       H2_DECODE_ERR(S_posterLib_NOT_OWNER)},       \
 *      }
 *
 *
 * with the array initialisation in the library source code :
 *
 *   static const H2_ERROR posterLibH2errMsgs[] = POSTER_LIB_H2_ERR_MSGS;
 *
 *
 * Then the array must be recorded within h2errorLib by the user of the 
 * library using h2recordErrMsgs() :
 *
 *	h2recordErrMsgs("posterInit", "posterLib", M_posterLib, 
 *			sizeof(posterLibH2errMsgs)/sizeof(H2_ERROR), 
 *			posterLibH2errMsgs);
 *
 * This function can be hiden within the initialisation function of the 
 * library, if any. If not, we recommend to provide a dedicated function 
 * that will be call once by the user of the library.
 *
 * Example :
 *
 *      int posterInit()  / * or posterRecordH2ErrMsgs() * /
 *      {
 *         return h2recordErrMsgs("posterInit", "posterLib", M_posterLib, 
 *			   sizeof(posterLibH2errMsgs)/sizeof(H2_ERROR), 
 *			   posterLibH2errMsgs);
 *      }
 *
 * Remark : Several calls of h2recordErrMsgs() has no effect (not a problem).
 *
 *
 * 3/ Utilisation
 * --------------
 * Just call h2perror() or h2getErrMsg()
 *
 *
 * 4/ "Standard" errors
 * --------------------
 * They may be several error types that are common to several libraries 
 * or modules. For instance, all GenoM modules can raise errors like 
 * "ACTIVITY_FAILED" or "POSTER_CLOSED". Instead of defining one such
 * error for each module (eg, S_m1_ACTIVITY_FAILED, S_m2_ACTIVITY_FAILED)
 * one library can define "standard" errors that will be shared.
 *
 * Such errors must be encoded using the macro H2_ENCODE_STD_ERR(M_lib,err) 
 * with a M_lib library id < 2^7=128. The remaining declaration procedure 
 * is unchanged. Eg :
 *
 *    #define M_stdGenom 100
 *    #define S_stdGenom_ACTIVITY_FAILED      H2_ENCODE_STD_ERR(M_stdGenom, 3)
 *    [...]

 * Then the module that wants to used a standard error just has to declare
 * it as following : 
 *    #define S_demo_stdGenom_ACTIVITY_FAILED \
 *            H2_ENCODE_ERR(M_mod, H2_DECODE_ERR(S_stdGenom_ACTIVITY_FAILED))
 * 
 * IT DOES NOT HAVE TO RECORDED IT WITH h2recordErrMsgs(). Thus do not put
 * it in the const H2_ERROR array of the errrors of the module.
 *
 * How it works ? To be identified std err are encoded as following :
 *        M_lib << 16 | - M_stdLib << 8 | err
 * thus M_lib    is encoded on a 'short' 
 *      M_stdLib is encoded on 'half signed short'
 *      err      is encoded on 'half short'
 * ---------------------------------------------------------------------- */


#ifndef  H2_ERROR_LIB_H
#define  H2_ERROR_LIB_H

#ifdef __cplusplus
extern "C" {
#endif


/* -- ERRORS ENCODING -------------------------------------------------- */

/* M_id and err are encoded on 'signed short' (ie, < 2^15 = 32768) and 'short' 
 *               S_lib_error = (M_lib << 16 | err)
 */
#define H2_ENCODE_ERR(M_id,err)   (M_id << 16 | (err&0xffff))

/* M_id and err are encoded on 'half signed short' (ie, < 2^7 = 128) *
            S_lib_std_err = (M_lib << 16 | - M_stdLib << 8 | err) 
*/
#define H2_ENCODE_STD_ERR(M_id,err) ((M_id&0xff80) || (err&0xff00) ? 0 : \
				     ((M_id&0x7f)|0x80) << 8 | (err&0xff))

#define H2_TEST_STD_ERR(err) ((err) & 0x8000)
#define H2_SOURCE_STD_ERR(numErr) ((numErr&0x7f00)>>8)
#define H2_NUMBER_STD_ERR(numErr) (numErr&0x00ff)

/* -- ERRORS DECODING ------------------------------------------------ 
 * according to VxWorks policy
 */

#define H2_SOURCE_ERR(numErr)      (numErr>>16)
#define H2_NUMBER_ERR(numErr)      (numErr&0xffff)
#define H2_DECODE_ERR(numErr)      H2_NUMBER_ERR(numErr)

/* #define H2_SOURCE_ERR(numErr)      (numErr >= 0 ? (numErr)>>16 : numErr/100*100) */
/* #define H2_NUMBER_ERR(numErr)      (numErr >= 0 ? ((numErr)<<16)>>16 : - numErr + numErr/100*100) */


/* -- ERRORS CLASSIFICATIONS ---------------------------------------- 
 *
 *   NUMERO SOURCE:         SOURCE:
 *         n < 0              comLib UNIX  (old version)
 *         n = 0              system
 *     n/100 = 1              genom
 *     n/100 = 5              comLib
 *     n/100 = 6              hardLib
 *     n/100 > 6              modules
 *
 */
#define H2_SYS_ERR_FLAG(numErr)                 /* Erreur systeme */ \
 (numErr > 0 && ((numErr)>>16) == 0 ? TRUE : FALSE)  
#define H2_VX_ERR_FLAG(numErr)                  /* Erreur vxworks */ \
 (numErr > 0 && ((numErr)>>16) > 0 && ((numErr)>>16)/100 < 1 ? TRUE : FALSE)
#define H2_COM_VX_ERR_FLAG(numErr)              /* Erreur comLib sur VxW */\
 (numErr >= 0 && ((numErr)>>16)/100 == 5 ? TRUE : FALSE)
#define H2_COM_UNIX_ERR_FLAG(numErr)            /* Erreur comLib sur UNIX */\
 (numErr > -700 && numErr <= -100  ? TRUE : FALSE)
#define H2_HARD_ERR_FLAG(numErr)                /* Erreur hardLib */\
 (numErr >= 0 && ((numErr)>>16)/100 == 6 ? TRUE : FALSE)
#define H2_MODULE_ERR_FLAG(numErr)              /* Erreur modules */\
 (numErr >= 0 && ((numErr)>>16)/100 > 6 ? TRUE : FALSE)

/* -- ERRORS STRUCTURES --------------------------------------------- */

typedef struct H2_ERROR {
  const char *name;           /* error name (without source name) */
  const short num;            /* error number (without source num) */
} H2_ERROR;

#include <errnoLib.h>


/*---------------- PROTOTYPES of EXPORTED FUNCTIONS -----------------------*/

int h2recordErrMsgs(const char *bywho, const char *moduleName, short moduleId, 
		    int nbErrors, const H2_ERROR errMsgs[]);
void h2printErrno(int numErr);
void h2perror(const char *string);
char * h2getMsgErrno(int fullError); /* OBSOLET */
char * h2getErrMsg(int fullError, char *string, int maxLength);
short h2decodeError(int error, short *num, 
		    short *srcStd, short *numStd);
void h2listModules(void);
void h2listErrors(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*--------------------- end file loading ----------------------*/
#endif /* H2_ERROR_LIB_H */
