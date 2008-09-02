/* $LAAS$ */
/*
 * Copyright (c) 1989, 2003 CNRS/LAAS
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

#ifndef  H2_MATH_LIB_H
#define  H2_MATH_LIB_H


#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************/
/*   LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS        */
/*   PROJET HILARE II  -  COMPLEMENT AUX ROUTINES MATH. DE VXWORKS           */
/*   FICHIER D'EN-TETE: h2mathLib.h                                          */
/*****************************************************************************/

/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1; dec89; Bauzil, Camargo et Vialaret; 1ere version;
*/

/* DESCRIPTION:
   Fichier d'en-tete pour la bibliotheque de routines mathematiques employees
   comme complement aux routines disponibles sur VXWORKS.
*/

extern const double sinTab[];
extern const double cosTab[];


/*---------------------------     CONSTANTES    ----------------------------*/

/* Definition de pi */
#define  PI                  3.14159265358979323846

/* Conversion degres -> radians */
#define  C_DEG_RAD           (PI/180.)
#define  DEG_TO_RAD(x)       ((x) * C_DEG_RAD)

/* Conversion radians -> degres */
#define  C_RAD_DEG           (180./PI)
#define  RAD_TO_DEG(x)       ((x) * C_RAD_DEG)

/*----------------------------      MACROS    ------------------------------*/

/* Definition de l'elevation au carre */
#define  SQR(x)              ((double)(x) * (double)(x))

/* Signal d'une variable */
#define  SIGN(x)             ((x)<0. ? -1. : 1.)

/* Sin et cos de l'angle en degre avec:  -180 <= angle <= 180 */
#define SIN_DEG_FAST(x)      (sinTab[(int)(x + 180 + .5)])
#define COS_DEG_FAST(x)      (cosTab[(int)(x + 180 + .5)])

/* Sin et cos de l'angle en radian avec:  -PI <= angle <= PI */
#define SIN_FAST(x)      (sinTab[(int)(C_RAD_DEG * x + 180 +.5)])
#define COS_FAST(x)      (cosTab[(int)(C_RAD_DEG * x + 180 +.5)])


/*------------------------     FONCTIONS EXTERNES    ------------------------*/

#ifdef __STDC__
double angleLimit (double angle);
double angleDegLimit (double angleDeg);
double superTab (int n, double *pTab);
double inferTab (int n, double *pTab);
double atan2Par (double dty, double dtx);
double h2cos (double angle);
double h2sin (double angle);
double integCosFresnel (double theta);
double integSinFresnel (double theta);
#else
double angleLimit ();
double angleDegLimit ();
double superTab ();
double inferTab ();
double atan2Par ();
double h2cos ();
double h2sin ();
double integCosFresnel ();
double integSinFresnel ();
#endif

#ifdef __cplusplus
};
#endif

#endif
