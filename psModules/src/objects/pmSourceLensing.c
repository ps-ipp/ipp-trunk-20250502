/** @file  pmSourceLensing.c
 *
 *  Functions to measure the local sky and sky variance for sources on images
 *  @author EAM, IfA: 
 *  @date $Date: 2014-03-20 $
 *
 *  Copyright 2014 Ifa, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>

#include "pmMoments.h"
#include "pmSourceLensing.h"

static void pmLensingParsFree (pmLensingPars *pars) {
  if (!pars) return;
  return;
}

pmLensingPars *pmLensingParsAlloc () {

  pmLensingPars *tmp = (pmLensingPars *) psAlloc(sizeof(pmLensingPars));;
  psMemSetDeallocator(tmp, (psFreeFunc) pmLensingParsFree);

  tmp->X11 = NAN;
  tmp->X12 = NAN;
  tmp->X22 = NAN;

  tmp->e1 = NAN;
  tmp->e2 = NAN;

  return tmp;
}

static void pmSourceLensingFree (pmSourceLensing *lensing) {
  if (!lensing) return;
  psFree (lensing->smear);
  psFree (lensing->shear);
  return;
}

pmSourceLensing *pmSourceLensingAlloc () {

  pmSourceLensing *tmp = (pmSourceLensing *) psAlloc(sizeof(pmSourceLensing));;
  psMemSetDeallocator(tmp, (psFreeFunc) pmSourceLensingFree);

  tmp->smear = NULL;
  tmp->shear = NULL;
  tmp->e1 = NAN;
  tmp->e2 = NAN;

  return tmp;
}

// need to supply the moments and the window-function sigma
bool pmSourceLensingShearFromMoments (pmSourceLensing *lensing, pmMoments *moments, float sigma) {
  
  if (!lensing->shear) {
    lensing->shear = pmLensingParsAlloc();
  }
  
  pmLensingPars *shear = lensing->shear;

  float R = 1.0 / (moments->Mxx + moments->Myy);
  float s2 = 1.0 / PS_SQR(sigma);
  // NOTE : not used by shear : float s4 = PS_SQR(s2);

  shear->X11 = R*(2.0*(moments->Mxx + moments->Myy) - s2 * (moments->Mxxxx - 2.0*moments->Mxxyy + moments->Myyyy));

  shear->X22 = R*(2.0*(moments->Mxx + moments->Myy) - s2 * 4.0 * moments->Mxxyy);

  shear->X12 = R*2.0*s2*(moments->Mxyyy - moments->Mxxxy);

  shear->e1  = R*(2.0*(moments->Mxx - moments->Myy) + s2 * (moments->Myyyy - moments->Mxxxx));

  shear->e2  = R*(4.0*moments->Mxy - 2.0*s2*(moments->Mxxxy + moments->Mxyyy));
  
  return true;
}

// need to supply the moments and the window-function sigma
// NOTE: I'm using the coefficients from Hoekstra et al 1998, not KSB96
bool pmSourceLensingSmearFromMoments (pmSourceLensing *lensing, pmMoments *moments, float sigma) {
  
  if (!lensing->smear) {
    lensing->smear = pmLensingParsAlloc();
  }
  
  pmLensingPars *smear = lensing->smear;

  float R = 1.0 / (moments->Mxx + moments->Myy);
  float s2 = 1.0 / PS_SQR(sigma);
  float s4 = PS_SQR(s2);

  smear->X11 = R*(1.0 - s2*(moments->Mxx + moments->Myy) + 0.25*s4 * (moments->Mxxxx - 2.0*moments->Mxxyy + moments->Myyyy));

  smear->X22 = R*(1.0 - s2*(moments->Mxx + moments->Myy) + 1.00*s4 * (moments->Mxxyy));

  smear->X12 = R*0.5*s4*(moments->Mxxxy - moments->Mxyyy);

  smear->e1  = R*(s2*(moments->Myy - moments->Mxx) + 0.25*s4 * (moments->Mxxxx - moments->Myyyy));

  smear->e2  = R*(0.5*s4*(moments->Mxxxy + moments->Mxyyy) - 2.0*s2*moments->Mxy);
  
  return true;
}
