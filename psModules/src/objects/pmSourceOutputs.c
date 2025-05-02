#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"

#include "pmSourceIO.h"
#include "pmSourceOutputs.h"

bool pmSourceOutputsCommonValues (float *magOffset, float *zeroptErr, float *fwhmMajor, float *fwhmMinor, pmReadout *readout, psMetadata *header) {

    pmFPA  *fpa  = readout->parent->parent->parent;

    bool status1 = false;
    bool status2 = false;
    float exptime   = psMetadataLookupF32 (&status1, fpa->concepts, "FPA.EXPOSURE");
    float zeropt    = psMetadataLookupF32(&status2, fpa->concepts, "FPA.ZP");
    if (!isfinite(zeropt)) {
        zeropt    = psMetadataLookupF32 (&status2, header, "ZPT_OBS");
    }
    if (status1 && status2 && (exptime > 0.0)) {
        *magOffset = zeropt + 2.5*log10(exptime);
    }
    *zeroptErr = psMetadataLookupF32 (&status2, header, "ZPT_ERR");

    // we need a measure of the image quality (FWHM) for this image, in order to get the positional errors
    *fwhmMajor = psMetadataLookupF32(&status1, readout->analysis, "FWHM_MAJ");
    if (status1) goto got_major;

    *fwhmMajor = psMetadataLookupF32(&status1, header, "FWHM_MAJ");
    if (status1) goto got_major;

    *fwhmMajor = psMetadataLookupF32(&status1, readout->analysis, "IQ_FW1");
    if (status1) goto got_major;

    *fwhmMajor = psMetadataLookupF32(&status1, header, "IQ_FW1");
    if (status1) goto got_major;

    *fwhmMajor = 5.0; // XXX just a guess!

    got_major:

    *fwhmMinor = psMetadataLookupF32(&status1, readout->analysis, "FWHM_MIN");
    if (status1) goto got_minor;
    *fwhmMinor = psMetadataLookupF32(&status1, header, "FWHM_MIN");
    if (status1) goto got_minor;

    *fwhmMinor = psMetadataLookupF32(&status1, readout->analysis, "IQ_FW2");
    if (status1) goto got_minor;
    *fwhmMinor = psMetadataLookupF32(&status1, header, "IQ_FW2");
    if (status1) goto got_minor;

    *fwhmMinor = 5.0; // XXX just a guess!

    got_minor:
    return true;

}

// what is the correct postion?
// * if we have a PSF model fit, use PM_PAR_XPOS,YPOS for X_PSF,Y_PSF 
// * if we do not have a model:
// ** if we have moments:
// *** if the star is saturated, use the moments
// *** if the moments and peak agree to < DR, use the moments
// *** otherwise, use the peak

bool pmSourceOutputsSetValues (pmSourceOutputs *outputs, pmSource *source, pmChip *chip, float fwhmMajor, float fwhmMinor, float magOffset) {

    psF32 *PAR, *dPAR;
    psEllipseAxes axes;

    // no difference between PSF and non-PSF model
    pmModel *model = source->modelPSF;

    if (model != NULL) {
	PAR = model->params->data.F32;
	dPAR = model->dparams->data.F32;
	outputs->xPos = PAR[PM_PAR_XPOS];
	outputs->yPos = PAR[PM_PAR_YPOS];
	if ((source->mode & PM_SOURCE_MODE_NONLINEAR_FIT) && !(source->mode & PM_SOURCE_MODE_EXTMODEL)) {
	    // we only do non-linear PSF fits for non-extended objects
	    outputs->xErr = dPAR[PM_PAR_XPOS];
	    outputs->yErr = dPAR[PM_PAR_YPOS];
	} else {
	    outputs->xErr = fwhmMajor * source->psfMagErr / 2.35;
	    outputs->yErr = fwhmMinor * source->psfMagErr / 2.35;
	}
	if (isfinite(PAR[PM_PAR_SXX]) && isfinite(PAR[PM_PAR_SXY]) && isfinite(PAR[PM_PAR_SYY])) {
	    axes = pmPSF_ModelToAxes (PAR, model->class->useReff);
	    outputs->psfMajor = axes.major;
	    outputs->psfMinor = axes.minor;
	    outputs->psfTheta = axes.theta*PS_DEG_RAD;

	    // some models (PS1_V1, QGAUSS) have an extra 'core' parameter
	    outputs->psfCore = NAN;
	    if (model->type == pmModelClassGetType ("PS_MODEL_PS1_V1")) {
		outputs->psfCore = PAR[PM_PAR_7];
	    }
	    if (model->type == pmModelClassGetType ("PS_MODEL_QGAUSS")) {
		outputs->psfCore = PAR[PM_PAR_7];
	    }
	    if (model->type == pmModelClassGetType ("PS_MODEL_HSC_V1")) {
		outputs->psfCore = PAR[PM_PAR_7];
	    }

	    outputs->psfMajorFWHM = model->class->modelSetFWHM(model->params, axes.major);
	    outputs->psfMinorFWHM = model->class->modelSetFWHM(model->params, axes.minor);
	} else {
	    outputs->psfMajor = NAN;
	    outputs->psfMinor = NAN;
	    outputs->psfTheta = NAN;
	    outputs->psfCore = NAN;
	}
	outputs->chisq = model->chisq;
	outputs->nDOF = model->nDOF;
	outputs->nPix = model->nPix;
    } else {
	bool useMoments = pmSourcePositionUseMoments(source);

	if (useMoments) {
	    outputs->xPos = source->moments->Mx;
	    outputs->yPos = source->moments->My;
	    outputs->xErr = fwhmMajor * source->psfMagErr / 2.35;
	    outputs->yErr = fwhmMinor * source->psfMagErr / 2.35;
	} else {
	    outputs->xPos = source->peak->xf;
	    outputs->yPos = source->peak->yf;
	    outputs->xErr = source->peak->dx;
	    outputs->yErr = source->peak->dy;
	}
	outputs->psfMajor = NAN;
	outputs->psfMinor = NAN;
	outputs->psfTheta = NAN;
	outputs->psfCore = NAN;
	outputs->chisq = NAN;
	outputs->nDOF = 0;
	outputs->nPix = 0;
    }

    outputs->calMag = isfinite(magOffset) ? source->psfMag + magOffset : NAN;
    outputs->peakMag = (source->peak->rawFlux > 0) ? -2.5*log10(source->peak->rawFlux) : NAN;

    psSphere ptSky = {0.0, 0.0, 0.0, 0.0};
    float posAngle = 0.0;
    float pltScale = 0.0;
    pmSourceLocalAstrometry (&ptSky, &posAngle, &pltScale, chip, outputs->xPos, outputs->yPos);

    outputs->posAngle = posAngle*PS_DEG_RAD;	   
    outputs->pltScale = pltScale*PS_DEG_RAD*3600.0;

    outputs->ra = ptSky.r*PS_DEG_RAD;
    outputs->dec = ptSky.d*PS_DEG_RAD;

    return true;
}

bool pmSourceOutputsSetMoments (pmSourceOutputsMoments *moments, pmSource *source) {

    // distinguish moments measure from window vs S/N > XX ??
    moments->Mxx = source->moments ? source->moments->Mxx : NAN;
    moments->Mxy = source->moments ? source->moments->Mxy : NAN;
    moments->Myy = source->moments ? source->moments->Myy : NAN;
    moments->M_c3 = source->moments ? 1.0*source->moments->Mxxx - 3.0*source->moments->Mxyy : NAN;
    moments->M_s3 = source->moments ? 3.0*source->moments->Mxxy - 1.0*source->moments->Myyy : NAN;
    moments->M_c4 = source->moments ? 1.0*source->moments->Mxxxx - 6.0*source->moments->Mxxyy + 1.0*source->moments->Myyyy : NAN;
    moments->M_s4 = source->moments ? 4.0*source->moments->Mxxxy - 4.0*source->moments->Mxyyy : NAN;
    moments->Mrf  = source->moments ? source->moments->Mrf : NAN;
    moments->Mrh  = source->moments ? source->moments->Mrh : NAN;
    moments->Krf  = source->moments ? source->moments->KronFlux : NAN;
    moments->dKrf = source->moments ? source->moments->KronFluxErr : NAN;
    moments->Kinner = source->moments ? source->moments->KronFinner : NAN;
    moments->Kouter = source->moments ? source->moments->KronFouter : NAN;
    moments->KronCore    = source->moments ? source->moments->KronCore : NAN;
    moments->KronCoreErr = source->moments ? source->moments->KronCoreErr : NAN;
    moments->KronPSF    = source->moments ? source->moments->KronFluxPSF : NAN;
    moments->KronPSFErr = source->moments ? source->moments->KronFluxPSFErr : NAN;

    return true;
}

bool pmSourceLocalAstrometry (psSphere *ptSky, float *posAngle, float *pltScale, pmChip *chip, float xPos, float yPos) {

    pmFPA *fpa = chip->parent;

    if (!chip->toFPA) goto escape;
    if (!fpa->toTPA) goto escape;
    if (!fpa->toSky) goto escape;

    // generate RA,DEC
    psPlane ptCH, ptFP, ptTP_o, ptTP_x, ptTP_y;

    // calculate the astrometry for the coordinate of interest
    ptCH.x = xPos;
    ptCH.y = yPos;
    psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
    psPlaneTransformApply (&ptTP_o, fpa->toTPA, &ptFP);
    psDeproject (ptSky, &ptTP_o, fpa->toSky);

    // calculate the astrometry for the coordinate + 1pix in X
    ptCH.x = xPos + 1.0;
    ptCH.y = yPos;
    psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
    psPlaneTransformApply (&ptTP_x, fpa->toTPA, &ptFP);

    // calculate the astrometry for the coordinate + 1pix in Y
    ptCH.x = xPos;
    ptCH.y = yPos + 1.0;
    psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
    psPlaneTransformApply (&ptTP_y, fpa->toTPA, &ptFP);

    // the resulting Tangent Plane coordinates are in TP pixels; convert to local Tangent Plane
    // degrees

    float dTPx_dCHx = fpa->toSky->Xs * (ptTP_x.x - ptTP_o.x);
    float dTPy_dCHx = fpa->toSky->Ys * (ptTP_x.y - ptTP_o.y);

    float dTPx_dCHy = fpa->toSky->Xs * (ptTP_y.x - ptTP_o.x);
    float dTPy_dCHy = fpa->toSky->Ys * (ptTP_y.y - ptTP_o.y);

    float pltScale_x = hypot(dTPx_dCHx, dTPy_dCHx);
    float pltScale_y = hypot(dTPx_dCHy, dTPy_dCHy);
    *pltScale = 0.5*(pltScale_x + pltScale_y);

    float posAngle_x, posAngle_y;
    float crossProduct = dTPx_dCHx * dTPy_dCHy - dTPx_dCHy * dTPy_dCHx;
    if  (crossProduct > 0.) {
      *pltScale *= -1.0;
      posAngle_x = atan2 (dTPy_dCHx, dTPx_dCHx);
      posAngle_y = atan2 (dTPy_dCHy, dTPx_dCHy) - M_PI_2;
    } else {
      posAngle_x = atan2 (dTPy_dCHx, -dTPx_dCHx);
      posAngle_y = atan2 (dTPy_dCHy,  dTPx_dCHy) - M_PI_2;
    }

    // with errors, these may end up on opposite sides of the M_PI boundary.  
    if (posAngle_x - posAngle_y > M_PI) {
      posAngle_y += 2.0 * M_PI;
    }
    if (posAngle_y - posAngle_x > M_PI) {
      posAngle_x += 2.0 * M_PI;
    }
    *posAngle = 0.5*(posAngle_x + posAngle_y);

    return true;

escape:
    // no astrometry calibration, give up
    ptSky->r = NAN;
    ptSky->d = NAN;
    *posAngle = NAN;
    *pltScale = NAN;

    return false;
}

