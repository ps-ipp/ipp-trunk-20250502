/** @file  pmAstrometryRegions.c
 *  @brief functions to define astrometry regions on FPA images
 *  @ingroup Astrometry
 *
 *  @author EAM, IfA
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-12-22 17:51:48 $
 *
 *  Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAExtent.h"
#include "pmAstrometryRegions.h"

// cell pixels corresponding to readout boundary
psRegion *pmAstromReadoutInCell (pmReadout *readout) {

    psRegion *region;
    region = pmReadoutExtent (readout);
    return (region);
}

// chip pixels corresponding to cell boundary
psRegion *pmAstromCellInChip (pmCell *cell) {

    psRegion *region;
    region = pmCellExtent (cell);
    return (region);
}

// FP pixels corresponding to chip boundary
// since the chip may be rotated in the fpa, this region does not correspond 
// exactly to the pixel grid of the chip
psRegion *pmAstromChipInFP (pmChip *chip) {

    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    // if we have not astrometry for this chip, just skip it silently (no error)
    if (!chip->toFPA) return NULL;

    // determine the bounding box of this chip in chip pixels
    psRegion *chipExtent = pmChipPixels (chip);
    if (!chipExtent) return NULL;

    // apply chip-to-fpa astrometry to determine fpa coordinates 
    psPlane *chPix = psPlaneAlloc ();
    psPlane *fpPix = psPlaneAlloc ();
    psRegion *chipRegion = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of chip

    // determine the outer bounding box -- does not correspond to the same square region!
    chPix->x = chipExtent->x0;
    chPix->y = chipExtent->y0;
    psPlaneTransformApply(fpPix, chip->toFPA, chPix); 
    chipRegion->x0 = PS_MIN (chipRegion->x0, fpPix->x);
    chipRegion->x1 = PS_MAX (chipRegion->x1, fpPix->x);
    chipRegion->y0 = PS_MIN (chipRegion->y0, fpPix->y);
    chipRegion->y1 = PS_MAX (chipRegion->y1, fpPix->y);

    chPix->x = chipExtent->x1;
    chPix->y = chipExtent->y0;
    psPlaneTransformApply(fpPix, chip->toFPA, chPix); 
    chipRegion->x0 = PS_MIN (chipRegion->x0, fpPix->x);
    chipRegion->x1 = PS_MAX (chipRegion->x1, fpPix->x);
    chipRegion->y0 = PS_MIN (chipRegion->y0, fpPix->y);
    chipRegion->y1 = PS_MAX (chipRegion->y1, fpPix->y);

    chPix->x = chipExtent->x1;
    chPix->y = chipExtent->y1;
    psPlaneTransformApply(fpPix, chip->toFPA, chPix); 
    chipRegion->x0 = PS_MIN (chipRegion->x0, fpPix->x);
    chipRegion->x1 = PS_MAX (chipRegion->x1, fpPix->x);
    chipRegion->y0 = PS_MIN (chipRegion->y0, fpPix->y);
    chipRegion->y1 = PS_MAX (chipRegion->y1, fpPix->y);

    chPix->x = chipExtent->x0;
    chPix->y = chipExtent->y1;
    psPlaneTransformApply(fpPix, chip->toFPA, chPix); 
    chipRegion->x0 = PS_MIN (chipRegion->x0, fpPix->x);
    chipRegion->x1 = PS_MAX (chipRegion->x1, fpPix->x);
    chipRegion->y0 = PS_MIN (chipRegion->y0, fpPix->y);
    chipRegion->y1 = PS_MAX (chipRegion->y1, fpPix->y);

    psFree (chPix);
    psFree (fpPix);
    psFree (chipExtent);
    return (chipRegion);
}

// return FPA pixels included in all chips
// this FPA grid has 0,0 at the mosaic center and is used for astrometric reference.
psRegion *pmAstromFPAExtent(const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    psArray *chips = fpa->chips;       // Array of component chips
    psRegion *fpaExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of fpa
    for (long i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        psRegion *chipExtent = pmAstromChipInFP(chip); // Extent of chip
	if (!chipExtent) { continue; }
        fpaExtent->x0 = PS_MIN(fpaExtent->x0, chipExtent->x0);
        fpaExtent->x1 = PS_MAX(fpaExtent->x1, chipExtent->x1);
        fpaExtent->y0 = PS_MIN(fpaExtent->y0, chipExtent->y0);
        fpaExtent->y1 = PS_MAX(fpaExtent->y1, chipExtent->y1);
        psFree(chipExtent);
    }

    return fpaExtent;
}

// TPA pixels corresponding to FPA boundary
psRegion *pmAstromFPInTP (pmFPA *fpa) {

    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa->toTPA, NULL);

    psRegion *fpaExtent = pmAstromFPAExtent (fpa);
    if (!fpaExtent) return NULL;

    // apply fpa-to-tpa astrometry to determine tpa coordinates 
    psPlane *fpPix = psPlaneAlloc ();
    psPlane *tpPix = psPlaneAlloc ();
    psRegion *fpaRegion = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of fpa

    // determine the outer bounding box -- does not correspond to the same square region!
    fpPix->x = fpaExtent->x0;
    fpPix->y = fpaExtent->y0;
    psPlaneTransformApply(tpPix, fpa->toTPA, fpPix); 
    fpaRegion->x0 = PS_MIN (fpaRegion->x0, tpPix->x);
    fpaRegion->x1 = PS_MAX (fpaRegion->x1, tpPix->x);
    fpaRegion->y0 = PS_MIN (fpaRegion->y0, tpPix->y);
    fpaRegion->y1 = PS_MAX (fpaRegion->y1, tpPix->y);

    fpPix->x = fpaExtent->x1;
    fpPix->y = fpaExtent->y0;
    psPlaneTransformApply(tpPix, fpa->toTPA, fpPix); 
    fpaRegion->x0 = PS_MIN (fpaRegion->x0, tpPix->x);
    fpaRegion->x1 = PS_MAX (fpaRegion->x1, tpPix->x);
    fpaRegion->y0 = PS_MIN (fpaRegion->y0, tpPix->y);
    fpaRegion->y1 = PS_MAX (fpaRegion->y1, tpPix->y);

    fpPix->x = fpaExtent->x1;
    fpPix->y = fpaExtent->y1;
    psPlaneTransformApply(tpPix, fpa->toTPA, fpPix); 
    fpaRegion->x0 = PS_MIN (fpaRegion->x0, tpPix->x);
    fpaRegion->x1 = PS_MAX (fpaRegion->x1, tpPix->x);
    fpaRegion->y0 = PS_MIN (fpaRegion->y0, tpPix->y);
    fpaRegion->y1 = PS_MAX (fpaRegion->y1, tpPix->y);

    fpPix->x = fpaExtent->x0;
    fpPix->y = fpaExtent->y1;
    psPlaneTransformApply(tpPix, fpa->toTPA, fpPix); 
    fpaRegion->x0 = PS_MIN (fpaRegion->x0, tpPix->x);
    fpaRegion->x1 = PS_MAX (fpaRegion->x1, tpPix->x);
    fpaRegion->y0 = PS_MIN (fpaRegion->y0, tpPix->y);
    fpaRegion->y1 = PS_MAX (fpaRegion->y1, tpPix->y);

    psFree (fpPix);
    psFree (tpPix);
    psFree (fpaExtent);
    return (fpaRegion);
}
