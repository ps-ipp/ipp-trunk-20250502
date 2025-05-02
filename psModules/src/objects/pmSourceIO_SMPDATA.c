/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
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

// elixir-style FITS table output (header + table in 1st extension)
// this format consists of a header derived from the image header
// followed by a zero-size matrix, followed by the table data
// XXX: input parameter imageHeader is never used
bool pmSourcesWrite_SMPDATA (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, psMetadata *tableHeader, char *extname, psMetadata *recipe)
{
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(extname, false);

    psArray *table;
    psMetadata *row;
    int i;
    psF32 *PAR;
    bool status;
    psEllipseAxes axes;
    psF32 xPos, yPos;

    // find config information for output header
    float ZERO_POINT = psMetadataLookupF32 (&status, imageHeader, "ZERO_PT");
    if (!status)
        ZERO_POINT = 25.0;

    float lsky = 0;
    int type = 0;

    table = psArrayAllocEmpty (sources->n);

    for (i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

	// no difference between PSF and non-PSF model
        pmModel *model = pmSourceGetModel (NULL, source);
        if (model != NULL) {
	    PAR = model->params->data.F32;
	    xPos = PAR[PM_PAR_XPOS];
	    yPos = PAR[PM_PAR_YPOS];

	    type = pmSourceGetDophotType (source);
	    lsky = (source->sky < 1.0) ? 0.0 : log10(source->sky);

	    axes = pmPSF_ModelToAxes (PAR, model->class->useReff);

	} else {
	    xPos = source->peak->xf;
	    yPos = source->peak->yf;
	    axes.major = 0.0;
	    axes.minor = 0.0;
	    axes.theta = 0.0;
	}

        row = psMetadataAlloc ();
        psMetadataAdd (row, PS_LIST_TAIL, "X_PIX",   PS_DATA_F32, "", xPos);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_PIX",   PS_DATA_F32, "", yPos);
        psMetadataAdd (row, PS_LIST_TAIL, "MAG_RAW", PS_DATA_F32, "", PS_MIN (99.0, source->psfMag + ZERO_POINT));
        psMetadataAdd (row, PS_LIST_TAIL, "MAG_ERR", PS_DATA_F32, "", PS_MIN (999, 1000*source->psfMagErr));
        psMetadataAdd (row, PS_LIST_TAIL, "MAG_GAL", PS_DATA_F32, "", PS_MIN (99.0, source->extMag + ZERO_POINT));
        psMetadataAdd (row, PS_LIST_TAIL, "MAG_AP",  PS_DATA_F32, "", PS_MIN (99.0, source->apMag + ZERO_POINT));
        psMetadataAdd (row, PS_LIST_TAIL, "LOG_SKY", PS_DATA_F32, "", lsky);
        psMetadataAdd (row, PS_LIST_TAIL, "FWHM_X",  PS_DATA_F32, "", axes.major);
        psMetadataAdd (row, PS_LIST_TAIL, "FWHM_Y",  PS_DATA_F32, "", axes.minor);
        psMetadataAdd (row, PS_LIST_TAIL, "THETA",   PS_DATA_F32, "", axes.theta);
        psMetadataAdd (row, PS_LIST_TAIL, "DOPHOT",  PS_DATA_U8,  "", type);
        psMetadataAdd (row, PS_LIST_TAIL, "WEIGHT",  PS_DATA_U8,  "", PS_MIN (255, PS_MAX(0, 255*source->pixWeightNotBad)));
        psMetadataAdd (row, PS_LIST_TAIL, "DUMMY",   PS_DATA_U16, "", 0);

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
psArray *pmSourcesRead_SMPDATA (psFits *fits, psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(header, false);

    bool status;
    psF32 *PAR;
    psEllipseAxes axes;
    float lsky;

    // define PSF model type
    int modelType = pmModelClassGetType ("PS_MODEL_GAUSS");

    char *PSF_NAME = psMetadataLookupStr (&status, header, "PSF_NAME");
    if (PSF_NAME != NULL) {
        modelType = pmModelClassGetType (PSF_NAME);
    }

    // find config information for output header
    float ZERO_POINT = psMetadataLookupF32 (&status, header, "ZERO_PT");
    if (!status)
        ZERO_POINT = 25.0;

    psArray *table = psFitsReadTable (fits);
    // validate a single row of the table (must match SMP)

    psArray *sources = psArrayAlloc (table->n);

    // convert the table to the pmSource entries
    // XXX need to chooose PSF vs EXT, based on type?
    for (int i = 0; i < table->n; i++) {
        pmSource *source = pmSourceAlloc ();
        pmModel *model = pmModelAlloc (modelType);
        source->modelPSF  = model;
        source->type = PM_SOURCE_TYPE_STAR;

        PAR = model->params->data.F32;

        psMetadata *row = table->data[i];

        lsky             = psMetadataLookupF32 (&status, row, "LOG_SKY");
        PAR[PM_PAR_SKY]  = pow(10.0, lsky);
        source->sky    = PAR[PM_PAR_SKY];

        PAR[PM_PAR_XPOS] = psMetadataLookupF32 (&status, row, "X_PIX");
        PAR[PM_PAR_YPOS] = psMetadataLookupF32 (&status, row, "Y_PIX");
        axes.major       = psMetadataLookupF32 (&status, row, "FWHM_X");
        axes.minor       = psMetadataLookupF32 (&status, row, "FWHM_Y");
        axes.theta       = psMetadataLookupF32 (&status, row, "THETA");

	pmPSF_AxesToModel (PAR, axes, model->class->useReff);

        source->psfMag = psMetadataLookupF32 (&status, row, "MAG_RAW") - ZERO_POINT;
        source->extMag = psMetadataLookupF32 (&status, row, "MAG_GAL") - ZERO_POINT;
        source->psfMagErr = psMetadataLookupF32 (&status, row, "MAG_ERR") * 0.001;
        source->apMag  = psMetadataLookupF32 (&status, row, "MAG_AP")  - ZERO_POINT;

        source->pixWeightNotBad = psMetadataLookupU8 (&status, row, "WEIGHT")/255.0;
        int dophot = psMetadataLookupU8 (&status, row, "DOPHOT");
	pmSourceSetDophotType (source, dophot);

	double Area = 2.0 * M_PI * axes.major * axes.minor;
	double peakFlux = source->psfMag / Area;

	source->peak = pmPeakAlloc(PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], peakFlux, PM_PEAK_LONE);

        sources->data[i] = source;
    }
    psFree (table);
    return (sources);
}

bool pmSourcesWrite_SMPDATA_XSRC (psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe)
{
    return true;
}

bool pmSourcesWrite_SMPDATA_XFIT(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname)
{
    return true;
} 

bool pmSourcesWrite_SMPDATA_XRAD(psFits *fits, pmReadout *readout, psArray *sources, psMetadata *imageHeader, char *extname, psMetadata *recipe)
{
    return true;
} 

bool pmSourcesWrite_SMPDATA_XGAL(psFits *fits, pmReadout *readout, psArray *sources, char *extname, psMetadata *recipe)
{
    return true;
} 
