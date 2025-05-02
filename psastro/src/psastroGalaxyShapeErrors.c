/** @file psastroGalaxyShapeError.c
 *
 *  @brief: monte carlo transformation of Sxx,Sxy,Syy -> Major,Minor,Theta
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

double psastroNormalizeAngleToMidpoint (double angle, double midpoint);

bool psastroGalaxyShapeErrors (psMetadata *recipe, pmReadout *readout) {

    bool status = false;

    bool TransformGalaxyShapeErrors = psMetadataLookupBool (&status, recipe, "TRANSFORM.GALAXY.SHAPE.ERRORS");
    if (!TransformGalaxyShapeErrors) return true;

    psTimerStart ("psastro.galaxy.shape.errors");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) return false;

    psArray *sources = detections->allSources;
    if (!detections) return false;

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);

    int nSample = 1000;
    psVector *majorValues = psVectorAllocEmpty (nSample, PS_TYPE_F32);
    psVector *minorValues = psVectorAllocEmpty (nSample, PS_TYPE_F32);
    psVector *thetaValues = psVectorAllocEmpty (nSample, PS_TYPE_F32);

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);

    int nObjects = 0;

    // don't try and use bad models
    pmModelStatus badModel = PM_MODEL_STATUS_NONE;
    // badModel |= PM_MODEL_STATUS_NONCONVERGE;  this causes most objects to get unmeasured
    badModel |= PM_MODEL_STATUS_BADARGS;
    badModel |= PM_MODEL_STATUS_OFFIMAGE;
    badModel |= PM_MODEL_STATUS_NAN_CHISQ;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GUESS;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GRID;
    badModel |= PM_MODEL_PCM_FAIL_GUESS;
    badModel |= PM_MODEL_STATUS_LIMITS;

    // transform all sources, regardless of quality.  the source flags tell us the state
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];
        if (source->modelFits == NULL) continue;

        // We have multiple model fits to each source
        for (int j = 0; j < source->modelFits->n; j++) {

            // choose each model
            pmModel *model = source->modelFits->data[j];
            assert (model);

	    // skip models which were not actually fitted
	    if (model->flags & badModel) continue;

            float *PAR = model->params->data.F32;
            float *dPAR = model->dparams->data.F32;

	    if (!(isfinite(PAR[PM_PAR_SXX]) && isfinite(PAR[PM_PAR_SXY]) && isfinite(PAR[PM_PAR_SYY]))) {
	      // set a mask bit
	      model->flags |= PM_MODEL_STATUS_NAN_SHAPE;
	      continue;
	    }
	    if (!(isfinite(dPAR[PM_PAR_SXX]) && isfinite(dPAR[PM_PAR_SXY]) && isfinite(dPAR[PM_PAR_SYY]))) {
	      // set a (different) mask bit
	      model->flags |= PM_MODEL_STATUS_NAN_SHAPE_ERR;
	      continue;
	    }

	    // psphot writes out dPAR, but PAR is writen as MAJOR,MINOR,THETA but transformed
	    // to PAR[SXX,SXY,SYY] when the cmf is read.  here we use a monte carlo to generate
	    // a set of PAR[] values which are converted to MAJOR,MINOR,THETA so sigma can be
	    // measured.

	    majorValues->n = 0;
	    minorValues->n = 0;
	    thetaValues->n = 0;

	    bool useReff = model->class->useReff;
	    psEllipseAxes axes;

	    float SxxRef = PAR[PM_PAR_SXX];
	    float SxyRef = PAR[PM_PAR_SXY];
	    float SyyRef = PAR[PM_PAR_SYY];
	    if (!(isfinite(SxxRef) && isfinite(SxyRef) && isfinite(SyyRef))) {
	      // skip objects which are already poor ellipses
	      model->flags |= PM_MODEL_STATUS_NAN_SHAPE;
	      continue;
	    }
	    pmModelParamsToAxes (&axes, SxxRef, SxyRef, SyyRef, useReff);
	    if (!(isfinite(axes.major) && isfinite(axes.minor) && isfinite(axes.theta))) {
	      // skip objects which are already poor ellipses
	      model->flags |= PM_MODEL_STATUS_NAN_SHAPE;
	      continue;
	    }
	    double ThetaRef = axes.theta;

	    for (int k = 0; k < nSample; k++) {

		float Sxx = PAR[PM_PAR_SXX] + dPAR[PM_PAR_SXX]*psRandomGaussian(rng);
		float Sxy = PAR[PM_PAR_SXY] + dPAR[PM_PAR_SXY]*psRandomGaussian(rng);
		float Syy = PAR[PM_PAR_SYY] + dPAR[PM_PAR_SYY]*psRandomGaussian(rng);
		if (!(isfinite(Sxx) && isfinite(Sxy) && isfinite(Syy))) {
		  continue;
		}

		pmModelParamsToAxes (&axes, Sxx, Sxy, Syy, useReff);
		if (!(isfinite(axes.major) && isfinite(axes.minor) && isfinite(axes.theta))) continue;

		// theta should be in range ThetaRef - PI : ThetaRef + PI
		double ThetaNorm = psastroNormalizeAngleToMidpoint (axes.theta, ThetaRef);

		psVectorAppend (majorValues, axes.major);
		psVectorAppend (minorValues, axes.minor);
		psVectorAppend (thetaValues, ThetaNorm);
	    }
	    if (majorValues->n == 0) {
		psWarning ("failed mc error search");
		model->flags |= PM_MODEL_STATUS_FAIL_SHAPE_ERR;
		continue;
	    }

	    psStatsInit (stats);
	    psVectorStats (stats, majorValues, NULL, NULL, 0);
	    float dMajor = stats->robustStdev;

	    psStatsInit (stats);
	    psVectorStats (stats, minorValues, NULL, NULL, 0);
	    float dMinor = stats->robustStdev;

	    // overload the results back on the PAR[] vectors
	    dPAR[PM_PAR_SXX] = dMajor;
	    dPAR[PM_PAR_SYY] = dMinor;
	    nObjects ++;
	}
    }
    psFree (rng);
    psFree (stats);
    psFree (majorValues);
    psFree (minorValues);
    psFree (thetaValues);

    psMetadata *header = psMetadataLookupMetadata (&status, readout->analysis, "PSASTRO.HEADER");
    psMetadataAddStr (header, PS_LIST_TAIL, "GALAXY.SHAPE.ERRORS", PS_META_REPLACE, "error analysis mode", "MONTE CARLO");

    psLogMsg ("psastro.galaxy", PS_LOG_INFO, "monte carlo shape errors for %d of %d objects: %f sec\n", nObjects, (int) sources->n, psTimerMark ("psastro.galaxy.shape.errors"));
    return true;
}

// these angles are in radians
double psastroNormalizeAngleToMidpoint (double angle, double midpoint) {

    double x = cos(angle);
    double y = sin(angle);

    double result = atan2 (y, x);
    if (result < midpoint - M_PI) result += 2.0*M_PI;
    if (result > midpoint + M_PI) result -= 2.0*M_PI;

    return result;
}

