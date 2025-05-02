/** @file psastroModelAdjust.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"
# define NONLIN_TOL 0.001 ///< tolerance in pixels 
# define DEBUG 0

bool psastroModelAdjustBoresite (pmFPAfile *output, pmChip *refChip);

bool psastroModelAdjust (pmConfig *config) {

    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
	return false;
    }


    
    // if we have not measured the boresite position, no adjustment is needed
    bool fitBoresite = psMetadataLookupBool (&status, recipe, "PSASTRO.MODEL.FIT.BORESITE");
    if (!status) psAbort ("Can't find recipe option PSASTRO.MODEL.FIT.BORESITE");

    // as an alternative to fit the boresite from a rotation sequence, we can set the boresite
    // relative to the reference chip coordinates
    bool setBoresite = psMetadataLookupBool (&status, recipe, "PSASTRO.MODEL.SET.BORESITE");
    if (!status) psAbort ("Can't find recipe option PSASTRO.MODEL.SET.BORESITE");

    if (fitBoresite && setBoresite) {
	psError(PS_ERR_IO, true, "invalid to choose both FIT.BORESITE and SET.BORESITE"); 
	return false; 
    } 
	
    pmFPAfile *output = psMetadataLookupPtr (&status, config->files, "PSASTRO.OUT.MODEL");
    if (!status) psAbort ("Can't find output pmFPAfile PSASTRO.OUT.MODEL");
    if (!output->fpa) psAbort ("no existing input fpa contains the reference chip");

    // physical pixel scale in microns per pixel
    char *refChipName = psMetadataLookupStr (&status, recipe, "PSASTRO.MODEL.REF.CHIP");
    if (!refChipName) {
	psError(PS_ERR_IO, true, "reference chip is missing from recipe"); 
	return false; 
    } 
    float refChipAngleNominal = PS_RAD_DEG*psMetadataLookupF32 (&status, recipe, "PSASTRO.MODEL.REF.CHIP.ANGLE");
    if (!refChipName) {
	psError(PS_ERR_IO, true, "reference chip is missing from recipe"); 
	return false; 
    } 

    int rotatorParity = psMetadataLookupS32(&status, recipe, "PSASTRO.MODEL.ROT.PARITY");
    if (!status) psAbort ("Can't find recipe option PSASTRO.MODEL.ROT.PARITY");
    psMetadataAddS32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.ROT_PARITY", PS_META_REPLACE, "rotator parity parameter", rotatorParity);
    
    // get reference chip from name
    pmChip *refChip = pmConceptsChipFromName (output->fpa, refChipName);
    if (!refChip) psAbort ("invalid chip name for reference");
    if (!refChip->toFPA) psAbort ("invalid astrometry for reference chip");

    // save the TPA region for tranformation inversions below
    // psRegion *tpaRegion = pmAstromFPInTP (output->fpa);
    psRegion *fpaRegion = pmAstromFPAExtent (output->fpa);

    if (DEBUG) psastroDumpCorners ("corners.up.raw.dat", "corners.dn.raw.dat", output->fpa);

    if (setBoresite) {
	float boreXchip = psMetadataLookupF32 (&status, recipe, "PSASTRO.MODEL.BORESITE.X");
	float boreYchip = psMetadataLookupF32 (&status, recipe, "PSASTRO.MODEL.BORESITE.Y");
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.X0", PS_META_REPLACE, "boresite parameter", boreXchip); 
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.Y0", PS_META_REPLACE, "boresite parameter", boreYchip); 
    }

    if (fitBoresite || setBoresite) {
	psastroModelAdjustBoresite (output, refChip);
    } else {
	// FPA.BORE.X0,Y0 should be 0,0 in the focal plane, not the chip.  Ask for the
	// coordinates which make refChip->toFPA(x,y) = (0,0)
	psPlane *PT = psPlaneTransformGetCenter (refChip->toFPA, NONLIN_TOL);
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.X0", PS_META_REPLACE, "boresite parameter", PT->x); 
	psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.BORE.Y0", PS_META_REPLACE, "boresite parameter", PT->y); 
	psFree (PT);
    }
    
    // rotate the chip-to-FPA transforms to have 0.0 posangle for refChip; 
    // compensate by rotating fpa to TPA transform

    // get the current posangle of the ref chip
    // this should have a negative sign : chipAngle = -atan2 (dM/dy, dM/dx)
    float chipAngle = -atan2 (refChip->toFPA->y->coeff[1][0], refChip->toFPA->x->coeff[1][0]);

    // chipAngle should be refChipAngleNominal @ POSANGLE = 0.0
    float posAngleOffset = rotatorParity * (chipAngle - refChipAngleNominal);

    fprintf (stderr, "chipAngle is: %f, at PA = 0.0, it should be %f, rotating model by %f (parity %d)\n", 
	     chipAngle*PS_DEG_RAD, refChipAngleNominal*PS_DEG_RAD, posAngleOffset*PS_DEG_RAD, rotatorParity);

    // psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.POSANGLE", PS_META_REPLACE, "boresite parameter", posangle);

    // rotate the chip transforms
    for (int i = 0; i < output->fpa->chips->n; i++) {
	pmChip *chip = output->fpa->chips->data[i];
	if (!chip->toFPA) continue;
	// skip chips without astrometry

	// save the region of this chip for the inversion below
	psRegion *region = pmChipPixels (chip);

	// this should ALSO have a negative sign: this rotates by +posAngleOffset
	psPlaneTransform *toFPA = psPlaneTransformRotate (NULL, chip->toFPA, posAngleOffset);
	psFree (chip->toFPA);
	chip->toFPA = toFPA;

	// invert the new fromFPA transform to get the new toFPA transform
	// NOTE: when we call psPlaneTransformInvert here, we do not increase the order as in other places
	psPlaneTransform *fromFPA = psPlaneTransformInvert(NULL, chip->toFPA, *region, 50, 0);
	psFree (chip->fromFPA);
	chip->fromFPA = fromFPA;

	psFree (region);

	// save the transformation in the header
	pmAstromWriteBilevelChip (chip->hdu->header, chip, NONLIN_TOL);
    }

    // get the current posangle of the fpa
    float fpaAngle = atan2 (output->fpa->toTPA->y->coeff[1][0], output->fpa->toTPA->x->coeff[1][0]);
    fprintf (stderr, "fpaAngle: %f\n", fpaAngle*PS_DEG_RAD);
    // psMetadataAddF32 (output->fpa->concepts, PS_LIST_TAIL, "FPA.POSANGLE", PS_META_REPLACE, "boresite parameter", posangle);

    // remove the fpa rotation to generate a rotation-free model
    psPlaneTransform *toTPA = psPlaneTransformRotate (NULL, output->fpa->toTPA, fpaAngle);
    psFree (output->fpa->toTPA);
    output->fpa->toTPA = toTPA;

    psFree (output->fpa->fromTPA);
    // NOTE: when we call psPlaneTransformInvert here, we do not increase the order as in other places
    output->fpa->fromTPA = psPlaneTransformInvert(NULL, output->fpa->toTPA, *fpaRegion, 50, 0);

    // the model now describes the unrotated focal-plane
    if (DEBUG) psastroDumpCorners ("corners.up.rot.dat", "corners.dn.rot.dat", output->fpa);

    psMetadata *header = output->fpa->hdu->header;
    
    pmAstromWriteBilevelMosaic (header, output->fpa, NONLIN_TOL);

    psFree (fpaRegion);

    return true;
}

bool psastroModelAdjustBoresite (pmFPAfile *output, pmChip *refChip) {

    bool status;

    psPlane  *boreCH  = psPlaneAlloc();
    psPlane  *boreFP  = psPlaneAlloc();    
    psPlane  *boreTP  = psPlaneAlloc();    
    psSphere *boreSky = psSphereAlloc();    

    // correct Xo,Yo to Lo,Mo using the ref chip toFPA
    // ref chip position of the true boresite center
    boreCH->x = psMetadataLookupF32 (&status, output->fpa->concepts, "FPA.BORE.X0"); 
    boreCH->y = psMetadataLookupF32 (&status, output->fpa->concepts, "FPA.BORE.Y0"); 
    psPlaneTransformApply (boreFP, refChip->toFPA, boreCH);

    // adjust the reference pixel for all chips
    for (int i = 0; i < output->fpa->chips->n; i++) {
	pmChip *chip = output->fpa->chips->data[i];
	if (!chip->fromFPA) continue;
	// skip the chips without astrometry

	// save the FPA region of this chip for the inversion below
	psRegion *region = pmChipPixels (chip);

	// the current toFPA returns boreFP->x,y for the boresite; subtract this from the transformations
	chip->toFPA->x->coeff[0][0] -= boreFP->x;
	chip->toFPA->y->coeff[0][0] -= boreFP->y;

	// psPlaneTransform *toFPA = psPlaneTransformSetCenter (NULL, chip->toFPA, -boreFP->x, -boreFP->y);
	// psFree (chip->toFPA);
	// chip->toFPA = toFPA;

	// invert the new fromFPA transform to get the new toFPA transform
	// the region used here is the region covered by the chip in the FPA
	// NOTE: when we call psPlaneTransformInvert here, we do not increase the order as in other places
	psPlaneTransform *fromFPA = psPlaneTransformInvert(NULL, chip->toFPA, *region, 50, 0); 
	psFree (chip->fromFPA);
	chip->fromFPA = fromFPA;

	psFree (region);
    }

    if (DEBUG) psastroDumpCorners ("corners.up.shf.dat", "corners.dn.shf.dat", output->fpa);

    // we have now adjusted the chips to use the correct boresite position as the center of the focal-plane system.
    // we now need to reconstruct the TP to FP transformation, starting from stars projected about this new boresite position.

    // find the R,D of the new boresite (boreFP -> 0,0; 0,0 -> -boreFP)
    boreFP->x = -boreFP->x;
    boreFP->y = -boreFP->y;
    psPlaneTransformApply (boreTP, output->fpa->toTPA, boreFP);
    psDeproject (boreSky, boreTP, output->fpa->toSky); // find the RA,DEC coord of the focal-plane coordinate

    psProjection *newSky = psProjectionAlloc (boreSky->r, boreSky->d, output->fpa->toSky->Xs, output->fpa->toSky->Ys, output->fpa->toSky->type);

    // generate a collection of points on the sky using the old toTPA transformation and toSky projection, projected with the newSky projection
    // this is the FPA coordinate range covered by the FP: 
    psRegion *fpaRegion = pmAstromFPAExtent (output->fpa);
    float dx = (fpaRegion->x1 - fpaRegion->x0) / 50.0;
    float dy = (fpaRegion->y1 - fpaRegion->y0) / 50.0;

    psPlane fp, tp;
    psSphere sky;

    psVector *FPx = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *FPy = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *TPx = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *TPy = psVectorAllocEmpty (100, PS_TYPE_F32);

    // XXX a test: boreFP->x,y, should transform to tp.x,y = 0,0
    fp.x = boreFP->x;
    fp.y = boreFP->y;
    psPlaneTransformApply (&tp, output->fpa->toTPA, &fp);
    psDeproject (&sky, &tp, output->fpa->toSky); // find the RA,DEC coord of the focal-plane coordinate
    psProject (&tp, &sky, newSky); // find the RA,DEC coord of the focal-plane coordinate

    int Npts = 0;
    for (fp.x = fpaRegion->x0; fp.x <= fpaRegion->x1; fp.x += dx) {
	for (fp.y = fpaRegion->y0; fp.y <= fpaRegion->y1; fp.y += dy) {
	    psPlaneTransformApply (&tp, output->fpa->toTPA, &fp);
	    psDeproject (&sky, &tp, output->fpa->toSky); // find the RA,DEC coord of the focal-plane coordinate
	    psProject (&tp, &sky, newSky); // find the RA,DEC coord of the focal-plane coordinate

	    // we are fitting points in the NEW FP system to points in the NEW TP system
	    FPx->data.F32[Npts] = fp.x - boreFP->x;
	    FPy->data.F32[Npts] = fp.y - boreFP->y;
	    TPx->data.F32[Npts] = tp.x;
	    TPy->data.F32[Npts] = tp.y;
	    psVectorExtend (FPx, 100, 1);
	    psVectorExtend (FPy, 100, 1);
	    psVectorExtend (TPx, 100, 1);
	    psVectorExtend (TPy, 100, 1);
	    Npts ++;
	}
    }
    psFree (fpaRegion);

    // fit both up and down transformations to the same points
    psVectorFitPolynomial2D (output->fpa->toTPA->x, NULL, 0, TPx, NULL, FPx, FPy);
    psVectorFitPolynomial2D (output->fpa->toTPA->y, NULL, 0, TPy, NULL, FPx, FPy);
    psVectorFitPolynomial2D (output->fpa->fromTPA->x, NULL, 0, FPx, NULL, TPx, TPy);
    psVectorFitPolynomial2D (output->fpa->fromTPA->y, NULL, 0, FPy, NULL, TPx, TPy);

    psFree (output->fpa->toSky);
    output->fpa->toSky = newSky;

    if (DEBUG) psastroDumpCorners ("corners.up.bore.dat", "corners.dn.bore.dat", output->fpa);

    psFree (FPx);
    psFree (FPy);
    psFree (TPx);
    psFree (TPy);

    psFree (boreCH);
    psFree (boreFP);
    psFree (boreTP);
    psFree (boreSky);
    return true;
}
