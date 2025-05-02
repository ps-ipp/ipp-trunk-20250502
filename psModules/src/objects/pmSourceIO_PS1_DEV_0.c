/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
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

// panstars-style FITS table output (header + table in 1st extension)
// this format consists of a header derived from the image header
// followed by a zero-size matrix, followed by the table data

// this output format is valid for psphot analysis of an image, and does not include calibrated
// values derived in the DVO database.
// XXX how do I generate the source tables which I need to send to PSPS?
// XXX: input parameter imageHeader is never used.
bool pmSourcesWrite_PS1_DEV_0 (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, psMetadata *tableHeader, char *extname, psMetadata *recipe)
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

    table = psArrayAllocEmpty (sources->n);

    // we write out all sources, regardless of quality.  the source flags tell us the state
    for (i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

        // no difference between PSF and non-PSF model
        pmModel *model = pmSourceGetModel(NULL, source);

        if (model != NULL) {
            PAR = model->params->data.F32;
            dPAR = model->dparams->data.F32;
            xPos = PAR[PM_PAR_XPOS];
            yPos = PAR[PM_PAR_YPOS];
            xErr = dPAR[PM_PAR_XPOS];
            yErr = dPAR[PM_PAR_YPOS];

            axes = pmPSF_ModelToAxes (PAR, model->class->useReff);
        } else {
            // XXX: This code seg faults if source->peak is NULL.
            xPos = source->peak->xf;
            yPos = source->peak->yf;
            xErr = 0.0; // XXX a better choice, please
            yErr = 0.0; // XXX a better choice, please
            axes.major = 0.0;
            axes.minor = 0.0;
            axes.theta = 0.0;
        }

        float peakMag = (source->peak->rawFlux > 0) ? -2.5*log10(source->peak->rawFlux) : NAN;
        psS16 nImageOverlap = 1;
        psS32 ID = 0; // XXX need to figure out how to generate this

        row = psMetadataAlloc ();
        // XXX we are not writing out the mode (flags) or the type (psf, ext, etc)
        psMetadataAdd (row, PS_LIST_TAIL, "IPP_IDET",         PS_DATA_U32, "IPP detection identifier index",             ID);
        psMetadataAdd (row, PS_LIST_TAIL, "X_PSF",            PS_DATA_F32, "PSF x coordinate",                           xPos);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_PSF",            PS_DATA_F32, "PSF y coordinate",                           yPos);
        psMetadataAdd (row, PS_LIST_TAIL, "X_PSF_SIG",        PS_DATA_F32, "Sigma in PSF x coordinate",                  xErr);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_PSF_SIG",        PS_DATA_F32, "Sigma in PSF y coordinate",                  yErr);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_INST_MAG",     PS_DATA_F32, "PSF fit instrumental magnitude",             PS_MIN (99.0, source->psfMag));
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_INST_MAG_SIG", PS_DATA_F32, "Sigma of PSF instrumental magnitude",        PS_MIN (99.0, source->psfMagErr));
        psMetadataAdd (row, PS_LIST_TAIL, "PEAK_FLUX_AS_MAG", PS_DATA_F32, "Peak flux expressed as magnitude",           PS_MIN (99.0, peakMag));
        psMetadataAdd (row, PS_LIST_TAIL, "SKY",              PS_DATA_F32, "Sky level",                                  source->sky);
        psMetadataAdd (row, PS_LIST_TAIL, "SKY_SIGMA",        PS_DATA_F32, "Sigma of sky level",                         source->skyErr);
        // XXX this is called STAR_GALAXY_SEP in the ICD; PSF_PROB is better
        // XXX need to set this value in psphotEvalPSF
        // XXX can I use the 2d polynomial peak fit to constrain this for the low-sn sources?
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_PROBABILITY",  PS_DATA_F32,  "Probability of PSF-ness",                   NAN);
        // XXX these should be major and minor, not 'x' and 'y'
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_WIDTH_X",      PS_DATA_F32, "PSF width in x coordinate",                  axes.major);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_WIDTH_Y",      PS_DATA_F32, "PSF width in y coordinate",                  axes.minor);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_THETA",        PS_DATA_F32, "PSF orientation angle",                      axes.theta);
        psMetadataAdd (row, PS_LIST_TAIL, "PSF_QF",           PS_DATA_F32, "PSF coverage/quality factor",                source->pixWeightNotBad);
        // XXX not sure how to get this : need to load Nimages with weight
        psMetadataAdd (row, PS_LIST_TAIL, "N_FRAMES",         PS_DATA_U16, "Number of frames overlapping source center", nImageOverlap);
        psMetadataAdd (row, PS_LIST_TAIL, "DUMMY",            PS_DATA_U16, "padding", 0);

        // XXX these calibrated values are not supplied by psphot analysis
        // psMetadataAdd (row, PS_LIST_TAIL, "RA_PSF",           PS_DATA_F64, "RA from PSF fit",                         RA);
        // psMetadataAdd (row, PS_LIST_TAIL, "DEC_PSF",          PS_DATA_F64, "DEC from PSF fit",                        DEC);
        // psMetadataAdd (row, PS_LIST_TAIL, "RA_PSF_SIG",       PS_DATA_F32, "Sigma of PSF fit RA",                     dRA);
        // psMetadataAdd (row, PS_LIST_TAIL, "DEC_PSF_SIG",      PS_DATA_F32, "Sigma of PSF fit DEC",                    dDEC);
        // psMetadataAdd (row, PS_LIST_TAIL, "CAL_PSF_MAG",      PS_DATA_F32, "Calibrated magnitude",                    calMag);
        // psMetadataAdd (row, PS_LIST_TAIL, "CAL_PSF_MAG_sIG",  PS_DATA_F32, "Sigma of calibrated magnitude",           calMagErr);

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
psArray *pmSourcesRead_PS1_DEV_0 (psFits *fits, psMetadata *header)
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

    psArray *table = psFitsReadTable (fits);
    // validate a single row of the table (must match SMP)
    // XXX: The following seg-faults if table returns NULL, so I added the ASSERT
    PS_ASSERT_PTR_NON_NULL(table, NULL);
    psArray *sources = psArrayAlloc (table->n);

    // convert the table to the pmSource entries
    // XXX need to chooose PSF vs EXT, based on type?
    for (int i = 0; i < table->n; i++) {
        pmSource *source = pmSourceAlloc ();
        pmModel *model = pmModelAlloc (modelType);
        source->modelPSF  = model;
        source->type = PM_SOURCE_TYPE_STAR;

        // NOTE: A SEGV here because "model" is NULL is probably caused by not initialising the models.
        PAR = model->params->data.F32;
        dPAR = model->dparams->data.F32;

        psMetadata *row = table->data[i];

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

        pmPSF_AxesToModel (PAR, axes, model->class->useReff);

        float peakMag = psMetadataLookupF32 (&status, row, "PEAK_FLUX_AS_MAG");
        float peakFlux = (isfinite(peakMag)) ? pow(10.0, -0.4*peakMag) : NAN;

        source->peak = pmPeakAlloc(PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], peakFlux, PM_PEAK_LONE);
        source->peak->rawFlux = peakFlux;
        source->peak->smoothFlux = peakFlux;
        source->peak->dx   = dPAR[PM_PAR_XPOS];
        source->peak->dy   = dPAR[PM_PAR_YPOS];
        source->peak->xf   = PAR[PM_PAR_XPOS]; // more accurate position
        source->peak->yf   = PAR[PM_PAR_YPOS]; // more accurate position

        source->pixWeightNotBad = psMetadataLookupF32 (&status, row, "PSF_QF");

        // XXX other values saved but not loaded?
        // psMetadataLookupS64 (&status, row, "IPP_IDET");
        // psMetadataLookupF32 (&status, row, "PSF_PROBABILITY");
        // psMetadataLookupF32 (&status, row, "N_FRAMES");

        sources->data[i] = source;
    }
    psFree (table);
    return (sources);
}

bool pmSourcesWrite_PS1_DEV_0_XSRC (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe)
{
    return true;
}

bool pmSourcesWrite_PS1_DEV_0_XFIT(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname)
{
    return true;
}

bool pmSourcesWrite_PS1_DEV_0_XRAD(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe)
{
    return true;
}

bool pmSourcesWrite_PS1_DEV_0_XGAL(psFits *fits, pmReadout *readout, psArray *sources, char *extname, psMetadata *recipe)
{
    return true;
}
