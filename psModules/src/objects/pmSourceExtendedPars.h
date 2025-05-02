/* @file  pmSourceExtendedPars.h
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:25 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_SOURCE_EXTENDED_PARS_H
# define PM_SOURCE_EXTENDED_PARS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
    psArray  *radii;			// radii for raw radial profiles at evenly-spaced angles
    psArray  *fluxes;			// fluxes measured at above radii
    psVector *theta;			// angles corresponding to above radial profiles
    psVector *isophotalRadii;		// isophotal radius for the above angles
} pmSourceRadialFlux;

typedef struct {
    psVector *flux;			// fluxes measured at above radii
    psVector *fluxStdev;		// scatter (standard deviation) of flux
    psVector *fluxErr;			// formal error on the fluxes (sqrt\sum(variance))
    psVector *fill;			// angles corresponding to above radial profiles
} pmSourceRadialApertures;

typedef struct {
    psVector *radiusElliptical;		// normalized radial coordinates for all relevant pixels
    psVector *fluxElliptical;		// flux for the above radial coordinates
} pmSourceEllipticalFlux;

typedef struct {
    psVector *binSB;			// mean surface brightness within radial bins
    psVector *binSBstdev;		// scatter of mean surface brightness within radial bins
    psVector *binSBerror;		// formal error on mean surface brightness within radial bins
    psVector *binSum;			// sum of flux within radial bins
    psVector *binFill;			// fraction of area actually lit
    psVector *radialBins;		// radii corresponding to above binnedFlux
    psVector *area;			// differential area of the non-overlapping radial bins
} pmSourceRadialProfile;

typedef struct {
    float flux;
    float fluxErr;
    float radius;
    float radiusErr;
} pmSourceExtendedFlux;

typedef struct {
    pmSourceRadialFlux     *radFlux;	    // raw radial flux information
    pmSourceEllipticalFlux *ellipticalFlux; // flux for elliptically-renormalized radii
    pmSourceRadialProfile  *radProfile;	    // surface brightness profile in specified fixed bins
    pmSourceRadialProfile  *petProfile;	    // surface brightness profile in petrosian bins
    psEllipseAxes axes;			    // shape of elliptical contour
    float petrosianFlux;
    float petrosianFluxErr;
    float petrosianRadius;
    float petrosianRadiusErr;
    float petrosianR90;
    float petrosianR90Err;
    float petrosianR50;
    float petrosianR50Err;
    float petrosianFill;
    float ghalfLightRadius;                 
    float gRT;                           // total residual (from eliptically symmetric model) Simard 2002
    float gRA;                           // assymetric residual (from eliptically symmetric model) Simard 2002
    float gS2;                           // (un) smoothness  (Simard, 2011 Cheng 2011)
    float gA;                            // assymetry index (Gyory & Bell 2010)
    float gbumpy;                        // Blakeslee bumpiness
} pmSourceExtendedPars;

// additional measurements related to the model fits
typedef struct {
    float Mxx;
    float Mxy;
    float Myy;
    
    float Mrf;
    float Mrh;

    float apMag;
    float krMag;
    float psfMag;
    float peakMag;
} pmSourceExtFitPars;

typedef struct {
  int       modelType;
  psVector *Flux;
  psVector *dFlux;
  psVector *chisq;
  int       nPix;
  bool      reducedTrials;
  float     fRmajorMin;
  float     fRmajorMax;
  float     fRmajorDel;
  float     fRminorMin;
  float     fRminorMax;
  float     fRminorDel;
} pmSourceGalaxyFits;

pmSourceRadialFlux *pmSourceRadialFluxAlloc();
bool psMemCheckSourceRadialFlux(psPtr ptr);

pmSourceRadialApertures *pmSourceRadialAperturesAlloc();
bool psMemCheckSourceRadialApertures(psPtr ptr);

pmSourceEllipticalFlux *pmSourceEllipticalFluxAlloc();
bool psMemCheckSourceEllipticalFlux(psPtr ptr);

// *** pmSourceRadialProfile describes the radial profile of a source in elliptical contours, and 
// intermediate data used to measure the profile
pmSourceRadialProfile *pmSourceRadialProfileAlloc();
bool psMemCheckSourceRadialProfile(psPtr ptr);

// *** pmSourceExtendedPars describes the possible collection of extended flux measurements for a source
pmSourceExtendedPars *pmSourceExtendedParsAlloc (void);
bool psMemCheckSourceExtendedPars(psPtr ptr);

// *** pmSourceExtendedFlux describes the flux within an elliptical aperture of some kind 
pmSourceExtendedFlux *pmSourceExtendedFluxAlloc(void);
bool psMemCheckSourceExtendedFlux(psPtr ptr);

// *** pmSourceRadialProfileSortPair is a utility function for sorting a pair of vectors
bool pmSourceRadialProfileSortPair(psVector *index, psVector *extra);

pmSourceExtFitPars *pmSourceExtFitParsAlloc (void);

pmSourceGalaxyFits *pmSourceGalaxyFitsAlloc (void);

/// @}
# endif /* PM_SOURCE_H */
