/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-18 02:44:19 $
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
#include "pmSourceOutputs.h"

// read in sources readout from a cff fits file
psArray *pmSourcesRead_CFF (psFits *fits, psMetadata *header, psMetadata *recipe)
{
    PS_ASSERT_PTR_NON_NULL(fits, false);
    PS_ASSERT_PTR_NON_NULL(header, false);

    bool status;
    psEllipseAxes axes;

    // define PSF model type
    // XXX need to carry the extra model parameters
    int modelType = pmModelClassGetType ("PS_MODEL_GAUSS");

    // Read lookup table for model classes (if defined)
    pmModelClassReadHeader(header);

    char *PSF_NAME = psMetadataLookupStr (&status, header, "PSFMODEL");
    if (PSF_NAME != NULL) {
        modelType = pmModelClassGetType (PSF_NAME);
    }
    PS_ASSERT_INT_NONNEGATIVE(modelType, NULL);

    pmModelType sersicModelType = pmModelClassGetType("PS_MODEL_SERSIC");
    pmModelType devModelType    = pmModelClassGetType("PS_MODEL_DEV");

    psString modelForce = psMetadataLookupStr(&status, recipe, "EXT_MODEL_TYPE_FORCE");
    psF32 forceDevSersicMin = NAN;
    bool forceAll = false;
    int modelTypeForce = 0;
    if (!strcmp(modelForce, "ALL")) {
        forceAll = true;
    } else if (!strcmp(modelForce, "PS_MODEL_SERSIC")) {
        modelTypeForce = sersicModelType;
        forceDevSersicMin = psMetadataLookupF32(&status, recipe, "EXT_MODEL_FORCE_DEV_SERSIC_MIN");
    } else {
        modelTypeForce = pmModelClassGetType(modelForce);
        PS_ASSERT_INT_NONNEGATIVE(modelTypeForce, NULL);
    }

    // skip extended model types for likely stars
    // max value of KronMag - psfMag to keep ...
    psF32 starCut = psMetadataLookupF32(&status, recipe, "EXT_MODEL_FORCE_MAGDIFF_MAX");
    if (!status) {
        starCut = 0;
    }
    // ... unless SN is less than this value
    psF32 SNMinForCut = psMetadataLookupF32(&status, recipe, "EXT_MODEL_FORCE_CUT_SN_MIN");
    if (!status) {
        SNMinForCut = 10;
    }

    // We get the size of the table, and allocate the array of sources first because the table
    // is large and ephemeral --- when the table gets blown away, whatever is allocated after
    // the table is read blocks the free.  In fact, it's better to read the table row by row.
    long numRows = psFitsTableSize(fits); // Number of rows in table
    psArray *sources = psArrayAllocEmpty(numRows); // Array of sources, to return

    // convert the table to the pmSource entries
    pmSource *source = NULL;
    for (int i = 0; i < numRows; i++) {
        psMetadata *row = psFitsReadTableRow(fits, i); // Table row
        if (!row) {
            psError(psErrorCodeLast(), false, "Unable to read row %d of sources", i);
            psFree(sources);
            return NULL;
        }

	// here are the things we read from the FITS table (XXX modify names if needed)

	unsigned int ID  = psMetadataLookupU32 (&status, row, "ID");
	float X          = psMetadataLookupF32 (&status, row, "X");
        assert(status);  // poorly constructed cff
	float Y          = psMetadataLookupF32 (&status, row, "Y");
        assert(status);  // poorly constructed cff
        float flux       = psMetadataLookupF32 (&status, row, "FLUX"); // guess at the instrumental flux
        if (!isfinite(X) || !isfinite(Y) || !isfinite(flux)) {
            psError(PS_ERR_IO, true, "source ID %d is invalid x: %f y: %f flux: %f", ID, X, Y, flux);
            psFree(sources);
            return NULL;
        }


        float apRadius   = psMetadataLookupF32 (&status, row, "AP_RADIUS");
        float kronRadius = psMetadataLookupF32 (&status, row, "KRON_RADIUS");
        float petRadius  = psMetadataLookupF32 (&status, row, "PETRO_RADIUS");
        float SN         = psMetadataLookupF32 (&status, row, "SN");
        bool fitGalaxy   = psMetadataLookupU8 (&status, row, "FIT_GALAXY");
        bool psfStar     = psMetadataLookupU8 (&status, row, "PSF_STAR");

        float Rmajor     = psMetadataLookupF32 (&status, row, "R_MAJOR");
        float Rminor     = psMetadataLookupF32 (&status, row, "R_MINOR");
        float theta      = psMetadataLookupF32 (&status, row, "THETA");
        float chisq      = psMetadataLookupF32 (&status, row, "CHISQ");
        float nDOF       = psMetadataLookupF32 (&status, row, "NDOF");
        float magDiff    = psMetadataLookupF32 (&status, row, "MAG_DIFF");
        psS16 modelFlags = psMetadataLookupS32 (&status, row, "MODEL_FLAGS");

        int   galaxyModelType = psMetadataLookupS32(&status, row, "MODEL_TYPE");
        if (status && galaxyModelType >= 0) {
            galaxyModelType = pmModelClassGetLocalType(galaxyModelType);
        } else {
            galaxyModelType = -1;
        }
        float Sindex     = psMetadataLookupF32 (&status, row, "INDEX"); // Should this be PAR_07 not sersic index

        if (!source || ID != source->seq) {
            if (source) {
                psArrayAdd (sources, 1, source);
                psFree(source);
            }
            source = pmSourceAlloc ();
            pmModel *model = pmModelAlloc (modelType);
            source->modelPSF  = model;
            //        RoughClass wants source type to be unknown
            //        source->type = PM_SOURCE_TYPE_STAR; // XXX this should be added to the flags
            source->type = PM_SOURCE_TYPE_UNKNOWN;

            // XXX we can set this in general, but for a specific image, we need to weed out SATSTARS and
            // stars that are masked
            if (psfStar) {
                source->tmpFlags |= PM_SOURCE_TMPF_CANDIDATE_PSFSTAR;
            }

            // NOTE: A SEGV here because "model" is NULL is probably caused by not initialising the models.
            psF32 *PAR = model->params->data.F32;
            psF32 *dPAR = model->dparams->data.F32;

            source->seq       = ID;

            PAR[PM_PAR_XPOS]  = X;
            PAR[PM_PAR_YPOS]  = Y;

            dPAR[PM_PAR_XPOS] = 0.0;
            dPAR[PM_PAR_YPOS] = 0.0;

            PAR[PM_PAR_SKY]   = 0.0;
            dPAR[PM_PAR_SKY]  = 0.0;

            PAR[PM_PAR_I0]    = 1.0;
            dPAR[PM_PAR_I0]   = 0.0;

            source->sky       = PAR[PM_PAR_SKY];
            source->skyErr    = dPAR[PM_PAR_SKY];

            source->psfMag    = 0.0;
            source->psfMagErr = 0.0;
            source->apMag     = 0.0;
            source->apRadius  = apRadius;

            // we generate a somewhat fake PSF model here -- 
            // in most (all?) contexts, we will replace this with a measured psf model
            // elsewhere
            axes.major        = 1.0;
            axes.minor        = 1.0;
            axes.theta        = 0.0;
            pmPSF_AxesToModel (PAR, axes, model->class->useReff);

            // peak->detValue, rawFlux, smoothFlux are all set to the flux argument which is counts per second
            source->peak      = pmPeakAlloc(X, Y, flux, PM_PEAK_LONE);
            source->peak->xf  = X; // pmPeakAlloc converts X,Y to int, so reset here
            source->peak->yf  = Y; // pmPeakAlloc converts X,Y to int, so reset here
            source->peak->dx  = 0.0;
            source->peak->dy  = 0.0;

            source->extSN     = SN;

            source->moments = pmMomentsAlloc ();
            source->moments->Mx = X;
            source->moments->My = Y;
            source->moments->Mrf = kronRadius * 0.4; // kronRadius is 2.5 * first radial moment

            // Don't mark the moments as measured because that causes many fields to be left blank.
            // The moments code knows not to change the position or the Mrf for external sources
            // source->tmpFlags |= PM_SOURCE_TMPF_MOMENTS_MEASURED;

            if (isfinite(petRadius)) {
                source->extpars = pmSourceExtendedParsAlloc ();
                source->extpars->petrosianRadius = petRadius;
            }

        }
        bool saveExtModelParams = false;
        if (fitGalaxy && galaxyModelType >= 0) {
            // skip likely stars 
            if (magDiff < starCut || SN < SNMinForCut) {
                if (forceAll) {
                    saveExtModelParams = true;
                } else if (galaxyModelType == modelTypeForce) {
                    // This is the model type that we are looking for
                    // proceed
                    saveExtModelParams = true;
                } else if (modelTypeForce == sersicModelType && galaxyModelType == devModelType) {
                    // We're doing sersic models, if sersic index is greater than the recipe's minimum index
                    // do dev model as well
                    if (isfinite(forceDevSersicMin) || Sindex >= forceDevSersicMin) {
                        saveExtModelParams = true;
                    }
                } else {
                    // not interested in this model
                }
            }
        }

        if (saveExtModelParams) {
            if (!source->modelFits) {
                source->modelFits = psArrayAllocEmpty (1);
            }
            pmModel *model = pmModelAlloc(galaxyModelType);
            psF32 *xPAR = model->params->data.F32;

            xPAR[PM_PAR_SKY]  = 0.0;
            xPAR[PM_PAR_I0]   = 1.0;
            xPAR[PM_PAR_XPOS] = X;
	    xPAR[PM_PAR_YPOS] = Y;
	    
	    psEllipseAxes galaxyAxes;
	    galaxyAxes.major = Rmajor;
	    galaxyAxes.minor = Rminor;
	    galaxyAxes.theta = theta * PS_RAD_DEG;

	    pmPSF_AxesToModel (xPAR, galaxyAxes, model->class->useReff);
	    if (model->params->n > 7) {
                xPAR[PM_PAR_7] = 0.5 / Sindex;
	    }

            model->chisq = chisq;
            model->nDOF = nDOF;
            model->flags = modelFlags;

	    psArrayAdd (source->modelFits, 1, model);

	    psFree (model);
        }

        psFree(row);
    }
    if (source) {
        // close out last source
        psArrayAdd (sources, 1, source);
        psFree(source);
    }

    return sources;
}

bool pmSourcesWrite_CFF(pmReadout *readout, psFits *fits, psArray *sources, psMetadata *header, psMetadata *recipe) {
    
    char *extname = "SkyChip.cff";
    
    bool mdok;
    psF32 exptime = psMetadataLookupF32(&mdok, readout->parent->concepts, "CELL.EXPOSURE");
    PS_ASSERT(mdok, false);

    // write the definition of the model class type values to the header
    psMetadata *outputHeader = psMetadataAlloc();
    pmModelClassWriteHeader(outputHeader);

    psArray *table = psArrayAllocEmpty(sources->n);

    pmModelType sersicModelType = pmModelClassGetType("PS_MODEL_SERSIC");
    pmModelType devModelType    = pmModelClassGetType("PS_MODEL_DEV");
    pmModelType selectedModelType = -1;
    bool chooseBest = false;
    bool chooseAll = false;

    psString modelToChoose = psMetadataLookupStr(&mdok, recipe, "EXT_MODEL_TYPE_FOR_CFF");

    if (mdok && modelToChoose != NULL) {
        if (!strcmp(modelToChoose, "BEST")) {
            chooseBest = true;
        } else if (!strcmp(modelToChoose, "ALL")) {
            chooseAll = true;
        } else if (strcmp(modelToChoose, "PS_MODEL_SERSIC")) {
            // We have selected a model type other than Sersic. 
            // Save it's type for use below.  Sersic is handled specially
            selectedModelType = pmModelClassGetType(modelToChoose);
        }
    }

    // minimum sersic index to force devModel
    psF32 sersicMinDev = psMetadataLookupF32(&mdok, recipe, "EXT_MODEL_FORCE_DEV_SERSIC_MIN");
    if (!mdok) {
        sersicMinDev = NAN;
    }

    sources = psArraySort (sources, pmSourceSortBySeq);

    for (int i = 0; i < sources->n; i++) {
        pmSource *thisSource = sources->data[i];
        pmSource *source = thisSource->parent ? thisSource->parent : thisSource;

        #define MAX_ROWS_PER_SRC 10
        psF32 xPos[MAX_ROWS_PER_SRC], yPos[MAX_ROWS_PER_SRC], flux[MAX_ROWS_PER_SRC];
        psF32 rMajor[MAX_ROWS_PER_SRC], rMinor[MAX_ROWS_PER_SRC], theta[MAX_ROWS_PER_SRC];
        psF32 chisq[MAX_ROWS_PER_SRC], nDOF[MAX_ROWS_PER_SRC];
        psS32 modelFlags[MAX_ROWS_PER_SRC];
        psS32 modelType[MAX_ROWS_PER_SRC];
        psF32 sersicIndex = NAN;
        bool fitGalaxy = false;
        bool psfStar = (source->mode & PM_SOURCE_MODE_PSFSTAR) ? true : false;
        int n_rows = 0;

        psF32 kronFlux = source->moments->KronFlux;
        psF32 SN = NAN;
        psF32 magDiff = NAN;
        if (isfinite(kronFlux) && isfinite(source->moments->KronFluxErr) && isfinite(source->psfMag)) {
            SN = kronFlux/source->moments->KronFluxErr;
            // kronMag - psfMag for use as star/glaxy separator
            magDiff = -2.5 * log10(kronFlux) - source->psfMag ;
        }

        // start with psf model
        pmModel *model = source->modelPSF;
        if (model == NULL) continue;
        psF32 *PAR = model->params->data.F32;
        if (!isfinite(PAR[PM_PAR_SXX]) || !isfinite(PAR[PM_PAR_SYY])  || !isfinite(PAR[PM_PAR_SXY]) ||
            !isfinite(source->psfFlux)) {
            continue;
        }

	// save the PSF model parameters for each object as the first entry
        xPos[0] = model->params->data.F32[PM_PAR_XPOS];
        yPos[0] = model->params->data.F32[PM_PAR_YPOS];
        flux[0] = source->psfFlux;
        rMajor[0] = 0;
        rMinor[0] = 0;
        theta[0] = 0;
        modelType[0] = -1;
        sersicIndex = NAN;
        chisq[0] = NAN;
        nDOF[0] = NAN;
        modelFlags[0] = 0;
	n_rows ++;

        if (source->modelFits != NULL) {
            // figure out which models to use based on recipe paramters
            if (chooseAll) {
                // Save parameters for all valid extended models

                // but make sure we aren't going to overflow our arrays
                assert (source->modelFits->n < MAX_ROWS_PER_SRC);

                for (int j=0; j<source->modelFits->n; j++) {
                    pmModel *model = source->modelFits->data[j];
                    psF32 *PAR = model->params->data.F32;

                    if (isfinite(PAR[PM_PAR_SXX]) && isfinite(PAR[PM_PAR_SYY])  && isfinite(PAR[PM_PAR_SXY]) &&
                        isfinite(model->mag)) {

                        xPos[n_rows] = PAR[PM_PAR_XPOS];
                        yPos[n_rows] = PAR[PM_PAR_YPOS];

                        psEllipseAxes axes = pmPSF_ModelToAxes (PAR, model->class->useReff);
                        rMajor[n_rows] = axes.major;
                        rMinor[n_rows] = axes.minor;
                        theta[n_rows]  = axes.theta*PS_DEG_RAD;
                        flux[n_rows] = pow(10.0, -0.4*model->mag);
                        modelType[n_rows] = model->type;
                        chisq[n_rows] = model->chisq;
                        nDOF[n_rows] = model->nDOF;
                        modelFlags[n_rows] = model->flags;
                        fitGalaxy = true;
                        if (model->params->n == 8) {
                            // this will save the sersic index in all models but that might
                            // be useful
                            sersicIndex = 0.5 / PAR[PM_PAR_7];
                        } else {
                            sersicIndex = NAN;
                        }

                        n_rows++;
                    }
                }
            } else {
                int jModelSersic = -1;
                int jModelDev = -1;
                int jModelSelected = -1;
                psF32 minChisq = NAN;
                for (int j=0; j<source->modelFits->n; j++) {
                    pmModel *model = source->modelFits->data[j];
                    if (chooseBest) {
                        // choose the model with lowest chisq
                        if (isfinite(model->chisq) && (!isfinite(minChisq) || model->chisq < minChisq)) {
                            jModelSelected = j;
                            minChisq = model->chisq;
                        }
                    } else {
                        // find the index of models of interest
                        if (model->type == selectedModelType) {
                            jModelSelected = j;
                        } else if (model->type == sersicModelType) {
                            jModelSersic = j;
                        } else if (model->type == devModelType) {
                            jModelDev = j;
                        }
                    }
                }
                if (jModelSelected >= 0 || jModelSersic >= 0) {
                    // If a specific non-sersic model we take paramers from that one.
                    // Otherwise we do the sersic model.
                    pmModel *model = jModelSelected >= 0 ? source->modelFits->data[jModelSelected] :
                                                           source->modelFits->data[jModelSersic];
                    psF32 *PAR = model->params->data.F32;

                    if (isfinite(PAR[PM_PAR_SXX]) && isfinite(PAR[PM_PAR_SYY])  && isfinite(PAR[PM_PAR_SXY]) &&
                        isfinite(model->mag)) {

                        xPos[0] = PAR[PM_PAR_XPOS];
                        yPos[0] = PAR[PM_PAR_YPOS];

                        psEllipseAxes axes = pmPSF_ModelToAxes (PAR, model->class->useReff);
                        rMajor[0] = axes.major;
                        rMinor[0] = axes.minor;
                        theta[0]  = axes.theta*PS_DEG_RAD;
                        flux[0] = pow(10.0, -0.4*model->mag);
                        modelType[0] = model->type;
                        chisq[0] = model->chisq;
                        nDOF[0] = model->nDOF;
                        modelFlags[0] = model->flags;
                        fitGalaxy = true;
                        if (model->type == sersicModelType) {
                            PS_ASSERT_FLOAT_LARGER_THAN(PAR[PM_PAR_7], 0.0, false);
                            sersicIndex = 0.5 / PAR[PM_PAR_7];
                        }

                        n_rows = 1;

                        // Unless a specific non-sersic model type was selected do dev model for sources with
                        // sersic index above the recipe limit.
                        if (jModelSelected == -1 && jModelDev >= 0 && isfinite(sersicMinDev) &&
                            sersicIndex > sersicMinDev) {

                            model = source->modelFits->data[jModelDev];
                            if (isfinite(PAR[PM_PAR_SXX]) && isfinite(PAR[PM_PAR_SYY])  && isfinite(PAR[PM_PAR_SXY]) &&
                                isfinite(model->mag)) { 

                                xPos[1] = PAR[PM_PAR_XPOS];
                                yPos[1] = PAR[PM_PAR_YPOS];

                                psEllipseAxes axes = pmPSF_ModelToAxes (PAR, model->class->useReff);
                                rMajor[1] = axes.major;
                                rMinor[1] = axes.minor;
                                theta[1]  = axes.theta*PS_DEG_RAD;
                                flux[1] = pow(10.0, -0.4*model->mag);
                                modelType[1] = model->type;
                                sersicIndex = NAN;
                                chisq[1] = model->chisq;
                                nDOF[1] = model->nDOF;
                                modelFlags[1] = model->flags;
                                n_rows = 2;
                            }
                        }
                    }
                }
            }
        }

        for (int j = 0; j < n_rows; j++) {
            psMetadata *row = psMetadataAlloc();
            psMetadataAddU32 (row, PS_LIST_TAIL, "ID",         0,   "IPP detection identifier",  source->seq);
            psMetadataAddF32 (row, PS_LIST_TAIL, "X",                0, "x coordinate",          xPos[j]);
            psMetadataAddF32 (row, PS_LIST_TAIL, "Y",                0, "y coordinate",          yPos[j]);
            psMetadataAddF32 (row, PS_LIST_TAIL, "FLUX",             0, "flux per second",       flux[j]/exptime);
            psMetadataAddF32 (row, PS_LIST_TAIL, "SN",               0, "kron flux signal to noise", SN);
            psMetadataAddF32 (row, PS_LIST_TAIL, "MAG_DIFF",         0, "psf mag - kron mag",    magDiff);
            psMetadataAddF32 (row, PS_LIST_TAIL, "AP_RADIUS",        0, "aperture radius",       source->apRadius);
            psMetadataAddF32 (row, PS_LIST_TAIL, "KRON_RADIUS",      0, "Kron radius",           source->moments->Mrf * 2.5);
            psMetadataAddF32 (row, PS_LIST_TAIL, "PETRO_RADIUS",     0, "Petrosian Radius",      source->extpars ? source->extpars->petrosianRadius : NAN);
            psMetadataAddBool (row, PS_LIST_TAIL, "FIT_GALAXY",      0, "source has xfit",       fitGalaxy); 
            psMetadataAddBool (row, PS_LIST_TAIL, "PSF_STAR",        0, "source was psf star",   psfStar);
            psMetadataAddF32 (row, PS_LIST_TAIL, "R_MAJOR",          0, "radius of major axis",  rMajor[j]);
            psMetadataAddF32 (row, PS_LIST_TAIL, "R_MINOR",          0, "radius of minor axis",  rMinor[j]);
            psMetadataAddF32 (row, PS_LIST_TAIL, "THETA",            0, "theta",                 theta[j]);
            psMetadataAddS32 (row, PS_LIST_TAIL, "MODEL_TYPE",       0, "model type",            modelType[j]);
            psMetadataAddF32 (row, PS_LIST_TAIL, "INDEX",            0, "sersic index",          sersicIndex);
            psMetadataAddF32 (row, PS_LIST_TAIL, "CHISQ",            0, "chisq",                 chisq[j]);
            psMetadataAddF32 (row, PS_LIST_TAIL, "NDOF",             0, "n degrees of freedom",  nDOF[j]);
            psMetadataAddS32 (row, PS_LIST_TAIL, "MODEL_FLAGS",      0, "model flags",           modelFlags[j]);

            psArrayAdd(table, 100, row);
            psFree(row);
        }
    }

    if (!psFitsWriteTable(fits, outputHeader, table, extname)) {
        psError(psErrorCodeLast(), false, "writing ext data %s\n", extname);
        psFree(table);
        psFree(outputHeader);
        return false;
    }
    psFree(table);
    psFree(outputHeader);

    return true;
}
