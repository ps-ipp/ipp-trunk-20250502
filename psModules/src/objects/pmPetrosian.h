/* @file  pmPetrosian.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: $
 * @date $Date: $
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_PETROSIAN_H
#define PM_PETROSIAN_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
    psArray  *radii;			// radii for raw radial profiles at evenly-spaced angles
    psArray  *fluxes;			// fluxes measured at above radii
    psVector *theta;			// angles corresponding to above radial profiles
    psVector *isophotalRadii;		// isophotal radius for the above angles

    psVector *radiusElliptical;		// normalized radial coordinates for all relevant pixels
    psVector *fluxElliptical;		// flux for the above radial coordinates

    psVector *binSB;			// mean surface brightness within radial bins
    psVector *binSBstdev;		// scatter of mean surface brightness within radial bins
    psVector *radialBins;		// radii corresponding to above binnedBlux
    psVector *area;			// differential area of the non-overlapping radial bins

    psEllipseAxes axes;			// shape of elliptical contour

    float petrosianRadius;
    float petrosianFlux;

} pmPetrosian;

pmPetrosian *pmPetrosianAlloc();

bool pmPetrosianFreeVectors(pmPetrosian *petrosian);
bool pmPetrosianSortPair (psVector *index, psVector *extra);

/// @}

# endif /* PM_PETROSIAN_H */
