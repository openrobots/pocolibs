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
/*****************************************************************************/
/*   LABORATOIRE D'AUTOMATIQUE ET D'ANALYSE DE SYSTEMES - LAAS / CNRS        */
/*   PROJET HILARE II  -  COMPLEMENT AUX ROUTINES MATH. DE VXWORKS           */
/*****************************************************************************/

/* VERSION ACTUELLE / HISTORIQUE DES MODIFICATIONS :
   version 1.1; dec89; Bauzil, Camargo et Vialaret; 1ere version;
   Modif le 5 Janv 93 par sf: cos et sin de Fresnel signe's.
*/

/* DESCRIPTION:
   Ensemble de routines mathematiques, fournies en complement aux routines
   de VXWORKS.
*/
  
#include "config.h"
__RCSID("$LAAS$");

#ifdef VXWORKS
#include <vxWorks.h>
#else
#include "portLib.h"
#endif
#include <sys/types.h>

#include "h2mathTab.c"
#include "h2mathLib.h"
#include <stdio.h>

/*----------------------- VARIABLES LOCALES --------------------------------*/

#ifdef VXWORKS
static long randx = 1;          /* Utilisee par generateur pseudo-aleatoire */
#endif

static double tabCosFresnel [] = {
  0.00000,     0.20000E-1,  0.40000E-1,  0.60000E-1,  0.79999E-1,
  0.99998E-1,  0.11999,     0.13999,     0.15997,     0.17995,
  0.19992,     0.21987,     0.23980,     0.25971,     0.27958,
  0.29940,     0.31917,     0.33888,     0.35851,     0.37805,
  0.39748,     0.41679,     0.43595,     0.45494,     0.47375, 
  0.49234,     0.51070,     0.52878,     0.54656,     0.56401,
  0.58110,     0.59777,     0.61401,     0.62976,     0.64499,
  0.65965,     0.67370,     0.68709,     0.69978,     0.71171,
  0.72284,     0.73313,     0.74252,     0.75096,     0.75841,
  0.76482,     0.77016,     0.77437,     0.77742,     0.77927,
  0.77989,     0.77926,     0.77735,     0.77414,     0.76963,
  0.76381,     0.75668,     0.74825,     0.73855,     0.72760,
  0.71544,     0.70212,     0.68769,     0.67224,     0.65583,
  0.63855,     0.62051,     0.60182,     0.58260,     0.56298,
  0.54310,     0.52311
  };

static double tabSinFresnel [] = {
  0.00000,     0.41888E-5,  0.33510E-4,  0.11310E-3,  0.26808E-3,
  0.52359E-3,  0.90475E-3,  0.14367E-2,  0.21444E-2,  0.30531E-2,
  0.41876E-2,  0.55730E-2,  0.72340E-2,  0.91954E-2,  0.11482E-1,
  0.14117E-1,  0.17126E-1,  0.20531E-1,  0.24357E-1,  0.28626E-1,
  0.33359E-1,  0.38580E-1,  0.44308E-1,  0.50564E-1,  0.57366E-1,
  0.64732E-1,  0.72679E-1,  0.81221E-1,  0.90371E-1,  0.10014,
  0.11054,     0.12158,     0.13325,     0.14557,     0.15854,
  0.17214,     0.18637,     0.20122,     0.21668,     0.23273,
  0.24934,     0.26649,     0.28415,     0.30228,     0.32084,
  0.33978,     0.35905,     0.37860,     0.39836,     0.41827,
  0.43826,     0.45825,     0.47815,     0.49789,     0.51737, 
  0.53650,     0.55518,     0.57331,     0.59080,     0.60753,
  0.62340,     0.63831,     0.65216,     0.66485,     0.67627,
  0.68633,     0.69496,     0.70205,     0.70756,     0.71140,
  0.71353,     0.71390
  };


/******************************************************************************
*
*   angleLimit  -  Limiter la valeur d'un angle entre -PI et PI
*
*   Description:
*   Decremente de 2PI la valeur d'un angle, jusqu'a ce que la valeur de cet
*   angle se trouve entre -PI et PI ([-PI, PI])
*
*   Retourne: le resultat de la limitation
*/

double angleLimit (angle)
     double angle;               /* Angle a limiter */

{
  /* Test angle */
  if (fabs(angle) > 10000) {
    printf ("angleLimit: bad angle %f\n", angle);
    return 0;
  }
  
  /* Ajuster l'angle s'il est superieur a PI */
  while (angle > PI)
    angle -= 2.*PI;

  /* L'ajuster s'il est inferieur a -PI */
  while (angle < -PI)
    angle += 2.*PI;

  /* Retourner l'angle ajuste */
  return (angle);
}

/******************************************************************************
*
*   angleDegLimit  -  Limiter la valeur d'un angle entre -180 et 180
*
*   Description:
*   Decremente de 360 la valeur d'un angle, jusqu'a ce que la valeur de cet
*   angle se trouve entre -180 et 180 ([-180, 180])
*
*   Retourne: le resultat de la limitation
*/

double angleDegLimit (angle)
     double angle;               /* Angle a limiter */

{
  /* Ajuster l'angle s'il est superieur a 180 */
  while (angle > 180)
    angle -= 360;

  /* L'ajuster s'il est inferieur a -180 */
  while (angle < -180)
    angle += 360;

  /* Retourner l'angle ajuste */
  return (angle);
}


/******************************************************************************
*
*  superTab  -  Plus grande valeur dans un tableau de flottants
*
*  Description:
*  Cherche le terme le plus grand dans un tableau de flottants.
*
*  Retourne : le terme le plus grand du tableau de flottants.
*/

double superTab (n, pTab)
     int n;                  /* Nombre d'elements du tableau */
     double *pTab;            /* Adresse de debut du tableau */

{
  int nTerme;                /* Indice d'un terme du tableau */
  double maxTerme;            /* Plus grand terme du tableau */
  
  /* Initialiser la var. qui garde la valeur + grande du tableau */
  maxTerme = *pTab++;

  /* Chercher le terme le plus grand du tableau */
  for (nTerme = 1 ; nTerme < n; nTerme++)
    {
      if (*pTab > maxTerme)
	maxTerme = *pTab;
      pTab++;
    }
  return (maxTerme);
}


/******************************************************************************
*
*  inferTab  -  Plus petite valeur dans un tableau de flottants
*
*  Description:
*  Cherche le terme le plus petit dans un tableau de flottants.
*
*  Retourne : le terme le plus petit du tableau de flottants.
*/

double inferTab (n, pTab)
     int n;                  /* Nombre d'elements du tableau */
     double *pTab;            /* Adresse de debut du tableau */

{
  int nTerme;                /* Indice d'un terme du tableau */
  double infTerme;            /* Plus petit terme du tableau */

  /* Initialiser la var. qui garde la valeur + petite du tableau */
  infTerme = *pTab++;
  
  /* Chercher le terme le plus petit */
  for (nTerme = 1 ; nTerme < n; nTerme++)
    {
      if (*pTab < infTerme)
	infTerme = *pTab;
      pTab++;
    }
  return (infTerme);
}


/******************************************************************************
*
*   atan2Par  -  Arctangente avec 2 parametres
*
*   Description:
*   Calcule l'arctangente, a partir de 2 differences de coordonnees.
* 
*   Retourne : angle entre -PI et PI
*/

double atan2Par (dty, dtx)
     double dty, dtx;            /* Differences de coordonnees */

{
  /* Verifier si le dtx est nul */
  if (dty + dtx == dty)
    return (dty < 0 ? -PI/2 : PI/2);

  /* Retourner l'angle entre -PI et PI */
  if (dtx < 0)
    return (dty < 0 ? -PI + atan (dty/dtx) : PI - atan (-dty/dtx));
  return (dty < 0 ? -atan (-dty/dtx) : atan (dty/dtx));
}


/******************************************************************************
*
*  h2cos  -  Cosinus de l'angle exprime en radian
*
*
*  Retourne: la valeur du cos
*/

double h2cos (theta)
     double theta;

{
  double cosAngle;

  cosAngle = cosTab[(int)(C_RAD_DEG * angleLimit(theta) + 180 + .5)];
  return (cosAngle);
}


/******************************************************************************
*
*  h2sin  -  Sinus de l'angle exprime en radian
*
*
*  Retourne: la valeur du sin
*/

double h2sin (double theta)
{
  double sinAngle;

  sinAngle = sinTab[(int)(C_RAD_DEG * angleLimit(theta) + 180 + .5)];
  return (sinAngle);
}


/******************************************************************************
*
*   integCosFresnel  -  Integrale du cosinus du carre d'une variable
*
*   Description:
*   Calcule l'integrale du cosinus du carre d'une variable (integrale
*   de Fresnel).  Obs.: integ de 0 a theta du cosinus de theta*theta*PI/2
*
*   Retourne : la valeur de l'integrale
*/

double integCosFresnel (double theta)
{
  int i;                       /* Position dans le tableau */
  double cosF;
  double signe;

  /* On met tout en positif */
  signe = theta < 0. ? -1. : 1.;
  theta *= signe;

  /* Trouver position dans tableau */
  i = (int) (50.*theta);
  if (i>= sizeof(tabCosFresnel)/sizeof(double)) {
    printf ("integCosFresnel: theta=%g out of bounds\n", theta);
    return 0;
  }

  /* Faire l'interpolation */
  cosF = tabCosFresnel[i] + 
	  (tabCosFresnel [i+1] - tabCosFresnel [i])*50.*(theta - i * 0.02);

  /* retour avec le signe */
  return (signe * cosF);
}


/*****************************************************************************
*
*   integSinFresnel  -  Integrale du sinus du carre d'une variable
*
*   Description:
*   Calcule l'integrale du sinus du carre d'une variable (integrale
*   de Fresnel).  Obs.: integ de 0 a theta du sinus de theta*theta*PI/2
*
*   Retourne : la valeur de l'integrale
*/

double integSinFresnel (double theta)
{
  int i;                       /* Position dans le tableau */
  double sinF;
  double signe;

  /* On met tout en positif */
  signe = theta < 0. ? -1. : 1.;
  theta *= signe;

  /* Trouver position dans tableau */
  i = (int) (50.*theta);
  if (i>= sizeof(tabSinFresnel)/sizeof(double)) {
    printf ("tabSinFresnel: theta=%g out of bounds\n", theta);
    return 0;
  }
  
  /* Faire l'interpolation */
  sinF = tabSinFresnel[i] +
	  (tabSinFresnel [i+1] - tabSinFresnel [i])*50.*(theta - i * 0.02);

  /* retour avec le signe */
  return (signe*sinF);
} 

#ifdef UNIX
double 
infinity(void)
{
    return HUGE_VAL;
}
#endif

#ifdef VXWORKS
/******************************************************************************
*
*  srand - Set du nombre de base du generateur pseudo-aleatoire
*
*  Description:
*  Set du nombre de base du generateur pseudo-aleatoire
* 
*  Retourne: Neant
*/

void srand (x)
     unsigned x;
{
  randx = x;
}



/******************************************************************************
*
*  rand  -  Generateur de nombres pseudo-aleatoires
*
*  Description:
*  Generateur de nombres pseudo-aleatoires
*
*  Retourne: nombre pseudo-aleatoire
*/

long rand ()
{
  register int r;
  r = randx * 1103515245;
  return (randx = (((r&0x3fffffff) + 12345) ^ (r&0x40000000)));
}

#endif

