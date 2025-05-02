#include "psphotInternal.h"
#define USE_SAMPLE 1 /* sample vs robust stats */

// XXX Lots of code duplication here

// for now, let's store the detections on the readout->analysis for each readout
bool psphotImageQuality (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
	if (!psphotImageQualityReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on to measure image quality for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// selecting the 'good' stars (likely to be psf stars), measure the M_cn, M_sn terms for n = 2,3,4
bool psphotImageQualityReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    bool status = true;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // if we have a PSF, skip this analysis
    if (psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF")) {
	return true;
    }

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->newSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping image quality");
	return true;
    }

    psVector *FWHM_MAJOR = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *FWHM_MINOR = psVectorAllocEmpty(100, PS_TYPE_F32);

    psVector *M2 = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *M3 = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *M4 = psVectorAllocEmpty(100, PS_TYPE_F32);

    psVector *M2c = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *M2s = psVectorAllocEmpty(100, PS_TYPE_F32);

    int num = 0;                        // Number of good sources
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];
        if (!source) {
            continue;
        }

        // select by S/N?
        // select by PSFSTAR mode?
        // ??
        if (source->type != PM_SOURCE_TYPE_STAR || !(source->tmpFlags & PM_SOURCE_TMPF_CANDIDATE_PSFSTAR)) {
            psTrace("psphot", 10, "Ignoring source for image quality because not a good star");
            continue;
        }

        pmMoments *moments = source->moments;
        if (!moments) {
            psTrace("psphot", 10, "Ignoring source for image quality because no moments");
            continue;
        }

        // if (source->moments->SN < XXX) continue;

        // M_cn = \sum (f r^2 cos(n t)) / \sum (f)
        // M_sn = \sum (f r^2 sin(n t)) / \sum (f)

        // r^2 cos(2t) = r^2 cos^2 t - r^2 sin^2 t = x^2 - y^2
        // r^2 sin(2t) = r^2 2 cos t sin t = 2 x y

        // r^2 cos(3t) = r^2 cos^3 t - r^2 3 cos t sin^2 t = (x^3 - 3 x y^2) / r
        // r^2 sin(3t) = r^2 3 cos^2 t sin t - sin^3 t = (3 x^2 y - y^3) / r

        // r^2 cos(4t) = r^2 cos^4 t - r^2 6 cos^2 t sin^2 t + r^2 sin^4 t = (x^4 - 6 x^2 y^2 + y^4) / r^2
        // r^2 sin(4t) = r^2 4 cos^3 t sin t - 4 sin^3 t cos t = (4 x^3 y - 4 y^3 x) / r^2

	// NOTE that Mxxx,etc have the factor of r in the denominator, while Mxxxx,etc have the factor of r^2.
	// See pmSourceMoments.c:418

        num++;

        psEllipseMoments emoments;
        emoments.x2 = moments->Mxx;
        emoments.xy = moments->Mxy;
        emoments.y2 = moments->Myy;
        psEllipseAxes axes = psEllipseMomentsToAxes (emoments, 20.0);

        psVectorAppend (FWHM_MAJOR, 2.355*axes.major);
        psVectorAppend (FWHM_MINOR, 2.355*axes.minor);

        // the moments->Mnnn terms contain the straight sums: for example: moments->Mxxx contains \sum x^3 / r
        float M_c2 = moments->Mxx - moments->Myy;
        float M_s2 = 2*moments->Mxy;
        psVectorAppend (M2, hypot(M_c2, M_s2));
        psVectorAppend (M2c, M_c2);
        psVectorAppend (M2s, M_s2);

        float M_c3 = moments->Mxxx - 3.0*moments->Mxyy;
        float M_s3 = 3.0*moments->Mxxy - moments->Myyy;
        psVectorAppend (M3, hypot (M_c3, M_s3));

        float M_c4 = moments->Mxxxx - 6.0*moments->Mxxyy + moments->Myyyy;
        float M_s4 = 4.0*moments->Mxxxy - 4.0*moments->Mxyyy;
        psVectorAppend (M4, hypot (M_c4, M_s4));
    }

    if (num == 0) {
	psLogMsg ("psphot", PS_LOG_INFO, "no valid sources for image quality, skipping");
	psFree(FWHM_MAJOR);
	psFree(FWHM_MINOR);
	psFree(M2);
	psFree(M2c);
	psFree(M2s);
	psFree(M3);
	psFree(M4);

	return true;
    }

    psMetadataAddS32(readout->analysis, PS_LIST_TAIL, "IQ_NSTAR", PS_META_REPLACE, "Number of stars used for IQ measurements", M2->n);

// XXX make this a recipe option
#if (USE_SAMPLE)
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV | PS_STAT_SAMPLE_QUARTILE);

    if (!psVectorStats(stats, FWHM_MAJOR, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }

    float fwhm_major = stats->sampleMean;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW1",   PS_META_REPLACE, "FWHM of Major Axis from moments", stats->sampleMean);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW1_E", PS_META_REPLACE, "FWHM scatter (Major) from moments", stats->sampleStdev);

    if (!psVectorStats(stats, FWHM_MINOR, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float fwhm_minor = stats->sampleMean;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW2",   PS_META_REPLACE, "FWHM of Minor Axis from moments", stats->sampleMean);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW2_E", PS_META_REPLACE, "FWHM scatter (Minor) from moments", stats->sampleStdev);

    if (!psVectorStats(stats, M2, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float vM2 = stats->sampleMean;
    float dM2 = stats->sampleStdev;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2",    PS_META_REPLACE, "M_2 = sqrt (M_c2^2 + M_s2^2)", vM2);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2_ER", PS_META_REPLACE, "Stdev of M_2", dM2);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2_LQ", PS_META_REPLACE, "Lower Quartile of M_2", stats->sampleLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2_UQ", PS_META_REPLACE, "Upper Quartile of M_2", stats->sampleUQ);

    if (!psVectorStats(stats, M2c, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C",   PS_META_REPLACE, "M_2c = sum f r^2 cos(2phi) / sum f", stats->sampleMean);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C_E", PS_META_REPLACE, "Stdev of M_2c", stats->sampleStdev);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C_L", PS_META_REPLACE, "Lower Quartile of M_2c", stats->sampleLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C_U", PS_META_REPLACE, "Upper Quartile of M_2c", stats->sampleUQ);

    if (!psVectorStats(stats, M2s, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S",   PS_META_REPLACE, "M_2s = sum f r^2 cos(2phi) / sum f", stats->sampleMean);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S_E", PS_META_REPLACE, "Stdev of M_2s", stats->sampleStdev);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S_L", PS_META_REPLACE, "Lower Quartile of M_2s", stats->sampleLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S_U", PS_META_REPLACE, "Upper Quartile of M_2s", stats->sampleUQ);

    if (!psVectorStats(stats, M3, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float vM3 = stats->sampleMean;
    float dM3 = stats->sampleStdev;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3",   PS_META_REPLACE, "M_3 = sqrt (M_c3^2 + M_s3^2)", vM3);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3_ER", PS_META_REPLACE, "Stdev of M_3", dM3);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3_LQ", PS_META_REPLACE, "Lower Quartile of M_3", stats->sampleLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3_UQ", PS_META_REPLACE, "Upper Quartile of M_3", stats->sampleUQ);

    if (!psVectorStats(stats, M4, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float vM4 = stats->sampleMean;
    float dM4 = stats->sampleStdev;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4",   PS_META_REPLACE, "M_4 = sqrt (M_c4^2 + M_s4^2)", vM4);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4_ER", PS_META_REPLACE, "Stdev of M_4", dM4);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4_LQ", PS_META_REPLACE, "Lower Quartile of M_4", stats->sampleLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4_UQ", PS_META_REPLACE, "Upper Quartile of M_4", stats->sampleUQ);

#else
    psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV | PS_STAT_ROBUST_QUARTILE);

    if (!psVectorStats(stats, FWHM_MAJOR, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float fwhm_major = stats->robustMedian;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW1",   PS_META_REPLACE, "FWHM of Major Axis from moments", stats->robustMedian);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW1_E", PS_META_REPLACE, "FWHM scatter (Major) from moments", stats->robustStdev);

    if (!psVectorStats(stats, FWHM_MINOR, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float fwhm_minor = stats->robustMedian;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW2",   PS_META_REPLACE, "FWHM of Minor Axis from moments", stats->robustMedian);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_FW2_E", PS_META_REPLACE, "FWHM scatter (Minor) from moments", stats->robustStdev);

    if (!psVectorStats(stats, M2, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float vM2 = stats->robustMedian;
    float dM2 = stats->robustStdev;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2",    PS_META_REPLACE, "M_2 = sqrt (M_c2^2 + M_s2^2)", vM2);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2_ER", PS_META_REPLACE, "Stdev of M_2", dM2);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2_LQ", PS_META_REPLACE, "Lower Quartile of M_2", stats->robustLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2_UQ", PS_META_REPLACE, "Upper Quartile of M_2", stats->robustUQ);

    if (!psVectorStats(stats, M2c, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C",   PS_META_REPLACE, "M_2c = sum f r^2 cos(2phi) / sum f", stats->robustMedian);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C_E", PS_META_REPLACE, "Stdev of M_2c", stats->robustStdev);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C_L", PS_META_REPLACE, "Lower Quartile of M_2c", stats->robustLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2C_U", PS_META_REPLACE, "Upper Quartile of M_2c", stats->robustUQ);

    if (!psVectorStats(stats, M2s, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S",   PS_META_REPLACE, "M_2s = sum f r^2 cos(2phi) / sum f", stats->robustMedian);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S_E", PS_META_REPLACE, "Stdev of M_2s", stats->robustStdev);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S_L", PS_META_REPLACE, "Lower Quartile of M_2s", stats->robustLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M2S_U", PS_META_REPLACE, "Upper Quartile of M_2s", stats->robustUQ);

    if (!psVectorStats(stats, M3, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float vM3 = stats->robustMedian;
    float dM3 = stats->robustStdev;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3",   PS_META_REPLACE, "M_3 = sqrt (M_c3^2 + M_s3^2)", vM3);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3_ER", PS_META_REPLACE, "Stdev of M_3", dM3);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3_LQ", PS_META_REPLACE, "Lower Quartile of M_3", stats->robustLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M3_UQ", PS_META_REPLACE, "Upper Quartile of M_3", stats->robustUQ);

    if (!psVectorStats(stats, M4, NULL, NULL, 0)) {
        psError(PSPHOT_ERR_UNKNOWN, false, "Unable to perform statistics to measure image quality");
        goto FAIL;
    }
    float vM4 = stats->robustMedian;
    float dM4 = stats->robustStdev;
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4",   PS_META_REPLACE, "M_4 = sqrt (M_c4^2 + M_s4^2)", vM4);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4_ER", PS_META_REPLACE, "Stdev of M_4", dM4);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4_LQ", PS_META_REPLACE, "Lower Quartile of M_4", stats->robustLQ);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "IQ_M4_UQ", PS_META_REPLACE, "Upper Quartile of M_4", stats->robustUQ);
#endif

    psLogMsg ("psphot", PS_LOG_WARN, "Image Quality Stats from %ld psf stars : FWHM (major, minor) [pixels]: %f, %f\n",
              M2->n, fwhm_major, fwhm_minor);

    psLogMsg ("psphot", PS_LOG_INFO, "M_2 : %f +/- %f, M_3 : %f +/- %f, M_4 : %f +/- %f  [pixels^n]\n",
              vM2, dM2, vM3, dM3, vM4, dM4);

    psFree(FWHM_MAJOR);
    psFree(FWHM_MINOR);
    psFree(M2);
    psFree(M2c);
    psFree(M2s);
    psFree(M3);
    psFree(M4);
    psFree(stats);

    return true;

 FAIL:
    psFree(FWHM_MAJOR);
    psFree(FWHM_MINOR);
    psFree(M2);
    psFree(M2c);
    psFree(M2s);
    psFree(M3);
    psFree(M4);
    psFree(stats);

    return false;
}
