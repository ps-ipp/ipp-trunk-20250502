/** @file  pmSourceExtendedPars.c
 *
 *  Functions to define and manipulate sources on images
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA: significant modifications.
 *
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:31:25 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include "pmSourceExtendedPars.h"

// pmSourceRadialFlux carries the raw radial flux information, including angular bins
static void pmSourceRadialFluxFree(pmSourceRadialFlux *flux)
{
    if (!flux) return;
    psFree(flux->radii);
    psFree(flux->fluxes);
    psFree(flux->theta);
    psFree(flux->isophotalRadii);
}

pmSourceRadialFlux *pmSourceRadialFluxAlloc()
{
    pmSourceRadialFlux *flux = (pmSourceRadialFlux *)psAlloc(sizeof(pmSourceRadialFlux));
    psMemSetDeallocator(flux, (psFreeFunc) pmSourceRadialFluxFree);

    flux->radii = NULL;
    flux->fluxes = NULL;
    flux->theta = NULL;
    flux->isophotalRadii = NULL;

    return flux;
}

bool psMemCheckSourceRadialFlux(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceRadialFluxFree);
}

// pmSourceRadialApertures carries the raw radial flux information, including angular bins
static void pmSourceRadialAperturesFree(pmSourceRadialApertures *radial)
{
    if (!radial) return;
    psFree(radial->flux);
    psFree(radial->fluxErr);
    psFree(radial->fluxStdev);
    psFree(radial->fill);
}

pmSourceRadialApertures *pmSourceRadialAperturesAlloc()
{
    pmSourceRadialApertures *radial = (pmSourceRadialApertures *)psAlloc(sizeof(pmSourceRadialApertures));
    psMemSetDeallocator(radial, (psFreeFunc) pmSourceRadialAperturesFree);

    radial->flux = NULL;
    radial->fluxErr = NULL;
    radial->fluxStdev = NULL;
    radial->fill = NULL;
    return radial;
}

bool psMemCheckSourceRadialApertures(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceRadialAperturesFree);
}

// pmSourceEllipticalFlux carries the elliptical renormalized radial flux info
static void pmSourceEllipticalFluxFree(pmSourceEllipticalFlux *flux)
{
    if (!flux) return;
    psFree(flux->radiusElliptical);
    psFree(flux->fluxElliptical);
}

pmSourceEllipticalFlux *pmSourceEllipticalFluxAlloc()
{
    pmSourceEllipticalFlux *flux = (pmSourceEllipticalFlux *)psAlloc(sizeof(pmSourceEllipticalFlux));
    psMemSetDeallocator(flux, (psFreeFunc) pmSourceEllipticalFluxFree);

    flux->radiusElliptical = NULL;
    flux->fluxElliptical = NULL;

    return flux;
}

bool psMemCheckSourceEllipticalFlux(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceEllipticalFluxFree);
}

// pmSourceRadialProfile defines flux information in radial bins
static void pmSourceRadialProfileFree(pmSourceRadialProfile *profile)
{
    if (!profile) return;
    psFree(profile->binSB);
    psFree(profile->binSBstdev);
    psFree(profile->binSBerror);
    psFree(profile->binSum);
    psFree(profile->binFill);
    psFree(profile->radialBins);
    psFree(profile->area);
}

pmSourceRadialProfile *pmSourceRadialProfileAlloc()
{
    pmSourceRadialProfile *profile = (pmSourceRadialProfile *)psAlloc(sizeof(pmSourceRadialProfile));
    psMemSetDeallocator(profile, (psFreeFunc) pmSourceRadialProfileFree);

    profile->binSB = NULL;
    profile->binSBstdev = NULL;
    profile->binSBerror = NULL;
    profile->binSum = NULL;
    profile->binFill = NULL;
    profile->radialBins = NULL;
    profile->area = NULL;
    return profile;
}

bool psMemCheckSourceRadialProfile(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceRadialProfileFree);
}

# if (0)
// pmSourceRadialProfileFreeVectors frees the intermediate data values
bool pmSourceRadialProfileFreeVectors(pmSourceRadialProfile *profile) {

    psFree(profile->radii);
    psFree(profile->fluxes);
    psFree(profile->theta);
    psFree(profile->isophotalRadii);

    psFree(profile->radiusElliptical);
    psFree(profile->fluxElliptical);

    // psFree(profile->binSB);
    // psFree(profile->binSBstdev);
    // psFree(profile->binSBerror);
    
    // psFree(profile->radialBins);
    psFree(profile->area);

    profile->radii = NULL;
    profile->fluxes = NULL;
    profile->theta = NULL;
    profile->isophotalRadii = NULL;

    profile->radiusElliptical = NULL;
    profile->fluxElliptical = NULL;

    // profile->binSB = NULL;
    // profile->binSBstdev = NULL;
    // profile->binSBerror = NULL;
    
    // profile->radialBins = NULL;
    profile->area = NULL;

    return true;
}
# endif

// *** pmSourceRadialProfileSortPair is a utility function for sorting a pair of vectors
# define COMPARE_INDEX(A,B) (index->data.F32[A] < index->data.F32[B])
# define SWAP_INDEX(TYPE,A,B) { \
  float tmp; \
  if (A != B) { \
    tmp = index->data.F32[A]; \
    index->data.F32[A] = index->data.F32[B]; \
    index->data.F32[B] = tmp; \
    tmp = extra->data.F32[A]; \
    extra->data.F32[A] = extra->data.F32[B]; \
    extra->data.F32[B] = tmp; \
  } \
}

bool pmSourceRadialProfileSortPair (psVector *index, psVector *extra) {

    psAssert (index->n == extra->n, "mismatched vector lengths");
    if (index->n < 2) return true;

    // sort the vector set by the radius
    PSSORT (index->n, COMPARE_INDEX, SWAP_INDEX, NONE);
    return true;
}

// *** pmSourceExtendedPars describes the possible collection of extended flux measurements for a source
static void pmSourceExtendedParsFree (pmSourceExtendedPars *pars) {
    if (!pars) return;

    psFree(pars->radFlux);
    psFree(pars->ellipticalFlux);
    psFree(pars->radProfile);
    psFree(pars->petProfile);
    return;
}

pmSourceExtendedPars *pmSourceExtendedParsAlloc (void) {
    pmSourceExtendedPars *pars = (pmSourceExtendedPars *) psAlloc(sizeof(pmSourceExtendedPars));
    psMemSetDeallocator(pars, (psFreeFunc) pmSourceExtendedParsFree);

    pars->radFlux = NULL;
    pars->ellipticalFlux = NULL;
    pars->radProfile = NULL;
    pars->petProfile = NULL;

    pars->petrosianFlux = NAN;
    pars->petrosianFluxErr = NAN;
    pars->petrosianRadius = NAN;
    pars->petrosianRadiusErr = NAN;
    pars->petrosianR90 = NAN;
    pars->petrosianR90Err = NAN;
    pars->petrosianR50 = NAN;
    pars->petrosianR50Err = NAN;
    pars->ghalfLightRadius = NAN;
    pars->gRT = NAN;
    pars->gRA = NAN;
    pars->gS2 = NAN;
    pars->gA = NAN;
    pars->gbumpy = NAN;
    return pars;
}

bool psMemCheckSourceExtendedPars(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceExtendedParsFree);
}


// *** pmSourceExtendedFlux describes the flux within an elliptical aperture of some kind 
static void pmSourceExtendedFluxFree (pmSourceExtendedFlux *flux) {
    if (!flux) return;
    return;
}

pmSourceExtendedFlux *pmSourceExtendedFluxAlloc (void) {

    pmSourceExtendedFlux *flux = (pmSourceExtendedFlux *) psAlloc(sizeof(pmSourceExtendedFlux));
    psMemSetDeallocator(flux, (psFreeFunc) pmSourceExtendedFluxFree);

    flux->flux = 0.0;
    flux->fluxErr = 0.0;
    flux->radius = 0.0;
    flux->radiusErr = 0.0;

    return flux;
}


bool psMemCheckSourceExtendedFlux(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmSourceExtendedFluxFree);
}

// *** pmSourceExtFitPars describes extra metadata related to an extended fit
static void pmSourceExtFitParsFree (pmSourceExtFitPars *pars) {
    return;
}

pmSourceExtFitPars *pmSourceExtFitParsAlloc (void) {

    pmSourceExtFitPars *pars = (pmSourceExtFitPars *) psAlloc(sizeof(pmSourceExtFitPars));
    psMemSetDeallocator(pars, (psFreeFunc) pmSourceExtFitParsFree);

    pars->Mxx = NAN;
    pars->Mxy = NAN;
    pars->Myy = NAN;

    pars->Mrf    = NAN;
    pars->Mrh    = NAN;

    pars->apMag  = NAN;
    pars->krMag  = NAN;
    pars->psfMag = NAN;
    pars->peakMag = NAN;

    return pars;
}

// *** pmSourceExtFitPars describes extra metadata related to an extended fit
static void pmSourceGalaxyFitsFree (pmSourceGalaxyFits *tmp) {
  
    psFree (tmp->Flux);
    psFree (tmp->dFlux);
    psFree (tmp->chisq);

    return;
}

pmSourceGalaxyFits *pmSourceGalaxyFitsAlloc (void) {

    pmSourceGalaxyFits *tmp = (pmSourceGalaxyFits *) psAlloc(sizeof(pmSourceGalaxyFits));
    psMemSetDeallocator(tmp, (psFreeFunc) pmSourceGalaxyFitsFree);

    tmp->Flux  = psVectorAllocEmpty (25, PS_TYPE_F32);
    tmp->dFlux = psVectorAllocEmpty (25, PS_TYPE_F32);
    tmp->chisq = psVectorAllocEmpty (25, PS_TYPE_F32);
    tmp->nPix = 0;

    return tmp;
}
