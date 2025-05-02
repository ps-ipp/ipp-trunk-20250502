
/***************

  In the process of developing psphot, I have written a bunch of code that has been used
  for some tests or for a period, but which we no longer think is appropriate.  I am
  trying to clean up the kruft of old psphot bits and will but various such functions in
  this file if they are worth keeping

****************/

bool psphotMaskCosmicRayFootprintCheck (psArray *sources);


// maybe move this into psModules, pmFootprints?
bool psphotMaskCosmicRayFootprintCheck (psArray *sources) {
#ifdef CHECK_FOOTPRINTS
    // This gets really expensive for complex images
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        pmPeak *peak = source->peak;
        pmFootprint *footprint = peak->footprint;
        if (!footprint) continue;
        for (int j = 0; j < footprint->spans->n; j++) {
            pmSpan *sp = footprint->spans->data[j];
            psAssert (sp, "missing span");
        }
    }
#endif
    return true;
}

// mark the isophotal boundary
bool psphotMaskCosmicRayIsophot (pmSource *source, psImageMaskType maskVal, psImageMaskType crMask) {

    source->mode |= PM_SOURCE_MODE_CR_LIMIT;
    pmPeak *peak = source->peak;
    psAssert (peak, "NULL peak");

    psImage *mask   = source->maskView;
    psImage *pixels = source->pixels;
    psImage *variance = source->variance;

    // XXX This should be a recipe variable
# define SN_LIMIT 5.0

    int xo = peak->x - pixels->col0;
    int yo = peak->y - pixels->row0;

    // mark the pixels in this row to the left, then the right
    for (int ix = xo; ix >= 0; ix--) {
        float SN = pixels->data.F32[yo][ix] / sqrt(variance->data.F32[yo][ix]);
        if (SN > SN_LIMIT) {
            mask->data.PS_TYPE_IMAGE_MASK_DATA[yo][ix] |= crMask;
        }
    }
    for (int ix = xo + 1; ix < pixels->numCols; ix++) {
        float SN = pixels->data.F32[yo][ix] / sqrt(variance->data.F32[yo][ix]);
        if (SN > SN_LIMIT) {
            mask->data.PS_TYPE_IMAGE_MASK_DATA[yo][ix] |= crMask;
        }
    }

    // for each of the neighboring rows, mark the high pixels if they have a marked neighbor
    // first go up:
    for (int iy = PS_MIN(yo, mask->numRows-2); iy >= 0; iy--) {
        // mark the pixels in this row to the left, then the right
        for (int ix = 0; ix < pixels->numCols; ix++) {
            float SN = pixels->data.F32[iy][ix] / sqrt(variance->data.F32[iy][ix]);
            if (SN < SN_LIMIT) continue;

            bool valid = false;
            valid |= (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+1][ix] & crMask);
            valid |= (ix > 0) ? (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+1][ix-1] & crMask) : 0;
            valid |= (ix <= mask->numCols) ? (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+1][ix+1] & crMask) : 0;

            if (!valid) continue;
            mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= crMask;
        }
    }
    // next go down:
    for (int iy = PS_MIN(yo+1, mask->numRows-1); iy < pixels->numRows; iy++) {
        // mark the pixels in this row to the left, then the right
        for (int ix = 0; ix < pixels->numCols; ix++) {
            float SN = pixels->data.F32[iy][ix] / sqrt(variance->data.F32[iy][ix]);
            if (SN < SN_LIMIT) continue;

            bool valid = false;
            valid |= (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy-1][ix] & crMask);
            valid |= (ix > 0) ? (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy-1][ix-1] & crMask) : 0;
            valid |= (ix <= mask->numCols) ? (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy-1][ix+1] & crMask) : 0;

            if (!valid) continue;
            mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= crMask;
        }
    }
    return true;
}

// This attempt to mask the cosmic rays used the isophotal boundary
bool psphotMaskCosmicRay_V1 (psImage *mask, pmSource *source, psImageMaskType maskVal, psImageMaskType crMask) {

    // replace the source flux
    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);

    // flag this as a CR
    source->mode |= PM_SOURCE_MODE_CR_LIMIT;
    pmPeak *peak = source->peak;
    psAssert (peak, "NULL peak");

    // grab the matching footprint
    pmFootprint *footprint = peak->footprint;
    if (!footprint) {
      psTrace("psphot.czw",2,"Using isophot CR mask code.");

        // if we have not footprint, use the old code to mask by isophot
        psphotMaskCosmicRayIsophot (source, maskVal, crMask);
        return true;
    }

    if (!footprint->spans) {
      psTrace("psphot.czw",2,"Using isophot CR mask code.");

        // if we have no footprint, use the old code to mask by isophot
        psphotMaskCosmicRayIsophot (source, maskVal, crMask);
        return true;
    }
    psphotMaskCosmicRayIsophot (source, maskVal, crMask);
    // mask all of the pixels covered by the spans of the footprint
    for (int j = 1; j < footprint->spans->n; j++) {
        pmSpan *span1 = footprint->spans->data[j];

        int iy = span1->y;
        int xs = span1->x0;
        int xe = span1->x1;

        for (int ix = xs; ix < xe; ix++) {
            mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= crMask;
        }
    }
    return true;
}

// given the PSF ellipse parameters, navigate around the 1sigma contour, return the total
// deviation in sigmas.  This is measured on the residual image - should we ignore negative
// deviations?  NOTE: This function was an early attempt to classify extended objects, and is
// no longer used by psphot.
float psphotModelContour(const psImage *image, const psImage *variance, const psImage *mask,
                         psImageMaskType maskVal, const pmModel *model, float Ro)
{
    psF32 *PAR = model->params->data.F32; // Model parameters
    float sxx = PAR[PM_PAR_SXX], sxy = PAR[PM_PAR_SXY], syy = PAR[PM_PAR_SYY]; // Ellipse parameters

    // We treat the contour as an ellipse:
    // Ro = (x / SXX)^2 + (y / SYY)^2 + x y SXY
    // y^2 (1/SYY^2) + y (x SXY) + (x / SXX)^2 - Ro = 0;
    // This is a quadratic, Ay^2 + By + C with A = 1/SYY^2, B = x*SXY, C = (x / SXX)^2 - Ro
    // The solution is y = [-B +/- sqrt (B^2 - 4 A C)] / [2 A], so:
    // y = [-(x SXY) +/- sqrt ((x SXY)^2 - 4 (1/SYY^2) ((x/SXX)^2 - Ro))] * [SYY^2 / 2]

    // min/max value of x is where B^2 - 4AC = 0; solve this for x
    float Q = Ro * PS_SQR(sxx) / (1.0 - PS_SQR(sxx * syy * sxy) / 4.0);
    if (Q < 0.0) {
        // ellipse is imaginary
        return NAN;
    }

    int radius = sqrtf(Q) + 0.5;        // Radius of ellipse
    int nPts = 0;                       // Number of points in ellipse
    float nSigma = 0.0;                 //

    for (int x = -radius; x <= radius; x++) {
        // Polynomial coefficients
        // XXX Should we be using the centre of the pixel as x or x+0.5?
        float A = PS_SQR (1.0 / syy);
        float B = x * sxy;
        float C = PS_SQR (x / sxx) - Ro;
        float T = PS_SQR(B) - 4*A*C;
        if (T < 0.0) {
            continue;
        }

        // y position in source frame
        float yP = (-B + sqrt (T)) / (2.0 * A);
        float yM = (-B - sqrt (T)) / (2.0 * A);

        // Get the closest pixel positions (image frame)
        int xPix  = x  + PAR[PM_PAR_XPOS] - image->col0 + 0.5;
        int yPixM = yM + PAR[PM_PAR_YPOS] - image->row0 + 0.5;
        int yPixP = yP + PAR[PM_PAR_YPOS] - image->row0 + 0.5;

        if (xPix < 0 || xPix >= image->numCols) {
            continue;
        }

        if (yPixM >= 0 && yPixM < image->numRows &&
            !(mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[yPixM][xPix] & maskVal))) {
            float dSigma = image->data.F32[yPixM][xPix] / sqrtf(variance->data.F32[yPixM][xPix]);
            nSigma += dSigma;
            nPts++;
        }

        if (yPixM == yPixP) {
            continue;
        }

        if (yPixP >= 0 && yPixP < image->numRows &&
            !(mask && (mask->data.PS_TYPE_IMAGE_MASK_DATA[yPixP][xPix] & maskVal))) {
            float dSigma = image->data.F32[yPixP][xPix] / sqrtf(variance->data.F32[yPixP][xPix]);
            nSigma += dSigma;
            nPts++;
        }
    }
    nSigma /= nPts;
    return nSigma;
}

// this was an old attempt to identify cosmic rays based on the peak curvature
bool psphotSourcePeakCurvature (pmReadout *readout, psArray *sources, psphotSourceSizeOptions *options) {

    // classify the sources based on the CR test (place this in a function?)
    // XXX use an internal flag to mark sources which have already been measured
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

        // skip source if it was already measured
        if (source->tmpFlags & PM_SOURCE_TMPF_SIZE_MEASURED) {
            psTrace("psphot", 7, "Not calculating source size since it has already been measured\n");
            continue;
        }

        // source must have been subtracted
        if (!(source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED)) {
            source->mode |= PM_SOURCE_MODE_SIZE_SKIPPED;
            psTrace("psphot", 7, "Not calculating source size since source is not subtracted\n");
            continue;
        }

        psF32 **resid  = source->pixels->data.F32;
        psF32 **variance = source->variance->data.F32;
        psImageMaskType **mask = source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

        // Integer position of peak
        int xPeak = source->peak->xf - source->pixels->col0 + 0.5;
        int yPeak = source->peak->yf - source->pixels->row0 + 0.5;

        // Skip sources which are too close to a boundary.  These are mostly caught as DEFECT
        if (xPeak < 1 || xPeak > source->pixels->numCols - 2 ||
            yPeak < 1 || yPeak > source->pixels->numRows - 2) {
            psTrace("psphot", 7, "Not calculating crNsigma due to edge\n");
            continue;
        }

        // Skip sources with masked pixels.  These are mostly caught as DEFECT
        bool keep = true;
        for (int iy = -1; (iy <= +1) && keep; iy++) {
            for (int ix = -1; (ix <= +1) && keep; ix++) {
                if (mask[yPeak+iy][xPeak+ix] & options->maskVal) {
                    keep = false;
                }
            }
        }
        if (!keep) {
            psTrace("psphot", 7, "Not calculating crNsigma due to masked pixels\n");
            continue;
        }

        // Compare the central pixel with those on either side, for the four possible lines through it.

        // Soften variances (add systematic error)
        float softening = options->soft * PS_SQR(source->peak->rawFlux); // Softening for variances

        // Across the middle: y = 0
        float cX = 2*resid[yPeak][xPeak]   - resid[yPeak+0][xPeak-1]  - resid[yPeak+0][xPeak+1];
        float dcX = 4*variance[yPeak][xPeak] + variance[yPeak+0][xPeak-1] + variance[yPeak+0][xPeak+1];
        float nX = cX / sqrtf(dcX + softening);

        // Up the centre: x = 0
        float cY = 2*resid[yPeak][xPeak]   - resid[yPeak-1][xPeak+0]  - resid[yPeak+1][xPeak+0];
        float dcY = 4*variance[yPeak][xPeak] + variance[yPeak-1][xPeak+0] + variance[yPeak+1][xPeak+0];
        float nY = cY / sqrtf(dcY + softening);

        // Diagonal: x = y
        float cL = 2*resid[yPeak][xPeak]   - resid[yPeak-1][xPeak-1]  - resid[yPeak+1][xPeak+1];
        float dcL = 4*variance[yPeak][xPeak] + variance[yPeak-1][xPeak-1] + variance[yPeak+1][xPeak+1];
        float nL = cL / sqrtf(dcL + softening);

        // Diagonal: x = - y
        float cR = 2*resid[yPeak][xPeak]   - resid[yPeak+1][xPeak-1]  - resid[yPeak-1][xPeak+1];
        float dcR = 4*variance[yPeak][xPeak] + variance[yPeak+1][xPeak-1] + variance[yPeak-1][xPeak+1];
        float nR = cR / sqrtf(dcR + softening);

        // P(chisq > chisq_obs; Ndof) = gamma_Q (Ndof/2, chisq/2)
        // Ndof = 4 ? (four measurements, no free parameters)
        // XXX this value is going to be biased low because of systematic errors.
        // we need to calibrate it somehow
        // source->psfProb = gsl_sf_gamma_inc_Q (2, 0.5*chisq);

        // not strictly accurate: overcounts the chisq contribution from the center pixel (by
        // factor of 4); also biases a bit low if any pixels are masked
        // XXX I am not sure I want to keep this value...
        source->psfChisq = PS_SQR(nX) + PS_SQR(nY) + PS_SQR(nL) + PS_SQR(nR);

        float fCR = 0.0;
        int nCR = 0;
        if (nX > 0.0) {
            fCR += nX;
            nCR ++;
        }
        if (nY > 0.0) {
            fCR += nY;
            nCR ++;
        }
        if (nL > 0.0) {
            fCR += nL;
            nCR ++;
        }
        if (nR > 0.0) {
            fCR += nR;
            nCR ++;
        }
        source->crNsigma  = (nCR > 0)  ? fCR / nCR : 0.0;
        source->tmpFlags |= PM_SOURCE_TMPF_SIZE_MEASURED;

        if (!isfinite(source->crNsigma)) {
            continue;
        }

        // this source is thought to be a cosmic ray.  flag the detection and mask the pixels
        if (source->crNsigma > options->nSigmaCR) {
            source->mode |= PM_SOURCE_MODE_CR_LIMIT;
            // XXX still testing... : psphotMaskCosmicRay (readout->mask, source, maskVal, crMask);
            // XXX acting strange... psphotMaskCosmicRay_Old (source, maskVal, crMask);
        }
    }

    // now that we have masked pixels associated with CRs, we can grow the mask
    if (options->grow > 0) {
        bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading for psImageConvolveMask
        psImage *newMask = psImageConvolveMask(NULL, readout->mask, options->crMask, options->crMask, -options->grow, options->grow, -options->grow, options->grow);
        psImageConvolveSetThreads(oldThreads);
        if (!newMask) {
            psError(PS_ERR_UNKNOWN, false, "Unable to grow CR mask");
            return false;
        }
        psFree(readout->mask);
        readout->mask = newMask;
    }
    return true;
}

