# include "psphotInternal.h"

# define RESIDUAL_SOFTENING 0.005 

bool psphotMakeResiduals (psArray *sources, psMetadata *recipe, pmPSF *psf, psImageMaskType maskVal) {

    bool status, isPSF;
    double flux, dflux;
    psImageMaskType mflux;

    psTimerStart ("psphot.residuals");

    if (!psMetadataLookupBool(&status, recipe, "PSF.RESIDUALS")) return true;

    int SPATIAL_ORDER = psMetadataLookupS32(&status, recipe, "PSF.RESIDUALS.SPATIAL_ORDER");
    PS_ASSERT (status, false);
    if (SPATIAL_ORDER != 0 && SPATIAL_ORDER != 1) {
        psError(PSPHOT_ERR_CONFIG, true, "PSF.RESIDUALS.SPATIAL_ORDER must be 0 or 1 (not %d)",
                SPATIAL_ORDER);
        return false;
    }

    int xBin = psMetadataLookupS32(&status, recipe, "PSF.RESIDUALS.XBIN");
    PS_ASSERT (status, false);
    PS_ASSERT (xBin != 0, false);

    int yBin = psMetadataLookupS32(&status, recipe, "PSF.RESIDUALS.YBIN");
    PS_ASSERT (status, false);
    PS_ASSERT (yBin != 0, false);

    float nSigma = psMetadataLookupF32(&status, recipe, "PSF.RESIDUALS.NSIGMA");
    PS_ASSERT (status, false);

    float pixelSN = psMetadataLookupF32(&status, recipe, "PSF.RESIDUALS.PIX.SN");
    PS_ASSERT (status, false);

    float radiusMax = psMetadataLookupF32(&status, recipe, "PSF.RESIDUALS.RADIUS");
    PS_ASSERT (status, false);

    char *modeString = psMetadataLookupStr(&status, recipe, "PSF.RESIDUALS.INTERPOLATION");
    PS_ASSERT (status, false);

    psImageInterpolateMode mode = psImageInterpolateModeFromString (modeString);
    if (mode == PS_INTERPOLATE_NONE) {
        psError(PSPHOT_ERR_CONFIG, false, "invalid interpolation in psphot.config");
        return false;
    }

    char *statString = psMetadataLookupStr(&status, recipe, "PSF.RESIDUALS.STATISTIC");
    PS_ASSERT (status, false);

    psStatsOptions statOption = psStatsOptionFromString (statString);
    if (!statOption) {
        psError(PSPHOT_ERR_CONFIG, false, "invalid residual statistic in psphot.config");
        return false;
    }

    // user parameters:
    // size of aperture (determine from source images?)
    // binning factor

    // select the subset of sources which are the PSFSTARs
    // for each input source:
    // - construct a residual image, renormalized
    // - construct a renormalized variance image
    // - construct a new mask image

    // construct the output residual table (Nx*DX,Ny*DY)
    // for each output pixel:
    // - construct a histogram of the values & variances (interpolate to the common pixel coordinate)
    // - measure the robust median & sigma
    // - reject (mask) input pixels which are outliers
    // - re-measure the robust median & sigma
    // - set output pixel, variance, and mask

    // these mask values do not correspond to the recipe values: they
    // are not propagated to images: they just need to fit in an 8-bit
    // value.  they are supplied to psImageInterpolate, which takes a
    // psImageMaskType; the mask portion of the result from
    // psImageInterpolate is supplied to fmasks, which is then used by
    // psVectorStats

    const psImageMaskType badMask     = 0x01;   // mask bits
    const psImageMaskType poorMask    = 0x02;   // from psImageInterpolate
    const psImageMaskType clippedMask = 0x04;   // mask bit set for clipped values
    const psVectorMaskType fmaskVal = badMask | poorMask | clippedMask;

    // determine the maximum image size from the input sources
    int xSize = 0;
    int ySize = 0;

    psVector *xC = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *yC = psVectorAllocEmpty (100, PS_TYPE_F32);

    // build (DATA - MODEL) [an image] for each psf star
    psArray *input = psArrayAllocEmpty (100);
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];

        if (!(source->mode & PM_SOURCE_MODE_PSFSTAR)) continue;

        // which model to use?
        pmModel *model = pmSourceGetModel (&isPSF, source);
        if (model == NULL) continue;  // model must be defined

        psImage *image    = psImageCopy (NULL, source->pixels,     PS_TYPE_F32);
        psImage *mask     = psImageCopy (NULL, source->maskView ? source->maskView : source->maskObj,  PS_TYPE_IMAGE_MASK);
        psImage *variance = psImageCopy (NULL, source->variance ? source->variance : source->pixels,   PS_TYPE_F32);
        pmModelSub (image, mask, model, PM_MODEL_OP_FUNC, maskVal);

        // re-normalize image and variance
        float Io = model->params->data.F32[PM_PAR_I0];
        psBinaryOp (image, image, "/", psScalarAlloc(Io, PS_TYPE_F32));
        psBinaryOp (variance, variance, "/", psScalarAlloc(Io*Io, PS_TYPE_F32));

        // we interpolate the image and variance - include the mask or not?
        // XXX why not the mask?
        // psImageInterpolation *interp = psImageInterpolationAlloc(mode, image, variance, mask, maskVal, 0.0, 0.0, badMask, poorMask, 0.0, 0);
        psImageInterpolation *interp = psImageInterpolationAlloc(mode, image, variance, NULL, 0xff, 0.0, 0.0, badMask, poorMask, 0.0, 0);
        psArrayAdd (input, 100, interp);

        // save the X,Y position for future reference
        xC->data.F32[xC->n] = model->params->data.F32[PM_PAR_XPOS];
        yC->data.F32[yC->n] = model->params->data.F32[PM_PAR_YPOS];
        psVectorExtend (xC, 100, 1);
        psVectorExtend (yC, 100, 1);

        xSize = PS_MAX (xSize, image->numCols);
        ySize = PS_MAX (ySize, image->numRows);

        // free up the excess references
        psFree (mask);
        psFree (image);
        psFree (variance);
        psFree (interp);
    }
    xSize = PS_MIN(xSize, 2*radiusMax+3);
    ySize = PS_MIN(ySize, 2*radiusMax+3);

    pmResiduals *resid = pmResidualsAlloc (xSize, ySize, xBin, yBin);
    psImageInit (resid->mask, 0);

    // x(resid) = (x(image) - Xo)*xBin + xCenter

    psVector *fluxes  = psVectorAlloc (input->n, PS_TYPE_F32);
    psVector *dfluxes = psVectorAlloc (input->n, PS_TYPE_F32);
    psVector *fmasks  = psVectorAlloc (input->n, PS_TYPE_VECTOR_MASK);

    // statistic to use to determine baseline for clipping
    psStats *fluxClip     = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    psStats *fluxClipDef  = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    // statistic to use to determine output flux
    // XXX make API to convert statOption for MEAN/MEDIAN in to corresponding STDEV?
    psStats *fluxStats    = psStatsAlloc (statOption | PS_STAT_SAMPLE_STDEV);
    psStats *fluxStatsDef = psStatsAlloc (statOption | PS_STAT_SAMPLE_STDEV);

    // Use psF64 to minimize overflow problems?
    psImage *A = psImageAlloc(3, 3, PS_TYPE_F64); // Least-squares matrix
    psVector *B = psVectorAlloc(3, PS_TYPE_F64); // Least-squares vector

    // Solve MODEL = R + x R_x + y R_y in pixel-by-pixel a least-squares sense
    // (If SPATIAL_ORDER == 0, just solve MODEL = R)
    for (int oy = 0; resid != NULL && oy < resid->Ro->numRows; oy++) {
        for (int ox = 0; ox < resid->Ro->numCols; ox++) {

            int nGoodPixel = 0;              // pixel is off the image

            // build the vector of data values for this output pixel
            for (int i = 0; i < input->n; i++) {

                psImageInterpolation *interp = input->data[i];

                // fractional image position
                float ix = (ox + 0.5 - resid->xCenter) / (float) xBin + xC->data.F32[i] - interp->image->col0;
                float iy = (oy + 0.5 - resid->yCenter) / (float) yBin + yC->data.F32[i] - interp->image->row0;

                mflux = 0;
                if (psImageInterpolate (&flux, &dflux, &mflux, ix, iy, interp) == PS_INTERPOLATE_STATUS_OFF) {
                    // This pixel is off the image
                    fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i] = badMask;
                } else {
                  fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i] = mflux; // XXX is mflux IMAGE or VECTOR type?
                }
                fluxes->data.F32[i] = flux;
                dfluxes->data.F32[i] = hypot(dflux, RESIDUAL_SOFTENING);
                if (isnan(flux)) {
                    fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i] = badMask;
                }
                if (isnan(dflux)) {
                    fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i] = badMask;
                }
                if (fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) {
                    nGoodPixel ++;
                }
            }

            // skip pixels with insufficient data
            bool validPixel = (SPATIAL_ORDER == 0) ? (nGoodPixel > 1) : (nGoodPixel > 3);
            if (!validPixel) {
                resid->Ro->data.F32[oy][ox] = 0.0;
                resid->Rx->data.F32[oy][ox] = 0.0;
                resid->Ry->data.F32[oy][ox] = 0.0;
                resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = badMask;
                continue;
            }

            // measure the robust median to determine a baseline reference value
            *fluxClip = *fluxClipDef;
            if (!psVectorStats (fluxClip, fluxes, NULL, fmasks, fmaskVal)) {
		psError(PSPHOT_ERR_CONFIG, false, "Error calculating residual stats");
		return false;
	    }
	    if (isnan(fluxClip->robustMedian)) {
                resid->Ro->data.F32[oy][ox] = 0.0;
                resid->Rx->data.F32[oy][ox] = 0.0;
                resid->Ry->data.F32[oy][ox] = 0.0;
                resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = badMask;
                continue;
	    }

            // mark input pixels which are more than N sigma from the median
            int nKeep = 0;
            for (int i = 0; i < fluxes->n; i++) {
                float delta = fluxes->data.F32[i] - fluxClip->robustMedian;
                float sigma = sqrt (dfluxes->data.F32[i]);
                float swing = fabs(delta) / sigma;

                // mask pixels which are out of range
                if (swing > nSigma) {
                    fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i] = clippedMask;
                }
                if (!fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i]) nKeep++;
            }

            if (SPATIAL_ORDER == 0) {
                // measure the desired statistic on the unclipped pixels
                *fluxStats = *fluxStatsDef;
                if (!psVectorStats (fluxStats, fluxes, NULL, fmasks, fmaskVal)) {
		    psError(PSPHOT_ERR_CONFIG, false, "Error calculating residual stats");
		    return false;
		}

		float radius = hypot((ox - 0.5*resid->Ro->numCols), (oy - 0.5*resid->Ro->numRows));
		if (radius > radiusMax) {
                  resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = 1;
		  continue;
                }

                resid->Ro->data.F32[oy][ox] = psStatsGetValue(fluxStats, statOption);
                resid->Rx->data.F32[oy][ox] = resid->Ry->data.F32[oy][ox] = 0.0;

		if (isnan(resid->Ro->data.F32[oy][ox])) {
		    resid->Ro->data.F32[oy][ox] = 0.0;
		    resid->Rx->data.F32[oy][ox] = 0.0;
		    resid->Ry->data.F32[oy][ox] = 0.0;
		    resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = badMask;
		    continue;
		}

                if (fabs(resid->Ro->data.F32[oy][ox]) < pixelSN*fluxStats->sampleStdev/sqrt(nKeep)) {
                  resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = 1;
                }
            } else {
                assert (SPATIAL_ORDER == 1);

		float radius = hypot((ox - 0.5*resid->Ro->numCols), (oy - 0.5*resid->Ro->numRows));
		if (radius > radiusMax) {
                  resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = 1;
		  continue;
                }

                psImageInit(A, 0.0);
                psVectorInit(B, 0.0);
                for (int i = 0; i < fluxes->n; i++) {
                    if (fmasks->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
                    B->data.F64[0] += fluxes->data.F32[i]/dfluxes->data.F32[i];
                    B->data.F64[1] += fluxes->data.F32[i]*xC->data.F32[i]/dfluxes->data.F32[i];
                    B->data.F64[2] += fluxes->data.F32[i]*yC->data.F32[i]/dfluxes->data.F32[i];

                    A->data.F64[0][0] += 1.0/dfluxes->data.F32[i];
                    A->data.F64[1][0] += xC->data.F32[i]/dfluxes->data.F32[i];
                    A->data.F64[2][0] += yC->data.F32[i]/dfluxes->data.F32[i];

                    A->data.F64[1][1] += PS_SQR(xC->data.F32[i])/dfluxes->data.F32[i];
                    A->data.F64[2][2] += PS_SQR(yC->data.F32[i])/dfluxes->data.F32[i];
                    A->data.F64[1][2] += xC->data.F32[i]*yC->data.F32[i]/dfluxes->data.F32[i];
                }

                A->data.F64[0][1] = A->data.F64[1][0];
                A->data.F64[0][2] = A->data.F64[2][0];
                A->data.F64[2][1] = A->data.F64[1][2];

                if (!psMatrixGJSolve(A, B)) {
		    resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = 1;
                    psWarning("Singular matrix solving for (y,x) = (%d,%d)'s residuals, masking", oy, ox);
		    continue;
                }

                resid->Ro->data.F32[oy][ox] = B->data.F64[0];
                resid->Rx->data.F32[oy][ox] = B->data.F64[1];
                resid->Ry->data.F32[oy][ox] = B->data.F64[2];

                float dRo = sqrt(A->data.F32[0][0]);

                if (fabs(resid->Ro->data.F32[oy][ox]) < pixelSN*dRo/sqrt(nKeep)) {
                  resid->mask->data.PM_TYPE_RESID_MASK_DATA[oy][ox] = 1;
		  resid->Ro->data.F32[oy][ox] = 0.0;
		  resid->Rx->data.F32[oy][ox] = 0.0;
		  resid->Ry->data.F32[oy][ox] = 0.0;
                }
            }
        }
    }

    psFree (A);
    psFree (B);

    psLogMsg ("psphot.pspsf", PS_LOG_DETAIL, "generate residuals for %ld objects: %f sec\n", input->n, psTimerMark ("psphot.residuals"));

    psFree (xC);
    psFree (yC);
    psFree (input);

    psFree (fluxes);
    psFree (dfluxes);
    psFree (fmasks);

    psFree (fluxStats);
    psFree (fluxStatsDef);
    psFree (fluxClip);
    psFree (fluxClipDef);

    if (resid != NULL && psTraceGetLevel("psphot") > 5) {
      psphotSaveImage (NULL, resid->Ro,     "resid.ro.fits");
      psphotSaveImage (NULL, resid->Rx,     "resid.rx.fits");
      psphotSaveImage (NULL, resid->Ry,     "resid.ry.fits");
      psphotSaveImage (NULL, resid->variance, "resid.wt.fits");
      psphotSaveImage (NULL, resid->mask,   "resid.mk.fits");
    }

    psf->residuals = resid;
    return (resid != NULL) ? true : false;
}
