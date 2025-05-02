/** @file  pmSubtractSky.c
 *
 *  This file will contain a module which will create a model of the
 *  background sky and subtract that from the input image.
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmSubtractSky.h"

// XXX: Get rid of the.  Create pmUtils.h
psImage *p_psDetermineTrimmedImage(
    pmReadout *in
);

/******************************************************************************
DetermineNumBits(data): This routine takes an enum psStatsOptions as an
argument and returns the number of non-zero bits.

XXX: This code is duplicated in the ReadoutCombine file.
 *****************************************************************************/
static psS32 DetermineNumBits(psStatsOptions data)
{
    psTrace("psModules.detrend", 4, "Calling DetermineNumBits(0x%x)\n", data);

    psS32 i;
    psU64 tmpData = data;
    psS32 numBits = 0;

    for (i=0;i<(8 * sizeof(psStatsOptions));i++) {
        if (0x0001 & tmpData) {
            numBits++;
        }
        tmpData = tmpData >> 1;
    }

    psTrace("psModules.detrend", 4,
            "Calling DetermineNumBits(0x%x) -> %d\n", data, numBits);
    return(numBits);
}

/******************************************************************************
getHighestPriorityStatOption(statOptions): this routine takes as input a
psStats->options with multiple options set and returns one with a single
option set according to the precedence set in the SDRS.
 *****************************************************************************/
static psU64 getHighestPriorityStatOption(psU64 statOptions)
{
    psTrace("psModules.detrend", 4,
            "Calling getHighestPriorityStatOption(0x%x)\n", statOptions);

    if (statOptions & PS_STAT_SAMPLE_MEAN) {
        return(PS_STAT_SAMPLE_MEAN);
    } else if (statOptions & PS_STAT_SAMPLE_MEDIAN) {
        return(PS_STAT_SAMPLE_MEDIAN);
    } else if (statOptions & PS_STAT_CLIPPED_MEAN) {
        return(PS_STAT_CLIPPED_MEAN);
    } else if (statOptions & PS_STAT_FITTED_MEAN) {
        return(PS_STAT_FITTED_MEAN);
    } else if (statOptions & PS_STAT_ROBUST_MEDIAN) {
        return(PS_STAT_ROBUST_MEDIAN);
    }
    psError(PS_ERR_UNKNOWN, true, "Unallowable option requested for statistically binning image pixels.\n");
    return(-1);
    // XXX
    //else if (statOptions & PS_STAT_ROBUST_MODE) {
    //    return(PS_STAT_ROBUST_MODE);
    //}
}

/******************************************************************************
psImage *binImage(origImage, binFactor, statOptions): This routine takes an
input psImage and scales it smaller by a factor of binFactor.  The statistic
used in combining input pixels is specified in statOptions.

XXX: use static vectors for myStats, binVector and binMask.
XXX: I coded this before I was aware of a psLib reBin function.  I don't
use this function in this module.  I'm keeping it here in the event that
requirements change and we might need a custom reBin function.
 *****************************************************************************/
#if 0
static psImage *binImage(psImage *origImage,
                         int binFactor,
                         psStatsOptions statOptions)
{
    psTrace("psModules.detrend", 4, "Calling binImage(%d)\n", binFactor);

    if (binFactor <= 0) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: binImage(): binFactor is %d\n", binFactor);
        return(origImage);
    }
    if (binFactor == 1) {
        return(origImage);
    }

    psVector *binVector = psVectorAlloc(binFactor * binFactor, PS_TYPE_F32);
    psVector *binMask = psVectorAlloc(binFactor * binFactor, PS_TYPE_VECTOR_MASK);
    psStats *myStats = psStatsAlloc(statOptions);

    for (psS32 row = 0; row < origImage->numRows ; row+=binFactor) {
        for (psS32 col = 0; col < origImage->numCols ; col+=binFactor) {
            psS32 count = 0;
            for (psS32 binRow = 0; binRow <= binFactor ; binRow++) {
                for (psS32 binCol = 0; binCol <= binFactor ; binCol++) {
                    if (((row + binRow) < origImage->numRows) &&
                            ((col + binCol) < origImage->numCols)) {
                        binVector->data.F32[count] =
                            origImage->data.F32[row + binRow][col + binCol];
                        binMask->data.PS_TYPE_VECTOR_MASK_DATA[count] = 0;
                    } else {
                        binVector->data.F32[count] = 0.0;
                        binMask->data.PS_TYPE_VECTOR_MASK_DATA[count] = 1;
                    }
                    count++;
                }
            }
            psStats *rc1 = psVectorStats(myStats, binVector, NULL, binMask, 1);
            if (rc1 == NULL) {
                psError(PS_ERR_UNKNOWN, false, "psVectorStats(): could not perform requested statistical operation.  Returning in image.\n");
                return(origImage);
            }
            psF64 statValue;
            psBool rc = p_psGetStatValue(rc1, &statValue);

            if (rc == true) {
                origImage->data.F32[row][col] = (psF32) statValue;
            } else {
                origImage->data.F32[row][col] = 0.0;
                psLogMsg(__func__, PS_LOG_WARN,
                         "WARNING: pmSubtractSky(), binImage(): p_psGetStatValue() was FALSE\n");
            }
        }
    }
    psFree(binVector);
    psFree(binMask);
    psFree(myStats);

    psTrace("psModules.detrend", 4, "Exiting binImage(%d)\n", binFactor);
    return(origImage);
}
#endif

/******************************************************************************
CalculatePolyTerms(xOrder, yOrder): this routine will calculate the number of
coefficients (or terms) in a 2-D polynomial of order (xOrder, yOrder).

XXX: Use your brain and figure out the analytical expression.

XXX: Why isn't it simply (xOrder+1) * (yOrder+1)?
 *****************************************************************************/
static psS32 CalculatePolyTerms(psS32 xOrder, psS32 yOrder)
{
    psTrace("psModules.detrend", 4,
            "Calling CalculatePolyTerms(%d, %d)\n", xOrder, yOrder);

    psS32 maxOrder = PS_MAX(xOrder, yOrder);
    psS32 localPolyTerms = 0;
    psS32 order = 0;
    psS32 num=0;

    for (order=0;order<=maxOrder;order++) {
        for (num=0;num<=order;num++) {
            if (((order-num) <= xOrder) && (num <= yOrder)) {
                localPolyTerms++;
            }
        }
    }
    psTrace("psModules.detrend", 4,
            "Exiting CalculatePolyTerms(%d, %d) -> %d\n", xOrder, yOrder, localPolyTerms);
    return(localPolyTerms);

    //    return((xOrder+1) * (yOrder+1));
}

/******************************************************************************
buildPolyTerms(): this routine computes a 2-D array polyTerms[][] that holds
terms for the polynomial that is used to model the sky background.  We use
this array primarily for convenience in computations involving sky model
polynomials.  It is defined as:
    polyTerms[i][0] = the power to which X is raised in the i-th term of in an
    poly-order sky background polynomial.

    polyTerms[i][1] = the power to which Y is raised in the i-th term of in an
    poly-order sky background polynomial.
 *****************************************************************************/
static psS32 **buildPolyTerms(psS32 xOrder, psS32 yOrder)
{
    psTrace("psModules.detrend", 4,
            "Calling buildPolyTerms(%d, %d)\n", xOrder, yOrder);

    psS32 i=0;
    psS32 order = 0;
    psS32 num=0;
    psS32 localPolyTerms = CalculatePolyTerms(xOrder, yOrder);
    psS32 maxOrder = PS_MAX(xOrder, yOrder);

    // Create the data structure which we hold the xy order of each coeff.
    psS32 **polyTerms = (psS32 **) psAlloc(localPolyTerms * sizeof(psS32 *));
    for (i=0; i < localPolyTerms ; i++) {
        polyTerms[i] = (psS32 *) psAlloc(2 * sizeof(psS32));
    }

    i=0;
    // This code segment loops through each term i in the polynomial and
    // calculates the power to which x/y are raised in that i-th term.
    // We first do the 0-order terms, then the 1-order terms, etc.
    for (order=0;order<=maxOrder;order++) {
        for (num=0;num<=order;num++) {
            if (((order-num) <= xOrder) && (num <= yOrder)) {
                polyTerms[i][0] = order-num;
                polyTerms[i][1] = num;
                i++;
            }
        }
    }

    if (psTraceGetLevel("psModules.detrend") >= 10) {
        for (i=0; i < localPolyTerms ; i++) {
            printf("x^%d * y^%d\n", polyTerms[i][0], polyTerms[i][1]);
        }
    }

    psTrace("psModules.detrend", 4,
            "Exiting buildPolyTerms(%d, %d)\n", xOrder, yOrder);
    return(polyTerms);
}

/******************************************************************************
This procedure calculates various combinations of powers of x and y and stores
them in the data structure p_psPolySums[][].  After it completes:

    p_psPolySums[i][j] == x^i * y^j

XXX: Use a psImage for the p_psPolySums data structure?
XXX: p_psPolySums: should this be a global?  Did you get the storage classifier
     and name correct?
XXX: Use variable size arrays for p_psPolySums[][].
XXX: Must initialize p_psPolySums[][]?
 *****************************************************************************/
#define PS_MAX_POLYNOMIAL_ORDER 20

psF64 p_psPolySums[PS_MAX_POLYNOMIAL_ORDER+1][PS_MAX_POLYNOMIAL_ORDER+1];
static void buildSums(psF64 x,
                      psF64 y,
                      psS32 xOrder,
                      psS32 yOrder)
{
    psTrace("psModules.detrend", 4,
            "Calling buildPolyTerms(%d, %d)\n", xOrder, yOrder);

    psS32 i = 0;
    psS32 j = 0;
    psF64 xSum = 0.0;
    psF64 ySum = 0.0;

    xSum = 1.0;
    ySum = 1.0;
    for(i=0;i<=xOrder;i++) {
        ySum = xSum;
        for(j=0;j<=yOrder;j++) {
            p_psPolySums[i][j] = ySum;
            ySum*= y;
        }
        xSum*= x;
    }
    psTrace("psModules.detrend", 4,
            "Exiting buildPolyTerms(%d, %d)\n", xOrder, yOrder);
}

/******************************************************************************
ImageFitPolynomial(myPoly, dataImage, maskImage): this private routine takes
an input image along with a mask and fits a polynomial to it.  The degree of
the polynomial is specified by input parameter myPoly, and need not be
symmetrical in orders of X and Y.  The polynomial must be type
PS_POLYNOMIAL_ORD.  If there are not enough rows or columns in the input image
for the order of the polynomial, then that order is reduced.  The algorithm
used in this routine is based on that of the pilot project ADD, but is not
documented anywhere.

XXX: Different trace message facilities in use here.
 *****************************************************************************/
static psPolynomial2D *ImageFitPolynomial(
    psPolynomial2D *myPoly,
    psImage *dataImage,
    psImage *maskImage)
{
    psTrace("psModules.detrend", 4,
            "Calling ImageFitPolynomial()\n");
    PS_ASSERT_POLY_NON_NULL(myPoly, NULL);
    PS_ASSERT_POLY_TYPE(myPoly, PS_POLYNOMIAL_ORD, NULL);
    PS_ASSERT_IMAGE_NON_NULL(dataImage, NULL);
    PS_ASSERT_IMAGE_NON_EMPTY(dataImage, NULL);
    PS_ASSERT_IMAGE_TYPE(dataImage, PS_TYPE_F32, NULL);
    PS_ASSERT_IMAGE_NON_NULL(maskImage, NULL);
    PS_ASSERT_IMAGE_NON_EMPTY(maskImage, NULL);
    PS_ASSERT_IMAGE_TYPE(maskImage, PS_TYPE_IMAGE_MASK, NULL);
    PS_ASSERT_IMAGES_SIZE_EQUAL(dataImage, maskImage, NULL);
    psS32 oldPolyX = -1;
    psS32 oldPolyY = -1;

    // The matrix equations become singular if there are more powers of X
    // in myPoly then there are rows of the image.  I think.  Similarly for
    // powers of Y and columns.  So.  Here we reduce the complexity of the
    // polynomial if there are not enough rows/columns in the input image.

    if ((myPoly->nX + 1) > dataImage->numRows) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: ImageFitPolynomial(): Reducing polynomial complexity in x-dimension.\n");
        oldPolyX = myPoly->nX;
        myPoly->nX = dataImage->numRows - 1;
    }
    if ((myPoly->nY + 1) > dataImage->numCols) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: ImageFitPolynomial(): Reducing polynomial complexity in y-dimension.\n");
        oldPolyY = myPoly->nY;
        myPoly->nY = dataImage->numCols - 1;
    }
    psS32 i;
    psS32 j;
    psS32 x;
    psS32 y;
    psS32 aRow;
    psS32 aCol;
    psS32 **polyTerms = buildPolyTerms(myPoly->nX, myPoly->nY);
    // We determine how many coefficients will be in the polynomial that we
    // are fitting to this image.
    psS32 localPolyTerms = CalculatePolyTerms(myPoly->nX, myPoly->nY);
    psImage *A = psImageAlloc(localPolyTerms, localPolyTerms, PS_TYPE_F64);
    psImage *Aout = psImageAlloc(localPolyTerms, localPolyTerms, PS_TYPE_F64);
    psVector *B = psVectorAlloc(localPolyTerms, PS_TYPE_F64);
    psVector *outPerm = NULL;

    //
    // Initialize A matrix and B vector.
    //
    psImageInit(A, 0.0);
    psVectorInit(B, 0.0);

    //
    // We build the A matrix and B vector.
    //
    for (x=0;x<dataImage->numRows;x++) {
        for (y=0;y<dataImage->numCols;y++) {
            if (maskImage->data.PS_TYPE_IMAGE_MASK_DATA[x][y] == 0) {
                buildSums((psF64) x, (psF64) y, myPoly->nX, myPoly->nY);

                /************************************************************
                This code dervies from equation (7) of the pilot ADD.  However,
                it is not exactly the same in that the order of the polynomial
                may be different in X And Y.

                Equation (7) from the pilot ADD describes 16 linear equations.
                The i-th equation is simply the partial derivative of the
                sky background polynomial (1) w.r.t. to the i-th term in
                that polynomial.  The i-th equation is stored in row i of
                matrix A[][] (matrix A[][] has origin (1,1), not (0,0)).  To
                compute A[i][j] we simply multiply the j-th term of the Sky
                Background Polynomial (SBP) by the i-th term of SBP.
                ************************************************************/
                for (aRow=0;aRow<localPolyTerms;aRow++) {
                    for (aCol=0;aCol<localPolyTerms;aCol++) {
                        A->data.F64[aRow][aCol]+=
                            (p_psPolySums[ polyTerms[aCol][0] ][ polyTerms[aCol][1] ] *
                             p_psPolySums[ polyTerms[aRow][0] ][ polyTerms[aRow][1] ]);
                    }
                }
                // Build the B[] vector, which is the right-hand side of (7).
                for (i=0;i<localPolyTerms;i++) {
                    B->data.F64[i]+= dataImage->data.F32[x][y] *
                                     p_psPolySums[ polyTerms[i][0] ][ polyTerms[i][1] ];
                }
            }
        }
    }

    if (psTraceGetLevel(".psModule.pmSubtractSky.ImageFitPolynomial") >= 8) {
        for (aRow=0;aRow<localPolyTerms;aRow++) {
            for (aCol=0;aCol<localPolyTerms;aCol++) {
                printf("A[%d][%d] is %f\n", aRow, aCol,
                       A->data.F64[aRow][aCol]);
            }
        }

        for (i=0;i<=localPolyTerms;i++) {
            printf("B[%d] is %f\n", i, B->data.F64[i]);
        }
    }

    //
    // Solve the matrix equations for the polynomial coefficients C.
    // XXX: How do we know if these matrix operations were successful?
    //
    Aout = psMatrixLUDecomposition(Aout, &outPerm, A);
    PS_ASSERT_IMAGE_NON_NULL(Aout, NULL);
    PS_ASSERT_IMAGE_NON_EMPTY(Aout, NULL);
    psVector *C = psVectorAlloc(localPolyTerms, PS_TYPE_F64);
    psMatrixLUSolution(C, Aout, B, outPerm);

    //
    // Set the appropriate coefficients in the myPoly structure.
    //
    for (i=0;i<localPolyTerms;i++) {
        myPoly->coeff[ polyTerms[i][0] ][ polyTerms[i][1] ] = C->data.F64[i];
        psTrace("psModules.detrend", 6,
                "myPoly->coeff[%d][%d] is %f\n", polyTerms[i][0], polyTerms[i][1], myPoly->coeff[ polyTerms[i][0] ][ polyTerms[i][1] ]);
    }

    //
    // Free data structures that were allocated in this module.
    //
    for (i=0;i<localPolyTerms;i++) {
        psFree(polyTerms[i]);
    }
    psFree(polyTerms);
    psFree(A);
    psFree(Aout);
    psFree(B);
    psFree(C);
    psFree(outPerm);

    //
    // We restore the original size of the polynomial and set remaining
    // coefficients to 0.0, if necessary.
    //
    // XXX: Verify this works after poly nOrder/nTerm change.
    //
    if (oldPolyX != -1) {
        myPoly->nX = oldPolyX;
        for (i=oldPolyX ; i < (1 + myPoly->nX) ; i++) {
            for (j=0;j<(1 + myPoly->nY) ; j++) {
                myPoly->coeff[i][j] = 0.0;
            }
        }
    }
    if (oldPolyY != -1) {
        myPoly->nY = oldPolyY;
        for (i=0 ; i < (1 + myPoly->nX) ; i++) {
            for (j=oldPolyY;j < (1 + myPoly->nY) ; j++) {
                myPoly->coeff[i][j] = 0.0;
            }
        }
    }

    psTrace("psModules.detrend", 4,
            "Exiting ImageFitPolynomial()\n");
    //    psTrace("psModules.detrend", 4,
    //            "---- ImageFitPolynomial() end successfully ----\n");
    return(myPoly);
}


/******************************************************************************
pmReadout pmSubtractSky():

XXX: use static vectors for myStats, and the binned image

XXX: The SDR is silent about types.  PS_TYPE_F32 is implemented here.

XXX: Sync the psTrace message facilities.
 *****************************************************************************/
pmReadout *pmSubtractSky(pmReadout *in,
                         void *fitSpec,
                         psFit fit,
                         psS32 binFactor,
                         psStats *stats,
                         psF32 clipSD)
{
    PS_ASSERT_READOUT_NON_NULL(in, NULL);
    PS_ASSERT_READOUT_NON_EMPTY(in, NULL);
    PS_ASSERT_READOUT_TYPE(in, PS_TYPE_F32, NULL);
    PS_WARN_PTR_NON_NULL(in->parent);
    if (in->parent != NULL) {
        PS_WARN_PTR_NON_NULL(in->parent->concepts);
    }
    psTrace("psModules.detrend", 4,
            "---- pmSubtractSky() begin ----\n");

    if ((fit != PM_FIT_NONE) &&
            (fit != PM_FIT_POLYNOMIAL) &&
            (fit != PM_FIT_SPLINE)) {
        psError(PS_ERR_UNKNOWN, true, "psFit is unallowable (%d).  Returning in image.\n", fit);
        return(in);
    }

    psStatsOptions statOptions = 0;

    //
    // Return the original input readout if the fit specs are poorly defined.
    // No warning or error messages should be generated.
    //
    if ((fitSpec == NULL) ||
            ((fit == PM_FIT_NONE) || (fit == PM_FIT_SPLINE))) {
        //        psLogMsg(__func__, PS_LOG_WARN, "Fit specs are poorly defined.  Returning in image.\n");
        return(in);
    }

    //
    // Determine trimmed image from metadata.
    //

    psImage *trimmedImg = p_psDetermineTrimmedImage(in);
    psImage *binnedImage = NULL;
    psPolynomial2D *myPoly = NULL;
    psImage *binnedMaskImage = NULL;
    psU32 oldStatOptions = 0;

    //
    // Determine which statistic to use when binning pixels, if any.
    //
    if (stats != NULL) {
        statOptions = stats->options;
        if (1 < DetermineNumBits(statOptions)) {
            psLogMsg(__func__, PS_LOG_WARN, "WARNING: Multiple statistical options have been requested.\n");
            statOptions = getHighestPriorityStatOption(statOptions);
            if (statOptions == -1) {
                psError(PS_ERR_UNKNOWN, true, "Not allowable stats->option was specified.  Returning in image.\n");
                return(in);
            }
            // Save old input "stats" parameter.
            oldStatOptions = stats->options;
            stats->options = statOptions;
        }
        if (0 == DetermineNumBits(statOptions)) {
            psLogMsg(__func__, PS_LOG_WARN,
                     "WARNING: pmSubtractSky(): no stats->options was requested\n");
        }
    }

    //
    // Generate required warning messages.
    //
    if (binFactor <= 0) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: pmSubtractSky(): binFactor is %d\n", binFactor);
    }
    if (stats == NULL) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: pmSubtractSky(): input parameter stats is NULL\n");
    }

    //
    // Bin the input image according to input parameters.
    // Create a new binned image mask.
    //
    if ((binFactor <= 1) || (stats == NULL) || (0 == DetermineNumBits(statOptions))) {
        // No binning is required here.  Simply create a copy of the image
        // and a mask.
        binnedImage = psImageCopy(binnedImage, trimmedImg, PS_TYPE_F32);
        if (binnedImage == NULL) {
            psError(PS_ERR_UNKNOWN, false, "psImageCopy() returned NULL.  Returning in image.\n");
            return(in);
        }

        if (in->mask != NULL) {
            binnedMaskImage = psImageCopy(binnedMaskImage, in->mask, PS_TYPE_IMAGE_MASK);
            if (binnedMaskImage == NULL) {
                psError(PS_ERR_UNKNOWN, false, "psImageCopy() returned NULL.  Returning in image.\n");
                psFree(binnedImage);
                return(in);
            }
        } else {
            binnedMaskImage = psImageAlloc(binnedImage->numCols,
                                           binnedImage->numRows,
                                           PS_TYPE_IMAGE_MASK);
            psImageInit(binnedMaskImage, 0);
        }
    } else {
        binnedImage = psImageRebin(NULL, trimmedImg, in->mask, 0, binFactor, stats);
        if (binnedImage == NULL) {
            psError(PS_ERR_UNKNOWN, false, "psImageRebin() returned NULL.  Returning in image.\n");
            return(in);
        }
        binnedMaskImage = psImageAlloc(binnedImage->numCols,
                                       binnedImage->numRows,
                                       PS_TYPE_IMAGE_MASK);
        psImageInit(binnedMaskImage, 0);
    }
    psTrace("psModules.detrend", 4,
            "binnedImage size is (%d, %d)\n", binnedImage->numRows, binnedImage->numCols);

    //
    // Clip pixels that are outside the acceptable range.
    //
    if (clipSD <= 0.0) {
        psLogMsg(__func__, PS_LOG_WARN,
                 "WARNING: pmSubtractSky(): clipSD is %f\n", clipSD);
    } else {
        // Determine the mean and standard deviation of the binned image.
        psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        if (!psImageStats(myStats, binnedImage, NULL, 0)) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Couldn't get statistics for image.\n");
            return NULL;
        }
        psF64 binnedMean = myStats->sampleMean;
        psF64 binnedStdev = myStats->sampleStdev;
        psFree(myStats);
        psTrace("psModules.detrend", 6,
                "binned StDev is %f\n", binnedStdev);

        // Clip all pixels which are more than clipSD sigmas from the mean.
        psTrace("psModules.detrend", 6,
                "clipSD is %f\n", clipSD);

        for (psS32 row = 0; row < binnedImage->numRows ; row++) {
            for (psS32 col = 0; col < binnedImage->numCols ; col++) {
                if (fabs(binnedImage->data.F32[row][col] - binnedMean) >
                        (clipSD * binnedStdev)) {
                    binnedMaskImage->data.PS_TYPE_IMAGE_MASK_DATA[row][col] = 1;
                }
            }
        }
    }

    //
    // Fit the polynomial to the binned image
    //
    if (fit == PM_FIT_POLYNOMIAL) {
        myPoly = (psPolynomial2D *) fitSpec;
        PS_ASSERT_POLY_NON_NULL(myPoly, NULL);
        PS_ASSERT_POLY_TYPE(myPoly, PS_POLYNOMIAL_ORD, NULL);

        myPoly = ImageFitPolynomial(myPoly, binnedImage, binnedMaskImage);

        if (myPoly != NULL) {
            // Set the pixels in the binned image to that of the polynomial.
            binnedImage = psImageEvalPolynomial(binnedImage, myPoly);
            if (binnedImage == NULL) {
                psError(PS_ERR_UNKNOWN, false, "psImageEvalPolynomial() returned NULL.  Returning in image.\n");
                psFree(binnedMaskImage);
                if (!((binFactor <= 1) || (stats == NULL))) {
                    psFree(binnedImage);
                }
                if (oldStatOptions != 0) {
                    stats->options = statOptions;
                }
                return(in);
            }
        } else {
            psLogMsg(__func__, PS_LOG_WARN,
                     "WARNING: pmSubtractSky(): could not model sky with a polynomial.\n");
            psFree(binnedMaskImage);
            if (!((binFactor <= 1) || (stats == NULL))) {
                psFree(binnedImage);
            }
            if (oldStatOptions != 0) {
                stats->options = statOptions;
            }
            return(in);
        }
    } else {
        // We shouldn't get here since we check this above.
        psError(PS_ERR_UNKNOWN, true, "Unallowable fit type.  Returning in image.\n");
        psFree(binnedMaskImage);
        if (!((binFactor <= 1) || (stats == NULL))) {
            psFree(binnedImage);
        }
        if (oldStatOptions != 0) {
            stats->options = statOptions;
        }
        return(in);
    }

    //
    //Subtract the polynomially fitted image from the original image
    //
    if (binFactor <= 1) {
        // The binned image is the same size as the original image.
        for (psS32 row = 0; row < trimmedImg->numRows ; row++) {
            for (psS32 col = 0; col < trimmedImg->numCols ; col++) {
                trimmedImg->data.F32[row][col]-= binnedImage->data.F32[row][col];
            }
        }
    } else {

        psImageInterpolateOptions *interp = psImageInterpolateOptionsAlloc(PS_INTERPOLATE_BILINEAR,
                                                                           binnedImage, NULL, NULL, 0,
                                                                           0.0, 0.0, 0, 0, 0.0);

        for (psS32 row = 0; row < trimmedImg->numRows ; row++) {
            for (psS32 col = 0; col < trimmedImg->numCols ; col++) {
                // We calculate the F32 value of the pixel coordinates in the
                // binned image and then use a pixel interpolation routine to
                // determine the value of the pixel at that location.
                psF32 binRowF64 = ((psF32) row) / ((psF32) binFactor);
                psF32 binColF64 = ((psF32) col) / ((psF32) binFactor);

                // We add 0.5 to the pixel locations since the pixel
                // interpolation routine defines the location of pixel
                // (i, j) as (i+0.5, j+0.5).
                binRowF64+= 0.5;
                binColF64+= 0.5;

                double binPixel;
                if (!psImagePixelInterpolate(&binPixel, NULL, NULL, binColF64, binRowF64, interp)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image.");
                    psFree(interp);
                    psFree(binnedImage);
                    return NULL;
                }
                trimmedImg->data.F32[row][col] -= binPixel;

                psTrace("psModules.detrend", 8,
                        "image[%d][%d] <--> binnedImage[%.2f][%.2f]: %lf\n",
                        row, col, binRowF64-0.5, binColF64-0.5, binPixel);
            }
        }
        psFree(interp);

    }
    psFree(binnedMaskImage);
    psFree(binnedImage);
    if (oldStatOptions != 0) {
        stats->options = statOptions;
    }

    psTrace("psModules.detrend", 4,
            "---- pmSubtractSky() exit successfully ----\n");
    return(in);
}
