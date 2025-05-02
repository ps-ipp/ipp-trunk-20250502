/*
*  C Implementation: %{MODULE}
*
* Description:
*
*
* Author: %{AUTHOR} <%{EMAIL}>, (C) %{YEAR}
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include <stdio.h>
#include <float.h>
#include <math.h>

#include "pslib_strict.h"
#include "psTest.h"

static psS32 testPsImageHistogram(void);
static psS32 testPsImageStats(void);
static psS32 testPsImageFitPolynomial(void);
static psS32 testPsImagePixelInterpolate(void);
static psS32 testPsImageEvalPolynom(void);
static psS32 testImageCountPixel(void);

static bool FitChebyF32(int numCols, int numRows);
static bool FitChebyF64(int numCols, int numRows);

testDescription tests[] = {
                              {testPsImageHistogram, 0, "psImageHistogram", 0, false},
                              {testPsImageStats, 1, "psImageStats", 0, false},
                              {testPsImageFitPolynomial, 2, "psImageFitPolynomial", 0, false},
                              {testPsImagePixelInterpolate, 3, "psImagePixelInterpolate", 0, false},
                              {testPsImageEvalPolynom, 4, "psImageEvalPolynom()", 0, false},
                              {testImageCountPixel, 5, "psImageCountPixel", 0, false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);

    return ! runTestSuite(stderr,"psImage",tests,argc,argv);
}



static psS32 testPsImageHistogram(void)
{
    const int NUM_BINS = 20;
    const int N = 32;
    const int M = 64;

    psHistogram * myHist = NULL;
    psHistogram *myHist2 = NULL;
    psImage *tmpImage = NULL;
    psImage *tmpImage2 = NULL;
    psImage *tmpMask = NULL;
    psImage *tmpMask2 = NULL;
    psS32 testStatus = true;
    psS32 nb = 0;
    psS32 i = 0;
    psS32 j = 0;
    psS32 IMAGE_X_SIZE = 0;
    psS32 IMAGE_Y_SIZE = 0;
    psS32 currentId = 0;

    currentId = psMemGetId();
    for ( nb = 0;nb < 7;nb++ ) {
        if ( nb == 0 ) {
            IMAGE_X_SIZE = 1;
            IMAGE_Y_SIZE = 1;
        }
        if ( nb == 1 ) {
            IMAGE_X_SIZE = 1;
            IMAGE_Y_SIZE = N;
        }
        if ( nb == 2 ) {
            IMAGE_X_SIZE = N;
            IMAGE_Y_SIZE = 1;
        }
        if ( nb == 3 ) {
            IMAGE_X_SIZE = N;
            IMAGE_Y_SIZE = N;
        }
        if ( nb == 4 ) {
            IMAGE_X_SIZE = N;
            IMAGE_Y_SIZE = M;
        }
        if ( nb == 5 ) {
            IMAGE_X_SIZE = M;
            IMAGE_Y_SIZE = N;
        }
        if ( nb == 6 ) {
            IMAGE_X_SIZE = M;
            IMAGE_Y_SIZE = N;
        }
        fprintf(stderr, "*******************************\n" );
        if (nb != 6) {
            fprintf(stderr, "* IMAGE SIZE is (%d by %d)\n", IMAGE_X_SIZE, IMAGE_Y_SIZE );
        } else {
            fprintf(stderr, "* IMAGE SUBSET SIZE is (%d by %d)\n", IMAGE_X_SIZE, IMAGE_Y_SIZE );
        }
        fprintf(stderr, "*******************************\n" );
        /*********************************************************************/
        /*  Allocate and initialize data structures                      */
        /*********************************************************************/
        if (nb != 6) {
            tmpImage = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_F32 );
            tmpMask = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_U8 );
            tmpImage2 = NULL;
            tmpMask2 = NULL;
        } else {
            tmpImage2 = psImageAlloc( IMAGE_X_SIZE*2, IMAGE_Y_SIZE*2, PS_TYPE_F32 );
            tmpMask2 = psImageAlloc( IMAGE_X_SIZE*2, IMAGE_Y_SIZE*2, PS_TYPE_U8 );
            tmpImage = psImageSubset(tmpImage2, psRegionSet(IMAGE_X_SIZE/4, IMAGE_X_SIZE/4+IMAGE_X_SIZE,
                                     IMAGE_Y_SIZE/4, IMAGE_Y_SIZE/4+IMAGE_Y_SIZE));
            tmpMask= psImageSubset(tmpMask2, psRegionSet(IMAGE_X_SIZE/4, IMAGE_X_SIZE/4+IMAGE_X_SIZE,
                                   IMAGE_Y_SIZE/4, IMAGE_Y_SIZE/4+IMAGE_Y_SIZE));
        }


        for ( i = 0;i < tmpImage->numRows;i++ ) {
            for ( j = 0;j < tmpImage->numCols;j++ ) {
                tmpImage->data.F32[ i ][ j ] = ( float ) ( i + j + 0.1 );
            }
        }


        for ( i = 0;i < tmpMask->numRows;i++ ) {
            for ( j = 0;j < tmpMask->numCols;j++ ) {
                if ( ( i > ( tmpMask->numRows / 2 ) ) &&
                        ( j > ( tmpMask->numCols / 2 ) ) ) {
                    tmpMask->data.U8[ i ][ j ] = 1;
                } else {
                    tmpMask->data.U8[ i ][ j ] = 0;
                }
            }
        }

        /*************************************************************************/
        /*  Calculate Histogram with no mask                             */
        /*************************************************************************/

        myHist = psHistogramAlloc( 0.0, ( float ) ( IMAGE_X_SIZE + IMAGE_Y_SIZE ),
                                   NUM_BINS );
        myHist = psImageHistogram( myHist, tmpImage, NULL, 0 );
        for ( i = 0;i < NUM_BINS;i++ ) {
            fprintf(stderr, "Bin number %d bounds: (%.1f - %.1f) data (%.2f)\n", i,
                    myHist->bounds->data.F32[ i ],
                    myHist->bounds->data.F32[ i + 1 ],
                    myHist->nums->data.F32[ i ] );
        }
        psFree( myHist );

        /*************************************************************************/
        /*  Calculate Histogram with mask                                */
        /*************************************************************************/

        myHist = psHistogramAlloc( 0.0, ( float ) ( IMAGE_X_SIZE + IMAGE_Y_SIZE ),
                                   NUM_BINS );
        myHist = psImageHistogram( myHist, tmpImage, tmpMask, 1 );
        for ( i = 0;i < NUM_BINS;i++ ) {
            fprintf( stderr, "Bin number %d bounds: (%.2f - %.2f) data (%.2f)\n", i,
                     myHist->bounds->data.F32[ i ],
                     myHist->bounds->data.F32[ i + 1 ],
                     myHist->nums->data.F32[ i ] );
        }

        /*************************************************************************/
        /*  Deallocate data structures                                   */
        /*************************************************************************/
        psFree( myHist );
        psFree( tmpImage );
        psFree( tmpImage2 );
        psFree( tmpMask );
        psFree( tmpMask2 );
    }
    tmpImage = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_F32 );
    myHist = psHistogramAlloc( 0.0, ( float ) ( IMAGE_X_SIZE + IMAGE_Y_SIZE ),
                               NUM_BINS );
    myHist2 = psImageHistogram( NULL, tmpImage, NULL, 0 );
    if ( myHist2 != NULL ) {
        fprintf(stderr, "ERROR: myHist2 not equal to NULL\n" );
    }

    myHist2 = psImageHistogram( myHist, NULL, NULL, 0 );
    myHist2 = psImageHistogram( NULL, tmpImage, NULL, 0 );
    if ( myHist2 != NULL ) {
        fprintf(stderr, "ERROR: myHist2 not equal to NULL\n" );
    }


    tmpMask = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_S8 );
    for ( i = 0;i < tmpMask->numRows;i++ ) {
        for ( j = 0;j < tmpMask->numCols;j++ ) {
            if ( ( i > ( tmpMask->numRows / 2 ) ) &&
                    ( j > ( tmpMask->numCols / 2 ) ) ) {
                tmpMask->data.S8[ i ][ j ] = 1;
            } else {
                tmpMask->data.S8[ i ][ j ] = 0;
            }
        }
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for invalid mask type.");
    myHist2 = psImageHistogram(myHist,tmpImage,tmpMask,1);
    if (myHist2 != NULL) {
        fprintf(stderr,"ERROR: myHist2 not equal to NULL\n");
    }

    psFree( tmpMask );
    psFree(myHist);
    psFree(myHist2);
    psFree( tmpImage );

    return ( !testStatus );
}

static psS32 testPsImageFitPolynomial()
{
    const int IMAGE_SIZE = 16;
    const int CHEBY_X_DIM = 8;
    const int CHEBY_Y_DIM = 8;
    const int THRESHOLD = 10;

    psStats * myStats = NULL;
    psImage *tmpImage = NULL;
    psImage *outImage = NULL;
    psImage *nullImage = NULL;
    psPolynomial2D *my2DPoly = NULL;
    psPolynomial2D *null2DPoly = NULL;
    psS32 testStatus = true;
    psS32 i = 0;
    psS32 j = 0;
    psS32 currentId = 0;

    currentId = psMemGetId();
    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    tmpImage = psImageAlloc( IMAGE_SIZE, IMAGE_SIZE, PS_TYPE_F32 );
    outImage = psImageAlloc( IMAGE_SIZE, IMAGE_SIZE, PS_TYPE_F32 );
    for ( i = 0;i < IMAGE_SIZE;i++ ) {
        for ( j = 0;j < IMAGE_SIZE;j++ ) {
            tmpImage->data.F32[ i ][ j ] = 4.0;
            tmpImage->data.F32[ i ][ j ] = ( float ) ( i + j );
            tmpImage->data.F32[ i ][ j ] = ( float ) ( i + j ) + ( 4.0 * ( float ) i );
            outImage->data.F32[ i ][ j ] = 0.0;
        }
    }
    my2DPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, CHEBY_X_DIM-1, CHEBY_Y_DIM-1);
    /*************************************************************************/
    /*  Calculate Chebyshev Polynomials, no mask                     */
    /*************************************************************************/
    my2DPoly = psImageFitPolynomial( my2DPoly, tmpImage );
    if (my2DPoly == NULL) {
        printf("TEST ERROR: psImageFitPolynomial() returned NULL.\n");
        testStatus = false;
    }
    for ( i = 0;i < CHEBY_X_DIM;i++ ) {
        for ( j = 0;j < CHEBY_Y_DIM;j++ ) {
            fprintf(stderr, "Cheby Polynomial (%d, %d) coefficient is %.2f\n", i, j,
                    0.0001f+my2DPoly->coeff[ i ][ j ] );
        }
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error message for NULL coeffs argument.");
    null2DPoly = psImageFitPolynomial( NULL, tmpImage );
    if ( null2DPoly != NULL ) {
        fprintf(stderr,"ERROR: psImageFitPolynomial did not return NULL with invalid coeffs argument.");
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be a error message for NULL input argument.");
    null2DPoly = psImageFitPolynomial( null2DPoly, NULL );
    if ( null2DPoly != NULL ) {
        fprintf(stderr,"ERROR: psImageFitPolynomial did not return NULL with null input image.");
    }

    /*************************************************************************/
    /*  Evaluate Chebyshev Polynomials, no mask                      */
    /*************************************************************************/
    outImage = psImageEvalPolynomial( outImage, my2DPoly );
    for ( i = 0;i < IMAGE_SIZE;i++ ) {
        for ( j = 0;j < IMAGE_SIZE;j++ ) {

            //             fprintf(stderr,"pixel[%d][%d] is (%.2f, %.2f)\n", i, j,
            //                     tmpImage->data.F32[i][j],
            //                     outImage->data.F32[i][j]);
            if ( fabs( outImage->data.F32[ i ][ j ] - tmpImage->data.F32[ i ][ j ] ) > THRESHOLD ) {
                fprintf(stderr, "Pixel (%d, %d) is %.2f, should be %.2f\n", i, j,
                        outImage->data.F32[ i ][ j ],
                        tmpImage->data.F32[ i ][ j ] );
            }

        }
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error message for NULL coeffs argument.");
    nullImage = psImageEvalPolynomial( outImage, NULL );
    if ( nullImage != NULL ) {
        fprintf(stderr,"ERROR: psImageEvalPolynomial did not return NULL with invalid coeffs argument.");
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be a error message for NULL input argument.");
    nullImage = psImageEvalPolynomial( NULL, my2DPoly );
    if ( nullImage != NULL ) {
        fprintf(stderr,"ERROR: psImageEvalPolynomial did not return NULL with null input image.");
    }

    /*************************************************************************/
    /*  Deallocate data structures                                   */
    /*************************************************************************/
    psFree( myStats );
    psFree( tmpImage );
    psFree( outImage );
    psFree( my2DPoly );

    return ( !testStatus );
}

static psS32 testPsImagePixelInterpolate()
{
    const int IMAGE_SIZE = 10;

    psImage *tmpImage   = NULL;
    psS32 testStatus      = true;
    psS32 i               = 0;
    psS32 j               = 0;
    float pixel         = 0.0;
    float x             = 0.0;
    float y             = 0.0;

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/
    tmpImage = psImageAlloc(IMAGE_SIZE, IMAGE_SIZE, PS_TYPE_F32);
    for (i=0;i<IMAGE_SIZE;i++) {
        for (j=0;j<IMAGE_SIZE;j++) {
            tmpImage->data.F32[i][j] = 4.0;
            tmpImage->data.F32[i][j] = (float) (i + j) + (4.0 * (float) i);
            tmpImage->data.F32[i][j] = (float) (i + j);
            fprintf(stderr,"%.1f ", tmpImage->data.F32[i][j]);
        }
        fprintf(stderr,"\n");
    }
    for (i=0;i<IMAGE_SIZE-1;i++) {
        for (j=0;j<IMAGE_SIZE-1;j++) {
            x = 0.2 + (float) i;
            y = 0.2 + (float) j;
            pixel = psImagePixelInterpolate(tmpImage,
                                            x, y,
                                            NULL, 0,
                                            0,
                                            PS_INTERPOLATE_BILINEAR);
            fprintf(stderr,"%.1f ", pixel);
        }
        fprintf(stderr,"\n");
    }

    for (i=0;i<IMAGE_SIZE-1;i++) {
        for (j=0;j<IMAGE_SIZE-1;j++) {
            x = 0.2 + (float) i;
            y = 0.2 + (float) j;
            pixel = psImagePixelInterpolate(tmpImage,
                                            x, y,
                                            NULL,0,
                                            0,
                                            PS_INTERPOLATE_BILINEAR);
            fprintf(stderr,"image[%.1f][%.1f] is interpolated at %.1f\n", x, y, pixel);
        }
    }

    psFree(tmpImage);

    return (!testStatus);
}

#define I_POLY(X, Y) \
((A) + (B * X) + (C * Y) + (D * X * Y) + (E * X * X) + (F * Y * Y)) \

static bool FitChebyF32(int numCols, int numRows)
{
    const double A = 1.0;
    const double B = 2.0;
    const double C = 3.0;
    const double D = 4.0;
    const double E = 5.0;
    const double F = 6.0;
    const double ERROR_TOL = 0.1;

    psImage *tmpImage     = NULL;
    psImage *outImage     = NULL;
    bool testStatus      = true;
    psS32 i               = 0;
    psS32 j               = 0;
    float chi2 = 0.0;


    fprintf(stderr,"psImageFitPolynomial(), psImageEvalPolynom(): (%d by %d)\n", numRows, numCols);

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/

    tmpImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    outImage = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            tmpImage->data.F32[i][j] = I_POLY(((float) i), ((float) j));
        }
    }

    psPolynomial2D *chebys = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, 5, 5);

    chebys = psImageFitPolynomial(chebys, tmpImage);

    outImage = psImageEvalPolynomial(outImage, chebys);

    chi2 = 0.0;
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            float expect = tmpImage->data.F32[i][j];
            float actual = outImage->data.F32[i][j];
            chi2+= (actual-expect) * (actual-expect);
            if (fabs((actual - expect) / expect) > ERROR_TOL) {
                fprintf(stderr,"pixel [%d][%d] is %.2f should be %.2f\n", i, j, actual, expect);
            }
        }
    }
    chi2/= ((float) (numCols * numRows));
    fprintf(stderr,"The chi-squared per pixel is %.2f\n", chi2);

    psFree(tmpImage);
    psFree(outImage);
    psFree(chebys);

    /*************************************************************************/
    /*  Deallocate data structures                                   */
    /*************************************************************************/

    return(testStatus);
}

static bool FitChebyF64(int numCols, int numRows)
{
    const double A = 1.0;
    const double B = 2.0;
    const double C = 3.0;
    const double D = 4.0;
    const double E = 5.0;
    const double F = 6.0;
    const double ERROR_TOL = 0.1;

    psImage *tmpImage     = NULL;
    psImage *outImage     = NULL;
    bool testStatus      = true;
    psS32 i               = 0;
    psS32 j               = 0;
    double chi2 = 0.0;


    fprintf(stderr,"psImageFitPolynomial(), psImageEvalPolynom(): (%d by %d)\n", numRows, numCols);

    /*************************************************************************/
    /*  Allocate and initialize data structures                      */
    /*************************************************************************/

    tmpImage = psImageAlloc(numCols, numRows, PS_TYPE_F64);
    outImage = psImageAlloc(numCols, numRows, PS_TYPE_F64);
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            tmpImage->data.F64[i][j] = I_POLY(((double) i), ((double) j));
        }
    }

    psPolynomial2D *chebys = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, 5, 5);

    chebys = psImageFitPolynomial(chebys, tmpImage);

    outImage = psImageEvalPolynomial(outImage, chebys);

    chi2 = 0.0;
    for (i=0;i<numRows;i++) {
        for (j=0;j<numCols;j++) {
            double expect = tmpImage->data.F64[i][j];
            double actual = outImage->data.F64[i][j];
            chi2+= (actual-expect) * (actual-expect);
            if (fabs((actual - expect) / expect) > ERROR_TOL) {
                fprintf(stderr,"pixel [%d][%d] is %.2f should be %.2f\n", i, j, actual, expect);
            }
        }
    }
    chi2/= ((double) (numCols * numRows));
    fprintf(stderr,"The chi-squared per pixel is %.2f\n", chi2);

    psFree(tmpImage);
    psFree(outImage);
    psFree(chebys);

    /*************************************************************************/
    /*  Deallocate data structures                                   */
    /*************************************************************************/

    return(testStatus);
}

static psS32 testPsImageEvalPolynom()
{
    if (!FitChebyF32(1, 1)) {
        return 1;
    }
    if (!FitChebyF32(1, 5)) {
        return 2;
    }
    if (!FitChebyF32(5, 1)) {
        return 3;
    }
    if (!FitChebyF32(5, 5)) {
        return 4;
    }
    if (!FitChebyF64(1, 1)) {
        return 5;
    }
    if (!FitChebyF64(1, 5)) {
        return 6;
    }
    if (!FitChebyF64(5, 1)) {
        return 7;
    }
    if (!FitChebyF64(5, 5)) {
        return 8;
    }
    return 0;
}

psS32 testImageCountPixel(void)
{
    long numPix = 0;
    long numPix2 = 0;
    psImage *in = NULL;
    psRegion reg;
    reg.x0 = 0;
    reg.x1 = 1;
    reg.y0 = 0;
    reg.y1 = 4;

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    numPix = psImageCountPixelMask(in, reg, 1);
    if (numPix != -1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask failed to return -1 for NULL Vector input.\n");
        return 1;
    }

    numPix = 0;
    in = psImageAlloc(5, 5, PS_TYPE_S32);

    psImage *in2 = NULL;
    in2 = psImageAlloc(1, 5, PS_TYPE_U8);

    in->data.S32[0][0] = 0;
    in->data.S32[1][0] = 1;
    in->data.S32[2][0] = 0;
    in->data.S32[3][0] = 1;
    in->data.S32[4][0] = 0;
    in2->data.U8[0][0] = 0;
    in2->data.U8[1][0] = 1;
    in2->data.U8[2][0] = 0;
    in2->data.U8[3][0] = 1;
    in2->data.U8[4][0] = 0;

    //    numPix = psImageCountPixelMask(in, reg, 1);
    numPix2 = psImageCountPixelMask(in2, reg, 1);
    numPix = -1;
    //numPix should be -1 from using S32's
    /*    if (numPix != -1) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                    "psImageCountPixelMask failed to return -1 for wrong type of Image input.\n");
            return 2;
        }
    */
    if (numPix2 == -1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask returned -1 for Correct Image input\n");
        return 3;
    }
    if (numPix2 != 2) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask returned incorrect pixel count %ld (!=2)\n", numPix2);
        return 4;
    }
    //Test for smaller region than image returns only 1 pixel counted
    reg.y1 = 2.1;
    numPix2 = psImageCountPixelMask(in2, reg, 1);
    if (numPix2 != 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask returned incorrect pixel count %ld\n", numPix2);
        return 5;
    }
    //Test for -1 upper bnds in region = 5, 1 limits = 2 pixels returned
    reg.x1 = 1;
    reg.y1 = -1;
    numPix2 = psImageCountPixelMask(in2, reg, 1);
    if (numPix2 != 2) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask returned incorrect pixel count %ld\n", numPix2);
        return 100;
    }

    //Test for invalid region (1 pixel, lower = upper)
    reg.x0 = 1;
    reg.y0 = 1;
    reg.x1 = 1;
    reg.y1 = 1;
    //    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    numPix2 = psImageCountPixelMask(in2, reg, 1);
    if (numPix2 != 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask failed to return correct value (0) for 1 pixel region.\n");
        return 9;
    }

    //Test for whole image (region = 0,0 0,0 )
    reg.x0 = 0;
    reg.x1 = 0;
    reg.y0 = 0;
    reg.y1 = 0;
    numPix2 = psImageCountPixelMask(in2, reg, 1);
    if (numPix2 != 2) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask returned incorrect pixel count %ld\n", numPix2);
        return 10;
    }

    //Test a subimage.
    reg.x1 = 1;
    reg.y0 = 1;
    reg.y1 = 5;
    numPix2 = -1;
    in2->row0 = 1;
    numPix2 = psImageCountPixelMask(in2, reg, 1);
    if (numPix2 != 2) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCountPixelMask returned incorrect pixel count %ld (!=2)\n", numPix2);
        return 11;
    }

    psFree(in);
    psFree(in2);
    return 0;
}

static psS32 testPsImageStats()
{
    psTraceSetLevel("p_psVectorRobustStats", 0);

    const int N  = 32;
    const int M = 64;

    psStats * myStats = NULL;
    psStats *myStats2 = NULL;
    psImage *tmpImage = NULL;
    psImage *tmpImage2 = NULL;
    psImage *tmpMask = NULL;
    psImage *tmpMask2 = NULL;
    psS32 testStatus = true;
    psS32 nb = 0;
    psS32 i = 0;
    psS32 j = 0;
    psS32 IMAGE_X_SIZE = 0;
    psS32 IMAGE_Y_SIZE = 0;
    psS32 currentId = 0;

    currentId = psMemGetId();
    for ( nb = 0;nb < 7;nb++ ) {
        if ( nb == 0 ) {
            IMAGE_X_SIZE = 1;
            IMAGE_Y_SIZE = 1;
        }
        if ( nb == 1 ) {
            IMAGE_X_SIZE = 1;
            IMAGE_Y_SIZE = N;
        }
        if ( nb == 2 ) {
            IMAGE_X_SIZE = N;
            IMAGE_Y_SIZE = 1;
        }
        if ( nb == 3 ) {
            IMAGE_X_SIZE = N;
            IMAGE_Y_SIZE = N;
        }
        if ( nb == 4 ) {
            IMAGE_X_SIZE = N;
            IMAGE_Y_SIZE = M;
        }
        if ( nb == 5 ) {
            IMAGE_X_SIZE = M;
            IMAGE_Y_SIZE = N;
        }
        if ( nb == 6 ) {
            IMAGE_X_SIZE = M;
            IMAGE_Y_SIZE = N;
        }
        fprintf(stderr, "*******************************\n" );
        if (nb != 6) {
            fprintf(stderr, "* IMAGE SIZE is (%d by %d)\n", IMAGE_X_SIZE, IMAGE_Y_SIZE );
        } else {
            fprintf(stderr, "* IMAGE SUBSET SIZE is (%d by %d)\n", IMAGE_X_SIZE, IMAGE_Y_SIZE );
        }
        fprintf(stderr, "*******************************\n" );

        /*************************************************************************/
        /*  Allocate and initialize data structures                      */
        /*************************************************************************/
        if (nb != 6) {
            tmpImage = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_F32 );
            tmpMask = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_U8 );
            tmpImage2 = NULL;
            tmpMask2 = NULL;
        } else {
            tmpImage2 = psImageAlloc( IMAGE_X_SIZE*2, IMAGE_Y_SIZE*2, PS_TYPE_F32 );
            tmpMask2 = psImageAlloc( IMAGE_X_SIZE*2, IMAGE_Y_SIZE*2, PS_TYPE_U8 );
            tmpImage = psImageSubset(tmpImage2, psRegionSet(IMAGE_X_SIZE/4, IMAGE_X_SIZE/4+IMAGE_X_SIZE,
                                     IMAGE_Y_SIZE/4, IMAGE_Y_SIZE/4+IMAGE_Y_SIZE));
            tmpMask = psImageSubset(tmpMask2, psRegionSet(IMAGE_X_SIZE/4, IMAGE_X_SIZE/4+IMAGE_X_SIZE,
                                    IMAGE_Y_SIZE/4, IMAGE_Y_SIZE/4+IMAGE_Y_SIZE));
        }

        for ( i = 0;i < tmpImage->numRows;i++ ) {
            for ( j = 0;j < tmpImage->numCols;j++ ) {
                tmpImage->data.F32[ i ][ j ] = ( float ) ( i + j );
            }
        }

        for ( i = 0;i < tmpMask->numRows;i++ ) {
            for ( j = 0;j < tmpMask->numCols;j++ ) {
                if ( ( i > ( tmpMask->numRows / 2 ) ) &&
                        ( j > ( tmpMask->numCols / 2 ) ) ) {
                    tmpMask->data.U8[ i ][ j ] = 1;
                } else {
                    tmpMask->data.U8[ i ][ j ] = 0;
                }
            }
        }

        myStats = psStatsAlloc( PS_STAT_SAMPLE_MEAN );
        /*************************************************************************/
        /*  Calculate Sample Mean with no mask                           */
        /*************************************************************************/
        psStats* tmpStats = myStats;
        myStats = psImageStats( myStats, tmpImage, NULL, 0 );
        if ( myStats != tmpStats ) {
            fprintf(stderr,"ERROR: input psStats not equal to return psStats\n");
            psAbort("Failed input psStats equal to returned psStats");
        }
        fprintf(stderr, "The sample mean was %.2f\n", myStats->sampleMean );

        /*************************************************************************/
        /*  Calculate Sample Mean with mask                              */
        /*************************************************************************/

        myStats = psImageStats( myStats, tmpImage, tmpMask, 1 );
        if (myStats == NULL) {
            fprintf(stderr,"TEST ERROR: psImageStats() returned NULL.\n");
        } else {
            fprintf(stderr, "The sample mean was %.2f\n", myStats->sampleMean );
        }

        /*************************************************************************/
        /*  Deallocate data structures                                   */
        /*************************************************************************/
        psFree( myStats );
        psFree( tmpImage );
        psFree( tmpMask );
        psFree( tmpImage2 );
        psFree( tmpMask2 );
    }

    /*************************************************************************/
    /*  Test With Various Null Inputs                                        */
    /*************************************************************************/

    tmpImage = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_F32 );
    tmpMask = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_U8 );
    myStats = psStatsAlloc( 0 );

    myStats2 = psImageStats( myStats, NULL, NULL, 0 );
    if ( myStats2 != NULL ) {
        fprintf(stderr, "ERROR: myStats2 = psImageStats(myStats, NULL, NULL, 0) != NULL\n" );
    }

    myStats2 = psImageStats( NULL, tmpImage, NULL, 0 );
    if ( myStats2 != NULL ) {
        fprintf(stderr, "ERROR: myStats2 = psImageStats(NULL, tmpImage, NULL, 0) != NULL\n" );
    }

    myStats2 = psImageStats( myStats, tmpImage, NULL, 0 );

    psFree( myStats );
    psFree( tmpImage );
    psFree( tmpMask );

    tmpImage = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_F32);
    tmpMask = psImageAlloc( IMAGE_X_SIZE, IMAGE_Y_SIZE, PS_TYPE_U32);
    myStats = psStatsAlloc( PS_STAT_SAMPLE_MEAN );
    myStats2 = psImageStats( myStats, tmpImage, tmpMask, 0 );
    if ( myStats2 != NULL ) {
        fprintf(stderr, "ERROR: psImageStats did not return null when mask is invalid type.\n");
    }
    psFree(tmpImage);
    psFree(tmpMask);
    psFree(myStats);

    return ( !testStatus );
}

