/* @file  pmAstrometryDistortion.h
 * @brief functions to convert FITS WCS keywords to / from pmFPA structures
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-24 01:08:17 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_ASTROMETRY_WCS_H
#define PM_ASTROMETRY_WCS_H

/// @addtogroup Astrometry
/// @{

#define PM_ASTROM_WCS_TYPE_SIZE 80
typedef struct
{
    char ctype1[PM_ASTROM_WCS_TYPE_SIZE];
    char ctype2[PM_ASTROM_WCS_TYPE_SIZE];
    double crval1, crval2;
    double crpix1, crpix2;
    double cdelt1, cdelt2;
    bool wcsCDkeys;
    psProjection *toSky;
    psPlaneTransform *trans;
}
pmAstromWCS;

// support function for the pmAstromWCS representation
pmAstromWCS *pmAstromWCSAlloc (int nXorder, int nYorder);
bool pmAstromWCStoSky (psSphere *sky, pmAstromWCS *wcs, psPlane *chip);
bool pmAstromWCStoChip (psPlane *chip, pmAstromWCS *wcs, psSphere *sky);

// read and write the pmAstromWCS representation to the header
bool pmAstromWCStoHeader (psMetadata *header, const pmAstromWCS *wcs);
pmAstromWCS *pmAstromWCSfromHeader (const psMetadata *header);

// convert from wcs terms to chip->toFPA, fpa->toSky,toTPA terms
bool pmAstromWCSBileveltoChip (pmChip *chip, const pmAstromWCS *wcs);
bool pmAstromWCSBileveltoFPA (pmFPA *fpa, const pmAstromWCS *wcs, psRegion region);

// convert from chip->toFPA, fpa->toSky,toTPA terms to wcs terms
pmAstromWCS *pmAstromWCSBilevelChipFromFPA (const pmChip *chip, double tol);
pmAstromWCS *pmAstromWCSBilevelMosaicFromFPA (const pmFPA *fpa, double tol);

// convert the pmAstromWCS representation to the FPA representation
bool pmAstromWCStoFPA (pmFPA *fpa, pmChip *chip, const pmAstromWCS *wcs, double plateScale);
pmAstromWCS *pmAstromWCSfromFPA (const pmFPA *fpa, const pmChip *chip, double tol);

// read wcs terms from the supplied header into the fpa hierarchy components
bool pmAstromReadWCS (pmFPA *fpa, pmChip *chip, const psMetadata *header, double plateScale);

// write the wcs terms from the fpa hierarchy components into the supplied header
// tol is the convergence tolerance for the non-linear solution to the reference pixel
bool pmAstromWriteWCS (psMetadata *header, const pmFPA *fpa, const pmChip *chip, double tol);

bool pmAstromReadBilevelChip (pmChip *chip, const psMetadata *header);
bool pmAstromReadBilevelMosaic (pmFPA *fpa, const psMetadata *header);

bool pmAstromWriteBilevelChip (psMetadata *header, const pmChip *chip, double tol);
bool pmAstromWriteBilevelMosaic (psMetadata *header, const pmFPA *fpa, double tol);

bool pmAstromLinearizeTransforms(pmFPA *inFPA, pmChip *inChip, pmFPA *outFPA, pmChip *outChip, psRegion *outputBounds, double offset_x, double offset_y);
bool pmAstromLinearizeToSky(pmFPA *inFPA, pmChip *inChip, pmFPA *outFPA, pmChip *outChip, psRegion *bounds);

// move to pslib
psPlaneDistort *psPlaneDistortIdentity (int order);
bool psPlaneDistortIsDiagonal (psPlaneDistort *distort);

// XXX probably should remove these and just use the PS_ version in the code
# define PM_DEG_RAD PS_DEG_RAD
# define PM_RAD_DEG PS_RAD_DEG

/// @}
#endif // PM_ASTROMETRY_WCS_H

/*
 * the wcs->trans component defines a polynomial which converts (x-crpix1),(y-crpix2) to
 * L,M in degrees
 */
