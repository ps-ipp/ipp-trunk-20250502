/* @file  pmPetrosian.c
 * low-level petrosian functions
 *
 * @author EAM, IfA
 *
 * @version $Revision: $
 * @date $Date: $
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pmPetrosian.h"

static void pmPetrosianFree(pmPetrosian *petrosian)
{
    if (!petrosian) {
        return;
    }
    psFree(petrosian->radii);
    psFree(petrosian->fluxes);
    psFree(petrosian->theta);
    psFree(petrosian->isophotalRadii);

    psFree(petrosian->radiusElliptical);
    psFree(petrosian->fluxElliptical);

    psFree(petrosian->binSB);
    psFree(petrosian->binSBstdev);
    psFree(petrosian->radialBins);
    psFree(petrosian->area);
}

pmPetrosian *pmPetrosianAlloc()
{
    pmPetrosian *petrosian = (pmPetrosian *)psAlloc(sizeof(pmPetrosian));
    psMemSetDeallocator(petrosian, (psFreeFunc) pmPetrosianFree);

    petrosian->radii = NULL;
    petrosian->fluxes = NULL;
    petrosian->theta = NULL;
    petrosian->isophotalRadii = NULL;

    petrosian->radiusElliptical = NULL;
    petrosian->fluxElliptical = NULL;

    petrosian->radialBins = NULL;
    petrosian->area = NULL;
    petrosian->binSB = NULL;
    petrosian->binSBstdev = NULL;

    petrosian->petrosianRadius = NAN;
    petrosian->petrosianFlux = NAN;

    // petrosian->axes = {0.0, 0.0, 0.0};

    return petrosian;
}

bool pmPetrosianFreeVectors(pmPetrosian *petrosian) {

    psFree(petrosian->radii);
    psFree(petrosian->fluxes);
    psFree(petrosian->theta);
    psFree(petrosian->isophotalRadii);

    psFree(petrosian->radiusElliptical);
    psFree(petrosian->fluxElliptical);

    psFree(petrosian->binSB);
    psFree(petrosian->binSBstdev);
    psFree(petrosian->radialBins);
    psFree(petrosian->area);

    petrosian->radii = NULL;
    petrosian->fluxes = NULL;
    petrosian->theta = NULL;
    petrosian->isophotalRadii = NULL;

    petrosian->radiusElliptical = NULL;
    petrosian->fluxElliptical = NULL;

    petrosian->radialBins = NULL;
    petrosian->area = NULL;
    petrosian->binSB = NULL;
    petrosian->binSBstdev = NULL;
    
    return true;
}

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

bool pmPetrosianSortPair (psVector *index, psVector *extra) {

    // sort the vector set by the radius
    PSSORT (index->n, COMPARE_INDEX, SWAP_INDEX, NONE);
    return true;
}
