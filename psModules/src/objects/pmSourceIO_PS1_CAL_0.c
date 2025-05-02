/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-16 22:30:50 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

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

// panstarrs-style FITS table output (header + table in 1st extension)
// this format consists of a header derived from the image header
// followed by a zero-size matrix, followed by the table data

// this output format is valid for psphot analysis of an image, and does not include calibrated
// values derived in the DVO database.
// XXX how do I generate the source tables which I need to send to PSPS?

bool pmSourcesWrite_PS1_CAL_0 (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, psMetadata *tableHeader, char *extname, psMetadata *recipe)
{
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(extname, false);

    psArray *table;
    psMetadata *row;
    int i;
    psF32 *PAR, *dPAR;
    psEllipseAxes axes;
    psF32 xPos, yPos;
    psF32 xErr, yErr;
    psF32 psfMagErr, chisq;

    pmChip *chip = readout->parent->parent;
    pmFPA  *fpa  = chip->parent;
    if (!chip->toFPA || !fpa->toTPA || !fpa->toSky) {
	psWarning ("astrometry calibration is missing, no calibrated coords");
    }

    bool status1 = false;
    bool status2 = false;
    float magOffset = NAN;
    float exptime = psMetadataLookupF32 (&status1, fpa->concepts, "FPA.EXPOSURE");
    float zeropt  = psMetadataLookupF32 (&status2, imageHeader, "ZPT_OBS");
    if (!status1 || !status2 || (exptime == 0.0)) {
	psWarning ("exposure time or measured zero point not found for a readout, no calibrated mags");
    } else {
	magOffset = zeropt + 2.5*log10(exptime);
    }

    // let's write these out in S/N order
    sources = psArraySort (sources, pmSourceSortByFlux);

    table = psArrayAllocEmpty (sources->n);

    // we write out PSF-fits for all sources, regardless of quality.  the source flags tell us the state
    for (i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];
	if (source->seq == -1) {
	    source->seq = i;
	}

        // no difference between PSF and non-PSF model
        pmModel *model = source->modelPSF;

        if (model != NULL) {
            PAR = model->params->data.F32;
            dPAR = model->dparams->data.F32;
            xPos = PAR[PM_PAR_XPOS];
            yPos = PAR[PM_PAR_YPOS];
            xErr = dPAR[PM_PAR_XPOS];
            yErr = dPAR[PM_PAR_YPOS];
	    if (isfinite(PAR[PM_PAR_SXX]) && isfinite(PAR[PM_PAR_SXX]) && isfinite(PAR[PM_PAR_SXX])) {
		axes = pmPSF_ModelToAxes (PAR, model->class->useReff);
	    } else {
		axes.major = NAN;
		axes.minor = NAN;
		axes.theta = NAN;
	    }
	    chisq = model->chisq;

	    // need to determine the PSF photometry error: source->psfMagErr is the error on the 'best' model mag.
	    psfMagErr = model->dparams->data.F32[PM_PAR_I0] / model->params->data.F32[PM_PAR_I0];
        } else {
            xPos = source->peak->xf;
            yPos = source->peak->yf;
            xErr = source->peak->dx;
            yErr = source->peak->dy;
            axes.major = NAN;
            axes.minor = NAN;
            axes.theta = NAN;
	    chisq = NAN;
	    psfMagErr = NAN;
        }

	float calMag = source->psfMag + magOffset;
        float peakMag = (source->peak->rawFlux > 0) ? -2.5*log10(source->peak->rawFlux) : NAN;
        psS16 nImageOverlap = 1;

	// generate RA,DEC
	psPlane ptCH, ptFP, ptTP;
	psSphere ptSky;

	ptCH.x = xPos;
	ptCH.y = yPos;
	if (chip->toFPA && fpa->toTPA && fpa->toSky) {
	    psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
	    psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	    psDeproject (&ptSky, &ptTP, fpa->toSky);
	} else {
	    ptSky.r = NAN;
	    ptSky.d = NAN;
	}

        row = psMetadataAlloc ();
        psMetadataAdd (row, PS_LIST_TAIL, "IPP_IDET",         PS_DATA_U32, "IPP detection identifier index",             source->seq);
        psMetadataAdd (row, PS_LIST_TAIL, "RA_PSF",           PS_DATA_F32, "PSF RA coordinate (degrees)",                ptSky.r*PS_DEG_RAD);
        psMetadataAdd (row, PS_LIST_TAIL, "DEC_PSF",          PS_DATA_F32, "PSF DEC coordinate (degrees)",               ptSky.d*PS_DEG_RAD);
	// XXX need to do the error propagation correctly..
        // psMetadataAdd (row, PS_LIST_TAIL, "RA_PSF_SIG",       PS_DATA_F32, "Sigma of PSF fit RA",                     dRA);
        // psMetadataAdd (row, PS_LIST_TAIL, "DEC_PSF_SIG",      PS_DATA_F32, "Sigma of PSF fit DEC",                    dDEC);

        psMetadataAdd (row, PS_LIST_TAIL, "X_PSF",            PS_DATA_F32, "PSF x coordinate",                           xPos);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_PSF",            PS_DATA_F32, "PSF y coordinate",                           yPos);
        psMetadataAdd (row, PS_LIST_TAIL, "X_PSF_SIG",        PS_DATA_F32, "Sigma in PSF x coordinate",                  xErr);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_PSF_SIG",        PS_DATA_F32, "Sigma in PSF y coordinate",                  yErr);
        psMetadataAdd (row, PS_LIST_TAIL, "CAL_PSF_MAG",      PS_DATA_F32, "Calibrated Magnitude from PSF Fit",          calMag);
        psMetadataAdd (row, PS_LIST_TAIL, "CAL_PSF_MAG_SIG",  PS_DATA_F32, "Calibrated Magnitude Error",                 psfMagErr);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_INST_MAG",     PS_DATA_F32, "PSF fit instrumental magnitude",             source->psfMag);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_INST_MAG_SIG", PS_DATA_F32, "Sigma of PSF instrumental magnitude",        psfMagErr);
        psMetadataAdd (row, PS_LIST_TAIL, "PEAK_FLUX_AS_MAG", PS_DATA_F32, "Peak flux expressed as magnitude",           peakMag);
        psMetadataAdd (row, PS_LIST_TAIL, "SKY",              PS_DATA_F32, "Sky level",                                  source->sky);
        psMetadataAdd (row, PS_LIST_TAIL, "SKY_SIGMA",        PS_DATA_F32, "Sigma of sky level",                         source->skyErr);

        psMetadataAdd (row, PS_LIST_TAIL, "PSF_CHISQ",        PS_DATA_F32,  "Chisq of PSF-fit",                          chisq);
        psMetadataAdd (row, PS_LIST_TAIL, "CR_NSIGMA",        PS_DATA_F32,  "Nsigma deviations from PSF to CF",          source->crNsigma);
        psMetadataAdd (row, PS_LIST_TAIL, "EXT_NSIGMA",       PS_DATA_F32,  "Nsigma deviations from PSF to EXT",         source->extNsigma);

	// EXT_NSIGMA will be NAN if: 1) contour ellipse is imaginary; 2) source is not
	// subtracted

	// CR_NSIGMA will be NAN if: 1) source is not subtracted; 2) source is on the image
	// edge; 3) any pixels in the 3x3 peak region are masked; 

        // XXX these should be major and minor, not 'x' and 'y'
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_WIDTH_X",      PS_DATA_F32, "PSF width in x coordinate",                  axes.major);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_WIDTH_Y",      PS_DATA_F32, "PSF width in y coordinate",                  axes.minor);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_THETA",        PS_DATA_F32, "PSF orientation angle",                      axes.theta);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_QF",           PS_DATA_F32, "PSF coverage/quality factor",                source->pixWeightNotBad);

        // XXX not sure how to get this : need to load Nimages with weight?
        psMetadataAdd (row, PS_LIST_TAIL, "N_FRAMES",         PS_DATA_U16, "Number of frames overlapping source center", nImageOverlap);
        psMetadataAdd (row, PS_LIST_TAIL, "FLAGS",            PS_DATA_U16, "psphot analysis flags",                      source->mode);

        psArrayAdd (table, 100, row);
        psFree (row);
    }

    if (table->n == 0) {
        psFitsWriteBlank (fits, tableHeader, extname);
        psFree (table);
        return true;
    }

    psTrace ("pmFPAfile", 5, "writing ext data %s\n", extname);
    if (!psFitsWriteTable (fits, tableHeader, table, extname)) {
        psError(PS_ERR_IO, false, "writing ext data %s\n", extname);
        psFree(table);
        return false;
    }
    psFree (table);

    return true;
}

// read in a readout from the fits file
psArray *pmSourcesRead_PS1_CAL_0 (psFits *fits, psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(header, false);

    bool status;
    psF32 *PAR, *dPAR;
    psEllipseAxes axes;

    // define PSF model type
    int modelType = pmModelClassGetType ("PS_MODEL_GAUSS");

    char *PSF_NAME = psMetadataLookupStr (&status, header, "PSF_NAME");
    if (PSF_NAME != NULL) {
        modelType = pmModelClassGetType (PSF_NAME);
    }
    assert (modelType > -1);

    // XXX need to look up the XSRCNAME entries

    // validate a single row of the table (must match SMP)

    // XXX test return values

    // XXX we have a memory problem, which is illustrated here: if I allocate the sources array
    // (line 1) before freeing the table, the data is not really freed (but not a leak?)  if I
    // allocate the sources array *after* freeing the table, the data is actually freed
    // psArray *fooSources = psArrayAllocEmpty (10000);
    // psFree (table);
    // return (fooSources);


    // We get the size of the table, and allocate the array of sources first because the table is large and
    // ephemeral --- when the table gets blown away, whatever is allocated after the table is read.  In fact,
    // it's better to read the table row by row.
    long numSources = psFitsTableSize(fits); // Number of sources in table
    psArray *sources = psArrayAlloc(numSources); // Array of sources, to return

    // convert the table to the pmSource entries
    // XXX need to chooose PSF vs EXT, based on type?
    for (int i = 0; i < numSources; i++) {
        psMetadata *row = psFitsReadTableRow(fits, i); // Table row

        pmSource *source = pmSourceAlloc ();
        pmModel *model = pmModelAlloc (modelType);
        source->modelPSF  = model;
        source->type = PM_SOURCE_TYPE_STAR;

        // NOTE: A SEGV here because "model" is NULL is probably caused by not initialising the models.
        PAR = model->params->data.F32;
        dPAR = model->dparams->data.F32;

        source->seq       = psMetadataLookupU32 (&status, row, "IPP_IDET");
        PAR[PM_PAR_XPOS]  = psMetadataLookupF32 (&status, row, "X_PSF");
        PAR[PM_PAR_YPOS]  = psMetadataLookupF32 (&status, row, "Y_PSF");
        dPAR[PM_PAR_XPOS] = psMetadataLookupF32 (&status, row, "X_PSF_SIG");
        dPAR[PM_PAR_YPOS] = psMetadataLookupF32 (&status, row, "Y_PSF_SIG");
        axes.major        = psMetadataLookupF32 (&status, row, "PSF_WIDTH_X");
        axes.minor        = psMetadataLookupF32 (&status, row, "PSF_WIDTH_Y");
        axes.theta        = psMetadataLookupF32 (&status, row, "PSF_THETA");

        PAR[PM_PAR_SKY]   = psMetadataLookupF32 (&status, row, "SKY");
        dPAR[PM_PAR_SKY]  = psMetadataLookupF32 (&status, row, "SKY_SIGMA");
        source->sky = PAR[PM_PAR_SKY];
        source->skyErr = dPAR[PM_PAR_SKY];

        // XXX use these to determine PAR[PM_PAR_I0]?
        source->psfMag    = psMetadataLookupF32 (&status, row, "PSF_INST_MAG");
        source->psfMagErr    = psMetadataLookupF32 (&status, row, "PSF_INST_MAG_SIG");
	PAR[PM_PAR_I0]    = (isfinite(source->psfMag)) ? pow(10.0, -0.4*source->psfMag) : NAN;
	dPAR[PM_PAR_I0]   = (isfinite(source->psfMag)) ? PAR[PM_PAR_I0] * source->psfMagErr : NAN;

        pmPSF_AxesToModel (PAR, axes, model->class->useReff);

        float peakMag     = psMetadataLookupF32 (&status, row, "PEAK_FLUX_AS_MAG");
        float peakFlux    = (isfinite(peakMag)) ? pow(10.0, -0.4*peakMag) : NAN;

        source->peak       = pmPeakAlloc(PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], peakFlux, PM_PEAK_LONE);
        source->peak->rawFlux = peakFlux;
        source->peak->smoothFlux = peakFlux;
        source->peak->dx   = dPAR[PM_PAR_XPOS];
        source->peak->dy   = dPAR[PM_PAR_YPOS];
        source->peak->xf   = PAR[PM_PAR_XPOS]; // more accurate position
        source->peak->yf   = PAR[PM_PAR_YPOS]; // more accurate position

        source->pixWeightNotBad = psMetadataLookupF32 (&status, row, "PSF_QF");

	// note that some older versions used PSF_PROBABILITY: this was not well defined.
        model->chisq      = psMetadataLookupF32 (&status, row, "PSF_CHISQ");
        source->crNsigma  = psMetadataLookupF32 (&status, row, "CR_NSIGMA");
        source->extNsigma = psMetadataLookupF32 (&status, row, "EXT_NSIGMA");

        source->mode      = psMetadataLookupU16 (&status, row, "FLAGS");
        assert (status);

        // XXX other values saved but not loaded?
        // psMetadataLookupS64 (&status, row, "IPP_IDET");
        // psMetadataLookupF32 (&status, row, "N_FRAMES");

        sources->data[i] = source;
        psFree(row);
    }

    return sources;
}

# define WRITE_AP_DATA 0

bool pmSourcesWrite_PS1_CAL_0_XSRC (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe)
{

    bool status;
    psArray *table;
    psMetadata *row;
    psF32 *PAR, *dPAR;
    psF32 xPos, yPos;
    psF32 xErr, yErr;

    pmChip *chip = readout->parent->parent;
    pmFPA  *fpa  = chip->parent;
    if (!chip->toFPA || !fpa->toTPA || !fpa->toSky) {
	psWarning ("astrometry calibration is missing, no calibrated coords");
    }

# if (WRITE_AP_DATA)
    bool calMags = false;
    bool status1 = false;
    bool status2 = false;
    float magOffset = NAN;
    float exptime = psMetadataLookupF32 (&status1, fpa->concepts, "FPA.EXPOSURE");
    float zeropt  = psMetadataLookupF32 (&status2, imageHeader, "ZPT_OBS");
    if (!status1 || !status2 || (exptime == 0.0)) {
	psWarning ("exposure time or measured zero point not found for a readout, no calibrated mags");
    } else {
	magOffset = zeropt + 2.5*log10(exptime);
	calMags = true;
    }
# endif

    // create a header to hold the output data
    psMetadata *outhead = psMetadataAlloc ();

    // write the links to the image header
    psMetadataAddStr (outhead, PS_LIST_TAIL, "EXTNAME", PS_META_REPLACE, "xsrc table extension", extname);

    // let's write these out in S/N order
    sources = psArraySort (sources, pmSourceSortByFlux);

    table = psArrayAllocEmpty (sources->n);

    // which extended source analyses should we perform?
    // bool doPetrosian    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");
    // bool doIsophotal    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ISOPHOTAL");
    // bool doAnnuli       = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ANNULI");
    // bool doKron         = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_KRON");

    psVector *radialBinsLower = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.LOWER");
    psVector *radialBinsUpper = psMetadataLookupPtr (&status, recipe, "RADIAL.ANNULAR.BINS.UPPER");
    psAssert (radialBinsLower->n == radialBinsUpper->n, "upper and lower bins must match");

    // we write out all sources, regardless of quality.  the source flags tell us the state
    for (int i = 0; i < sources->n; i++) {
	// skip source if it is not a ext sourc
	// XXX we have two places that extended source parameters are measured:
	// psphotExtendedSources, which measures the aperture-like parameters and (potentially) the psf-convolved extended source models,
	// psphotFitEXT, which does the simple extended source model fit (not psf-convolved)
	// should we require both?

	pmSource *source = sources->data[i];

	// skip sources without measurements
	if (source->extpars == NULL) continue;

	// we require a PSF model fit (ignore the real crud)
	pmModel *model = source->modelPSF;
	if (model == NULL) continue;

	// XXX I need to split the extended models from the extended aperture measurements
	PAR = model->params->data.F32;
	dPAR = model->dparams->data.F32;
	xPos = PAR[PM_PAR_XPOS];
	yPos = PAR[PM_PAR_YPOS];
	xErr = dPAR[PM_PAR_XPOS];
	yErr = dPAR[PM_PAR_YPOS];

	// generate RA,DEC
	psPlane ptCH, ptFP, ptTP;
	psSphere ptSky;

	ptCH.x = xPos;
	ptCH.y = yPos;
	if (chip->toFPA && fpa->toTPA && fpa->toSky) {
	    psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
	    psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
	    psDeproject (&ptSky, &ptTP, fpa->toSky);
	} else {
	    ptSky.r = NAN;
	    ptSky.d = NAN;
	}

        row = psMetadataAlloc ();

        // XXX we are not writing out the mode (flags) or the type (psf, ext, etc)
        psMetadataAdd (row, PS_LIST_TAIL, "IPP_IDET",         PS_DATA_U32, "IPP detection identifier index",             source->seq);
        psMetadataAdd (row, PS_LIST_TAIL, "RA_EXT",           PS_DATA_F32, "EXT model RA (degrees)",                     ptSky.r*PS_DEG_RAD);
        psMetadataAdd (row, PS_LIST_TAIL, "DEC_EXT",          PS_DATA_F32, "EXT model DEC (degrees)",                    ptSky.d*PS_DEG_RAD);

        psMetadataAdd (row, PS_LIST_TAIL, "X_EXT",            PS_DATA_F32, "EXT model x coordinate",                     xPos);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_EXT",            PS_DATA_F32, "EXT model y coordinate",                     yPos);
        psMetadataAdd (row, PS_LIST_TAIL, "X_EXT_SIG",        PS_DATA_F32, "Sigma in EXT x coordinate",                  xErr);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_EXT_SIG",        PS_DATA_F32, "Sigma in EXT y coordinate",                  yErr);

# if (WRITE_AP_DATA)
	// Petrosian measurements
	// XXX insert header data: petrosian ref radius, flux ratio
	if (doPetrosian) {
	    pmSourcePetrosianValues *petrosian = source->extpars->petrosian;
	    if (petrosian) {
		if (calMags) {
		    psMetadataAdd (row, PS_LIST_TAIL, "PETRO_MAG_CAL",    PS_DATA_F32, "Petrosian Magnitude (calibrated)", petrosian->mag + magOffset);
		} else {
		    psMetadataAdd (row, PS_LIST_TAIL, "PETRO_MAG_INST",   PS_DATA_F32, "Petrosian Magnitude (instrumental)", petrosian->mag);
		}
		psMetadataAdd (row, PS_LIST_TAIL, "PETRO_MAG_ERR",    PS_DATA_F32, "Petrosian Magnitude Error", petrosian->magErr);
		psMetadataAdd (row, PS_LIST_TAIL, "PETRO_RADIUS",     PS_DATA_F32, "Petrosian Radius",          petrosian->rad);
		psMetadataAdd (row, PS_LIST_TAIL, "PETRO_RADIUS_ERR", PS_DATA_F32, "Petrosian Radius Error",    petrosian->radErr);
	    } else {
		psMetadataAdd (row, PS_LIST_TAIL, "PETRO_MAG",        PS_DATA_F32, "Petrosian Magnitude",       NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "PETRO_MAG_ERR",    PS_DATA_F32, "Petrosian Magnitude Error", NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "PETRO_RADIUS",     PS_DATA_F32, "Petrosian Radius",          NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "PETRO_RADIUS_ERR", PS_DATA_F32, "Petrosian Radius Error",    NAN);
	    }
	} 

	// Kron measurements
	if (doKron) {
	    pmSourceKronValues *kron = source->extpars->kron;
	    if (kron) {
		if (calMags) {
		    psMetadataAdd (row, PS_LIST_TAIL, "KRON_MAG_CAL",  PS_DATA_F32, "Kron Magnitude",     kron->mag + magOffset);
		} else {
		    psMetadataAdd (row, PS_LIST_TAIL, "KRON_MAG_INST", PS_DATA_F32, "Kron Magnitude",     kron->mag);
		}
		psMetadataAdd (row, PS_LIST_TAIL, "KRON_MAG_ERR",    PS_DATA_F32, "Kron Magnitude Error", kron->magErr);
		psMetadataAdd (row, PS_LIST_TAIL, "KRON_RADIUS",     PS_DATA_F32, "Kron Radius",          kron->rad);
		psMetadataAdd (row, PS_LIST_TAIL, "KRON_RADIUS_ERR", PS_DATA_F32, "Kron Radius Error",    kron->radErr);
	    } else {
		psMetadataAdd (row, PS_LIST_TAIL, "KRON_MAG",        PS_DATA_F32, "Kron Magnitude",       NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "KRON_MAG_ERR",    PS_DATA_F32, "Kron Magnitude Error", NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "KRON_RADIUS",     PS_DATA_F32, "Kron Radius",          NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "KRON_RADIUS_ERR", PS_DATA_F32, "Kron Radius Error",    NAN);
	    }
	}

	// Isophot measurements
	// XXX insert header data: isophotal level
	if (doIsophotal) {
	    pmSourceIsophotalValues *isophot = source->extpars->isophot;
	    if (isophot) {
		if (calMags) {
		    psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_MAG_CAL",    PS_DATA_F32, "Isophot Magnitude (calibrated)",   isophot->mag + magOffset);
		} else {
		    psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_MAG_INST",   PS_DATA_F32, "Isophot Magnitude (uncalibrated)", isophot->mag);
		}
		psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_MAG_ERR",    PS_DATA_F32, "Isophot Magnitude Error", isophot->magErr);
		psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_RADIUS",     PS_DATA_F32, "Isophot Radius",          isophot->rad);
		psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_RADIUS_ERR", PS_DATA_F32, "Isophot Radius Error",    isophot->radErr);
	    } else {
		psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_MAG",        PS_DATA_F32, "Isophot Magnitude",       NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_MAG_ERR",    PS_DATA_F32, "Isophot Magnitude Error", NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_RADIUS",     PS_DATA_F32, "Isophot Radius",          NAN);
		psMetadataAdd (row, PS_LIST_TAIL, "ISOPHOT_RADIUS_ERR", PS_DATA_F32, "Isophot Radius Error",    NAN);
	    }
	}

	// Flux Annuli
	if (doAnnuli) {
	    pmSourceAnnuli *annuli = source->extpars->annuli;
	    if (annuli) {
		psVector *fluxVal = annuli->flux;
		psVector *fluxErr = annuli->fluxErr;
		psVector *fluxVar = annuli->fluxVar;

		for (int j = 0; j < fluxVal->n; j++) {
		    char name[32];
		    sprintf (name, "FLUX_VAL_R_%02d", j);
		    psMetadataAdd (row, PS_LIST_TAIL, name, PS_DATA_F32, "flux value in annulus", fluxVal->data.F32[j]);
		    sprintf (name, "FLUX_ERR_R_%02d", j);
		    psMetadataAdd (row, PS_LIST_TAIL, name, PS_DATA_F32, "flux error in annulus", fluxErr->data.F32[j]);
		    sprintf (name, "FLUX_VAR_R_%02d", j);
		    psMetadataAdd (row, PS_LIST_TAIL, name, PS_DATA_F32, "flux stdev in annulus", fluxVar->data.F32[j]);
		} 
	    } else {
		for (int j = 0; j < radialBinsLower->n; j++) {
		    char name[32];
		    sprintf (name, "FLUX_VAL_R_%02d", j);
		    psMetadataAdd (row, PS_LIST_TAIL, name, PS_DATA_F32, "flux value in annulus", NAN);
		    sprintf (name, "FLUX_ERR_R_%02d", j);
		    psMetadataAdd (row, PS_LIST_TAIL, name, PS_DATA_F32, "flux error in annulus", NAN);
		    sprintf (name, "FLUX_VAR_R_%02d", j);
		    psMetadataAdd (row, PS_LIST_TAIL, name, PS_DATA_F32, "flux stdev in annulus", NAN);
		} 
	    }
	}
# endif

	psArrayAdd (table, 100, row);
	psFree (row);
    }

    if (table->n == 0) {
	psFitsWriteBlank (fits, outhead, extname);
	psFree (outhead);
	psFree (table);
	return true;
    }

    psTrace ("pmFPAfile", 5, "writing ext data %s\n", extname);
    if (!psFitsWriteTable (fits, outhead, table, extname)) {
	psError(PS_ERR_IO, false, "writing ext data %s\n", extname);
	psFree (outhead);
	psFree(table);
	return false;
    }
    psFree (outhead);
    psFree (table);

    return true;
}

bool pmSourcesWrite_PS1_CAL_0_XFIT (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname)
{

    psArray *table;
    psMetadata *row;
    psF32 *PAR, *dPAR;
    psEllipseAxes axes;
    psF32 xPos, yPos;
    psF32 xErr, yErr;
    char name[64];

    pmChip *chip = readout->parent->parent;
    pmFPA  *fpa  = chip->parent;
    if (!chip->toFPA || !fpa->toTPA || !fpa->toSky) {
	psWarning ("astrometry calibration is missing, no calibrated coords");
    }

    bool status1 = false;
    bool status2 = false;
    float magOffset = NAN;
    float exptime = psMetadataLookupF32 (&status1, fpa->concepts, "FPA.EXPOSURE");
    float zeropt  = psMetadataLookupF32 (&status2, imageHeader, "ZPT_OBS");
    if (!status1 || !status2 || (exptime == 0.0)) {
	psWarning ("exposure time or measured zero point not found for a readout, no calibrated mags");
    } else {
	magOffset = zeropt + 2.5*log10(exptime);
    }

    // create a header to hold the output data
    psMetadata *outhead = psMetadataAlloc ();

    // write the links to the image header
    psMetadataAddStr (outhead, PS_LIST_TAIL, "EXTNAME", PS_META_REPLACE, "xsrc table extension", extname);

    // let's write these out in S/N order
    sources = psArraySort (sources, pmSourceSortByFlux);

    // we are writing one row per model; we need to write out same number of columns for each row: find the max Nparams
    int nParamMax = 0;
    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (source->modelFits == NULL) continue;
	for (int j = 0; j < source->modelFits->n; j++) {
	    pmModel *model = source->modelFits->data[j];
	    assert (model);
	    nParamMax = PS_MAX (nParamMax, model->params->n);
	}
    }

    table = psArrayAllocEmpty (sources->n);

    // we write out all sources, regardless of quality.  the source flags tell us the state
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	// XXX if no model fits are saved, write out modelEXT?
	if (source->modelFits == NULL) continue;

	// We have multiple sources : need to flag the one used to subtract the light (the 'best' model)
	for (int j = 0; j < source->modelFits->n; j++) {

	    // choose the convolved EXT model, if available, otherwise the simple one
	    pmModel *model = source->modelFits->data[j];
	    assert (model);

	    // skip models which were not actually fitted
	    if (model->flags & PM_MODEL_STATUS_BADARGS) continue;

	    PAR = model->params->data.F32;
	    dPAR = model->dparams->data.F32;
	    xPos = PAR[PM_PAR_XPOS];
	    yPos = PAR[PM_PAR_YPOS];
	    xErr = dPAR[PM_PAR_XPOS];
	    yErr = dPAR[PM_PAR_YPOS];

	    axes = pmPSF_ModelToAxes (PAR, model->class->useReff);

	    // generate RA,DEC
	    psPlane ptCH, ptFP, ptTP;
	    psSphere ptSky;

	    ptCH.x = xPos;
	    ptCH.y = yPos;
	    if (chip->toFPA && fpa->toTPA && fpa->toSky) {
		psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
		psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
		psDeproject (&ptSky, &ptTP, fpa->toSky);
	    } else {
		ptSky.r = NAN;
		ptSky.d = NAN;
	    }

	    row = psMetadataAlloc ();

	    // XXX we are not writing out the mode (flags) or the type (psf, ext, etc)
	    psMetadataAddU32 (row, PS_LIST_TAIL, "IPP_IDET",         0, "IPP detection identifier index",             source->seq);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "RA_EXT",           0, "EXT model RA (degrees)",                     ptSky.r);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "DEC_EXT",          0, "EXT model DEC (degrees)",                    ptSky.d);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "X_EXT",            0, "EXT model x coordinate",                     xPos);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "Y_EXT",            0, "EXT model y coordinate",                     yPos);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "X_EXT_SIG",        0, "Sigma in EXT x coordinate",                  xErr);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "Y_EXT_SIG",        0, "Sigma in EXT y coordinate",                  yErr);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "EXT_CAL_MAG",      0, "EXT fit calibrated magnitude",               model->mag + magOffset);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "EXT_INST_MAG",     0, "EXT fit instrumental magnitude",             model->mag);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "EXT_INST_MAG_SIG", 0, "Sigma of PSF instrumental magnitude",        model->magErr);

	    psMetadataAddF32 (row, PS_LIST_TAIL, "NPARAMS",          0, "number of model parameters",                 model->params->n);
	    psMetadataAddStr (row, PS_LIST_TAIL, "MODEL_TYPE",       0, "name of model",                              pmModelClassGetName (model->type));

	    // XXX these should be major and minor, not 'x' and 'y'
	    psMetadataAddF32 (row, PS_LIST_TAIL, "EXT_WIDTH_MAJ",    0, "EXT width in x coordinate",                  axes.major);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "EXT_WIDTH_MIN",    0, "EXT width in y coordinate",                  axes.minor);
	    psMetadataAddF32 (row, PS_LIST_TAIL, "EXT_THETA",        0, "EXT orientation angle",                      axes.theta);

	    // write out the other generic parameters
	    for (int k = 0; k < nParamMax; k++) {
		if (k == PM_PAR_I0) continue;
		if (k == PM_PAR_SKY) continue;
		if (k == PM_PAR_XPOS) continue;
		if (k == PM_PAR_YPOS) continue;
		if (k == PM_PAR_SXX) continue;
		if (k == PM_PAR_SXY) continue;
		if (k == PM_PAR_SYY) continue;

		snprintf (name, 64, "EXT_PAR_%02d", k);

		if (k < model->params->n) {
		    psMetadataAdd (row, PS_LIST_TAIL, name, PS_DATA_F32, "", model->params->data.F32[k]);
		} else {
		    psMetadataAddF32 (row, PS_LIST_TAIL, name, PS_DATA_F32, "", NAN);
		}
	    }

	    // XXX other parameters which may be set.
	    // XXX flag / value to define the model
	    // XXX write out the model type, fit status flags

	    psArrayAdd (table, 100, row);
	    psFree (row);
	}
    }

    if (table->n == 0) {
	psFitsWriteBlank (fits, outhead, extname);
	psFree (outhead);
	psFree (table);
	return true;
    }

    psTrace ("pmFPAfile", 5, "writing ext data %s\n", extname);
    if (!psFitsWriteTable (fits, outhead, table, extname)) {
	psError(PS_ERR_IO, false, "writing ext data %s\n", extname);
	psFree (outhead);
	psFree(table);
	return false;
    }
    psFree (outhead);
    psFree (table);
    return true;
}

bool pmSourcesWrite_PS1_CAL_0_XRAD (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe)
{
    return true;
}

bool pmSourcesWrite_PS1_CAL_0_XGAL (psFits *fits, pmReadout *readout, psArray *sources, char *extname, psMetadata *recipe)
{
    return true;
}
