/** @file psastroLuminosityFunction.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define dMag 0.1
// XXX put this in config?

static void pmLumFuncFree (pmLumFunc *func) {

    if (func == NULL) return;

    return;
}

pmLumFunc *pmLumFuncAlloc () {

    pmLumFunc *func = (pmLumFunc *) psAlloc(sizeof(pmLumFunc));
    psMemSetDeallocator(func, (psFreeFunc) pmLumFuncFree);

    func->mMin = 0.0;
    func->mMax = 0.0;
    func->slope = 0.0;
    func->offset = 0.0;

    func->mPeak = 0.0;
    func->nPeak = 0;
    func->sPeak = 0;

    return func;
}

pmLumFunc *psastroLuminosityFunction (psArray *stars, pmLumFunc *rawFunc) {

    if (stars->n == 0) return NULL;

    // determine the max and min magnitude for the array of stars
    pmAstromObj *star = stars->data[0];
    double mMin = star->Mag;
    double mMax = star->Mag;
    for (int i = 0; i < stars->n; i++) {
        star = stars->data[i];
        mMin = PS_MIN (star->Mag, mMin);
        mMax = PS_MAX (star->Mag, mMax);
    }

    int nBin = 1 + (mMax - mMin) / dMag;
    if (nBin <= 1) {
        psLogMsg ("psastro", 4, "insufficient valid stars for this readout\n");
        return NULL;
    }

    // create a histogram of the magnitudes
    // bin[0] = mMin : mMin + dMag
    // bin[i] = mMin + i*dMag : mMin + (i+1)*dMag
    psVector *nMags = psVectorAlloc (nBin, PS_TYPE_F32);
    psVectorInit (nMags, 0);

    for (int i = 0; i < stars->n; i++) {
        star = stars->data[i];
        if (!isfinite(star->Mag)) continue;
        int bin = (star->Mag - mMin) / dMag;
        if (bin < 0) psAbort("bin cannot be negative!");
        if (bin >= nBin) psAbort("bin cannot be > %d!", nBin);
        nMags->data.F32[bin] += 1.0;
    }

    // find the peak and position
    int iPeak = 0;
    int nPeak = 0;
    int sPeak = 0;
    for (int i = 0; i < nMags->n; i++) {
        if (nMags->data.F32[i] < nPeak) continue;
        iPeak = i;
        nPeak = nMags->data.F32[i];
        sPeak += nPeak;
    }

    psVector *lnMag = psVectorAllocEmpty (nBin, PS_TYPE_F32);
    psVector *Mag = psVectorAllocEmpty (nBin, PS_TYPE_F32);

    // create 2 vectors represnting the dlogN/dlogS line
    // exclude bins with no stars
    // exclude points after the peak with N < 0.8*peak
    int n = 0;
    for (int i = 0; i < nMags->n; i++) {
        if (nMags->data.F32[i] < 1) continue;
        if ((i > iPeak) && (nMags->data.F32[i] < 0.8*nPeak)) continue;
        lnMag->data.F32[n] = log10 (nMags->data.F32[i]);
        Mag->data.F32[n] = mMin + (i + 0.5)*dMag;
        n++;
    }
    lnMag->n = n;
    Mag->n = n;
    psLogMsg ("psastro", 4, "fitting %d points to luminosity function\n", n);

    psVector *mask = psVectorAlloc (Mag->n, PS_TYPE_VECTOR_MASK);
    psVectorInit (mask, 0);

    psPolynomial1D *line = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 1);
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    stats->clipSigma = 3.0;
    stats->clipIter = 3;

    if (!psVectorClipFitPolynomial1D(line, stats, mask, 0xff, lnMag, NULL, Mag)) {
        psLogMsg ("psastro", 4, "Failed to fit the luminosity function\n");
        return(NULL);
    }

    // find min and max unmasked magnitudes
    double mMinValid = NAN;
    double mMaxValid = NAN;
    for (int i = 0; i < Mag->n; i++) {
        if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
        if (isnan(mMinValid) || (Mag->data.F32[i] < mMinValid)) {
            mMinValid = Mag->data.F32[i];
        }
        if (isnan(mMaxValid) || (Mag->data.F32[i] > mMaxValid)) {
            mMaxValid = Mag->data.F32[i];
        }
    }

    psLogMsg ("psastro", 4, "logN = %f + %f mag\n", line->coeff[0], line->coeff[1]);
    psLogMsg ("psastro", 4, "logN vs mag residuals: %f +/- %f\n", stats->sampleMedian, stats->sampleStdev);

    pmLumFunc *lumFunc = pmLumFuncAlloc ();
    lumFunc->mMin = mMinValid;
    lumFunc->mMax = mMaxValid;
    lumFunc->offset = line->coeff[0];
    lumFunc->slope = line->coeff[1];
    lumFunc->mPeak = mMin + (iPeak + 0.5)*dMag;
    lumFunc->nPeak = nPeak;
    lumFunc->sPeak = sPeak;

    pmAstromVisualPlotLuminosityFunction(lnMag, Mag, lumFunc, rawFunc);

    psFree (lnMag);
    psFree (nMags);
    psFree (Mag);
    psFree (mask);
    psFree (line);
    psFree (stats);

    return (lumFunc);
}


