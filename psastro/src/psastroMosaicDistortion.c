/** @file psastroMosaicDistortion.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define DEBUG 0

bool psastroMosaicDistortion (pmFPA *fpa, psMetadata *recipe, int pass) {

    // fit a linear transformation from reference TP to observed FP coords
    psPlaneTransform *TPtoFP = psastroMosaicFitRotAndScale (fpa);
    if (!TPtoFP) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to fit a linear TP correction\n");
        return false;
    }

    if (DEBUG && (pass == 0)) psastroDumpCorners ("corners.up.v1.dat", "corners.dn.v1.dat", fpa);

    // Correct the current reference star TP coordinates to the nearly-FP
    // system.  We note two points here: 1) the corrected TP coordinates are
    // NOT consistent with the sky coordinates, 2) the remaining differnce
    // between the new TP reference coords and the observerd FP coordinates is
    // only distortion
    if (!psastroMosaicApplyRotAndScale (fpa, TPtoFP)) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to apply the linear TP correction\n");
	psFree (TPtoFP);
        return false;
    }

    if (DEBUG && (pass == 0)) psastroDumpCorners ("corners.up.v2.dat", "corners.dn.v2.dat", fpa);

    if (!psastroMosaicDistortionFromGradients (fpa, recipe)) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to fit the distortion field\n");
	psFree (TPtoFP);
        return false;
    }

    if (DEBUG && (pass == 0)) psastroDumpCorners ("corners.up.v3.dat", "corners.dn.v3.dat", fpa);

    if (!psastroMosaicCorrectDistortion (fpa, TPtoFP)) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to correct the distortion for the linear fit\n");
	psFree (TPtoFP);
        return false;
    }
	
    if (DEBUG && (pass == 0)) psastroDumpCorners ("corners.up.v4.dat", "corners.dn.v4.dat", fpa);

    if (!psastroMosaicSetAstrom (fpa)) {
	psError(PSASTRO_ERR_UNKNOWN, false, "failed to apply mosaic distortion terms\n");
	psFree (TPtoFP);
        return false;
    }

    if (DEBUG && (pass == 0)) psastroDumpCorners ("corners.up.v5.dat", "corners.dn.v5.dat", fpa);

    psFree (TPtoFP);
    return true;
}

