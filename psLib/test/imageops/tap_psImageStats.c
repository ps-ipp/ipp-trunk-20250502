/*
*  C Implementation: %{MODULE}
*
* Description:
*
* XXX: Must do sub Region tests
* XXX: Must do (1, N) and (N, 1) size tests
*
* Copyright: See COPYING file that comes with this distribution
*
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

bool testInRegion(psRegion region, int row, int col)
{
    return(row >= region.y0 && row < region.y1 && col >= region.x0 && col < region.x1);
}

void genericImageCountPixelMaskTest(int numRows, int numCols)
{
    psMemId id = psMemGetId();

    psRegion reg;
    reg.x0 = 0;
    reg.x1 = 0;
    reg.y0 = 0;
    reg.y1 = 0;
    psImage *img = psImageAlloc(numRows, numCols, PS_TYPE_MASK);
    int expNumPix = 0;
    for (int i = 0 ; i < numRows ; i++) {
        for (int j = 0 ; j < numCols ; j++) {
            if (0 == i%2) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 1;
                expNumPix++;
            } else {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
    }
    psS32 numPix = psImageCountPixelMask(img, reg, 1);
    ok(numPix == expNumPix, "psImageCountPixelMask() returned the correct number of pixels (no region)");

    reg.x0 = numCols/4;
    reg.x1 = 3 * numCols/4;
    reg.y0 = numRows/4;
    reg.y1 = 3 * numRows/4;
    expNumPix = 0;
    img->col0 = 0;
    img->row0 = 0;
    for (int i = 0 ; i < numRows ; i++) {
        for (int j = 0 ; j < numCols ; j++) {
            if (testInRegion(reg, i, j)) {
                if (0 == i%2) {
                    img->data.PS_TYPE_MASK_DATA[i][j] = 1;
                    expNumPix++;
                } else {
                    img->data.PS_TYPE_MASK_DATA[i][j] = 0;
                }
            } else {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
    }
    numPix = psImageCountPixelMask(img, reg, 1);
    ok(numPix == expNumPix, "psImageCountPixelMask() returned the correct number of pixels (with region)");
    psFree(img);

    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}


void genericImageStatsTest(int numRows, int numCols)
{
    psMemId id = psMemGetId();
    psImage *img = psImageAlloc(numRows, numCols, PS_TYPE_F32);
    psImage *msk = psImageAlloc(numRows, numCols, PS_TYPE_MASK);
    psImage *imgSub = psImageSubset(img, psRegionSet(numRows/4, 3*numRows/4,
                                    numCols/4, 3*numCols/4));
    psImage *mskSub = psImageSubset(img, psRegionSet(numRows/4, 3*numRows/4,
                                    numCols/4, 3*numCols/4));
    float meanNoMask = 0.0;
    float numPixelsNoMask = 0.0;
    float meanMask = 0.0;
    float numPixelsMask = 0.0;
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numRows; j++) {
            if (0 == i%2) {
                img->data.F32[i][j] = (float) (i + j);
                msk->data.PS_TYPE_MASK_DATA[i][j] = 1;
                meanNoMask+= img->data.F32[i][j];
                numPixelsNoMask+= 1.0;
            } else {
                img->data.F32[i][j] = (float) (i + j + 100);
                msk->data.PS_TYPE_MASK_DATA[i][j] = 0;
                meanNoMask+= img->data.F32[i][j];
                meanMask+= img->data.F32[i][j];
                numPixelsMask+= 1.0;
                numPixelsNoMask+= 1.0;
            }
        }
    }
    meanNoMask/= numPixelsNoMask;
    meanMask/= numPixelsMask;
    psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psStats* tmpStats = myStats;

    // Calculate Sample Mean with nomask
    myStats = psImageStats(myStats, img, NULL, 0);
    ok(myStats != NULL, "psImageStats() returned non-NULL");
    ok(myStats == tmpStats, "psImageStats() returned the original psStats struct");
    ok(fabs(myStats->sampleMean - meanNoMask) < .001, "psImageStats() calculated the mean correctly (no mask)");

    // Calculate Sample Mean with mask
    myStats = psImageStats(myStats, img, msk, 1);
    ok(myStats != NULL, "psImageStats() returned non-NULL");
    ok(fabs(myStats->sampleMean - meanMask) < .001, "psImageStats() calculated the mean correctly (with mask)");


    // Repeat previous tests using the sub image
    meanNoMask = 0.0;
    numPixelsNoMask = 0.0;
    meanMask = 0.0;
    numPixelsMask = 0.0;
    for (int i = 0; i < imgSub->numRows; i++) {
        for (int j = 0; j < imgSub->numRows; j++) {
            if (0 == i%2) {
                imgSub->data.F32[i][j] = (float) (i + j);
                mskSub->data.PS_TYPE_MASK_DATA[i][j] = 1;
                meanNoMask+= imgSub->data.F32[i][j];
                numPixelsNoMask+= 1.0;
            } else {
                imgSub->data.F32[i][j] = (float) (i + j + 100);
                mskSub->data.PS_TYPE_MASK_DATA[i][j] = 0;
                meanNoMask+= imgSub->data.F32[i][j];
                meanMask+= imgSub->data.F32[i][j];
                numPixelsMask+= 1.0;
                numPixelsNoMask+= 1.0;
            }
        }
    }
    meanNoMask/= numPixelsNoMask;
    meanMask/= numPixelsMask;
    tmpStats = myStats;

    // Calculate Sample Mean with nomask (sub image)
    // XXXX: We skip these tests because they seg-fault.  Not sure if the source
    // is wrong or the tests are.
    ok(false, "XXXX: skipping psImageStats() tests on a subRegion because of seg faults");
    if (0) {
        myStats = psImageStats(myStats, imgSub, NULL, 0);
        ok(myStats != NULL, "psImageStats() returned non-NULL");
        ok(myStats == tmpStats, "psImageStats() returned the original psStats struct");
        ok(fabs(myStats->sampleMean - meanNoMask) < .001, "psImageStats() calculated the mean correctly (no mask) (subImage)");
        printf("Expected %f, got %f\n", meanNoMask, myStats->sampleMean);

        // Calculate Sample Mean with mask (sub image)
        myStats = psImageStats(myStats, imgSub, mskSub, 1);
        ok(myStats != NULL, "psImageStats() returned non-NULL");
        ok(fabs(myStats->sampleMean - meanMask) < .001, "psImageStats() calculated the mean correctly (with mask) (subImage)");
    }

    psFree(myStats);
    psFree(img);
    psFree(msk);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}


#define I_POLY(X, Y) ((A) + (B * X) + (C * Y) + (D * X * Y) + (E * X * X) + (F * Y * Y))

void genericFitChebyF32Test(int numCols, int numRows)
{
    psMemId id = psMemGetId();
    const double A = 1.0;
    const double B = 2.0;
    const double C = 3.0;
    const double D = 4.0;
    const double E = 5.0;
    const double F = 6.0;
    const double ERROR_TOL = 0.1;
    psImage *tmpImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *outImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    for (int i=0;i<numRows;i++) {
        for (int j=0;j<numCols;j++) {
            tmpImage->data.F32[i][j] = I_POLY(((float) i), ((float) j));
        }
    }

    psPolynomial2D *chebys = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, 5, 5);
    skip_start(chebys == NULL, 5, "Skipping tests because psPolynomial2DAlloc() returned NULL");
    psPolynomial2D *chebys2 = psImageFitPolynomial(chebys, tmpImage);
    ok(chebys2 != NULL, "psImageFitPolynomial() returned non-NULL");
    ok(chebys2 == chebys, "psImageFitPolynomial() correctly recylced polynomial input");
    psImage *outImage2 = psImageEvalPolynomial(outImage, chebys);
    ok(outImage2 != NULL, "psImageEvalPolynomial() returned non-NULL");
    ok(outImage2 == outImage, "psImageEvalPolynomial() correctly recylced input image");

    bool errorFlag = false;
    for (int i=1 ; i < numRows ; i++) {
        for (int j=0 ; j < numCols ; j++) {
            float expect = tmpImage->data.F32[i][j];
            float actual = outImage->data.F32[i][j];
            if (fabs((actual - expect) / expect) > ERROR_TOL) {
                diag("ERROR: pixel [%d][%d] is %.2f should be %.2f\n", i, j, actual, expect);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageFitPolynomial() and psImageEvalPolynomial() produced the correct results (F32)");
    skip_end();
    psFree(tmpImage);
    psFree(outImage);
    psFree(chebys);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}

void genericFitChebyF64Test(int numCols, int numRows)
{
    psMemId id = psMemGetId();
    const double A = 1.0;
    const double B = 2.0;
    const double C = 3.0;
    const double D = 4.0;
    const double E = 5.0;
    const double F = 6.0;
    const double ERROR_TOL = 0.1;
    psImage *tmpImage = psImageAlloc(numCols, numRows, PS_TYPE_F64);
    psImage *outImage = psImageAlloc(numCols, numRows, PS_TYPE_F64);
    for (int i=0;i<numRows;i++) {
        for (int j=0;j<numCols;j++) {
            tmpImage->data.F64[i][j] = I_POLY(((float) i), ((float) j));
        }
    }

    psPolynomial2D *chebys = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, 5, 5);
    skip_start(chebys == NULL, 5, "Skipping tests because psPolynomial2DAlloc() returned NULL");
    psPolynomial2D *chebys2 = psImageFitPolynomial(chebys, tmpImage);
    ok(chebys2 != NULL, "psImageFitPolynomial() returned non-NULL");
    ok(chebys2 == chebys, "psImageFitPolynomial() correctly recylced polynomial input");
    psImage *outImage2 = psImageEvalPolynomial(outImage, chebys);
    ok(outImage2 != NULL, "psImageEvalPolynomial() returned non-NULL");
    ok(outImage2 == outImage, "psImageEvalPolynomial() correctly recylced input image");

    bool errorFlag = false;
    for (int i=1 ; i < numRows ; i++) {
        for (int j=0 ; j < numCols ; j++) {
            psF64 expect = tmpImage->data.F64[i][j];
            psF64 actual = outImage->data.F64[i][j];
            if (fabs((actual - expect) / expect) > ERROR_TOL) {
                diag("ERROR: pixel [%d][%d] is %.2f should be %.2f\n", i, j, actual, expect);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageFitPolynomial() and psImageEvalPolynomial() produced the correct results (F64)");
    skip_end();
    psFree(tmpImage);
    psFree(outImage);
    psFree(chebys);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}


void testPsImageFitPolynomial()
{
    psMemId id = psMemGetId();
    const int IMAGE_SIZE = 16;
    const int CHEBY_X_DIM = 8;
    const int CHEBY_Y_DIM = 8;
    const int THRESHOLD = 1;
    psImage *imgIn = psImageAlloc(IMAGE_SIZE, IMAGE_SIZE, PS_TYPE_F32);
    psImage *imgOut = psImageAlloc(IMAGE_SIZE, IMAGE_SIZE, PS_TYPE_F32);
    for (int i = 0;i < IMAGE_SIZE;i++) {
        for (int j = 0;j < IMAGE_SIZE;j++) {
            imgIn->data.F32[ i ][ j ] = 4.0;
            imgIn->data.F32[ i ][ j ] = (float) (i + j);
            imgIn->data.F32[ i ][ j ] = (float) (i + j) + (4.0 * (float) i);
            imgOut->data.F32[ i ][ j ] = 0.0;
        }
    }
    psPolynomial2D *my2DPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, CHEBY_X_DIM-1, CHEBY_Y_DIM-1);
    my2DPoly = psImageFitPolynomial(my2DPoly, imgIn);
    ok(my2DPoly != NULL, "psImageFitPolynomial() returned non-NULL");

    imgOut = psImageEvalPolynomial(imgOut, my2DPoly);
    bool errorFlag = false;
    for (int i = 0;i < IMAGE_SIZE;i++) {
        for (int j = 0;j < IMAGE_SIZE;j++) {
            if (fabs(imgOut->data.F32[ i ][ j ] - imgIn->data.F32[ i ][ j ]) > THRESHOLD) {
                diag("Pixel (%d, %d) is %.2f, should be %.2f\n", i, j,
                     imgOut->data.F32[ i ][ j ],
                     imgIn->data.F32[ i ][ j ]);
                errorFlag = true;
            }

        }
    }
    ok(!errorFlag, "psImageFitPolynomial() and psImageEvalPolynomial() produced the correct result");
    psFree(imgIn);
    psFree(imgOut);
    psFree(my2DPoly);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}

void genericpsImageHistogramTest(int numRows, int numCols, int numBins)
{
    if ((0 != numRows%4) ||
        (0 != numRows%numBins) ||
        (0 != numBins%2)) {
        ok(false, "numRows must be a multiple of 4, and must be a integer multiple of numBins");
    }

    psImage *img = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    psImage *msk = psImageAlloc(numCols, numRows, PS_TYPE_MASK);
    // XXX: Do subRegion tests later
//    psImage *imgSub = psImageSubset(img, psRegionSet(numRows/4, 3*numRows/4,
//                                    numCols/4, 3*numCols/4));
//    psImage *mskSub = psImageSubset(img, psRegionSet(numRows/4, 3*numRows/4,
//                                    numCols/4, 3*numCols/4));
    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols; j++) {
            img->data.F32[i][j] = 0.4 + (float) (i);
            if (i >= numRows/2) {
                msk->data.PS_TYPE_MASK_DATA[i][j] = 1;
            } else {
                msk->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
    }

    if (0) {
        for (int i = 0; i < numRows; i++) {
            for (int j = 0; j < numCols; j++) {
                printf("(%.1f) ", img->data.F32[i][j]);
            }
            printf("\n");
        }
        for (int i = 0; i < numRows; i++) {
            for (int j = 0; j < numCols; j++) {
                printf("(%d) ", msk->data.PS_TYPE_MASK_DATA[i][j]);
            }
            printf("\n");
        }
    }
    
    // Calculate Histogram with no mask
    {
        psMemId id = psMemGetId();
        psHistogram *hist = psHistogramAlloc(0.0, (float) (numRows), numBins);
        psHistogram *hist2 = psImageHistogram(hist, img, NULL, 0);
        ok(hist2 != NULL, "psImageHistogram() returned non-NULL");
        skip_start(hist2 == NULL, 1, "Skipping tests because psImageHistogram() returned NULL");
        ok(hist2 == hist, "psImageHistogram() correctly recycled the input psHistogram");
        bool errorFlag = false;
        for (int i = 0;i < numBins;i++) {
            if (hist2->nums->data.F32[i] != (numCols * (numRows/numBins))) {
                diag("ERROR: Bin number %d bounds: (%.1f - %.1f) data (%.2f)\n", i,
                      hist2->bounds->data.F32[i],
                      hist2->bounds->data.F32[i+1],
                      hist2->nums->data.F32[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psImageHistogram() produced the correct results (no mask)");
        skip_end();
        psFree(hist);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Calculate Histogram with mask
    {
        psMemId id = psMemGetId();
        psHistogram *hist = psHistogramAlloc(0.0, (float) (numRows), numBins);
        psHistogram *hist2 = psImageHistogram(hist, img, msk, 1);
        ok(hist2 != NULL, "psImageHistogram() returned non-NULL");
        skip_start(hist2 == NULL, 1, "Skipping tests because psImageHistogram() returned NULL");
        ok(hist2 == hist, "psImageHistogram() correctly recycled the input psHistogram");
        bool errorFlag = false;
        for (int i = 0 ; i < numBins ; i++) {
            if (i < numBins/2) {
                if (hist2->nums->data.F32[i] != (numCols * (numRows/numBins))) {
                    diag("ERROR: Bin number %d bounds: (%.1f - %.1f) data (%.2f)\n", i,
                          hist2->bounds->data.F32[i],
                          hist2->bounds->data.F32[i+1],
                          hist2->nums->data.F32[i]);
                    errorFlag = true;
                }
            } else {
                if (hist2->nums->data.F32[i] != 0) {
                    diag("ERROR: Bin number %d bounds: (%.1f - %.1f) data (%.2f)\n", i,
                          hist2->bounds->data.F32[i],
                          hist2->bounds->data.F32[i+1],
                          hist2->nums->data.F32[i]);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageHistogram() produced the correct results (no mask)");
        skip_end();
        psFree(hist);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(img);
    psFree(msk);
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(58);

    // --------------------------------------------------------------
    // psImageCountPixelMask()
    // Ensure psImageCountPixelMask returns -1 for NULL input image
    // Following should generate error
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psRegion reg;
        reg.x0 = 0;
        reg.x1 = 1;
        reg.y0 = 0;
        reg.y1 = 4;
        psS32 numPix = psImageCountPixelMask(NULL, reg, 1);
        ok(numPix == -1, "psImageCountPixelMask() returned -1 for NULL input image");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Ensure psImageCountPixelMask returns -1 for wrong input image type
    // Following should generate error
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImage *in = psImageAlloc(16, 16, PS_TYPE_F32);
        psRegion reg;
        reg.x0 = 0;
        reg.x1 = 1;
        reg.y0 = 0;
        reg.y1 = 4;
        psS32 numPix = psImageCountPixelMask(in, reg, 1);
        ok(numPix == -1, "psImageCountPixelMask() returned -1 for incorrect input image type");
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Ensure psImageCountPixelMask returns -1 for bad region
    // Following should generate error
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImage *in = psImageAlloc(16, 16, PS_TYPE_MASK);
        psRegion reg;
        reg.x0 = -1;
        reg.x1 = 0;
        reg.y0 = 0;
        reg.y1 = 0;
        psS32 numPix = psImageCountPixelMask(in, reg, 1);
        ok(numPix == -1, "psImageCountPixelMask() returned -1 for bad region x0");
        reg.x0 = 0;
        reg.y0 = -1;
        numPix = psImageCountPixelMask(in, reg, 1);
        ok(numPix == -1, "psImageCountPixelMask() returned -1 for bad region y0");
        // XXX: Should this fail?
        if (0) {
            reg.x0 = 1;
            reg.x1 = 1;
            reg.y0 = 1;
            reg.y1 = 1;
            numPix = psImageCountPixelMask(in, reg, 1);
            ok(numPix == -1, "psImageCountPixelMask() returned -1 for bad region ");
        }
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    genericImageCountPixelMaskTest(10, 10);



    // --------------------------------------------------------------
    // psImageStats()
    // Ensure psImageStats() returns NULL for NULL input image
    // Following should generate error
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psStats *myStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psStats *myStats2 = psImageStats(myStats, NULL, NULL, 0);
        ok(myStats2 == NULL, "psImageStats() returned NULL with NULL input image");
        psFree(myStats);
        psFree(myStats2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Ensure psImageStats() returns NULL for NULL psStats
    // Following should generate error
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(10, 10, PS_TYPE_F32);
        psStats *myStats = psImageStats(NULL, img, NULL, 0);
        ok(myStats == NULL, "psImageStats() returned NULL with NULL psStats arg");
        psFree(img);
        psFree(myStats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    genericImageStatsTest(10, 10);



    // --------------------------------------------------------------
    // psImageFitPolynomial() and psImageEvalPolynomial()
    // Following should be an error message for NULL coeffs argument
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(10, 10, PS_TYPE_F32);
        psImage *img2 = psImageEvalPolynomial(img, NULL);
        ok(img2 == NULL, "psImageEvalPolynomial() returned NULL with NULL coeffs argument");
        psFree(img);
        psFree(img2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Following should be a error message for NULL input argument
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psPolynomial2D *poly2D = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, 5, 5);
        psImage *img = psImageEvalPolynomial(NULL, poly2D);
        ok(img == NULL, "psImageEvalPolynomial() returned NULL with NULL input image");
        psFree(img);
        psFree(poly2D);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Following should be an error message for NULL coeffs argument
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(10, 10, PS_TYPE_F32);
        psPolynomial2D *poly2D = psImageFitPolynomial(NULL, img);
        ok(poly2D == NULL, "psImageFitPolynomial() returned a NULL with NULL coeffs arg");
        psFree(img);
        psFree(poly2D);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Following should be a error message for NULL input argument
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psPolynomial2D *poly2D = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, 5, 5);
        psPolynomial2D *poly2D2 = psImageFitPolynomial(poly2D, NULL);
        ok(poly2D2 == NULL, "psImageFitPolynomial() returned a NULL with NULL input image");
        psFree(poly2D);
        psFree(poly2D2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    genericFitChebyF32Test(10, 10);
    genericFitChebyF64Test(10, 10);
    // XXX: This next test is not necessary.  Previous two functions make it redundant.
    testPsImageFitPolynomial();



    // --------------------------------------------------------------
    // psImageHistogram()
    // Following should be a error message for NULL psHistogram argument
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(10, 10, PS_TYPE_F32);
        psHistogram *hist = psImageHistogram(NULL, img, NULL, 0);
        ok(hist == NULL, "psImageHistogram() returned NULL for NULL psHistogram arg");
        psFree(img);
        psFree(hist);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Following should be a error message for NULL psImage argument
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psHistogram *hist = psHistogramAlloc(0.0, 100000.0, 1000);
        psHistogram *hist2 = psImageHistogram(hist, NULL, NULL, 0);
        ok(hist2 == NULL, "psImageHistogram() returned NULL for NULL psHistogram arg");
        psFree(hist);
        psFree(hist2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Following should be a error message for incorrect mask type
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(10, 10, PS_TYPE_F32);
        psHistogram *hist = psHistogramAlloc(0.0, 100000.0, 1000);
        psImage *msk = psImageAlloc(10, 10, PS_TYPE_S8);
        psHistogram *hist2 = psImageHistogram(hist, img, msk, 1);
        ok(hist2 == NULL, "psImageHistogram() returned NULL for incorrect mask type");
        psFree(hist);
        psFree(hist2);
        psFree(img);
        psFree(msk);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    genericpsImageHistogramTest(16, 15, 4);
}
