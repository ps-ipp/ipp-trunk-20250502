# include "ppSim.h"
static char *defaultModel = "PS_MODEL_QGAUSS";

bool ppSimSetPSF (pmChip *chip, pmConfig *config) {

    bool status, mdok;
    pmPSF *psf = NULL;
    pmTrend2D *param = NULL;

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSIM_RECIPE); // Recipe

    // the pmPSF IO functions stores the PSF on the chip->analysis
    psf = psMetadataLookupPtr (&status, chip->analysis, "PSPHOT.PSF");
    if (psf) {
        return true;
    }

    // no supplied PSF, build one using supplied value for seeing seeing is already corrected
    // for the pixel scale, and is converted from FWHM to SIGMA (this is done in
    // ppSimArguments)
    float seeing   = psMetadataLookupF32(&status, recipe, "SEEING"); // Seeing SIGMA (pixels)
    float aRatio   = psMetadataLookupF32(&status, recipe, "PSF.ARATIO"); // Seeing SIGMA (pixels)
    float theta    = psMetadataLookupF32(&status, recipe, "PSF.THETA"); // Seeing SIGMA (pixels)

    float seeingMax = psMetadataLookupF32(&status, recipe, "SEEING.MAX"); // Seeing SIGMA (pixels)
    bool seeingRamp = psMetadataLookupBool(&status, recipe, "SEEING.RAMP"); // Seeing SIGMA (pixels)

    char *psfModelName = psMetadataLookupStr(&status, recipe, "PSF.MODEL"); // Name of PSF model
    if (psfModelName == NULL) {
        psfModelName = defaultModel;
    }

    // structure to store user options defining the psf
    pmPSFOptions *options = pmPSFOptionsAlloc ();
    options->type = pmModelClassGetType (psfModelName);
    if (options->type == -1) {
        psError (PS_ERR_UNKNOWN, false, "invalid model name");
        return false;
    }

    // XXX this is messed up:  CHIP.XSIZE and CHIP.YSIZE are not seta
    int xSize = psMetadataLookupS32(NULL, chip->concepts, "CHIP.XSIZE");
    int ySize = psMetadataLookupS32(NULL, chip->concepts, "CHIP.YSIZE");

    if (seeingRamp) {
	psEllipseAxes axes;
	psEllipsePol pol;
	psEllipsePol polMax;

	// try spatial variation
	options->psfTrendMode = PM_TREND_POLY_ORD;
	options->psfTrendNx = 1;
	options->psfTrendNy = 1;
	options->psfFieldNx = xSize;
	options->psfFieldNy = ySize;

	// generate the psf
	psf = pmPSFAlloc (options);

	// supply the semi-major axis (these are SIGMA values in PIXELS)
	axes.major = seeing;
	axes.minor = aRatio * seeing;
	axes.theta = theta * PS_RAD_DEG;

	pol = psEllipseAxesToPol (axes);
    
	axes.major = seeingMax;
	axes.minor = aRatio * seeingMax;
	polMax = psEllipseAxesToPol (axes);

	param = psf->params->data[PM_PAR_E0];
	param->poly->coeff[0][0] = pol.e0;
	param->poly->coeff[1][0] = (polMax.e0 - pol.e0) / xSize;
	param->poly->coeff[0][1] = (polMax.e0 - pol.e0) / ySize;

	param = psf->params->data[PM_PAR_E1];
	param->poly->coeff[0][0] = pol.e1;
	param->poly->coeff[1][0] = 0.0;
	param->poly->coeff[0][1] = 0.0;

	param = psf->params->data[PM_PAR_E2];
	param->poly->coeff[0][0] = pol.e2;
	param->poly->coeff[1][0] = 0.0;
	param->poly->coeff[0][1] = 0.0;

	if (!strcasecmp (psfModelName, "PS_MODEL_QGAUSS")) {
	    param = psf->params->data[PM_PAR_7];
	    param->poly->coeff[0][0] = 1.0;
	    param->poly->coeff[1][0] = 0.0;
	    param->poly->coeff[0][1] = 0.0;
	}

	if (!strcasecmp (psfModelName, "PS_MODEL_RGAUSS")) {
	    param = psf->params->data[PM_PAR_7];
	    param->poly->coeff[0][0] = 1.0;
	    param->poly->coeff[1][0] = 0.0;
	    param->poly->coeff[0][1] = 0.0;
	}

    } else {
	psEllipseAxes axes;
	psEllipsePol pol;

	// no spatial variation
	options->psfTrendMode = PM_TREND_POLY_ORD;
	options->psfTrendNx = 0;
	options->psfTrendNy = 0;
	options->psfFieldNx = xSize;
	options->psfFieldNy = ySize;

	// generate the psf
	psf = pmPSFAlloc (options);

	// supply the semi-major axis (these are SIGMA values in PIXELS)
	axes.major = seeing;
	axes.minor = aRatio * seeing;
	axes.theta = theta * PS_RAD_DEG;

	pol = psEllipseAxesToPol (axes);
	psEllipseAxes testaxes = psEllipsePolToAxes (pol, 0.1);
	fprintf (stderr, "psf in axes: %f x %f @ %f\n", axes.major, axes.minor, axes.theta*PS_DEG_RAD);
	fprintf (stderr, "psf ot axes: %f x %f @ %f\n", testaxes.major, testaxes.minor, testaxes.theta*PS_DEG_RAD);
    
	param = psf->params->data[PM_PAR_E0];
	param->poly->coeff[0][0] = pol.e0;

	param = psf->params->data[PM_PAR_E1];
	param->poly->coeff[0][0] = pol.e1;

	param = psf->params->data[PM_PAR_E2];
	param->poly->coeff[0][0] = pol.e2;

	if (!strcasecmp (psfModelName, "PS_MODEL_QGAUSS")) {
	    param = psf->params->data[PM_PAR_7];
	    param->poly->coeff[0][0] = 1.0;
	}

	if (!strcasecmp (psfModelName, "PS_MODEL_RGAUSS")) {
	    param = psf->params->data[PM_PAR_7];
	    param->poly->coeff[0][0] = 1.0;
	}
    }

    psMetadataAdd (chip->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_DATA_UNKNOWN,  "psphot psf", psf);
    psFree(psf);                        // Drop reference

    return true;
}
