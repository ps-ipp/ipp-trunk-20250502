#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include "psMemory.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psVector.h"
#include "psImage.h"
#include "psBinaryOp.h"
#include "psStats.h"
#include "psAssert.h"
#include "psPolynomial.h"
#include "psMinimizePolyFit.h"
#include "psCoord.h"
#include "psPolynomialUtils.h"

bool psVectorChiClipFitPolynomial4D(
    psPolynomial4D *poly,
    psStats *stats,
    const psVector *mask,
    psVectorMaskType maskValue,
    const psVector *f,
    const psVector *fErr,
    const psVector *x,
    const psVector *y,
    const psVector *z,
    const psVector *t)
{
    PS_ASSERT_POLY_NON_NULL(poly, NULL);
    PS_ASSERT_POLY_TYPE(poly, PS_POLYNOMIAL_ORD, NULL);
    PS_ASSERT_PTR_NON_NULL(stats, NULL);
    PS_ASSERT_VECTOR_NON_NULL(f, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(f, NULL);
    if (mask != NULL) {
        PS_ASSERT_VECTORS_SIZE_EQUAL(f, mask, NULL);
        PS_ASSERT_VECTOR_TYPE(mask, PS_TYPE_VECTOR_MASK, NULL);
    }
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, x, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(x, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, y, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(y, NULL);
    PS_ASSERT_VECTOR_NON_NULL(z, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, z, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(z, NULL);
    PS_ASSERT_VECTOR_NON_NULL(t, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(f, t, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(t, NULL);
    PS_ASSERT_VECTOR_NON_NULL(fErr, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(fErr, mask, NULL);
    PS_ASSERT_VECTOR_TYPE_F32_OR_F64(fErr, NULL);

    // clipping range defined by min and max and/or clipSigma
    float minClipSigma;
    float maxClipSigma;
    if (isfinite(stats->max)) {
        maxClipSigma = +fabs(stats->max);
    } else {
        maxClipSigma = +fabs(stats->clipSigma);
    }
    if (isfinite(stats->min)) {
        minClipSigma = -fabs(stats->min);
    } else {
        minClipSigma = -fabs(stats->clipSigma);
    }
    psVector *fit   = NULL;
    psVector *resid = psVectorAlloc (x->n, PS_TYPE_F64);

    // eventual expansion: user supplies one of various stats option pairs,
    // eg (SAMPLE_MEAN | SAMPLE_STDEV) and the correct pair is used to
    // evaluate the clipping sigma
    // for now, for the SAMPLE_MEDIAN and SAMPLE_STDEV to be used
    stats->options |= (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);

    for (int N = 0; N < stats->clipIter; N++) {
        int Nkeep = 0;

        if (!psVectorFitPolynomial4D (poly, mask, maskValue, f, fErr, x, y, z, t)) {
            psError(PS_ERR_UNKNOWN, true, "Could not fit a polynomial to the data.  Returning NULL.\n");
            psFree (resid);
            return false;
        }

        fit = psPolynomial4DEvalVector (poly, x, y, z, t);
        if (fit == NULL) {
            psError(PS_ERR_UNKNOWN, false, "Could not call psPolynomial4DEvalVector().  Returning NULL.\n");
            psFree(resid);
            return false;
        }

        resid = (psVector *) psBinaryOp (resid, (void *) f, "-", (void *) fit);

        if (!psVectorStats (stats, resid, NULL, mask, maskValue)) {
            psError(PS_ERR_UNKNOWN, false, "failed to measure vector stats");
            psFree (fit);
            psFree (resid);
            return false;
        }
        psTrace (__func__, 5, "resid stats: %f +/- %f\n", stats->sampleMedian, stats->sampleStdev);

        // set mask if pts are not valid
        // we are masking out any point which is out of range
        // recovery is not allowed with this scheme
        for (int i = 0; i < resid->n; i++) {
            if ((mask != NULL) && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) {
                continue;
            }
            float sigma = hypot (psVectorGet (fErr, i), stats->sampleStdev);
            if (resid->data.F64[i] - stats->sampleMedian > sigma*maxClipSigma) {
                if (mask != NULL) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                }
                continue;
            }
            if (resid->data.F64[i] - stats->sampleMedian < sigma*minClipSigma) {
                if (mask != NULL) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                }
                continue;
            }
            Nkeep ++;
        }

        psTrace (__func__, 4, "keeping %d of %ld pts for fit\n", Nkeep, x->n);
        stats->clippedNvalues = Nkeep;
        psFree (fit);
    }
    // Free local temporary variables
    psFree (resid);

    return true;
}

// this function expects x,y in parent coords
// XXX add a mask, fit only the valid pixels
// XXX determine the errors, and propagate to the output of psImageBicubeMin
// XXX x,y are the image pixel index, but the fit is implied for the image pixel coordinates (0.5,0.5 center)
// this solution is determined assuming constant weight per pixel
psPolynomial2D *psImageBicubeFit(const psImage *image, int x, int y)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);

    int ix = x - image->col0;
    int iy = y - image->row0;

    PS_ASSERT_INT_WITHIN_RANGE(ix, 1, image->numCols - 1, NULL);
    PS_ASSERT_INT_WITHIN_RANGE(iy, 1, image->numRows - 1, NULL);

    psF32 *Fm = &image->data.F32[iy - 1][ix];
    psF32 *Fo = &image->data.F32[iy + 0][ix];
    psF32 *Fp = &image->data.F32[iy + 1][ix];

    double Fxm = Fm[-1] + Fo[-1] + Fp[-1];
    double Fxp = Fm[+1] + Fo[+1] + Fp[+1];
    double Fym = Fm[-1] + Fm[+0] + Fm[+1];
    double Fyp = Fp[-1] + Fp[+0] + Fp[+1];
    double Foo = Fym + Fyp + Fo[-1] + Fo[+0] + Fo[+1];

    psPolynomial2D *poly = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, 2, 2);
    poly->coeffMask[2][2] = PS_POLY_MASK_SET;
    poly->coeffMask[1][2] = PS_POLY_MASK_SET;
    poly->coeffMask[2][1] = PS_POLY_MASK_SET;

    poly->coeff[0][0] = Foo*(5.0/9.0) - (Fxp + Fxm)/3.0 - (Fyp + Fym)/3.0 ;

    poly->coeff[1][0] = (Fxp - Fxm)/6.0;
    poly->coeff[0][1] = (Fyp - Fym)/6.0;

    poly->coeff[2][0] = (Fxp + Fxm)/2.0 - Foo/3.0;
    poly->coeff[0][2] = (Fyp + Fym)/2.0 - Foo/3.0;

    poly->coeff[1][1] = (Fp[+1] + Fm[-1] - Fm[+1] - Fp[-1])/4.0;

    return poly;
}

// XXX min position is relative to the pixel index; in psModules/src/objects/pmPeaks.c:AddPeak, we offset by 0.5,0.5
// to get to pixel coordinates...
psPlane psImageBicubeMin(const psPolynomial2D *poly)
{
    psPlane min = { NAN, NAN, NAN, NAN };   // Minimum value to return

    PS_ASSERT_PTR_NON_NULL(poly, min);
    PS_ASSERT_INT_EQUAL(poly->nX, 2, min);
    PS_ASSERT_INT_EQUAL(poly->nY, 2, min);

    double det = 4*poly->coeff[2][0]*poly->coeff[0][2] - PS_SQR(poly->coeff[1][1]); // Determinant
    double xn = (poly->coeff[1][1]*poly->coeff[0][1] - 2*poly->coeff[0][2]*poly->coeff[1][0]);
    double yn = (poly->coeff[1][1]*poly->coeff[1][0] - 2*poly->coeff[2][0]*poly->coeff[0][1]);

    min.x = xn / det;
    min.y = yn / det;

    // sigma_xn^2 / xn^2 = sigma_11^2 / C11

    // without a supplied error, we calculate the normalized error
    double fdetErr2 = 0.5/PS_SQR(poly->coeff[0][2]) + 0.5/PS_SQR(poly->coeff[2][0]) + 4.0/(6.0*PS_SQR(poly->coeff[1][1]));
    double fxnErr2 = 1.0/(6.0*PS_SQR(poly->coeff[0][1])) + 1.0/(6.0*PS_SQR(poly->coeff[1][0])) + 1.0/(6.0*PS_SQR(poly->coeff[1][1])) + 0.5/PS_SQR(poly->coeff[0][2]);
    double fynErr2 = 1.0/(6.0*PS_SQR(poly->coeff[0][1])) + 1.0/(6.0*PS_SQR(poly->coeff[1][0])) + 1.0/(6.0*PS_SQR(poly->coeff[1][1])) + 0.5/PS_SQR(poly->coeff[2][0]);

    min.xErr = fabs(min.x) * sqrt(fxnErr2 + fdetErr2);
    min.yErr = fabs(min.y) * sqrt(fynErr2 + fdetErr2);

    return min;
}

psPolynomial2D *psPolynomial2D_dX (psPolynomial2D *out, psPolynomial2D *poly)
{
    int nXout = poly->nX - 1;
    int nYout = poly->nY;

    if (out == poly) {
        psError(PS_ERR_UNKNOWN, false, "cannot assign output to input polynomial");
        return NULL;
    }

    if (poly->nX == 0)
        return NULL;

    if (out == NULL) {
        out = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, nXout, nYout);
    } else {
        psPolynomial2DRecycle (out, poly->type, nXout, nYout);
    }

    for (int i = 0; i < nXout + 1; i++) {
        for (int j = 0; j < nYout + 1; j++) {
            out->coeffMask[i][j] = poly->coeffMask[i+1][j];
            out->coeff[i][j] = poly->coeff[i+1][j] * (i+1);
        }
    }
    return out;
}

psPolynomial2D *psPolynomial2D_dY (psPolynomial2D *out, psPolynomial2D *poly)
{
    int nXout = poly->nX;
    int nYout = poly->nY - 1;

    if (out == poly) {
        psError(PS_ERR_UNKNOWN, false, "cannot assign output to input polynomial");
        return NULL;
    }

    if (poly->nY == 0)
        return NULL;

    if (out == NULL) {
        out = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, nXout, nYout);
    } else {
        psPolynomial2DRecycle (out, poly->type, nXout, nYout);
    }

    for (int i = 0; i < nXout + 1; i++) {
        for (int j = 0; j < nYout + 1; j++) {
            out->coeffMask[i][j] = poly->coeffMask[i][j+1];
            out->coeff[i][j] = poly->coeff[i][j+1] * (j+1);
        }
    }
    return out;
}
