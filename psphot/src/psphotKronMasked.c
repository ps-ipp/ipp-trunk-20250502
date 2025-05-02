# include "psphotInternal.h"
# ifndef ROUND
# define ROUND(X) ((int) ((X) + 0.5*SIGN(X)))
# endif

bool psphotKronMask (pmSource *source, psImage *kronMask, psImageMaskType markVal);
bool psphotKronMag (pmSource *source, float radius, float minKronRadius, psImageMaskType maskVal);

bool psphotKronMasked (pmConfig *config, const pmFPAview *view, const char *filerule)
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

        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->allSources;
        psAssert (sources, "missing sources?");

        pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
        // psAssert (psf, "missing psf?");

        if (!psphotKronMaskedReadout (config, recipe, view, readout, sources, psf)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure magnitudes for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

int psphotKapaChannel (int channel);
bool psphotVisualShowMask (int kapaFD, psImage *inImage, const char *name, int channel);
bool psphotVisualRangeImage (int kapaFD, psImage *inImage, const char *name, int channel, float min, float max);

bool psphotKronMaskedReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, pmReadout *readout, psArray *sources, pmPSF *psf) {

    bool status = false;

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping masked kron");
        return true;
    }

    psTimerStart ("psphot.kron");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    float RADIUS = psMetadataLookupF32 (&status, readout->analysis, "PSF_MOMENTS_RADIUS");
    if (!status) {
        RADIUS = psMetadataLookupF32 (&status, recipe, "PSF_MOMENTS_RADIUS");
    }

    float MIN_KRON_RADIUS = psMetadataLookupF32 (&status, readout->analysis, "MOMENTS_MIN_KRON");
    if (!status) {
        MIN_KRON_RADIUS = 0.25*RADIUS;
    }

    float EXT_FIT_MAX_RADIUS = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MAX_RADIUS");

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // psphotSaveImage (NULL, readout->mask, "kron.unmasked.fits");

    pmSourcePhotometryMode photMode = PM_SOURCE_PHOT_PSFONLY;

    // XXX tmp visualization
    // int kapa = psphotKapaChannel (1);

    // generate the mask image: increment counter for every source overlapping the pixel
    psImage *kronMask = psImageAlloc (readout->image->numCols, readout->image->numRows, PS_TYPE_S32);
    psImageInit (kronMask, 0);
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];
        if (!source->peak) continue; // XXX how can we have a peak-less source?

        // allocate space for moments
        if (!source->moments) continue;

	// replace object in image
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	}

	// save the PSF-based values
	// source->moments->KronFluxPSF    = source->moments->KronFlux;
	// source->moments->KronFluxPSFErr = source->moments->KronFluxErr;
	// source->moments->KronRadiusPSF  = source->moments->Mrf;

	// iterate to the window radius
	float windowRadius = RADIUS;
	for (int j = 0; j < 4; j++) {
	    // XXX use some S/N criterion to limit?
	    // if (source->moments->KronFlux / source->moments->KronFluxErr > 10.0) {
	    // windowRadius = PS_MAX (RADIUS, 10*source->moments->Mrf);
	    // }

	    // re-allocate image, weight, mask arrays for each peak with box big enough to fit BIG_RADIUS
	    pmSourceRedefinePixels (source, readout, source->peak->x, source->peak->y, windowRadius + 2);

	    // mask the pixels not contained by the footprint
	    // if (psphotMaskFootprint (readout, source, markVal)) {
	    // 	// psphotVisualShowMask (kapa, source->maskObj, "mask", 2);
	    // 	// psphotVisualRangeImage (kapa, source->pixels, "image", 1, -50, 300);
	    // 	// fprintf (stderr, "masked\n");
	    // }	    

	    // mask the pixels associated with near neighbors
	    if (psphotKronMask (source, kronMask, markVal)) {
		// psphotVisualShowMask (kapa, source->maskObj, "mask", 2);
		// psphotVisualRangeImage (kapa, source->pixels, "image", 1, -50, 300);
	     	// fprintf (stderr, "masked\n");
	    }	    
	    // this function populates moments->Mrf,KronFlux,KronFluxErr
	    // XXX what about KronInner, KronOuter, etc?
	    psphotKronMag (source, windowRadius, MIN_KRON_RADIUS, maskVal);
	    windowRadius = PS_MIN(PS_MAX(RADIUS, 4.0*source->moments->Mrf), EXT_FIT_MAX_RADIUS);
	    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));
	}
        pmSourceMagnitudes (source, psf, photMode, maskVal, markVal, source->apRadius);
	float kmag = -2.5*log10(source->moments->KronFlux);
	if (source->psfMag - kmag > 0.25) {
	    // psphotVisualShowMask (kapa, source->maskObj, "mask", 1);
	    // psphotVisualRangeImage (kapa, source->pixels, "image", 0, -50, 300);
	    // fprintf (stderr, "psf: %f, kron: %f, dmag: %f\n", source->psfMag, kmag, source->psfMag - kmag);
	    // fprintf (stderr, "continue\n");
	}
	// re-subtract the object, leave local sky
	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }

    // psphotSaveImage (NULL, readout->mask, "kron.masked.fits");
    // psphotSaveImage (NULL, kronMask, "kronmask.fits");

    psLogMsg ("psphot.kron", PS_LOG_DETAIL, "measure masked kron magnitudes : %f sec for %ld objects\n", psTimerMark ("psphot.kron"), sources->n);

    psFree (kronMask);
    return true;
}

# define WEIGHTED 0

bool psphotKronMag (pmSource *source, float radius, float minKronRadius, psImageMaskType maskVal) {

    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_FLOAT_LARGER_THAN(radius, 0.0, false);

    psF32 R2 = PS_SQR(radius);

# if (WEIGHTED)
    float rsigma2 = 0.5 / PS_SQR(radius / 4.0); // use a Gaussian window with sigma = R_window / 2
# endif

    // a note about coordinates: coordinates of objects throughout psphot refer to the primary
    // image coordinates.  the source->pixels image has an offset relative to its parent of
    // col0,row0: a pixel (x,y) in the primary image has coordinates of (x-col0, y-row0) in
    // this subimage.  we subtract off the peak coordinates, adjusted to this subimage, to have
    // minimal round-off error in the sums.  since these values are subtracted just to minimize
    // the dynamic range and are added back below, the exact value does not matter. these are
    // (int) so they can be used in the image index below.

    // Now calculate higher-order moments, using the above-calculated first moments to adjust coordinates
    // Xn  = SUM (x - xc)^n * (z - sky)

    psF32 RF = 0.0;
    psF32 RS = 0.0;

    // the peak position is less accurate but less subject to extreme deviations
    float dX = source->moments->Mx - source->peak->xf;
    float dY = source->moments->My - source->peak->yf;
    float dR = hypot(dX, dY);
    float Xo = (dR < 2.0) ? source->moments->Mx : source->peak->xf;
    float Yo = (dR < 2.0) ? source->moments->My : source->peak->yf;

    // center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
    // xCM, yCM from pixel coords to pixel index here.
    psF32 xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
    psF32 yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

    for (psS32 row = 0; row < source->pixels->numRows ; row++) {

	psF32 yDiff = row - yCM;
	if (fabs(yDiff) > radius) continue;

	psF32 *vPix = source->pixels->data.F32[row];

	psImageMaskType *vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[row];

	for (psS32 col = 0; col < source->pixels->numCols ; col++, vPix++) {
	    if (vMsk) {
		if (*vMsk & maskVal) {
		    vMsk++;
		    continue;
		}
		vMsk++;
	    }
	    if (isnan(*vPix)) continue;

	    psF32 xDiff = col - xCM;
	    if (fabs(xDiff) > radius) continue;

	    // radius is just a function of (xDiff, yDiff)
	    psF32 r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
	    if (r2 > R2) continue;

# if (WEIGHTED)
	    float z = r2 * rsigma2;
	    assert (z >= 0.0);
	    float weight  = exp(-z);
	    psF32 pDiff = *vPix * weight;
# else
	    psF32 pDiff = *vPix;
# endif

	    // Kron Flux uses the 1st radial moment (maybe Gaussian windowed?)
	    psF32 rf = pDiff * sqrt(r2);
	    psF32 rs = pDiff;

	    RF  += rf;
	    RS  += rs;
	}
    }

    // Saturate the 1st radial moment
    float Mrf = MAX(minKronRadius, RF/RS);
    if (sqrt(source->peak->detValue) < 10.0) {
	Mrf = MIN (radius, Mrf);
    }

    // Calculate the Kron magnitude (make this block optional?)
    float radKron  = 2.5*Mrf;
    float radKron2 = radKron*radKron;

    int nKronPix = 0;
    float Sum = 0.0;
    float Var = 0.0;

    for (psS32 row = 0; row < source->pixels->numRows ; row++) {

	psF32 yDiff = row - yCM;
	if (fabs(yDiff) > radKron) continue;

	psF32 *vPix = source->pixels->data.F32[row];
	psF32 *vWgt = source->variance->data.F32[row];

	psImageMaskType *vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[row];

	for (psS32 col = 0; col < source->pixels->numCols ; col++, vPix++, vWgt++) {
	    if (vMsk) {
		if (*vMsk & maskVal) {
		    vMsk++;
		    continue;
		}
		vMsk++;
	    }
	    if (isnan(*vPix)) continue;

	    psF32 xDiff = col - xCM;
	    if (fabs(xDiff) > radKron) continue;

	    // radKron is just a function of (xDiff, yDiff)
	    psF32 r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
	    if (r2 > radKron2) continue;

# if (WEIGHTED)
	    float z = r2 * rsigma2;
	    assert (z >= 0.0);
	    float weight  = exp(-z);
	    psF32 pDiff = *vPix * weight;
	    psF32 wDiff = *vWgt * weight;
# else
	    psF32 pDiff = *vPix;
	    psF32 wDiff = *vWgt;
# endif

	    Sum += pDiff;
	    Var += wDiff;
	    nKronPix ++;
	}
    }

    source->moments->Mrf = Mrf;
    source->moments->KronFlux    = Sum;
    source->moments->KronFluxErr = sqrt(Var);
    // source->moments->KronFlux    = Sum       * M_PI * PS_SQR(radKron) / nKronPix;
    // source->moments->KronFluxErr = sqrt(Var) * M_PI * PS_SQR(radKron) / nKronPix;

    return true;
}

enum {Y_P, Y_M, X_P, X_M};

bool psphotKronMask (pmSource *source, psImage *kronMask, psImageMaskType markVal) {

    int X, Y, e, dXs, dYs;

    if (!source->peak) return false;
    if (!source->peak->saddlePoints) return false;
    if (!source->peak->saddlePoints->n) return false;

    // return true;

    int xMax = source->maskObj->numCols;
    int yMax = source->maskObj->numRows;
    // int xOff = source->maskObj->col0;
    // int yOff = source->maskObj->row0;

    int Xo = ROUND(source->peak->xf) - source->maskObj->col0;
    int Yo = ROUND(source->peak->yf) - source->maskObj->row0;

    for (int i = 0; i < source->peak->saddlePoints->n; i++) {

	psVector *saddlePoint = source->peak->saddlePoints->data[i];
	int Xs = ROUND(saddlePoint->data.S32[0]) - source->maskObj->col0;
	int Ys = ROUND(saddlePoint->data.S32[1]) - source->maskObj->row0;
	
	// fprintf (stderr, "mask %d,%d @ %d,%d (%d,%d)\n", Xo, Yo, Xs, Ys, saddlePoint->data.S32[0], saddlePoint->data.S32[1]);

	// We want to mask the pixels between the edge of the image and the line perpendicular
	// to the connecting line.  Call dX,dY = (Xs-Xo,Ys-Yo).  There are 4 cases: 
	// +y : dY > 0, fabs(dY) > fabs(dX)
	// -y : dY < 0, fabs(dY) > fabs(dX)
	// +x : dX > 0, fabs(dX) > fabs(dY)
	// -x : dX < 0, fabs(dX) > fabs(dY)

	int dX = Xs - Xo;
	int dY = Ys - Yo;

	// +y : dY > 0, fabs(dY) > fabs(dX)
	// -y : dY < 0, fabs(dY) > fabs(dX)
	if (fabs(dY) > fabs(dX)) {
	    // points to right of saddle
	    Y = Ys;
	    e = 0;
	    dXs = (dY > 0) ? +dY : -dY;  // this forces dXs > 0
	    dYs = (dY > 0) ? -dX : +dX;
	    for (X = Xs; X < xMax; X++) {
		if (dY > 0) {
		    for (int Ym = Y; Ym < yMax; Ym++) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Ym][X] |= markVal;
			// kronMask->data.S32[Ym+yOff][X+xOff] = source->id;
		    }
		} else {
		    for (int Ym = Y; Ym >= 0; Ym--) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Ym][X] |= markVal;
			// kronMask->data.S32[Ym+yOff][X+xOff] = source->id;
		    }
		}
		e += dYs;
		int e2 = 2 * e;
		if (e2 > dXs) {
		    Y ++;
		    e -= dXs;
		}
		if (e2 < -dXs) {
		    Y --;
		    e += dXs;
		}
		if (Y >= yMax) break;
		if (Y < 0) break;
	    }
	    // points to left of saddle
	    Y = Ys;
	    e = 0;
	    for (X = Xs; X >= 0; X--) {
		if (dY > 0) {
		    for (int Ym = Y; Ym < yMax; Ym++) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Ym][X] |= markVal;
			// kronMask->data.S32[Ym+yOff][X+xOff] = source->id;
		    }
		} else {
		    for (int Ym = Y; Ym >= 0; Ym--) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Ym][X] |= markVal;
			// kronMask->data.S32[Ym+yOff][X+xOff] = source->id;
		    }
		}
		e -= dYs;
		int e2 = 2 * e;
		if (e2 > dXs) {
		    Y ++;
		    e -= dXs;
		}
		if (e2 < -dXs) {
		    Y --;
		    e += dXs;
		}
		if (Y >= yMax) break;
		if (Y < 0) break;
	    }
	} else {
	    // points to right of saddle
	    X = Xs;
	    e = 0;
	    dXs = (dX > 0) ? -dY : +dY;
	    dYs = (dX > 0) ? +dX : -dX;  // this forces dYs > 0
	    for (Y = Ys; Y < yMax; Y++) {
		if (dX > 0) {
		    for (int Xm = X; Xm < xMax; Xm++) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Y][Xm] |= markVal;
			// kronMask->data.S32[Y+yOff][Xm+xOff] = source->id;
		    }
		} else {
		    for (int Xm = X; Xm >= 0; Xm--) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Y][Xm] |= markVal;
			// kronMask->data.S32[Y+yOff][Xm+xOff] = source->id;
		    }
		}
		e += dXs;
		int e2 = 2 * e;
		if (e2 > dYs) {
		    X ++;
		    e -= dYs;
		}
		if (e2 < -dYs) {
		    X --;
		    e += dYs;
		}
		if (X >= yMax) break;
		if (X < 0) break;
	    }
	    // points to left of saddle
	    X = Xs;
	    e = 0;
	    for (Y = Ys; Y >= 0; Y--) {
		if (dX > 0) {
		    for (int Xm = X; Xm < xMax; Xm++) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Y][Xm] |= markVal;
			// kronMask->data.S32[Y+yOff][Xm+xOff] = source->id;
		    }
		} else {
		    for (int Xm = X; Xm >= 0; Xm--) {
			source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[Y][Xm] |= markVal;
			// kronMask->data.S32[Y+yOff][Xm+xOff] = source->id;
		    }
		}
		e -= dXs;
		int e2 = 2 * e;
		if (e2 > dYs) {
		    X ++;
		    e -= dYs;
		}
		if (e2 < -dYs) {
		    X --;
		    e += dYs;
		}
		if (X >= yMax) break;
		if (X < 0) break;
	    }
	}
    }
    return true;
}
