/** @file  psImageMapFit.c
 *
 *  @brief Functions define a 2d coarse representation of a finer 2D field
 *
 *  @ingroup Image
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:37 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "psError.h"
#include "psAbort.h"
#include "psTrace.h"

// #include "psFits.h"
#include "psAssert.h"
#include "psRegion.h"
// #include "psFitsImage.h"

#include "psMemory.h"
#include "psVector.h"
#include "psImage.h"
#include "psMatrix.h"
#include "psStats.h"
#include "psImageBinning.h"
#include "psImageStructManip.h"
#include "psImageMap.h"
#include "psSparse.h"
// #include "psImagePixelInterpolate.h"
// #include "psImageUnbin.h"

#include "psImageMapFit.h"


// given a randomly-sampled field of values & weights at points: (f, df) @ (x, y), find the
// best fit image from which Bilinear interpolation yields the input field.  The fitted image
// consists of a grid of values g(n,m) at coordinates (n,m).

// relationship between x,y and n,m coordinates:

// map defines the output image dimensions and scaling.
bool psImageMapFit(bool *pGoodFit, psImageMap *map, const psVector *mask, psVectorMaskType maskValue,
                   const psVector *x, const psVector *y, const psVector *f, const psVector *df)
{
    // XXX Add Asserts

    *pGoodFit = false;

    // dimensions of the output map image
    int Nx = map->binning->nXruff;
    int Ny = map->binning->nYruff;

    // no spatial information, just calculate mean & stdev
    if ((Nx == 1) && (Ny == 1)) {
        psStatsInit(map->stats);

        // the user has supplied one of various stats option pairs,
        psStatsOptions mean = psStatsMeanOption(map->stats->options);
        psStatsOptions stdev = psStatsStdevOption(map->stats->options);
        if (!psStatsSingleOption(mean)) {
            psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
            return false;
        }
        if (!psStatsSingleOption(stdev)) {
            psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
            return false;
        }

        // XXX does ROBUST_MEDIAN work with weight?
        if (!psVectorStats(map->stats, f, NULL, mask, maskValue)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	    return false;
	}

        map->map->data.F32[0][0]   = psStatsGetValue(map->stats, mean);
        map->error->data.F32[0][0] = psStatsGetValue(map->stats, stdev);
        if (isfinite(map->map->data.F32[0][0]) && isfinite( map->error->data.F32[0][0])) {
            *pGoodFit = true;
        }
        return true;
    }

    if (Nx == 1) {
        bool status;
        status = psImageMapFit1DinY (pGoodFit, map, mask, maskValue, x, y, f, df);
        return status;
    }
    if (Ny == 1) {
        bool status;
        status = psImageMapFit1DinX (pGoodFit, map, mask, maskValue, x, y, f, df);
        return status;
    }

    // set up the redirection table so we can use sA[-1][-1], etc
    // XXX psKernel does this for you --- PAP.
    float SAm[3][3], *SAv[3], **sA;
    // float TAm[3][3], *TAv[3], **tA;

    for (int i = 0; i < 3; i++) {
        SAv[i] = SAm[i] + 1;
        // TAv[i] = TAm[i] + 1;
    }
    sA = SAv + 1;
    // tA = TAv + 1;

    // elements of the matrix equation Ax = B; we are solving for the vector x
    psImage *A = psImageAlloc (Nx*Ny, Nx*Ny, PS_TYPE_F32);
    psVector *B = psVectorAlloc (Nx*Ny, PS_TYPE_F32);

    psImageInit (A, 0.0);
    psVectorInit (B, 0.0);
    
    // we are looping over the Nx,Ny image map elements;
    // the matrix equation contains Nx*Ny rows and columns
    // for (int n = 1; n < Nx - 1; n++) {
    // for (int m = 1; m < Ny - 1; m++) {

    // float Total = 0.0;
    for (int n = 0; n < Nx; n++) {
        for (int m = 0; m < Ny; m++) {
            // define & init summing variables
            float rx_rx_ry_ry = 0;
            float rx_rx_dy_ry = 0;
            float dx_rx_ry_ry = 0;
            float dx_rx_dy_ry = 0;
            float fi_rx_ry    = 0;
            float rx_rx_py_py = 0;
            float rx_rx_qy_py = 0;
            float dx_rx_py_py = 0;
            float dx_rx_qy_py = 0;
            float fi_rx_py    = 0;
            float px_px_ry_ry = 0;
            float px_px_dy_ry = 0;
            float qx_px_ry_ry = 0;
            float qx_px_dy_ry = 0;
            float fi_px_ry    = 0;
            float px_px_py_py = 0;
            float px_px_qy_py = 0;
            float qx_px_py_py = 0;
            float qx_px_qy_py = 0;
            float fi_px_py    = 0;

            // generate the sums for the fitting matrix element I,J
            // I = n + nX*m
            // J = (n + jn) + nX*(m + jm)
            for (int i = 0; i < x->n; i++) {

                if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) continue;

                // base coordinate offset for this point (x,y) relative to this map element (n,m)
                // float dx = x->data.F32[i] - psImageBinningGetFineX (map->binning, n + 0.5);
                // float dy = y->data.F32[i] - psImageBinningGetFineY (map->binning, m + 0.5);

                float dx = psImageBinningGetRuffX (map->binning, x->data.F32[i]) - (n + 0.5);
                float dy = psImageBinningGetRuffY (map->binning, y->data.F32[i]) - (m + 0.5);

                // edge cases to include:
                bool edgeX = false;
                edgeX |= ((n == 1) && (dx < -1.0));
                edgeX |= ((n == Nx - 2) && (dx > +1.0));

                bool edgeY = false;
                edgeY |= ((m == 1) && (dy < -1.0));
                edgeY |= ((m == Ny - 2) && (dy > +1.0));

                // skip points outside of 2x2 grid centered on n,m:
                if (!edgeX && (fabs(dx) > 1.0)) continue;
                if (!edgeY && (fabs(dy) > 1.0)) continue;

                // related offset values
                float rx = 1.0 - dx;
                float ry = 1.0 - dy;
                float px = 1.0 + dx;
                float py = 1.0 + dy;
                float qx = -dx;
                float qy = -dy;

                // data value & weight for this point
                float fi = f->data.F32[i];
                if (!isfinite(fi)) continue;

                float wt = 1.0;
                if (df != NULL) {
                    if (df->data.F32[i] == 0.0) {
                        wt = 0.0;
                    } else {
			if (!isfinite(df->data.F32[i])) continue;
                        wt = 1.0 / PS_SQR(df->data.F32[i]); // XXX test for dz == NULL or dz_i = 0
                    }
                }

                // sum the appropriate elements for the different quadrants

                int Qx = (dx >= 0) ? 1 : 0;
                if (n ==      0) Qx = 1;
                if (n == Nx - 1) Qx = 0;

                int Qy = (dy >= 0) ? 1 : 0;
                if (m ==      0) Qy = 1;
                if (m == Ny - 1) Qy = 0;

                assert (isfinite(fi));
                assert (isfinite(wt));
                assert (isfinite(rx));
                assert (isfinite(ry));

                // points at offset 1,1
                if ((Qx == 1) && (Qy == 1)) {
                    rx_rx_ry_ry += rx*rx*ry*ry*wt;
                    rx_rx_dy_ry += rx*rx*dy*ry*wt;
                    dx_rx_ry_ry += dx*rx*ry*ry*wt;
                    dx_rx_dy_ry += dx*rx*dy*ry*wt;
                    fi_rx_ry    += fi*rx*ry*wt;
                }
                // points at offset 1,0
                if ((Qx == 1) && (Qy == 0)) {
                    rx_rx_py_py += rx*rx*py*py*wt;
                    rx_rx_qy_py += rx*rx*qy*py*wt;
                    dx_rx_py_py += dx*rx*py*py*wt;
                    dx_rx_qy_py += dx*rx*qy*py*wt;
                    fi_rx_py    += fi*rx*py*wt;
                }
                // points at offset 0,1
                if ((Qx == 0) && (Qy == 1)) {
                    px_px_ry_ry += px*px*ry*ry*wt;
                    px_px_dy_ry += px*px*dy*ry*wt;
                    qx_px_ry_ry += qx*px*ry*ry*wt;
                    qx_px_dy_ry += qx*px*dy*ry*wt;
                    fi_px_ry    += fi*px*ry*wt;
                }
                // points at offset 0,0
                if ((Qx == 0) && (Qy == 0)) {
                    px_px_py_py += px*px*py*py*wt;
                    px_px_qy_py += px*px*qy*py*wt;
                    qx_px_py_py += qx*px*py*py*wt;
                    qx_px_qy_py += qx*px*qy*py*wt;
                    fi_px_py    += fi*px*py*wt;
                }
            }

            // the chi-square derivatives have elements of the form g(n+jn,m+jm)*A(jn,jm),
            // jn,jm = -1 to +1. Convert the sums above into the correct coefficients
            sA[-1][-1] = qx_px_qy_py;
            sA[-1][ 0] = qx_px_ry_ry + qx_px_py_py;
            sA[-1][+1] = qx_px_dy_ry;
            sA[ 0][-1] = rx_rx_qy_py + px_px_qy_py;
            sA[ 0][ 0] = rx_rx_ry_ry + px_px_ry_ry + rx_rx_py_py + px_px_py_py;
            sA[ 0][+1] = rx_rx_dy_ry + px_px_dy_ry;
            sA[+1][-1] = dx_rx_qy_py;
            sA[+1][ 0] = dx_rx_ry_ry + dx_rx_py_py;
            sA[+1][+1] = dx_rx_dy_ry;

            // I[ 0][ 0] = index for this n,m element:
            int I = n + Nx * m;
            B->data.F32[I] = fi_rx_ry + fi_rx_py + fi_px_ry + fi_px_py;
	    
            // insert these values into their corresponding locations in A, B
            // float Sum = 0.0;
            for (int jn = -1; jn <= +1; jn++) {
                if (n + jn <   0) continue;
                if (n + jn >= Nx) continue;
                for (int jm = -1; jm <= +1; jm++) {
                    if (m + jm <   0) continue;
                    if (m + jm >= Ny) continue;
                    int J = (n + jn) + Nx * (m + jm);
                    A->data.F32[J][I] = sA[jn][jm];
		    
                    // fprintf (stderr, "A %d %d (%d %d : %d %d): %f\n", I, J, n, m, n + jn, m + jm, sA[jn][jm]);
                    // Sum += sA[jn][jm];
                }
            }
            // fprintf (stderr, "B %d (%d %d) : %f  :  %f\n", I, n, m, B->data.F32[I], Sum);
            // Total += Sum;
        }
    }
    // fprintf (stderr, "Total: %f\n", Total);

    double MaxPivot = 0.0;
    for (int i = 0; i < Nx*Ny; i++) {
      MaxPivot = PS_MAX(MaxPivot, fabs(A->data.F32[i][i]));
      // fprintf (stderr, "piv, max: %f : %f\n", A->data.F32[i][i], MaxPivot);
    }

    // test for empty diagonal elements (unconstained cells), mark, and set pivots to 1.0
    psVector *Empty = psVectorAlloc (Nx*Ny, PS_TYPE_S8);
    psVectorInit (Empty, 0);
    double MinPivot = 0.025*MaxPivot;
    for (int i = 0; i < Nx*Ny; i++) {
      if (fabs(A->data.F32[i][i]) < MinPivot) {
            Empty->data.S8[i] = 1;
            for (int j = 0; j < Nx*Ny; j++) {
                A->data.F32[i][j] = 0.0;
                A->data.F32[j][i] = 0.0;
            }
            A->data.F32[i][i] = 1.0;
            B->data.F32[i] = 0.0;
        }
    }

# if (0)
    psFits *fits = psFitsOpen ("Agj.fits", "w");
    psFitsWriteImage (fits, NULL, A, 0, NULL);
    psFitsClose (fits);

    psImage *vector = psImageAlloc (1, B->n, PS_TYPE_F32);
    for (int n = 0; n < B->n; n++) {
        vector->data.F32[0][n] = B->data.F32[n];
    }

    fits = psFitsOpen ("Bgj.fits", "w");
    psFitsWriteImage (fits, NULL, vector, 0, NULL);
    psFitsClose (fits);
    psFree (vector);
# endif

    if (!psMatrixGJSolve(A, B)) {
        psFree (A);
        psFree (B);
	psFree (Empty);
        return true;
    }
    
    // set bad values to NaN
    for (int i = 0; i < Nx*Ny; i++) {
        if (Empty->data.S8[i]) {
            B->data.F32[i] = NAN;
            A->data.F32[i][i] = 0;
        }
    }


    for (int n = 0; n < Nx; n++) {
        for (int m = 0; m < Ny; m++) {
            int I = n + Nx * m;
            map->map->data.F32[m][n] = B->data.F32[I];
            map->error->data.F32[m][n] = sqrt(A->data.F32[I][I]);
        }
    }

    psFree (A);
    psFree (B);
    psFree (Empty);
    *pGoodFit = true;
    return true;
}

// measure residuals on each pass and clip outliers based on stats
bool psImageMapClipFit(bool *pGoodFit, psImageMap *map, psStats *stats, psVector *inMask, psVectorMaskType maskValue,
                       const psVector *x, const psVector *y, const psVector *f, const psVector *df)
{
    // XXX add in full PS_ASSERTS
    psAssert(map, "impossible");
    psAssert(stats, "impossible");
    psAssert(x, "impossible");
    psAssert(y, "impossible");
    psAssert(f, "impossible");

    *pGoodFit = false;

    // the user supplies one of various stats option pairs,
    // determine the desired mean and stdev STATS options:
    psStatsOptions meanOption = psStatsMeanOption(stats->options);
    psStatsOptions stdevOption = psStatsStdevOption(stats->options);
    if (!psStatsSingleOption(meanOption)) {
        psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
        return false;
    }
    if (!psStatsSingleOption(stdevOption)) {
        psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
        return false;
    }

    // clipping range defined by min and max and/or clipSigma
    psF32 minClipSigma;
    psF32 maxClipSigma;
    if (isfinite(stats->max)) {
        maxClipSigma = fabs(stats->max);
    } else {
        maxClipSigma = fabs(stats->clipSigma);
    }
    if (isfinite(stats->min)) {
        minClipSigma = fabs(stats->min);
    } else {
        minClipSigma = fabs(stats->clipSigma);
    }

    psVector *mask = inMask;
    if (!inMask) {
        mask = psVectorAlloc (x->n, PS_TYPE_VECTOR_MASK);
        psVectorInit (mask, 0);
    }

    // vector to store residuals
    psVector *resid = psVectorAlloc(f->n, PS_TYPE_F32);

    psTrace("psLib.imageops", 4, "stats->clipIter is %d\n", stats->clipIter);
    psTrace("psLib.imageops", 4, "(minClipSigma, maxClipSigma) is (%.2f, %.2f)\n", minClipSigma, maxClipSigma);

    for (psS32 N = 0; N < stats->clipIter; N++) {
        psTrace("psLib.imageops", 6, "Loop iteration %d.  Calling psImageMapFit()\n", N);
        psS32 Nkeep = 0;
        if (!psImageMapFit(pGoodFit, map, mask, maskValue, x, y, f, df)) {
            psError(PS_ERR_UNKNOWN, false, "Could not fit image map.\n");
            psFree(resid);
            if (!inMask) psFree (mask);
            return false;
        }
	if (!*pGoodFit) {
	    psWarning ("bad fit to image map, try something else");
            psFree(resid);
            if (!inMask) psFree (mask);
	    return true;
	}

        psVector *fit = psImageMapEvalVector(map, mask, maskValue, x, y);
        if (fit == NULL) {
            psError(PS_ERR_UNKNOWN, false, "Failure in psImageMapEvalVector().\n");
            psFree(resid);
            if (!inMask) psFree (mask);
            return false;
        }
        for (int i = 0 ; i < f->n ; i++) {
            resid->data.F32[i] = (f->data.F32[i] - fit->data.F32[i]);
        }

        if (!psVectorStats(stats, resid, NULL, mask, maskValue)) {
            psError(PS_ERR_UNKNOWN, false, "Failure to compute statistics on the resid vector.\n");
            psFree(resid);
            psFree(fit);
            if (!inMask) psFree (mask);
            return false;
        }

        double meanValue = psStatsGetValue (stats, meanOption);
        double stdevValue = psStatsGetValue (stats, stdevOption);

        psTrace("psLib.imageops", 5, "Mean is %f\n", meanValue);
        psTrace("psLib.imageops", 5, "Stdev is %f\n", stdevValue);
        psF32 minClipValue = -minClipSigma*stdevValue;
        psF32 maxClipValue = +maxClipSigma*stdevValue;

        // set mask if pts are not valid
        // we are masking out any point which is out of range
        // recovery is not allowed with this scheme
        for (psS32 i = 0; i < resid->n; i++) {
            // XXX this prevents recovery of previously masked values
            if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue) {
                continue;
            }

            if ((resid->data.F32[i] - meanValue > maxClipValue) || (resid->data.F32[i] - meanValue < minClipValue)) {
                psTrace("psLib.imageops", 6, "Masking element %d  : %f vs %f : resid is %f\n", i, f->data.F32[i], fit->data.F32[i], resid->data.F32[i]);
                mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                continue;
            }
            Nkeep++;
        }

        // We should probably exit this loop if no new elements were masked since the fit won't
        // change.
        psTrace("psLib.imageops", 6, "keeping %d of %ld pts for fit\n", Nkeep, x->n);
        stats->clippedNvalues = Nkeep;
        psFree(fit);
    }

    // Free local temporary variables
    psFree(resid);
    if (!inMask) psFree (mask);
    *pGoodFit = true; // XXX probably don't need to set this (set by psImageMapFit)
    return true;
}

// CZW: 2014-10-09
// Sparse versions of MapFit and MapFitClip that assume the matrices are not filled.
bool psImageMapFitSparse(bool *pGoodFit, psImageMap *map, const psVector *mask, psVectorMaskType maskValue,
                   const psVector *x, const psVector *y, const psVector *f, const psVector *df)
{
    // XXX Add Asserts

    *pGoodFit = false;

    // dimensions of the output map image
    int Nx = map->binning->nXruff;
    int Ny = map->binning->nYruff;

    
    // no spatial information, just calculate mean & stdev
    if ((Nx == 1) && (Ny == 1)) {
        psStatsInit(map->stats);

        // the user has supplied one of various stats option pairs,
        psStatsOptions mean = psStatsMeanOption(map->stats->options);
        psStatsOptions stdev = psStatsStdevOption(map->stats->options);
        if (!psStatsSingleOption(mean)) {
            psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
            return false;
        }
        if (!psStatsSingleOption(stdev)) {
            psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
            return false;
        }

        // XXX does ROBUST_MEDIAN work with weight?
        if (!psVectorStats(map->stats, f, NULL, mask, maskValue)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	    return false;
	}

        map->map->data.F32[0][0]   = psStatsGetValue(map->stats, mean);
        map->error->data.F32[0][0] = psStatsGetValue(map->stats, stdev);
        if (isfinite(map->map->data.F32[0][0]) && isfinite( map->error->data.F32[0][0])) {
            *pGoodFit = true;
        }
        return true;
    }

    if (Nx == 1) {
        bool status;
        status = psImageMapFit1DinY (pGoodFit, map, mask, maskValue, x, y, f, df);
        return status;
    }
    if (Ny == 1) {
        bool status;
        status = psImageMapFit1DinX (pGoodFit, map, mask, maskValue, x, y, f, df);
        return status;
    }

    // set up the redirection table so we can use sA[-1][-1], etc
    // XXX psKernel does this for you --- PAP.
    float SAm[3][3], *SAv[3], **sA;

    for (int i = 0; i < 3; i++) {
        SAv[i] = SAm[i] + 1;
    }
    sA = SAv + 1;

    // elements of the matrix equation Ax = B; we are solving for the vector x
    // psImage *A = psImageAlloc (Nx*Ny, Nx*Ny, PS_TYPE_F32);
    // psVector *B = psVectorAlloc (Nx*Ny, PS_TYPE_F32);

    // psImageInit (A, 0.0);
    // psVectorInit (B, 0.0);

    // CZW: call to psSparseAlloc
    // It should match old A, and each element of that should only touch four others.
    psSparse *Asparse = psSparseAlloc(Nx * Ny, 4 * Nx * Ny); 
    
    // we are looping over the Nx,Ny image map elements;
    // the matrix equation contains Nx*Ny rows and columns

    for (int n = 0; n < Nx; n++) {
        for (int m = 0; m < Ny; m++) {
            // define & init summing variables
            float rx_rx_ry_ry = 0;
            float rx_rx_dy_ry = 0;
            float dx_rx_ry_ry = 0;
            float dx_rx_dy_ry = 0;
            float fi_rx_ry    = 0;
            float rx_rx_py_py = 0;
            float rx_rx_qy_py = 0;
            float dx_rx_py_py = 0;
            float dx_rx_qy_py = 0;
            float fi_rx_py    = 0;
            float px_px_ry_ry = 0;
            float px_px_dy_ry = 0;
            float qx_px_ry_ry = 0;
            float qx_px_dy_ry = 0;
            float fi_px_ry    = 0;
            float px_px_py_py = 0;
            float px_px_qy_py = 0;
            float qx_px_py_py = 0;
            float qx_px_qy_py = 0;
            float fi_px_py    = 0;

            // generate the sums for the fitting matrix element I,J
            // I = n + nX*m
            // J = (n + jn) + nX*(m + jm)
            for (int i = 0; i < x->n; i++) {

                if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) continue;

                // base coordinate offset for this point (x,y) relative to this map element (n,m)
                float dx = psImageBinningGetRuffX (map->binning, x->data.F32[i]) - (n + 0.5);
                float dy = psImageBinningGetRuffY (map->binning, y->data.F32[i]) - (m + 0.5);

                // edge cases to include:
                bool edgeX = false;
                edgeX |= ((n == 1) && (dx < -1.0));
                edgeX |= ((n == Nx - 2) && (dx > +1.0));

                bool edgeY = false;
                edgeY |= ((m == 1) && (dy < -1.0));
                edgeY |= ((m == Ny - 2) && (dy > +1.0));

                // skip points outside of 2x2 grid centered on n,m:
                if (!edgeX && (fabs(dx) > 1.0)) continue;
                if (!edgeY && (fabs(dy) > 1.0)) continue;

                // related offset values
                float rx = 1.0 - dx;
                float ry = 1.0 - dy;
                float px = 1.0 + dx;
                float py = 1.0 + dy;
                float qx = -dx;
                float qy = -dy;

                // data value & weight for this point
                float fi = f->data.F32[i];
                if (!isfinite(fi)) continue;

                float wt = 1.0;
                if (df != NULL) {
                    if (df->data.F32[i] == 0.0) {
                        wt = 0.0;
                    } else {
			if (!isfinite(df->data.F32[i])) continue;
                        wt = 1.0 / PS_SQR(df->data.F32[i]); // XXX test for dz == NULL or dz_i = 0
                    }
                }

                // sum the appropriate elements for the different quadrants

                int Qx = (dx >= 0) ? 1 : 0;
                if (n ==      0) Qx = 1;
                if (n == Nx - 1) Qx = 0;

                int Qy = (dy >= 0) ? 1 : 0;
                if (m ==      0) Qy = 1;
                if (m == Ny - 1) Qy = 0;

                assert (isfinite(fi));
                assert (isfinite(wt));
                assert (isfinite(rx));
                assert (isfinite(ry));

                // points at offset 1,1
                if ((Qx == 1) && (Qy == 1)) {
                    rx_rx_ry_ry += rx*rx*ry*ry*wt;
                    rx_rx_dy_ry += rx*rx*dy*ry*wt;
                    dx_rx_ry_ry += dx*rx*ry*ry*wt;
                    dx_rx_dy_ry += dx*rx*dy*ry*wt;
                    fi_rx_ry    += fi*rx*ry*wt;
                }
                // points at offset 1,0
                if ((Qx == 1) && (Qy == 0)) {
                    rx_rx_py_py += rx*rx*py*py*wt;
                    rx_rx_qy_py += rx*rx*qy*py*wt;
                    dx_rx_py_py += dx*rx*py*py*wt;
                    dx_rx_qy_py += dx*rx*qy*py*wt;
                    fi_rx_py    += fi*rx*py*wt;
                }
                // points at offset 0,1
                if ((Qx == 0) && (Qy == 1)) {
                    px_px_ry_ry += px*px*ry*ry*wt;
                    px_px_dy_ry += px*px*dy*ry*wt;
                    qx_px_ry_ry += qx*px*ry*ry*wt;
                    qx_px_dy_ry += qx*px*dy*ry*wt;
                    fi_px_ry    += fi*px*ry*wt;
                }
                // points at offset 0,0
                if ((Qx == 0) && (Qy == 0)) {
                    px_px_py_py += px*px*py*py*wt;
                    px_px_qy_py += px*px*qy*py*wt;
                    qx_px_py_py += qx*px*py*py*wt;
                    qx_px_qy_py += qx*px*qy*py*wt;
                    fi_px_py    += fi*px*py*wt;
                }
            }

            // the chi-square derivatives have elements of the form g(n+jn,m+jm)*A(jn,jm),
            // jn,jm = -1 to +1. Convert the sums above into the correct coefficients
            sA[-1][-1] = qx_px_qy_py;
            sA[-1][ 0] = qx_px_ry_ry + qx_px_py_py;
            sA[-1][+1] = qx_px_dy_ry;
            sA[ 0][-1] = rx_rx_qy_py + px_px_qy_py;
            sA[ 0][ 0] = rx_rx_ry_ry + px_px_ry_ry + rx_rx_py_py + px_px_py_py;
            sA[ 0][+1] = rx_rx_dy_ry + px_px_dy_ry;
            sA[+1][-1] = dx_rx_qy_py;
            sA[+1][ 0] = dx_rx_ry_ry + dx_rx_py_py;
            sA[+1][+1] = dx_rx_dy_ry;

            // I[ 0][ 0] = index for this n,m element:
            int I = n + Nx * m;
	    //            B->data.F32[I] = fi_rx_ry + fi_rx_py + fi_px_ry + fi_px_py;
	    // CZW: call to psSparseVector Element
	    if (fi_rx_ry + fi_rx_py + fi_px_ry + fi_px_py == 0.0) {
	      psSparseVectorElement(Asparse, I, 1.0);
	    }
	    else {
	      psSparseVectorElement(Asparse, I, fi_rx_ry + fi_rx_py + fi_px_ry + fi_px_py);
	    }
	    
	    //	    printf("ADDING: %d %g \n",I, fi_rx_ry + fi_rx_py + fi_px_ry + fi_px_py);
            // insert these values into their corresponding locations in A, B
            for (int jn = -1; jn <= +1; jn++) {
                if (n + jn <   0) continue;
                if (n + jn >= Nx) continue;
                for (int jm = -1; jm <= +1; jm++) {
                    if (m + jm <   0) continue;
                    if (m + jm >= Ny) continue;
                    int J = (n + jn) + Nx * (m + jm);
		    //		    printf("A: %d %d %g\n",J,I,sA[jn][jm]);
                    // A->data.F32[J][I] = sA[jn][jm];
		    // CZW: call to psSparseMatrixElement
		    if (J < I) { continue; }
		    psSparseMatrixElement(Asparse,J,I,sA[jn][jm]); // Ensure J < I?
		    
                }
            }
        }
    }

    // test for empty diagonal elements (unconstained cells), mark, and set pivots to 1.0
    // CZW: I'm not totally sure how to check these in the sparse context.
    // Iterate over all ii pairs, and manually check the structure?
#if (0)
    psVector *Empty = psVectorAlloc (Nx*Ny, PS_TYPE_S8);
    psVectorInit (Empty, 0);
    for (int i = 0; i < Nx*Ny; i++) {
        if (A->data.F32[i][i] == 0.0) {
            Empty->data.S8[i] = 1;
            for (int j = 0; j < Nx*Ny; j++) {
                A->data.F32[i][j] = 0.0;
                A->data.F32[j][i] = 0.0;
            }
            A->data.F32[i][i] = 1.0;
            B->data.F32[i] = 0.0;
        }
    }
#endif 

    // CZW: call to psSparseSolve
    psVector *solution = psVectorAlloc(Nx*Ny, PS_TYPE_F32);
    psSparseConstraint Constraint;
    Constraint.paramDelta = 1e-3;
    Constraint.paramMin   = -1e5;
    Constraint.paramMax   = 1e5;
    solution = psSparseSolve(solution, Constraint, Asparse, 1000);
    if (!solution) {
      psFree(solution);
      psFree(Asparse);
      return(false);
    }

#if (0)
    // CZW: This is a continuation of the above information
    // set bad values to NaN
    for (int i = 0; i < Nx*Ny; i++) {
        if (Empty->data.S8[i]) {
            B->data.F32[i] = NAN;
            A->data.F32[i][i] = 0;
        }
    }
#endif

    for (int n = 0; n < Nx; n++) {
        for (int m = 0; m < Ny; m++) {
            int I = n + Nx * m;
            map->map->data.F32[m][n] = solution->data.F32[I];
            map->error->data.F32[m][n] = NAN; // sqrt(A->data.F32[I][I]); // CZW: fix this to be a real error.
        }
    }

    //    psFree (A);
    //    psFree (B);
    //    psFree (Empty);
    // CZW: free things
    psFree(solution);
    psFree(Asparse);
    
    *pGoodFit = true;
    return true;
}

// measure residuals on each pass and clip outliers based on stats
bool psImageMapClipFitSparse(bool *pGoodFit, psImageMap *map, psStats *stats, psVector *inMask, psVectorMaskType maskValue,
			     const psVector *x, const psVector *y, const psVector *f, const psVector *df)
{
    // XXX add in full PS_ASSERTS
    psAssert(map, "impossible");
    psAssert(stats, "impossible");
    psAssert(x, "impossible");
    psAssert(y, "impossible");
    psAssert(f, "impossible");

    *pGoodFit = false;

    // the user supplies one of various stats option pairs,
    // determine the desired mean and stdev STATS options:
    psStatsOptions meanOption = psStatsMeanOption(stats->options);
    psStatsOptions stdevOption = psStatsStdevOption(stats->options);
    if (!psStatsSingleOption(meanOption)) {
        psError(PS_ERR_UNKNOWN, true, "no valid mean stats option selected");
        return false;
    }
    if (!psStatsSingleOption(stdevOption)) {
        psError(PS_ERR_UNKNOWN, true, "no valid stdev stats option selected");
        return false;
    }

    // clipping range defined by min and max and/or clipSigma
    psF32 minClipSigma;
    psF32 maxClipSigma;
    if (isfinite(stats->max)) {
        maxClipSigma = fabs(stats->max);
    } else {
        maxClipSigma = fabs(stats->clipSigma);
    }
    if (isfinite(stats->min)) {
        minClipSigma = fabs(stats->min);
    } else {
        minClipSigma = fabs(stats->clipSigma);
    }

    psVector *mask = inMask;
    if (!inMask) {
        mask = psVectorAlloc (x->n, PS_TYPE_VECTOR_MASK);
        psVectorInit (mask, 0);
    }

    // vector to store residuals
    psVector *resid = psVectorAlloc(f->n, PS_TYPE_F32);

    psTrace("psLib.imageops", 4, "stats->clipIter is %d\n", stats->clipIter);
    psTrace("psLib.imageops", 4, "(minClipSigma, maxClipSigma) is (%.2f, %.2f)\n", minClipSigma, maxClipSigma);

    for (psS32 N = 0; N < stats->clipIter; N++) {
        psTrace("psLib.imageops", 6, "Loop iteration %d.  Calling psImageMapFit()\n", N);
        psS32 Nkeep = 0;
        if (!psImageMapFitSparse(pGoodFit, map, mask, maskValue, x, y, f, df)) {
            psError(PS_ERR_UNKNOWN, false, "Could not fit image map.\n");
            psFree(resid);
            if (!inMask) psFree (mask);
            return false;
        }
	if (!*pGoodFit) {
	    psWarning ("bad fit to image map, try something else");
            psFree(resid);
            if (!inMask) psFree (mask);
	    return true;
	}

        psVector *fit = psImageMapEvalVector(map, mask, maskValue, x, y);
        if (fit == NULL) {
            psError(PS_ERR_UNKNOWN, false, "Failure in psImageMapEvalVector().\n");
            psFree(resid);
            if (!inMask) psFree (mask);
            return false;
        }
        for (int i = 0 ; i < f->n ; i++) {
            resid->data.F32[i] = (f->data.F32[i] - fit->data.F32[i]);
        }

        if (!psVectorStats(stats, resid, NULL, mask, maskValue)) {
            psError(PS_ERR_UNKNOWN, false, "Failure to compute statistics on the resid vector.\n");
            psFree(resid);
            psFree(fit);
            if (!inMask) psFree (mask);
            return false;
        }

        double meanValue = psStatsGetValue (stats, meanOption);
        double stdevValue = psStatsGetValue (stats, stdevOption);

        psTrace("psLib.imageops", 5, "Mean is %f\n", meanValue);
        psTrace("psLib.imageops", 5, "Stdev is %f\n", stdevValue);
        psF32 minClipValue = -minClipSigma*stdevValue;
        psF32 maxClipValue = +maxClipSigma*stdevValue;

        // set mask if pts are not valid
        // we are masking out any point which is out of range
        // recovery is not allowed with this scheme
        for (psS32 i = 0; i < resid->n; i++) {
            // XXX this prevents recovery of previously masked values
            if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue) {
                continue;
            }

            if ((resid->data.F32[i] - meanValue > maxClipValue) || (resid->data.F32[i] - meanValue < minClipValue)) {
                psTrace("psLib.imageops", 6, "Masking element %d  : %f vs %f : resid is %f\n", i, f->data.F32[i], fit->data.F32[i], resid->data.F32[i]);
                mask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= 0x01;
                continue;
            }
            Nkeep++;
        }

        // We should probably exit this loop if no new elements were masked since the fit won't
        // change.
        psTrace("psLib.imageops", 6, "keeping %d of %ld pts for fit\n", Nkeep, x->n);
        stats->clippedNvalues = Nkeep;
        psFree(fit);
    }

    // Free local temporary variables
    psFree(resid);
    if (!inMask) psFree (mask);
    *pGoodFit = true; // XXX probably don't need to set this (set by psImageMapFit)
    return true;
}

// map defines the output image dimensions and scaling.
bool psImageMapFit1DinY(bool *pGoodFit, psImageMap *map, const psVector *mask, psVectorMaskType maskValue,
                        const psVector *x, const psVector *y, const psVector *f, const psVector *df)
{
    // XXX Add Asserts
    assert (map->binning->nXruff == 1);

    *pGoodFit = false;

    // dimensions of the output map image
    int Ny = map->binning->nYruff;

    // set up the redirection table so we can use sA[-1][-1], etc
    float SAv[3], *sA;

    sA = SAv + 1;

    // elements of the matrix equation Ax = B; we are solving for the vector x
    psImage *A = psImageAlloc (Ny, Ny, PS_TYPE_F32);
    psVector *B = psVectorAlloc (Ny, PS_TYPE_F32);

    psImageInit (A, 0.0);
    psVectorInit (B, 0.0);

    for (int m = 0; m < Ny; m++) {
        // define & init summing variables
        float ry_ry = 0;
        float dy_ry = 0;
        float fi_ry = 0;
        float py_py = 0;
        float qy_py = 0;
        float fi_py = 0;

        // generate the sums for the fitting matrix element I,J
        // I = m
        // J = m + jm
        for (int i = 0; i < y->n; i++) {

            if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) continue;

            float dy = psImageBinningGetRuffY (map->binning, y->data.F32[i]) - (m + 0.5);

            bool edgeY = false;
            edgeY |= ((m == 1) && (dy < -1.0));
            edgeY |= ((m == Ny - 2) && (dy > +1.0));

            // skip points outside of 2x2 grid centered on n,m:
            if (!edgeY && (fabs(dy) > 1.0)) continue;

            // related offset values
            float ry = 1.0 - dy;
            float py = 1.0 + dy;
            float qy = -dy;

            // data value & weight for this point
            float fi = f->data.F32[i];
            float wt = 1.0;
            if (df != NULL) {
                if (df->data.F32[i] == 0.0) {
                    wt = 0.0;
                } else {
                    wt = 1.0 / PS_SQR(df->data.F32[i]); // XXX test for dz == NULL or dz_i = 0
                }
            }

            // sum the appropriate elements for the different quadrants
            int Qy = (dy >= 0) ? 1 : 0;
            if (m ==      0) Qy = 1;
            if (m == Ny - 1) Qy = 0;

            assert (isfinite(fi));
            assert (isfinite(wt));
            assert (isfinite(ry));

            // points at offset 1,1
            if (Qy == 1) {
                ry_ry += ry*ry*wt;
                dy_ry += dy*ry*wt;
                fi_ry += fi*ry*wt;
            }
            // points at offset 1,0
            if (Qy == 0) {
                py_py += py*py*wt;
                qy_py += qy*py*wt;
                fi_py += fi*py*wt;
            }
        }

        // the chi-square derivatives have elements of the form g(n+jn,m+jm)*A(jn,jm),
        // jn,jm = -1 to +1. Convert the sums above into the correct coefficients
        sA[-1] = qy_py;
        sA[ 0] = ry_ry + py_py;
        sA[+1] = dy_ry;

        // I[ 0][ 0] = index for this n,m element:
        int I = m;
        B->data.F32[I] = fi_ry + fi_py;

        // insert these values into their corresponding locations in A, B
        for (int jm = -1; jm <= +1; jm++) {
            if (m + jm <   0) continue;
            if (m + jm >= Ny) continue;
            int J = (m + jm);
            A->data.F32[J][I] = sA[jm];
        }
    }

    // test for empty diagonal elements (unconstained cells), mark, and set pivots to 1.0
    psVector *Empty = psVectorAlloc (Ny, PS_TYPE_S8);
    psVectorInit (Empty, 0);
    for (int i = 0; i < Ny; i++) {
        if (A->data.F32[i][i] == 0.0) {
            Empty->data.S8[i] = 1;
            for (int j = 0; j < Ny; j++) {
                A->data.F32[i][j] = 0.0;
                A->data.F32[j][i] = 0.0;
            }
            A->data.F32[i][i] = 1.0;
            B->data.F32[i] = 0.0;
        }
    }

    if (!psMatrixGJSolve(A, B)) {
        psFree (A);
        psFree (B);
	psFree (Empty);
        return true;
    }

    // set bad values to NaN
    for (int i = 0; i < Ny; i++) {
        if (Empty->data.S8[i]) {
            B->data.F32[i] = NAN;
            A->data.F32[i][i] = 0;
        }
    }

    for (int m = 0; m < Ny; m++) {
        map->map->data.F32[m][0] = B->data.F32[m];
        map->error->data.F32[m][0] = sqrt(A->data.F32[m][m]);
    }

    psFree (A);
    psFree (B);
    psFree (Empty);

    *pGoodFit = true;
    return true;
}

// map defines the output image dimensions and scaling.
bool psImageMapFit1DinX(bool *pGoodFit, psImageMap *map, const psVector *mask, psVectorMaskType maskValue,
                        const psVector *x, const psVector *y, const psVector *f, const psVector *df)
{
    // XXX Add Asserts
    assert (map->binning->nYruff == 1);

    *pGoodFit = false;

    // dimensions of the output map image
    int Nx = map->binning->nXruff;

    // set up the redirection table so we can use sA[-1][-1], etc
    float SAv[3], *sA;

    sA = SAv + 1;

    // elements of the matrix equation Ax = B; we are solving for the vector x
    psImage *A = psImageAlloc (Nx, Nx, PS_TYPE_F32);
    psVector *B = psVectorAlloc (Nx, PS_TYPE_F32);

    psImageInit (A, 0.0);
    psVectorInit (B, 0.0);

    for (int m = 0; m < Nx; m++) {
        // define & init summing variables
        float rx_rx = 0;
        float dx_rx = 0;
        float fi_rx = 0;
        float px_px = 0;
        float qx_px = 0;
        float fi_px = 0;

        // generate the sums for the fitting matrix element I,J
        // I = m
        // J = m + jm
        for (int i = 0; i < x->n; i++) {

            if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) continue;

            float dx = psImageBinningGetRuffX (map->binning, x->data.F32[i]) - (m + 0.5);

            bool edgeX = false;
            edgeX |= ((m == 1) && (dx < -1.0));
            edgeX |= ((m == Nx - 2) && (dx > +1.0));

            // skip points outside of 2x2 grid centered on n,m:
            if (!edgeX && (fabs(dx) > 1.0)) continue;

            // related offset values
            float rx = 1.0 - dx;
            float px = 1.0 + dx;
            float qx = -dx;

            // data value & weight for this point
            float fi = f->data.F32[i];
            float wt = 1.0;
            if (df != NULL) {
                if (df->data.F32[i] == 0.0) {
                    wt = 0.0;
                } else {
                    wt = 1.0 / PS_SQR(df->data.F32[i]); // XXX test for dz == NULL or dz_i = 0
                }
            }

            // sum the appropriate elements for the different quadrants
            int Qx = (dx >= 0) ? 1 : 0;
            if (m ==      0) Qx = 1;
            if (m == Nx - 1) Qx = 0;

            assert (isfinite(fi));
            assert (isfinite(wt));
            assert (isfinite(rx));

            // points at offset 1,1
            if (Qx == 1) {
                rx_rx += rx*rx*wt;
                dx_rx += dx*rx*wt;
                fi_rx += fi*rx*wt;
            }
            // points at offset 1,0
            if (Qx == 0) {
                px_px += px*px*wt;
                qx_px += qx*px*wt;
                fi_px += fi*px*wt;
            }
        }

        // the chi-square derivatives have elements of the form g(n+jn,m+jm)*A(jn,jm),
        // jn,jm = -1 to +1. Convert the sums above into the correct coefficients
        sA[-1] = qx_px;
        sA[ 0] = rx_rx + px_px;
        sA[+1] = dx_rx;

        // I[ 0][ 0] = index for this n,m element:
        int I = m;
        B->data.F32[I] = fi_rx + fi_px;

        // insert these values into their corresponding locations in A, B
        for (int jm = -1; jm <= +1; jm++) {
            if (m + jm <   0) continue;
            if (m + jm >= Nx) continue;
            int J = (m + jm);
            A->data.F32[J][I] = sA[jm];
        }
    }

    // test for empty diagonal elements (unconstained cells), mark, and set pivots to 1.0
    psVector *Empty = psVectorAlloc (Nx, PS_TYPE_S8);
    psVectorInit (Empty, 0);
    for (int i = 0; i < Nx; i++) {
        if (A->data.F32[i][i] == 0.0) {
            Empty->data.S8[i] = 1;
            for (int j = 0; j < Nx; j++) {
                A->data.F32[i][j] = 0.0;
                A->data.F32[j][i] = 0.0;
            }
            A->data.F32[i][i] = 1.0;
            B->data.F32[i] = 0.0;
        }
    }

    if (!psMatrixGJSolve(A, B)) {
        psFree (A);
        psFree (B);
	psFree (Empty);
        return true;
    }

    // set bad values to NaN
    for (int i = 0; i < Nx; i++) {
        if (Empty->data.S8[i]) {
            B->data.F32[i] = NAN;
            A->data.F32[i][i] = 0;
        }
    }

    for (int m = 0; m < Nx; m++) {
        map->map->data.F32[0][m] = B->data.F32[m];
        map->error->data.F32[0][m] = sqrt(A->data.F32[m][m]);
    }

    psFree (A);
    psFree (B);
    psFree (Empty);

    *pGoodFit = true;
    return true;
}

// this function repairs an image with NAN pixels (only valid for a small-scale map -- no robust mean)
bool psImageMapRepair (psImage *image) {

    // we are going to repair the image by:
    // 1) finding NAN pixels
    // 2) if any of the neighbors are valid,
    //    replace with the mean of the neighbors
    // 3) otherwise, replace with the image mean

    // copy the image so the repaired pixels do not affect the input
    psImage *fixed = psImageCopy (NULL, image, PS_TYPE_F32);

    // find the global mean
    float mean = 0.0;
    float npix = 0.0;
    for (int iy = 0; iy < image->numRows; iy++) {
	for (int ix = 0; ix < image->numCols; ix++) {
	    if (!isfinite(image->data.F32[iy][ix])) continue;
	    mean += image->data.F32[iy][ix];
	    npix += 1.0;
	}
    }
    mean /= npix;

    // find the NAN pixels:
    for (int iy = 0; iy < image->numRows; iy++) {
	for (int ix = 0; ix < image->numCols; ix++) {
	    if (isfinite(image->data.F32[iy][ix])) {
		fixed->data.F32[iy][ix] = image->data.F32[iy][ix];
		continue;
	    }

	    // find mean of all possible neighbors
	    float meanLocal = 0.0;
	    float npixLocal = 0.0;
	    for (int jy = -1; jy <= +1; jy++) {
		int ny = iy + jy;
		if (ny < 0) continue;
		if (ny >= image->numRows) continue;
		for (int jx = -1; jx <= +1; jx++) {
		    int nx = ix + jx;
		    if (nx < 0) continue;
		    if (nx >= image->numCols) continue;
		    if (!isfinite(image->data.F32[ny][nx])) continue;
		    meanLocal += image->data.F32[ny][nx];
		    npixLocal += 1.0;
		}
	    }
	    meanLocal = (npixLocal > 0.0) ? meanLocal / npixLocal : mean;
	    fixed->data.F32[iy][ix] = meanLocal;
	}
    }
    
    // find the NAN pixels:
    for (int iy = 0; iy < image->numRows; iy++) {
	for (int ix = 0; ix < image->numCols; ix++) {
	    image->data.F32[iy][ix] = fixed->data.F32[iy][ix];
	}
    }
    psFree (fixed);

    return true;
}
