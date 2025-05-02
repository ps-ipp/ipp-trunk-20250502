#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmSubtraction.h"
#include "pmSubtractionTypes.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionThreads.h"

#include "pmSubtractionEquation.h"
#include "pmSubtractionVisual.h"

//#define TESTING                         // TESTING output for debugging; may not work with threads!

//#define USE_WEIGHT                      // Include weight (1/variance) in equation?
//#define USE_WINDOW                      // Include weight (1/variance) in equation?


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Calculate the least-squares matrix and vector
static bool calculateMatrixVector(psImage *matrix, // Least-squares matrix, updated
                                  psVector *vector, // Least-squares vector, updated
                                  double *norm,     // Normalisation, updated
                                  const psKernel *input, // Input image (target)
                                  const psKernel *reference, // Reference image (convolution source)
                                  const psKernel *weight,  // Weight image
                                  const psKernel *window,  // Window image
                                  const psArray *convolutions,         // Convolutions for each kernel
                                  const pmSubtractionKernels *kernels, // Kernels
                                  const psImage *polyValues, // Spatial polynomial values
                                  int footprint, // (Half-)Size of stamp
                                  int normWindow1, // Window (half-)size for normalisation measurement
                                  int normWindow2, // Window (half-)size for normalisation measurement
                                  const pmSubtractionEquationCalculationMode mode
                                  )
{
    // (I - R * sum_i a_i k_i - g) (R * k_j) = 0
    // I C_j = sum_i C_i C_j

    // Background: C_i = 1.0
    // Normalisation: C_i = R

    int numKernels = kernels->num;                      // Number of kernels
    int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
    int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index in matrix for background
    int spatialOrder = kernels->spatialOrder;       // Order of spatial variation
    int numPoly = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of polynomial terms
    double poly[numPoly];                                 // Polynomial terms
    double poly2[numPoly][numPoly];                       // Polynomial-polynomial values

    // Evaluate polynomial-polynomial terms
    // XXX we can skip this if we are not calculating kernel coeffs
    for (int iyOrder = 0, iIndex = 0; iyOrder <= spatialOrder; iyOrder++) {
        for (int ixOrder = 0; ixOrder <= spatialOrder - iyOrder; ixOrder++, iIndex++) {
            double iPoly = polyValues->data.F64[iyOrder][ixOrder]; // Value of polynomial
            poly[iIndex] = iPoly;
            for (int jyOrder = 0, jIndex = 0; jyOrder <= spatialOrder; jyOrder++) {
                for (int jxOrder = 0; jxOrder <= spatialOrder - jyOrder; jxOrder++, jIndex++) {
                    double jPoly = polyValues->data.F64[jyOrder][jxOrder];
                    poly2[iIndex][jIndex] = iPoly * jPoly;
                }
            }
        }
    }

    // initialize the matrix and vector for NOP on all coeffs.  we only fill in the coeffs we
    // choose to calculate
    psImageInit(matrix, 0.0);
    psVectorInit(vector, 1.0);
    for (int i = 0; i < matrix->numCols; i++) {
        matrix->data.F64[i][i] = 1.0;
    }

    // the order of the elements in the matrix and vector is:
    // [kernel 0, x^0 y^0][kernel 1 x^0 y^0]...[kernel N, x^0 y^0]
    // [kernel 0, x^1 y^0][kernel 1 x^1 y^0]...[kernel N, x^1 y^0]
    // [kernel 0, x^n y^m][kernel 1 x^n y^m]...[kernel N, x^n y^m]
    // normalization
    // bg 0, bg 1, bg 2 (only 0 is currently used?)

    for (int i = 0; i < numKernels; i++) {
        psKernel *iConv = convolutions->data[i]; // Convolution for index i
        for (int j = i; j < numKernels; j++) {
            psKernel *jConv = convolutions->data[j]; // Convolution for index j

            double sumCC = 0.0;         // Sum of convolution products
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    double cc = iConv->kernel[y][x] * jConv->kernel[y][x];
                    if (weight) {
                        cc *= weight->kernel[y][x];
                    }
                    if (window) {
                        cc *= window->kernel[y][x];
                    }
                    sumCC += cc;
                }
            }

            // Spatial variation of kernel coeffs
            if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
                for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
                    for (int jTerm = 0, jIndex = j; jTerm < numPoly; jTerm++, jIndex += numKernels) {
                        double value = sumCC * poly2[iTerm][jTerm];
                        matrix->data.F64[iIndex][jIndex] = value;
                        matrix->data.F64[jIndex][iIndex] = value;
                    }
                }
            }
        }

        double sumRC = 0.0;             // Sum of the reference-convolution products
        double sumIC = 0.0;             // Sum of the input-convolution products
        double sumC = 0.0;              // Sum of the convolution
        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                float conv = iConv->kernel[y][x];
                float in = input->kernel[y][x];
                float ref = reference->kernel[y][x];
                double ic = in * conv;
                double rc = ref * conv;
                double c = conv;
                if (weight) {
                    float wtVal = weight->kernel[y][x];
                    ic *= wtVal;
                    rc *= wtVal;
                    c *= wtVal;
                }
                if (window) {
                    float winVal = window->kernel[y][x];
                    ic *= winVal;
                    rc *= winVal;
                    c  *= winVal;
                }
                sumIC += ic;
                sumRC += rc;
                sumC += c;
            }
        }
        // Spatial variation
        for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
            double normTerm = sumRC * poly[iTerm];
            double bgTerm = sumC * poly[iTerm];
            if ((mode & PM_SUBTRACTION_EQUATION_NORM) && (mode & PM_SUBTRACTION_EQUATION_KERNELS)) {
                matrix->data.F64[iIndex][normIndex] = normTerm;
                matrix->data.F64[normIndex][iIndex] = normTerm;
            }
            if ((mode & PM_SUBTRACTION_EQUATION_BG) && (mode & PM_SUBTRACTION_EQUATION_KERNELS)) {
                matrix->data.F64[iIndex][bgIndex] = bgTerm;
                matrix->data.F64[bgIndex][iIndex] = bgTerm;
            }
            if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
                vector->data.F64[iIndex] = sumIC * poly[iTerm];
                if (!(mode & PM_SUBTRACTION_EQUATION_NORM)) {
                    // subtract norm * sumRC * poly[iTerm]
                    psAssert (kernels->solution1, "programming error: define solution first!");
                    int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
                    double norm = fabs(kernels->solution1->data.F64[normIndex]);  // Normalisation
                    vector->data.F64[iIndex] -= norm * normTerm;
                }
            }
        }
    }

    double sumRR = 0.0;                 // Sum of the reference product
    double sumIR = 0.0;                 // Sum of the input-reference product
    double sum1 = 0.0;                  // Sum of the background
    double sumR = 0.0;                  // Sum of the reference
    double sumI = 0.0;                  // Sum of the input
    double normI1 = 0.0, normI2 = 0.0;  // Sum of I_1 and I_2 within the normalisation window
    for (int y = - footprint; y <= footprint; y++) {
        for (int x = - footprint; x <= footprint; x++) {
            double in = input->kernel[y][x];
            double ref = reference->kernel[y][x];
            double ir = in * ref;
            double rr = PS_SQR(ref);
            double one = 1.0;

            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(normWindow1)) {
                normI1 += ref;
            }
            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(normWindow2)) {
                normI2 += in;
            }

            if (weight) {
                float wtVal = weight->kernel[y][x];
                rr *= wtVal;
                ir *= wtVal;
                in *= wtVal;
                ref *= wtVal;
                one *= wtVal;
            }
            if (window) {
                float  winVal = window->kernel[y][x];
                rr      *= winVal;
                ir      *= winVal;
                in      *= winVal;
                ref *= winVal;
                one *= winVal;
            }
            sumRR += rr;
            sumIR += ir;
            sumR += ref;
            sumI += in;
            sum1 += one;
        }
    }

    *norm = normI2 / normI1;

    fprintf (stderr, "normValue: %f %f %f\n", normI1, normI2, *norm);

    if (mode & PM_SUBTRACTION_EQUATION_NORM) {
        matrix->data.F64[normIndex][normIndex] = sumRR;
        vector->data.F64[normIndex] = sumIR;
        // subtract sum over kernels * kernel solution
    }
    if (mode & PM_SUBTRACTION_EQUATION_BG) {
        matrix->data.F64[bgIndex][bgIndex] = sum1;
        vector->data.F64[bgIndex] = sumI;
    }
    if ((mode & PM_SUBTRACTION_EQUATION_NORM) && (mode & PM_SUBTRACTION_EQUATION_BG)) {
        matrix->data.F64[normIndex][bgIndex] = sumR;
        matrix->data.F64[bgIndex][normIndex] = sumR;
    }

    // check for any NAN values in the result, skip if found:
    for (int iy = 0; iy < matrix->numRows; iy++) {
        for (int ix = 0; ix < matrix->numCols; ix++) {
            if (!isfinite(matrix->data.F64[iy][ix])) {
                fprintf (stderr, "WARNING: NAN in matrix\n");
                return false;
            }
        }
    }
    for (int ix = 0; ix < vector->n; ix++) {
        if (!isfinite(vector->data.F64[ix])) {
            fprintf (stderr, "WARNING: NAN in vector\n");
            return false;
        }
    }

    return true;
}


// Calculate the least-squares matrix and vector for dual convolution
static bool calculateDualMatrixVector(psImage *matrix, // Least-squares matrix, updated
                                      psVector *vector, // Least-squares vector, updated
                                      double *norm,     // Normalisation, updated
                                      const psKernel *image1, // Image 1
                                      const psKernel *image2, // Image 2
                                      const psKernel *weight,  // Weight image
                                      const psKernel *window,  // Window image
                                      const psArray *convolutions1, // Convolutions of image 1 for each kernel
                                      const psArray *convolutions2, // Convolutions of image 2 for each kernel
                                      const pmSubtractionKernels *kernels, // Kernels
                                      const psImage *polyValues, // Spatial polynomial values
                                      int footprint, // (Half-)Size of stamp
                                      int normWindow1, // Window (half-)size for normalisation measurement
                                      int normWindow2, // Window (half-)size for normalisation measurement
                                      const pmSubtractionEquationCalculationMode mode
                                      )
{
    int numKernels = kernels->num;                      // Number of kernels
    int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
    int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index in matrix for background
    int spatialOrder = kernels->spatialOrder;       // Order of spatial variation
    int numPoly = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of polynomial terms
    double poly[numPoly];                                 // Polynomial terms
    double poly2[numPoly][numPoly];                       // Polynomial-polynomial values

    int numBackground = PM_SUBTRACTION_POLYTERMS(kernels->bgOrder); // Number of background terms
    int numParams = numKernels * numPoly + 1 + numBackground;       // Number of regular parameters
    int numParams2 = numKernels * numPoly;                          // Number of additional parameters for dual
    int numDual = numParams + numParams2;                           // Total number of parameters for dual

    psAssert(matrix &&
             matrix->type.type == PS_TYPE_F64 &&
             matrix->numCols == numDual &&
             matrix->numRows == numDual,
             "Least-squares matrix is bad.");
    psAssert(vector &&
             vector->type.type == PS_TYPE_F64 &&
             vector->n == numDual,
             "Least-squares vector is bad.");

    // Evaluate polynomial-polynomial terms
    for (int iyOrder = 0, iIndex = 0; iyOrder <= spatialOrder; iyOrder++) {
        for (int ixOrder = 0; ixOrder <= spatialOrder - iyOrder; ixOrder++, iIndex++) {
            double iPoly = polyValues->data.F64[iyOrder][ixOrder]; // Value of polynomial
            poly[iIndex] = iPoly;
            for (int jyOrder = 0, jIndex = 0; jyOrder <= spatialOrder; jyOrder++) {
                for (int jxOrder = 0; jxOrder <= spatialOrder - jyOrder; jxOrder++, jIndex++) {
                    double jPoly = polyValues->data.F64[jyOrder][jxOrder];
                    poly2[iIndex][jIndex] = iPoly * jPoly;
                }
            }
        }
    }


    // initialize the matrix and vector for NOP on all coeffs.  we only fill in the coeffs we
    // choose to calculate
    psImageInit(matrix, 0.0);
    psVectorInit(vector, 1.0);
    for (int i = 0; i < matrix->numCols; i++) {
        matrix->data.F64[i][i] = 1.0;
    }

    for (int i = 0; i < numKernels; i++) {
        psKernel *iConv1 = convolutions1->data[i]; // Convolution 1 for index i
        psKernel *iConv2 = convolutions2->data[i]; // Convolution 2 for index i
        for (int j = i; j < numKernels; j++) {
            psKernel *jConv1 = convolutions1->data[j]; // Convolution 1 for index j
            psKernel *jConv2 = convolutions2->data[j]; // Convolution 2 for index j

            double sumAA = 0.0;         // Sum of convolution products between image 1
            double sumBB = 0.0;         // Sum of convolution products between image 2
            double sumAB = 0.0;         // Sum of convolution products across images 1 and 2
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    double aa = iConv1->kernel[y][x] * jConv1->kernel[y][x];
                    double bb = iConv2->kernel[y][x] * jConv2->kernel[y][x];
                    double ab = iConv1->kernel[y][x] * jConv2->kernel[y][x];
                    if (weight) {
                        float wtVal = weight->kernel[y][x];
                        aa *= wtVal;
                        bb *= wtVal;
                        ab *= wtVal;
                    }
                    if (window) {
                        float wtVal = window->kernel[y][x];
                        aa *= wtVal;
                        bb *= wtVal;
                        ab *= wtVal;
                    }
                    sumAA += aa;
                    sumBB += bb;
                    sumAB += ab;
                }
            }

            // Spatial variation of kernel coeffs
            if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
                for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
                    for (int jTerm = 0, jIndex = j; jTerm < numPoly; jTerm++, jIndex += numKernels) {
                        double aa = sumAA * poly2[iTerm][jTerm];
                        double bb = sumBB * poly2[iTerm][jTerm];
                        double ab = sumAB * poly2[iTerm][jTerm];

                        matrix->data.F64[iIndex][jIndex] = aa;
                        matrix->data.F64[jIndex][iIndex] = aa;

                        matrix->data.F64[iIndex + numParams][jIndex + numParams] = bb;
                        matrix->data.F64[jIndex + numParams][iIndex + numParams] = bb;

                        matrix->data.F64[iIndex][jIndex + numParams] = ab;
                        matrix->data.F64[jIndex + numParams][iIndex] = ab;
                    }
                }
            }
        }
        for (int j = 0; j < i; j++) {
            psKernel *jConv2 = convolutions2->data[j]; // Convolution 2 for index j
            double sumAB = 0.0;         // Sum of convolution products for matrix C
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    double ab = iConv1->kernel[y][x] * jConv2->kernel[y][x];
                    if (weight) {
                        ab *= weight->kernel[y][x];
                    }
                    if (window) {
                        ab *= window->kernel[y][x];
                    }
                    sumAB += ab;
                }
            }

            // Spatial variation of kernel coeffs
            if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
                for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
                    for (int jTerm = 0, jIndex = j; jTerm < numPoly; jTerm++, jIndex += numKernels) {
                        double ab = sumAB * poly2[iTerm][jTerm];
                        matrix->data.F64[iIndex][jIndex + numParams] = ab;
                        matrix->data.F64[jIndex + numParams][iIndex] = ab;
                    }
                }
            }
        }

        double sumAI2 = 0.0;            // Sum of A.I_2 products (for vector)
        double sumBI2 = 0.0;            // Sum of B.I_2 products (for vector)
        double sumAI1 = 0.0;            // Sum of A.I_1 products (for matrix, normalisation)
        double sumA = 0.0;              // Sum of A (for matrix, background)
        double sumBI1 = 0.0;            // Sum of B.I_1 products (for matrix, normalisation)
        double sumB = 0.0;              // Sum of B products (for matrix, background)
        double sumI2 = 0.0;             // Sum of I_2 (for vector, background)
        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                double a = iConv1->kernel[y][x];
                double b = iConv2->kernel[y][x];
                float i1 = image1->kernel[y][x];
                float i2 = image2->kernel[y][x];

                double ai2 = a * i2;
                double bi2 = b * i2;
                double ai1 = a * i1;
                double bi1 = b * i1;

                if (weight) {
                    float wtVal = weight->kernel[y][x];
                    ai2 *= wtVal;
                    bi2 *= wtVal;
                    ai1 *= wtVal;
                    bi1 *= wtVal;
                    a *= wtVal;
                    b *= wtVal;
                    i2 *= wtVal;
                }
                if (window) {
                    float wtVal = window->kernel[y][x];
                    ai2 *= wtVal;
                    bi2 *= wtVal;
                    ai1 *= wtVal;
                    bi1 *= wtVal;
                    a *= wtVal;
                    b *= wtVal;
                    i2 *= wtVal;
                }
                sumAI2 += ai2;
                sumBI2 += bi2;
                sumAI1 += ai1;
                sumA += a;
                sumBI1 += bi1;
                sumB += b;
                sumI2 += i2;
            }
        }
        // Spatial variation
        for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
            double ai2 = sumAI2 * poly[iTerm];
            double bi2 = sumBI2 * poly[iTerm];
            double ai1 = sumAI1 * poly[iTerm];
            double a   = sumA * poly[iTerm];
            double bi1 = sumBI1 * poly[iTerm];
            double b   = sumB * poly[iTerm];

            if ((mode & PM_SUBTRACTION_EQUATION_NORM) && (mode & PM_SUBTRACTION_EQUATION_KERNELS)) {
                matrix->data.F64[iIndex][normIndex] = ai1;
                matrix->data.F64[normIndex][iIndex] = ai1;
                matrix->data.F64[iIndex + numParams][normIndex] = bi1;
                matrix->data.F64[normIndex][iIndex + numParams] = bi1;
            }
            if ((mode & PM_SUBTRACTION_EQUATION_BG) && (mode & PM_SUBTRACTION_EQUATION_KERNELS)) {
                matrix->data.F64[iIndex][bgIndex] = a;
                matrix->data.F64[bgIndex][iIndex] = a;
                matrix->data.F64[iIndex + numParams][bgIndex] = b;
                matrix->data.F64[bgIndex][iIndex + numParams] = b;
            }
            if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
                vector->data.F64[iIndex] = ai2;
                vector->data.F64[iIndex + numParams] = bi2;
                if (!(mode & PM_SUBTRACTION_EQUATION_NORM)) {
                    // subtract norm * sumRC * poly[iTerm]
                    psAssert (kernels->solution1, "programming error: define solution first!");
                    int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
                    double norm = fabs(kernels->solution1->data.F64[normIndex]);  // Normalisation
                    vector->data.F64[iIndex] -= norm * ai1;
                    vector->data.F64[iIndex + numParams] -= norm * bi1;
                }
            }
        }
    }

    double sumI1 = 0.0;                 // Sum of I_1 (for matrix, background-normalisation)
    double sumI1I1 = 0.0;               // Sum of I_1^2 (for matrix, normalisation-normalisation)
    double sum1 = 0.0;                  // Sum of 1 (for matrix, background-background)
    double sumI2 = 0.0;                 // Sum of I_2 (for vector, background)
    double sumI1I2 = 0.0;               // Sum of I_1.I_2 (for vector, normalisation)
    double normI1 = 0.0, normI2 = 0.0;  // Sum of I_1 and I_2 within the normalisation window
    for (int y = - footprint; y <= footprint; y++) {
        for (int x = - footprint; x <= footprint; x++) {
            double i1 = image1->kernel[y][x];
            double i2 = image2->kernel[y][x];

            double i1i1 = i1 * i1;
            double one = 1.0;
            double i1i2 = i1 * i2;

            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(normWindow1)) {
                normI1 += i1;
            }
            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(normWindow2)) {
                normI2 += i2;
            }

            if (weight) {
                float wtVal = weight->kernel[y][x];
                i1 *= wtVal;
                i1i1 *= wtVal;
                one *= wtVal;
                i2 *= wtVal;
                i1i2 *= wtVal;
            }
            if (window) {
                float wtVal = window->kernel[y][x];
                i1 *= wtVal;
                i1i1 *= wtVal;
                one *= wtVal;
                i2 *= wtVal;
                i1i2 *= wtVal;
            }
            sumI1 += i1;
            sumI1I1 += i1i1;
            sum1 += one;
            sumI2 += i2;
            sumI1I2 += i1i2;
        }
    }

    *norm = normI2 / normI1;
    fprintf (stderr, "normValue: %f %f %f\n", normI1, normI2, *norm);

    if (mode & PM_SUBTRACTION_EQUATION_NORM) {
        matrix->data.F64[normIndex][normIndex] = sumI1I1;
        vector->data.F64[normIndex] = sumI1I2;
    }
    if (mode & PM_SUBTRACTION_EQUATION_BG) {
        matrix->data.F64[bgIndex][bgIndex] = sum1;
        vector->data.F64[bgIndex] = sumI2;
    }
    if ((mode & PM_SUBTRACTION_EQUATION_NORM) && (mode & PM_SUBTRACTION_EQUATION_BG)) {
        matrix->data.F64[bgIndex][normIndex] = sumI1;
        matrix->data.F64[normIndex][bgIndex] = sumI1;
    }

    // check for any NAN values in the result, skip if found:
    for (int iy = 0; iy < matrix->numRows; iy++) {
        for (int ix = 0; ix < matrix->numCols; ix++) {
            if (!isfinite(matrix->data.F64[iy][ix])) {
                fprintf (stderr, "WARNING: NAN in matrix\n");
                return false;
            }
        }
    }
    for (int ix = 0; ix < vector->n; ix++) {
        if (!isfinite(vector->data.F64[ix])) {
            fprintf (stderr, "WARNING: NAN in vector\n");
            return false;
        }
    }


    return true;
}

#if 1
// Add in penalty term to least-squares vector
bool calculatePenalty(psImage *matrix,                     // Matrix to which to add in penalty term
		      psVector *vector,                    // Vector to which to add in penalty term
		      const pmSubtractionKernels *kernels, // Kernel parameters
		      float norm                           // Normalisation
  )
{
    if (kernels->penalty == 0.0) {
        return true;
    }

    psVector *penalties1 = kernels->penalties1; // Penalties for each kernel component (input)
    psVector *penalties2 = kernels->penalties2; // Penalties for each kernel component (ref)

    int spatialOrder = kernels->spatialOrder; // Order of spatial variations
    int numKernels = kernels->num; // Number of kernel components
    int numSpatial = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of spatial variations
    int numParams = numKernels * numSpatial;                 // Number of kernel parameters

    // order is :
    // [p_0,x_0,y_0 p_1,x_0,y_0, p_2,x_0,y_0]
    // [p_0,x_1,y_0 p_1,x_1,y_0, p_2,x_1,y_0]
    // [p_0,x_0,y_1 p_1,x_0,y_1, p_2,x_0,y_1]
    // [norm]
    // [bg]
    // [q_0,x_0,y_0 q_1,x_0,y_0, q_2,x_0,y_0]
    // [q_0,x_1,y_0 q_1,x_1,y_0, q_2,x_1,y_0]
    // [q_0,x_0,y_1 q_1,x_0,y_1, q_2,x_0,y_1]

    for (int i = 0; i < numKernels; i++) {
        for (int yOrder = 0, index = i; yOrder <= spatialOrder; yOrder++) {
            for (int xOrder = 0; xOrder <= spatialOrder - yOrder; xOrder++, index += numKernels) {
                // Contribution to chi^2: a_i^2 P_i
                psAssert(isfinite(penalties1->data.F32[i]), "Invalid penalty");
		fprintf (stderr, "penalty: %f + %f (%f * %f)\n", matrix->data.F64[index][index], norm * penalties1->data.F32[i], norm, penalties1->data.F32[i]);
                matrix->data.F64[index][index] += norm * penalties1->data.F32[i];
                if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
		    fprintf (stderr, "penalty: (x^%d y^%d fwhm %f) : %f + %f (%f * %f)\n", kernels->u->data.S32[index], kernels->v->data.S32[index], kernels->widths->data.F32[index], 
			     matrix->data.F64[index + numParams + 2][index + numParams + 2], norm * penalties2->data.F32[i], norm, penalties2->data.F32[i]);
		    matrix->data.F64[index + numParams + 2][index + numParams + 2] += norm * penalties2->data.F32[i];			     
                    // matrix[i][i] is ~ (k_i * I_1)(k_i * I_1)
                    // penalties scale with second moments
                    //
                }
            }
        }
    }

    return true;
}
# endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Semi-public functions
// XXX We might like to define these functions as "extern inline" but gcc currently doesn't handle this in c99
// mode.  See http://gcc.gnu.org/ml/gcc/2006-11/msg00006.html
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Calculate the value of a polynomial, specified by coefficients and polynomial values
double p_pmSubtractionCalculatePolynomial(const psVector *coeff, // Coefficients
                                          const psImage *polyValues, // Polynomial values
                                          int order, // Order of polynomials
                                          int index, // Index at which to begin
                                          int step // Step between subsequent indices
                                          )
{
    double sum = 0.0;                   // Value of the polynomial sum
    for (int yOrder = 0; yOrder <= order; yOrder++) {
        for (int xOrder = 0; xOrder <= order - yOrder; xOrder++, index += step) {

            assert(index < coeff->n);

            sum += coeff->data.F64[index] * polyValues->data.F64[yOrder][xOrder];
        }
    }
    return sum;
}

double p_pmSubtractionSolutionCoeff(const pmSubtractionKernels *kernels, const psImage *polyValues,
                                    int index, bool wantDual)
{
#if 0
    // This is probably in a tight loop, so don't check inputs
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NAN);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NAN);
    PS_ASSERT_IMAGE_NON_NULL(polyValues, NAN);
    PS_ASSERT_INT_POSITIVE(index, NAN);
#endif

    psVector *solution = wantDual ? kernels->solution2 : kernels->solution1; // Solution vector
    return p_pmSubtractionCalculatePolynomial(solution, polyValues, kernels->spatialOrder, index,
                                              kernels->num);
}

double p_pmSubtractionSolutionNorm(const pmSubtractionKernels *kernels)
{
#if 0
    // This is probably in a tight loop, so don't check inputs
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NAN);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NAN);
    PS_ASSERT_IMAGE_NON_NULL(polyValues, NAN);
#endif

    int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
    return kernels->solution1->data.F64[normIndex];
}

double p_pmSubtractionSolutionBackground(const pmSubtractionKernels *kernels,
                                                const psImage *polyValues)
{
#if 0
    // This is probably in a tight loop, so don't check inputs
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NAN);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NAN);
    PS_ASSERT_IMAGE_NON_NULL(polyValues, NAN);
#endif

    int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background
    return p_pmSubtractionCalculatePolynomial(kernels->solution1, polyValues, kernels->bgOrder, bgIndex, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmSubtractionCalculateEquationThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    pmSubtractionStampList *stamps = job->args->data[0]; // List of stamps
    pmSubtractionKernels *kernels = job->args->data[1]; // Kernels
    int index = PS_SCALAR_VALUE(job->args->data[2], S32); // Stamp index
    pmSubtractionEquationCalculationMode mode  = PS_SCALAR_VALUE(job->args->data[3], S32); // calculation model

    return pmSubtractionCalculateEquationStamp(stamps, kernels, index, mode);
}

bool pmSubtractionCalculateEquationStamp(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels,
                                         int index, const pmSubtractionEquationCalculationMode mode)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PS_ASSERT_INT_NONNEGATIVE(index, false);
    PS_ASSERT_INT_LESS_THAN(index, stamps->num, false);

    int footprint = stamps->footprint;  // Half-size of stamps
    int spatialOrder = kernels->spatialOrder; // Maximum order of spatial variation
    int numKernels = kernels->num;      // Number of kernel basis functions
    int numSpatial = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of spatial variations
    int numBackground = PM_SUBTRACTION_POLYTERMS(kernels->bgOrder); // Number of background terms

    // numKernels is the number of unique kernel images (one for each Gaussian modified by a specific polynomial).
    // = \sum_i^N_Gaussians [(order + 1) * (order + 2) / 2], eg for 1 Gauss and 1st order, numKernels = 3

    // Total number of parameters to solve for: coefficient of each kernel basis function, multipled by the
    // number of coefficients for the spatial polynomial, normalisation and a constant background offset.
    int numParams = numKernels * numSpatial + 1 + numBackground;
    if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        // An additional image is convolved
        numParams += numKernels * numSpatial;
    }

    pmSubtractionStamp *stamp = stamps->stamps->data[index]; // Stamp of interest
    psAssert(stamp->status == PM_SUBTRACTION_STAMP_CALCULATE, "We only operate on stamps with this state.");

    // Generate convolutions: these are generated once and saved
    if (!pmSubtractionConvolveStamp(stamp, kernels, footprint)) {
        psError(psErrorCodeLast(), false, "Unable to convolve stamp %d.", index);
        return NULL;
    }

#ifdef TESTING
    for (int j = 0; j < numKernels; j++) {
        if (stamp->convolutions1) {
            psString convName = NULL;
            psStringAppend(&convName, "conv1_%03d_%03d.fits", index, j);
            psFits *fits = psFitsOpen(convName, "w");
            psFree(convName);
            psKernel *conv = stamp->convolutions1->data[j];
            psFitsWriteImage(fits, NULL, conv->image, 0, NULL);
            psFitsClose(fits);
        }

        if (stamp->convolutions2) {
            psString convName = NULL;
            psStringAppend(&convName, "conv2_%03d_%03d.fits", index, j);
            psFits *fits = psFitsOpen(convName, "w");
            psFree(convName);
            psKernel *conv = stamp->convolutions2->data[j];
            psFitsWriteImage(fits, NULL, conv->image, 0, NULL);
            psFitsClose(fits);
        }
    }
#endif

    // XXX visualize the set of convolved stamps

    psImage *polyValues = p_pmSubtractionPolynomial(NULL, spatialOrder,
                                                    stamp->xNorm, stamp->yNorm); // Polynomial terms

    bool new = stamp->vector ? false : true; // Is this a new run?
    if (new) {
        stamp->matrix = psImageAlloc(numParams, numParams, PS_TYPE_F64);
        stamp->vector = psVectorAlloc(numParams, PS_TYPE_F64);
    }
#ifdef TESTING
    psImageInit(stamp->matrix, NAN);
    psVectorInit(stamp->vector, NAN);
#endif

    bool status;                    // Status of least-squares matrix/vector calculation

    psKernel *weight = NULL;
    psKernel *window = NULL;

#ifdef USE_WEIGHT
    weight = stamp->weight;
#endif
#ifdef USE_WINDOW
    window = stamps->window;
#endif

    switch (kernels->mode) {
      case PM_SUBTRACTION_MODE_1:
        status = calculateMatrixVector(stamp->matrix, stamp->vector, &stamp->norm, stamp->image2, stamp->image1,
                                       weight, window, stamp->convolutions1, kernels,
                                       polyValues, footprint, stamps->normWindow1, stamps->normWindow2, mode);
        break;
      case PM_SUBTRACTION_MODE_2:
        status = calculateMatrixVector(stamp->matrix, stamp->vector, &stamp->norm, stamp->image1, stamp->image2,
                                       weight, window, stamp->convolutions2, kernels,
                                       polyValues, footprint, stamps->normWindow2, stamps->normWindow1, mode);
        break;
      case PM_SUBTRACTION_MODE_DUAL:
        status = calculateDualMatrixVector(stamp->matrix, stamp->vector, &stamp->norm,
                                           stamp->image1, stamp->image2,
                                           weight, window, stamp->convolutions1, stamp->convolutions2,
                                           kernels, polyValues, footprint, stamps->normWindow1, stamps->normWindow2, mode);
        break;
      default:
        psAbort("Unsupported subtraction mode: %x", kernels->mode);
    }

    if (!status) {
        stamp->status = PM_SUBTRACTION_STAMP_REJECTED;
        psWarning("Rejecting stamp %d (%d,%d) because of bad equation",
                  index, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5));
    } else {
        stamp->status = PM_SUBTRACTION_STAMP_USED;
    }

#ifdef TESTING
    {
        psString matrixName = NULL;
        psStringAppend(&matrixName, "matrix_%d.fits", index);
        psFits *matrixFile = psFitsOpen(matrixName, "w");
        psFree(matrixName);
        psFitsWriteImage(matrixFile, NULL, stamp->matrix, 0, NULL);
        psFitsClose(matrixFile);

        matrixName = NULL;
        psStringAppend(&matrixName, "vector_%d.fits", index);
        psImage *dummy = psImageAlloc(stamp->vector->n, 1, PS_TYPE_F64);
        memcpy(dummy->data.F64[0], stamp->vector->data.F64,
               PSELEMTYPE_SIZEOF(PS_TYPE_F64) * stamp->vector->n);
        matrixFile = psFitsOpen(matrixName, "w");
        psFree(matrixName);
        psFitsWriteImage(matrixFile, NULL, dummy, 0, NULL);
        psFree(dummy);
        psFitsClose(matrixFile);
    }
#endif

    psFree(polyValues);

    return true;
}

bool pmSubtractionCalculateEquation(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels,
                                    const pmSubtractionEquationCalculationMode mode)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);

    psTimerStart("pmSubtractionCalculateEquation");

    // We iterate over each stamp, allocate the matrix and vectors if
    // necessary, and then calculate those matrix/vectors.
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (stamp->status != PM_SUBTRACTION_STAMP_CALCULATE) {
            continue;
        }

        if ((stamp->x <= 0.0) && (stamp->y <= 0.0)) {
            psAbort ("bad stamp");
        }
        if (!isfinite(stamp->x) && !isfinite(stamp->y)) {
            psAbort ("bad stamp");
        }

        if (pmSubtractionThreaded()) {
            psThreadJob *job = psThreadJobAlloc("PSMODULES_SUBTRACTION_CALCULATE_EQUATION");
            psArrayAdd(job->args, 1, stamps);
            psArrayAdd(job->args, 1, (pmSubtractionKernels*)kernels); // Casting away const to put on array
            PS_ARRAY_ADD_SCALAR(job->args, i, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, mode, PS_TYPE_S32);
            if (!psThreadJobAddPending(job)) {
                return false;
            }
        } else {
            pmSubtractionCalculateEquationStamp(stamps, kernels, i, mode);
        }
    }

    if (!psThreadPoolWait(true, true)) {
        psError(psErrorCodeLast(), false, "Error waiting for threads.");
        return false;
    }

    pmSubtractionVisualPlotLeastSquares(stamps);
    pmSubtractionVisualShowKernels((pmSubtractionKernels  *)kernels);
    pmSubtractionVisualShowBasis(stamps);

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Calculate equation: %f sec",
             psTimerClear("pmSubtractionCalculateEquation"));


    return true;
}

// private functions used on pmSubtractionSolveEquation
bool psVectorWriteFile (char *filename, const psVector *vector);
bool psFitsWriteImageSimple (char *filename, psImage *image, psMetadata *header);

psImage *p_pmSubSolve_wUt (psVector *w, psImage *U);
psImage *p_pmSubSolve_VwUt (psImage *V, psImage *wUt);

bool p_pmSubSolve_SetWeights (psVector *wApply, psVector *w, psVector *wMask);

bool p_pmSubSolve_UtB (psVector **UtB, psImage *U, psVector *B);
bool p_pmSubSolve_wUtB (psVector **wUtB, psVector *w, psVector *UtB);
bool p_pmSubSolve_VwUtB (psVector **VwUtB, psImage *V, psVector *wUtB);

bool p_pmSubSolve_Ax (psVector **B, psImage *A, psVector *x);
bool p_pmSubSolve_VdV (double *value, psVector *x, psVector *y);
bool p_pmSubSolve_y2 (double *y2, pmSubtractionKernels *kernels, const pmSubtractionStampList *stamps);

psImage *p_pmSubSolve_Xvar (psImage *V, psVector *w);

double p_pmSubSolve_ChiSquare (pmSubtractionKernels *kernels, const pmSubtractionStampList *stamps);

bool pmSubtractionSolveEquation(pmSubtractionKernels *kernels,
                                const pmSubtractionStampList *stamps,
                                const pmSubtractionEquationCalculationMode mode)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);

    // Check inputs
    int numKernels = kernels->num;      // Number of kernel basis functions
    int numSpatial = PM_SUBTRACTION_POLYTERMS(kernels->spatialOrder); // Number of spatial variations
    int numBackground = PM_SUBTRACTION_POLYTERMS(kernels->bgOrder); // Number of background terms
    int numParams = numKernels * numSpatial + 1 + numBackground;    // Number of parameters being solved for
    int numSolution1 = numParams, numSolution2 = 0;                 // Number of parameters for each solution
    if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        // An additional image is convolved
        numSolution2 = numKernels * numSpatial;
        numParams += numSolution2;
    }

    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        PS_ASSERT_PTR_NON_NULL(stamp, false);
        if (stamp->status != PM_SUBTRACTION_STAMP_USED) {
            continue;
        }

        PS_ASSERT_VECTOR_NON_NULL(stamp->vector, false);
        PS_ASSERT_VECTOR_SIZE(stamp->vector, (long)numParams, false);
        PS_ASSERT_VECTOR_TYPE(stamp->vector, PS_TYPE_F64, false);
        PS_ASSERT_IMAGE_NON_NULL(stamp->matrix, false);
        PS_ASSERT_IMAGE_SIZE(stamp->matrix, numParams, numParams, false);
        PS_ASSERT_IMAGE_TYPE(stamp->matrix, PS_TYPE_F64, false);
    }

    psString ds9name = NULL;            // Filename for ds9 region file
    static int ds9num = 0;              // File number for ds9 region file
    psStringAppend(&ds9name, "stamps_solution_%d.ds9", ds9num);
    FILE *ds9 = pmSubtractionStampsFile(stamps, ds9name, "solution stamps");
    psFree(ds9name);
    ds9num++;

    if (kernels->mode != PM_SUBTRACTION_MODE_DUAL) {
        // Accumulate the least-squares matricies and vectors
        psImage *sumMatrix = psImageAlloc(numParams, numParams, PS_TYPE_F64); // Combined matrix
        psVector *sumVector = psVectorAlloc(numParams, PS_TYPE_F64); // Combined vector
        psVectorInit(sumVector, 0.0);
        psImageInit(sumMatrix, 0.0);

        psVector *norms = psVectorAllocEmpty(stamps->num, PS_TYPE_F64); // Normalisations

        int numStamps = 0;              // Number of good stamps
        for (int i = 0; i < stamps->num; i++) {
            pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
            if (stamp->status == PM_SUBTRACTION_STAMP_USED) {
		
                (void)psBinaryOp(sumMatrix, sumMatrix, "+", stamp->matrix);
                (void)psBinaryOp(sumVector, sumVector, "+", stamp->vector);

                psVectorAppend(norms, stamp->norm);

                pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "green");
                numStamps++;
            } else if (stamp->status == PM_SUBTRACTION_STAMP_REJECTED) {
                pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "red");
            }
        }

#if 0
        int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background
        calculatePenalty(sumMatrix, sumVector, kernels, sumMatrix->data.F64[bgIndex][bgIndex]);
#endif

        psVector *solution = NULL;                       // Solution to equation!
        solution = psVectorAlloc(numParams, PS_TYPE_F64);
        psVectorInit(solution, 0);

#if 0
        // Regular, straight-forward solution
        solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, NAN);
#else
        {
            // Solve normalisation and background separately
            int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
            int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background

            psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN); // Statistics for norm
            if (!psVectorStats(stats, norms, NULL, NULL, 0)) {
                psError(PM_ERR_DATA, false, "Unable to determine median normalisation");
                psFree(stats);
                psFree(sumMatrix);
                psFree(sumVector);
                psFree(norms);
                return false;
            }

            // double normValue = 1.0;
            double normValue = stats->robustMedian;
            // double bgValue = 0.0;

            psFree(stats);

#ifdef TESTING
            fprintf(stderr, "Norm: %lf\n", normValue);
#endif
            // Solve kernel components
            for (int i = 0; i < numSolution1; i++) {
                sumVector->data.F64[i] -= normValue * sumMatrix->data.F64[normIndex][i];

                sumMatrix->data.F64[i][normIndex] = 0.0;
                sumMatrix->data.F64[normIndex][i] = 0.0;
            }
            sumVector->data.F64[bgIndex] -= normValue * sumMatrix->data.F64[normIndex][bgIndex];
            sumMatrix->data.F64[bgIndex][normIndex] = 0.0;
            sumMatrix->data.F64[normIndex][bgIndex] = 0.0;

            sumMatrix->data.F64[normIndex][normIndex] = 1.0;
            sumVector->data.F64[normIndex] = 0.0;

            solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, NAN);

            solution->data.F64[normIndex] = normValue;
        }
# endif

#if (1)
        for (int i = 0; i < solution->n; i++) {
            fprintf(stderr, "Single solution %d: %lf\n", i, solution->data.F64[i]);
        }
#endif

        if (!kernels->solution1) {
            kernels->solution1 = psVectorAlloc(sumVector->n, PS_TYPE_F64);
            psVectorInit(kernels->solution1, 0.0);
        }

        // only update the solutions that we chose to calculate:
        if (mode & PM_SUBTRACTION_EQUATION_NORM) {
            int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
            kernels->solution1->data.F64[normIndex] = solution->data.F64[normIndex];
        }
        if (mode & PM_SUBTRACTION_EQUATION_BG) {
            int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index in matrix for background
            kernels->solution1->data.F64[bgIndex] = solution->data.F64[bgIndex];
        }
        if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
            int numKernels = kernels->num;
            int spatialOrder = kernels->spatialOrder;       // Order of spatial variation
            int numPoly = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of polynomial terms
            for (int i = 0; i < numKernels * numPoly; i++) {
                kernels->solution1->data.F64[i] = solution->data.F64[i];
            }
        }

        psFree(norms);
        psFree(solution);
        psFree(sumVector);
        psFree(sumMatrix);

#ifdef TESTING
        // XXX double-check for NAN in data:
        for (int ix = 0; ix < kernels->solution1->n; ix++) {
            if (!isfinite(kernels->solution1->data.F64[ix])) {
                fprintf (stderr, "WARNING: NAN in vector\n");
            }
        }
#endif

    } else {
        // Dual convolution solution

        // Accumulation of stamp matrices/vectors
        psImage *sumMatrix = psImageAlloc(numParams, numParams, PS_TYPE_F64);
        psVector *sumVector = psVectorAlloc(numParams, PS_TYPE_F64);
        psImageInit(sumMatrix, 0.0);
        psVectorInit(sumVector, 0.0);

        psVector *norms = psVectorAllocEmpty(stamps->num, PS_TYPE_F64); // Normalisations

        int numStamps = 0;              // Number of good stamps
        for (int i = 0; i < stamps->num; i++) {
            pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
            if (stamp->status == PM_SUBTRACTION_STAMP_USED) {
                (void)psBinaryOp(sumMatrix, sumMatrix, "+", stamp->matrix);
                (void)psBinaryOp(sumVector, sumVector, "+", stamp->vector);

                psVectorAppend(norms, stamp->norm);

                pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "green");
                numStamps++;
            }
        }

#if 0
	psImage *save = psImageCopy(NULL, sumMatrix, PS_TYPE_F32);
        psFitsWriteImageSimple ("sumMatrix.fits", save, NULL);
        psVectorWriteFile("sumVector.dat", sumVector);
	psFree (save);
#endif

#if 1
        // int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background
        // calculatePenalty(sumMatrix, sumVector, kernels, sumMatrix->data.F64[bgIndex][bgIndex]);

        int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
        calculatePenalty(sumMatrix, sumVector, kernels, sumMatrix->data.F64[normIndex][normIndex] / 100.0);
#endif

        psVector *solution = NULL;                       // Solution to equation!
        solution = psVectorAlloc(numParams, PS_TYPE_F64);
        psVectorInit(solution, 0);

#if 0
        // Regular, straight-forward solution
        solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, NAN);
#else
        {
            // Solve normalisation and background separately
            int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
            int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background

#if 0
            psImage *normMatrix = psImageAlloc(2, 2, PS_TYPE_F64);
            psVector *normVector = psVectorAlloc(2, PS_TYPE_F64);

            normMatrix->data.F64[0][0] = sumMatrix->data.F64[normIndex][normIndex];
            normMatrix->data.F64[1][1] = sumMatrix->data.F64[bgIndex][bgIndex];
            normMatrix->data.F64[0][1] = normMatrix->data.F64[1][0] = sumMatrix->data.F64[normIndex][bgIndex];

            normVector->data.F64[0] = sumVector->data.F64[normIndex];
            normVector->data.F64[1] = sumVector->data.F64[bgIndex];

            psVector *normSolution = psMatrixSolveSVD(NULL, normMatrix, normVector, NAN);

            double normValue = normSolution->data.F64[0];
            double bgValue = normSolution->data.F64[1];

            psFree(normMatrix);
            psFree(normVector);
            psFree(normSolution);
#endif

            psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN); // Statistics for norm
            if (!psVectorStats(stats, norms, NULL, NULL, 0)) {
                psError(PM_ERR_DATA, false, "Unable to determine median normalisation");
                psFree(stats);
                psFree(sumMatrix);
                psFree(sumVector);
                psFree(norms);
                return false;
            }

            double normValue = stats->robustMedian;

            psFree(stats);

#ifdef TESTING
            fprintf(stderr, "Norm: %lf\n", normValue);
#endif

            // Solve kernel components
            for (int i = 0; i < numSolution2; i++) {
                sumVector->data.F64[i] -= normValue * sumMatrix->data.F64[normIndex][i];
                sumVector->data.F64[i + numSolution1] -= normValue * sumMatrix->data.F64[normIndex][i + numSolution1];

                sumMatrix->data.F64[i][normIndex] = 0.0;
                sumMatrix->data.F64[normIndex][i] = 0.0;

                sumMatrix->data.F64[i + numSolution1][normIndex] = 0.0;
                sumMatrix->data.F64[normIndex][i + numSolution1] = 0.0;
            }
            sumVector->data.F64[bgIndex] -= normValue * sumMatrix->data.F64[normIndex][bgIndex];
            sumMatrix->data.F64[bgIndex][normIndex] = 0.0;
            sumMatrix->data.F64[normIndex][bgIndex] = 0.0;

            sumMatrix->data.F64[normIndex][normIndex] = 1.0;

            sumVector->data.F64[normIndex] = 0.0;

// save the matrix and vector after the NULLs have been set
#if 0
	    psImage *save = psImageCopy(NULL, sumMatrix, PS_TYPE_F32);
	    psFitsWriteImageSimple ("sumMatrix.fits", save, NULL);
	    psVectorWriteFile("sumVector.dat", sumVector);
	    psFree (save);
#endif

	    solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, 1e-6);
	    // solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, 3e-4);
	    // psVectorCopy (solution, sumVector, PS_TYPE_F64);
            // psMatrixGJSolve(sumMatrix, solution);
            solution->data.F64[normIndex] = normValue;
        }
#endif


#if (1)
        for (int i = 0; i < solution->n; i++) {
            fprintf(stderr, "Dual solution %d: %lf\n", i, solution->data.F64[i]);
        }
#endif

        psFree(sumMatrix);
        psFree(sumVector);

        psFree(norms);

        if (!kernels->solution1) {
            kernels->solution1 = psVectorAlloc(numSolution1, PS_TYPE_F64);
            psVectorInit (kernels->solution1, 0.0);
        }
        if (!kernels->solution2) {
            kernels->solution2 = psVectorAlloc(numSolution2, PS_TYPE_F64);
            psVectorInit (kernels->solution2, 0.0);
        }

        // only update the solutions that we chose to calculate:
        if (mode & PM_SUBTRACTION_EQUATION_NORM) {
            int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
            kernels->solution1->data.F64[normIndex] = solution->data.F64[normIndex];
        }
        if (mode & PM_SUBTRACTION_EQUATION_BG) {
            int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index in matrix for background
            kernels->solution1->data.F64[bgIndex] = solution->data.F64[bgIndex];
        }
        if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
            int numKernels = kernels->num;
            for (int i = 0; i < numKernels * numSpatial; i++) {
                // XXX fprintf (stderr, "keep\n");
                kernels->solution1->data.F64[i] = solution->data.F64[i];
                kernels->solution2->data.F64[i] = solution->data.F64[i + numSolution1];
            }
        }


        memcpy(kernels->solution1->data.F64, solution->data.F64,
               numSolution1 * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        memcpy(kernels->solution2->data.F64, &solution->data.F64[numSolution1],
               numSolution2 * PSELEMTYPE_SIZEOF(PS_TYPE_F64));

        psFree(solution);

    }

    if (ds9) {
        fclose(ds9);
    }

    if (psTraceGetLevel("psModules.imcombine") >= 7) {
        for (int i = 0; i < kernels->solution1->n; i++) {
            psTrace("psModules.imcombine", 7, "Solution 1 %d: %f\n", i, kernels->solution1->data.F64[i]);
        }
        if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
            for (int i = 0; i < kernels->solution2->n; i++) {
                psTrace("psModules.imcombine", 7, "Solution 2 %d: %f\n", i, kernels->solution2->data.F64[i]);
            }
        }
     }

    // pmSubtractionVisualPlotLeastSquares((pmSubtractionStampList *) stamps); //casting away const
    return true;
}

// measure some useful stats on the stamp residuals:
// fResSigma : the residual stdev / total flux
// fResOuter : the residual fabs / total flux for R > 2 pix
// fResTotal : the residual fabs / total flux for R > 0 pix
bool pmSubtractionResidualStats(psVector *fResSigma, psVector *fResOuter, psVector *fResTotal, psKernel *target, psKernel *source, psKernel *residual, double norm, int footprint) {

    float sum = 0.0;
    float peak = 0.0;
    for (int y = - footprint; y <= footprint; y++) {
        for (int x = - footprint; x <= footprint; x++) {
            sum += 0.5*(target->kernel[y][x] + source->kernel[y][x] * norm);
            peak = PS_MAX(peak, 0.5*(target->kernel[y][x] + source->kernel[y][x] * norm));
        }
    }

    // init counters
    int npix = 0;
    float dflux1 = 0.0;
    float dflux2 = 0.0;
    float dOuter = 0.0;
    float dTotal = 0.0;

    for (int y = - footprint; y <= footprint; y++) {
        for (int x = - footprint; x <= footprint; x++) {
            dflux1 += residual->kernel[y][x];
            dflux2 += PS_SQR(residual->kernel[y][x]);
            dTotal += fabs(residual->kernel[y][x]);
	    if (hypot(x,y) > 2.0) {
	      dOuter += fabs(residual->kernel[y][x]);
	    }
            npix ++;
        }
    }
    float sigma = sqrt(dflux2 / npix - PS_SQR(dflux1/npix));
    if (!isfinite(sum))  return false;
    if (!isfinite(peak)) return false;
    if (!isfinite(dOuter)) return false;
    if (!isfinite(dTotal)) return false;

    fprintf (stderr, "sum: %f, peak: %f, sigma: %f, fsigma: %f, fmax: %f, fmin: %f\n", sum, peak, sigma, sigma/sum, dOuter/sum, dTotal/sum);
    psVectorAppend(fResSigma, sigma/sum);
    psVectorAppend(fResOuter, dOuter/sum);
    psVectorAppend(fResTotal, dTotal/sum);
    return true;
}

psVector *pmSubtractionCalculateDeviations(pmSubtractionStampList *stamps,
                                           pmSubtractionKernels *kernels)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NULL);

    psVector *deviations = psVectorAlloc(stamps->num, PS_TYPE_F32); // Mean deviation for stamps
    int footprint = stamps->footprint; // Half-size of stamps
    long numPixels = PS_SQR(2 * footprint + 1); // Number of pixels in footprint
    double devNorm = 1.0 / (double)numPixels; // Normalisation for deviations
    int numKernels = kernels->num;      // Number of kernels

    psImage *polyValues = NULL;         // Polynomial values
    psKernel *residual = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Residual image

    // set up holding images for the visualization
    pmSubtractionVisualShowFitInit (stamps);

    psVector *fResSigma = psVectorAllocEmpty(stamps->num, PS_TYPE_F32);
    psVector *fResOuter = psVectorAllocEmpty(stamps->num, PS_TYPE_F32);
    psVector *fResTotal = psVectorAllocEmpty(stamps->num, PS_TYPE_F32);

    // we want to save the residual images for the 9 brightest stamps.
    // identify the 9 brightest stamps
    psVector *keepStamps  = psVectorAlloc(stamps->num, PS_TYPE_S32);
    psVectorInit (keepStamps, 0);
    {
        psVector *flux  = psVectorAlloc(stamps->num, PS_TYPE_F32);
        psVectorInit (flux, 0.0);

        for (int i = 0; i < stamps->num; i++) {
            pmSubtractionStamp *stamp = stamps->stamps->data[i];
            if (!isfinite(stamp->flux)) continue;
            flux->data.F32[i] = stamp->flux;
        }

        psVector *index = psVectorSortIndex(NULL, flux);
        for (int i = 0; (i < stamps->num) && (i < 9); i++) {
            int n = stamps->num - i - 1;
            keepStamps->data.S32[index->data.S32[n]] = 1;
        }
        psFree (flux);
        psFree (index);

        // this function is called multiple times in the iteration, but
        // we only know after the interation is done if we will try again.
        // therefore we must save the sample each time, and blow away the old one
        // if it exists.
        psFree (kernels->sampleStamps);
        kernels->sampleStamps = psArrayAllocEmpty(9);
    }

    psString log = psStringCopy("Deviations:\n");               // Log message with deviations
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // The stamp of interest
        if (stamp->status != PM_SUBTRACTION_STAMP_USED) {
            deviations->data.F32[i] = NAN;
            continue;
        }

        // Calculate coefficients of the kernel basis functions
        polyValues = p_pmSubtractionPolynomial(polyValues, kernels->spatialOrder, stamp->xNorm, stamp->yNorm);
        double norm = p_pmSubtractionSolutionNorm(kernels); // Normalisation
        double background = p_pmSubtractionSolutionBackground(kernels, polyValues);// Difference in background

        // Calculate residuals
        psKernel *weight = stamp->weight; // Weight postage stamp
        psImageInit(residual->image, 0.0);
        if (kernels->mode != PM_SUBTRACTION_MODE_DUAL) {
            psKernel *target;           // Target postage stamp
            psKernel *source;           // Source postage stamp
            psArray *convolutions;      // Convolution postage stamps for each kernel basis function
            switch (kernels->mode) {
              case PM_SUBTRACTION_MODE_1:
                target = stamp->image2;
                source = stamp->image1;
                convolutions = stamp->convolutions1;

                // Having convolved image1 and changed its normalisation, we need to renormalise the residual
                // so that it is on the scale of image1.
                psImage *image = pmSubtractionKernelImage(kernels, stamp->xNorm, stamp->yNorm,
                                                          false); // Kernel image
                if (!image) {
                    psError(psErrorCodeLast(), false, "Unable to generate image of kernel.");
                    return false;
                }
                double sumKernel = 0;   // Sum of kernel, for normalising residual
                int size = kernels->size; // Half-size of kernel
                int fullSize = 2 * size + 1; // Full size of kernel
                for (int y = 0; y < fullSize; y++) {
                    for (int x = 0; x < fullSize; x++) {
                        sumKernel += image->data.F32[y][x];
                    }
                }
                psFree(image);
                devNorm = 1.0 / sumKernel / numPixels;
                break;
              case PM_SUBTRACTION_MODE_2:
                target = stamp->image1;
                source = stamp->image2;
                convolutions = stamp->convolutions2;
                break;
              default:
                psAbort("Unsupported subtraction mode: %x", kernels->mode);
            }

            for (int j = 0; j < numKernels; j++) {
                psKernel *convolution = convolutions->data[j]; // Convolution
                double coefficient = p_pmSubtractionSolutionCoeff(kernels, polyValues, j,
                                                                  false); // Coefficient
                for (int y = - footprint; y <= footprint; y++) {
                    for (int x = - footprint; x <= footprint; x++) {
                        residual->kernel[y][x] += convolution->kernel[y][x] * coefficient;
                    }
                }
            }

            // XXX visualize the target, source, convolution and residual
            pmSubtractionVisualShowFitAddStamp (target, source, residual, background, norm, i);

            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    residual->kernel[y][x] += background + source->kernel[y][x] * norm - target->kernel[y][x];
                }
            }

            if (keepStamps->data.S32[i]) {
                psImage *sample = psImageCopy(NULL, residual->image, PS_TYPE_F32);
                psArrayAdd (kernels->sampleStamps, 9, sample);
                psFree (sample);
            }

            pmSubtractionResidualStats(fResSigma, fResOuter, fResTotal, target, source, residual, norm, footprint);

        } else {
            // Dual convolution
            psArray *convolutions1 = stamp->convolutions1; // Convolutions of the first image
            psArray *convolutions2 = stamp->convolutions2; // Convolutions of the second image
            psKernel *image1 = stamp->image1; // The first image
            psKernel *image2 = stamp->image2; // The second image

            for (int j = 0; j < numKernels; j++) {
                psKernel *conv1 = convolutions1->data[j]; // Convolution of first image
                psKernel *conv2 = convolutions2->data[j]; // Convolution of second image
                double coeff1 = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, false); // Coefficient 1
                double coeff2 = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, true); // Coefficient 2

                for (int y = - footprint; y <= footprint; y++) {
                    for (int x = - footprint; x <= footprint; x++) {
                        residual->kernel[y][x] += conv2->kernel[y][x] * coeff2 + conv1->kernel[y][x] * coeff1;
                    }
                }
            }

            // XXX visualize the target, source, convolution and residual
            pmSubtractionVisualShowFitAddStamp (image2, image1, residual, background, norm, i);

            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    residual->kernel[y][x] += background + image1->kernel[y][x] * norm - image2->kernel[y][x];
                }
            }
            if (keepStamps->data.S32[i]) {
                psImage *sample = psImageCopy(NULL, residual->image, PS_TYPE_F32);
                psArrayAdd (kernels->sampleStamps, 9, sample);
                psFree (sample);
            }

            pmSubtractionResidualStats(fResSigma, fResOuter, fResTotal, image1, image2, residual, norm, footprint);
        }

        double deviation = 0.0;         // Sum of differences
        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                double dev = PS_SQR(residual->kernel[y][x]) * weight->kernel[y][x];
                deviation += dev;
#ifdef TESTING
                residual->kernel[y][x] = dev;
#endif
            }
        }
        deviations->data.F32[i] = devNorm * deviation;
        psTrace("psModules.imcombine", 5, "Deviation for stamp %d (%d,%d): %f\n",
                i, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5), deviations->data.F32[i]);
        psStringAppend(&log, "Stamp %d (%d,%d): %f\n",
                       i, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5), deviations->data.F32[i]);
        if (!isfinite(deviations->data.F32[i])) {
            stamp->status = PM_SUBTRACTION_STAMP_REJECTED;
            psTrace("psModules.imcombine", 5,
                    "Rejecting stamp %d (%d,%d) because of non-finite deviation\n",
                    i, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5));
            continue;
        }

#ifdef TESTING
        {
            psString filename = NULL;
            psStringAppend(&filename, "resid_%03d.fits", i);
            psFits *fits = psFitsOpen(filename, "w");
            psFree(filename);
            psFitsWriteImage(fits, NULL, residual->image, 0, NULL);
            psFitsClose(fits);
        }
        if (stamp->image1) {
            psString filename = NULL;
            psStringAppend(&filename, "stamp_image1_%03d.fits", i);
            psFits *fits = psFitsOpen(filename, "w");
            psFree(filename);
            psFitsWriteImage(fits, NULL, stamp->image1->image, 0, NULL);
            psFitsClose(fits);
        }
        if (stamp->image2) {
            psString filename = NULL;
            psStringAppend(&filename, "stamp_image2_%03d.fits", i);
            psFits *fits = psFitsOpen(filename, "w");
            psFree(filename);
            psFitsWriteImage(fits, NULL, stamp->image2->image, 0, NULL);
            psFitsClose(fits);
        }
        if (stamp->weight) {
            psString filename = NULL;
            psStringAppend(&filename, "stamp_weight_%03d.fits", i);
            psFits *fits = psFitsOpen(filename, "w");
            psFree(filename);
            psFitsWriteImage(fits, NULL, stamp->weight->image, 0, NULL);
            psFitsClose(fits);
        }
#endif

    }

    psFree(keepStamps);

    psLogMsg("psModules.imcombine", PS_LOG_DETAIL, "%s", log);
    psFree(log);

    // calculate and report the normalization and background for the image center
    {
        polyValues = p_pmSubtractionPolynomial(polyValues, kernels->spatialOrder, 0.0, 0.0);
        double norm = p_pmSubtractionSolutionNorm(kernels); // Normalisation
        double background = p_pmSubtractionSolutionBackground(kernels, polyValues);// Difference in background
        psLogMsg("psModules.imcombine", PS_LOG_INFO, "normalization: %f, background: %f", norm, background);

        pmSubtractionVisualShowFit(norm);
        pmSubtractionVisualPlotFit(kernels);

        psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
        psVectorStats (stats, fResSigma, NULL, NULL, 0);
        kernels->fResSigmaMean  = stats->robustMedian;
        kernels->fResSigmaStdev = stats->robustStdev;

        psStatsInit (stats);
        psVectorStats (stats, fResOuter, NULL, NULL, 0);
        kernels->fResOuterMean  = stats->robustMedian;
        kernels->fResOuterStdev = stats->robustStdev;

        psStatsInit (stats);
        psVectorStats (stats, fResTotal, NULL, NULL, 0);
        kernels->fResTotalMean  = stats->robustMedian;
        kernels->fResTotalStdev = stats->robustStdev;

        // XXX save these values somewhere
        psLogMsg("psModules.imcombine", PS_LOG_INFO, "fResSigma: %f +/- %f, fResOuter: %f +/- %f, fResTotal: %f +/- %f",
                 kernels->fResSigmaMean, kernels->fResSigmaStdev,
                 kernels->fResOuterMean, kernels->fResOuterStdev,
                 kernels->fResTotalMean, kernels->fResTotalStdev);

        psFree (fResSigma);
        psFree (fResOuter);
        psFree (fResTotal);
        psFree (stats);
    }

    psFree(residual);
    psFree(polyValues);

    return deviations;
}

// we are supplied U, not Ut; w represents a diagonal matrix (also, we apply 1/w instead of w)
psImage *p_pmSubSolve_wUt (psVector *w, psImage *U) {

    psAssert (w->n == U->numCols, "w and U dimensions do not match");

    // wUt has dimensions transposed relative to Ut.
    psImage *wUt = psImageAlloc (U->numRows, U->numCols, PS_TYPE_F64);
    psImageInit (wUt, 0.0);

    for (int i = 0; i < wUt->numCols; i++) {
        for (int j = 0; j < wUt->numRows; j++) {
            if (!isfinite(w->data.F64[j])) continue;
            if (w->data.F64[j] == 0.0) continue;
            wUt->data.F64[j][i] = U->data.F64[i][j] / w->data.F64[j];
        }
    }
    return wUt;
}

// XXX this is just standard matrix multiplication: use psMatrixMultiply?
psImage *p_pmSubSolve_VwUt (psImage *V, psImage *wUt) {

    psAssert (V->numCols == wUt->numRows, "matrix dimensions do not match");

    psImage *Ainv = psImageAlloc (wUt->numCols, V->numRows, PS_TYPE_F64);

    for (int i = 0; i < Ainv->numCols; i++) {
        for (int j = 0; j < Ainv->numRows; j++) {
            double sum = 0.0;
            for (int k = 0; k < V->numCols; k++) {
                sum += V->data.F64[j][k] * wUt->data.F64[k][i];
            }
            Ainv->data.F64[j][i] = sum;
        }
    }
    return Ainv;
}

// we are supplied U, not Ut
bool p_pmSubSolve_UtB (psVector **UtB, psImage *U, psVector *B) {

    psAssert (U->numRows == B->n, "U and B dimensions do not match");

    UtB[0] = psVectorRecycle (UtB[0], U->numCols, PS_TYPE_F64);

    for (int i = 0; i < U->numCols; i++) {
        double sum = 0.0;
        for (int j = 0; j < U->numRows; j++) {
            sum += B->data.F64[j] * U->data.F64[j][i];
        }
        UtB[0]->data.F64[i] = sum;
    }
    return true;
}

// w is diagonal
bool p_pmSubSolve_wUtB (psVector **wUtB, psVector *w, psVector *UtB) {

    psAssert (w->n == UtB->n, "w and UtB dimensions do not match");

    // wUt has dimensions transposed relative to Ut.
    wUtB[0] = psVectorRecycle (wUtB[0], w->n, PS_TYPE_F64);
    psVectorInit (wUtB[0], 0.0);

    for (int i = 0; i < w->n; i++) {
        if (!isfinite(w->data.F64[i])) continue;
        if (w->data.F64[i] == 0.0) continue;
        wUtB[0]->data.F64[i] = UtB->data.F64[i] / w->data.F64[i];
    }
    return true;
}

// this is basically matrix * vector
bool p_pmSubSolve_VwUtB (psVector **VwUtB, psImage *V, psVector *wUtB) {

    psAssert (V->numCols == wUtB->n, "V and wUtB dimensions do not match");

    VwUtB[0] = psVectorRecycle (*VwUtB, V->numRows, PS_TYPE_F64);

    for (int j = 0; j < V->numRows; j++) {
        double sum = 0.0;
        for (int i = 0; i < V->numCols; i++) {
            sum += V->data.F64[j][i] * wUtB->data.F64[i];
        }
        VwUtB[0]->data.F64[j] = sum;
    }
    return true;
}

// this is basically matrix * vector
bool p_pmSubSolve_Ax (psVector **B, psImage *A, psVector *x) {

    psAssert (A->numCols == x->n, "A and x dimensions do not match");

    B[0] = psVectorRecycle (*B, A->numRows, PS_TYPE_F64);

    for (int j = 0; j < A->numRows; j++) {
        double sum = 0.0;
        for (int i = 0; i < A->numCols; i++) {
            sum += A->data.F64[j][i] * x->data.F64[i];
        }
        B[0]->data.F64[j] = sum;
    }
    return true;
}

// this is basically Vector * vector
bool p_pmSubSolve_VdV (double *value, psVector *x, psVector *y) {

    psAssert (x->n == y->n, "x and y dimensions do not match");

    double sum = 0.0;
    for (int i = 0; i < x->n; i++) {
        sum += x->data.F64[i] * y->data.F64[i];
    }
    *value = sum;
    return true;
}

bool p_pmSubSolve_y2 (double *y2, pmSubtractionKernels *kernels, const pmSubtractionStampList *stamps) {

    int footprint = stamps->footprint; // Half-size of stamps

    double sum = 0.0;
    for (int i = 0; i < stamps->num; i++) {

        pmSubtractionStamp *stamp = stamps->stamps->data[i];
        if (stamp->status != PM_SUBTRACTION_STAMP_USED) continue;

        psKernel *weight = NULL;
        psKernel *window = NULL;
        psKernel *input = NULL;

#ifdef USE_WEIGHT
        weight = stamp->weight;
#endif
#ifdef USE_WINDOW
        window = stamps->window;
#endif

        switch (kernels->mode) {
            // MODE_1 : convolve image 1 to match image 2 (and vice versa)
          case PM_SUBTRACTION_MODE_1:
            input = stamp->image2;
            break;
          case PM_SUBTRACTION_MODE_2:
            input = stamp->image1;
            break;
          default:
            psAbort ("programming error");
        }

        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                double in = input->kernel[y][x];
                double value = in*in;
                if (weight) {
                    float wtVal = weight->kernel[y][x];
                    value *= wtVal;
                }
                if (window) {
                    float  winVal = window->kernel[y][x];
                    value *= winVal;
                }
                sum += value;
            }
        }
    }
    *y2 = sum;
    return true;
}

double p_pmSubSolve_ChiSquare (pmSubtractionKernels *kernels, const pmSubtractionStampList *stamps) {

    int footprint = stamps->footprint; // Half-size of stamps
    int numKernels = kernels->num;      // Number of kernels

    double sum = 0.0;

    psKernel *residual = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Residual image
    psImageInit(residual->image, 0.0);

    psImage *polyValues = NULL;         // Polynomial values

    for (int i = 0; i < stamps->num; i++) {

        pmSubtractionStamp *stamp = stamps->stamps->data[i];
        if (stamp->status != PM_SUBTRACTION_STAMP_USED) continue;

        psKernel *weight = NULL;
        psKernel *window = NULL;
        psKernel *target = NULL;
        psKernel *source = NULL;

        psArray *convolutions = NULL;

#ifdef USE_WEIGHT
        weight = stamp->weight;
#endif
#ifdef USE_WINDOW
        window = stamps->window;
#endif

        switch (kernels->mode) {
            // MODE_1 : convolve image 1 to match image 2 (and vice versa)
          case PM_SUBTRACTION_MODE_1:
            target = stamp->image2;
            source = stamp->image1;
            convolutions = stamp->convolutions1;
            break;
          case PM_SUBTRACTION_MODE_2:
            target = stamp->image1;
            source = stamp->image2;
            convolutions = stamp->convolutions2;
            break;
          default:
            psAbort ("programming error");
        }

        // Calculate coefficients of the kernel basis functions
        polyValues = p_pmSubtractionPolynomial(polyValues, kernels->spatialOrder, stamp->xNorm, stamp->yNorm);
        double norm = p_pmSubtractionSolutionNorm(kernels); // Normalisation
        double background = p_pmSubtractionSolutionBackground(kernels, polyValues);// Difference in background

        psImageInit(residual->image, 0.0);
        for (int j = 0; j < numKernels; j++) {
            psKernel *convolution = convolutions->data[j]; // Convolution
            double coefficient = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, false); // Coefficient
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    residual->kernel[y][x] -= convolution->kernel[y][x] * coefficient;
                }
            }
        }

        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                double resid = target->kernel[y][x] - background - source->kernel[y][x] * norm + residual->kernel[y][x];
                double value = PS_SQR(resid);
                if (weight) {
                    float wtVal = weight->kernel[y][x];
                    value *= wtVal;
                }
                if (window) {
                    float  winVal = window->kernel[y][x];
                    value *= winVal;
                }
                sum += value;
            }
        }
    }
    psFree (polyValues);
    psFree (residual);

    return sum;
}

bool p_pmSubSolve_SetWeights (psVector *wApply, psVector *w, psVector *wMask) {

    for (int i = 0; i < w->n; i++) {
        wApply->data.F64[i] = wMask->data.U8[i] ? 0.0 : w->data.F64[i];
    }
    return true;
}

// we are supplied V and w; w represents a diagonal matrix (also, we apply 1/w instead of w)
psImage *p_pmSubSolve_Xvar (psImage *V, psVector *w) {

    psAssert (w->n == V->numCols, "w and U dimensions do not match");

    psImage *Vn = psImageAlloc (V->numCols, V->numRows, PS_TYPE_F64);
    psImageInit (Vn, 0.0);

    // generate Vn = V * w^{-1}
    for (int j = 0; j < Vn->numRows; j++) {
        for (int i = 0; i < Vn->numCols; i++) {
            if (!isfinite(w->data.F64[i])) continue;
            if (w->data.F64[i] == 0.0) continue;
            Vn->data.F64[j][i] = V->data.F64[j][i] / w->data.F64[i];
        }
    }

    psImage *Xvar = psImageAlloc (V->numCols, V->numRows, PS_TYPE_F64);
    psImageInit (Xvar, 0.0);

    // generate Xvar = Vn * Vn^T
    for (int j = 0; j < Vn->numRows; j++) {
        for (int i = 0; i < Vn->numCols; i++) {
            double sum = 0.0;
            for (int k = 0; k < Vn->numCols; k++) {
                sum += Vn->data.F64[k][i]*Vn->data.F64[k][j];
            }
            Xvar->data.F64[j][i] = sum;
        }
    }
    return Xvar;
}

// I get confused by the index values between the image vs matrix usage:  In terms
// of the elements of an image A(x,y) = A->data.F64[y][x] = A_x,y, a matrix
// multiplication is: A_k,j * B_i,k = C_i,j


bool psFitsWriteImageSimple (char *filename, psImage *image, psMetadata *header) {

    psFits *fits = psFitsOpen(filename, "w");
    psFitsWriteImage(fits, header, image, 0, NULL);
    psFitsClose(fits);

    return true;
}

bool psVectorWriteFile (char *filename, const psVector *vector) {

    FILE *f = fopen (filename, "w");
    int fd = fileno(f);
    p_psVectorPrint (fd, vector, "unnamed");
    fclose (f);

    return true;
}


# if 0

#ifdef TESTING
        psFitsWriteImageSimple("A.fits", sumMatrix, NULL);
        psVectorWriteFile ("B.dat", sumVector);
#endif

# define SVD_ANALYSIS 0
# define COEFF_SIG 0.0
# define SVD_TOL 0.0

        // Use SVD to determine the kernel coeffs (and validate)
        if (SVD_ANALYSIS) {

            // We have sumVector and sumMatrix.  we are trying to solve the following equation:
            // sumMatrix * x = sumVector.

            // we can use any standard matrix inversion to solve this.  However, the basis
            // functions in general have substantial correlation, so that the solution may be
            // somewhat poorly determined or unstable.  If not numerically ill-conditioned, the
            // system of equations may be statistically ill-conditioned.  Noise in the image
            // will drive insignificant, but correlated, terms in the solution.  To avoid these
            // problems, we can use SVD to identify numerically unconstrained values and to
            // avoid statistically badly determined value.

            // A = sumMatrix, B = sumVector
            // SVD: A = U w V^T  -> A^{-1} = V (1/w) U^T
            // x = V (1/w) (U^T B)
            // \sigma_x = sqrt(diag(A^{-1}))
            // solve for x and A^{-1} to get x & dx
            // identify the elements of (1/w) that are nan (1/0.0) -> set to 0.0
            // identify the elements of x that are insignificant (x / dx < 1.0? < 0.5?) -> set to 0.0

            // If I use the SVD trick to re-condition the matrix, I need to break out the
            // kernel and normalization terms from the background term.
            // XXX is this true?  or was this due to an error in the analysis?

            int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index in matrix for background

            // now pull out the kernel elements into their own square matrix
            psImage  *kernelMatrix = psImageAlloc  (sumMatrix->numCols - 1, sumMatrix->numRows - 1, PS_TYPE_F64);
            psVector *kernelVector = psVectorAlloc (sumMatrix->numCols - 1, PS_TYPE_F64);

            for (int ix = 0, kx = 0; ix < sumMatrix->numCols; ix++) {
                if (ix == bgIndex) continue;
                for (int iy = 0, ky = 0; iy < sumMatrix->numRows; iy++) {
                    if (iy == bgIndex) continue;
                    kernelMatrix->data.F64[ky][kx] = sumMatrix->data.F64[iy][ix];
                    ky++;
                }
                kernelVector->data.F64[kx] = sumVector->data.F64[ix];
                kx++;
            }

            psImage *U = NULL;
            psImage *V = NULL;
            psVector *w = NULL;
            if (!psMatrixSVD (&U, &w, &V, kernelMatrix)) {
                psError(psErrorCodeLast(), false, "failed to perform SVD on sumMatrix\n");
                return NULL;
            }

            // calculate A_inverse:
            // Ainv = V * w * U^T
            psImage *wUt  = p_pmSubSolve_wUt (w, U);
            psImage *Ainv = p_pmSubSolve_VwUt (V, wUt);
            psImage *Xvar = NULL;
            psFree (wUt);

# ifdef TESTING
            // kernel terms:
            for (int i = 0; i < w->n; i++) {
                fprintf (stderr, "w: %f\n", w->data.F64[i]);
            }
# endif
            // loop over w adding in more and more of the values until chisquare is no longer
            // dropping significantly.
            // XXX this does not seem to work very well: we seem to need all terms even for
            // simple cases...

            psVector *Xsvd = NULL;
            {
                psVector *Ax = NULL;
                psVector *UtB = NULL;
                psVector *wUtB = NULL;

                psVector *wApply = psVectorAlloc(w->n, PS_TYPE_F64);
                psVector *wMask = psVectorAlloc(w->n, PS_TYPE_U8);
                psVectorInit (wMask, 1); // start by masking everything

                double chiSquareLast = NAN;
                int maxWeight = 0;

                double Axx, Bx, y2;

                // XXX this is an attempt to exclude insignificant modes.
                // it was not successful with the ISIS kernel set: removing even
                // the least significant mode leaves additional ringing / noise
                // because the terms are so coupled.
                for (int k = 0; false && (k < w->n); k++) {

                    // unmask the k-th weight
                    wMask->data.U8[k] = 0;
                    p_pmSubSolve_SetWeights(wApply, w, wMask);

                    // solve for x:
                    // x = V * w * (U^T * B)
                    p_pmSubSolve_UtB (&UtB, U, kernelVector);
                    p_pmSubSolve_wUtB (&wUtB, wApply, UtB);
                    p_pmSubSolve_VwUtB (&Xsvd, V, wUtB);

                    // chi-square for this system of equations:
                    // chi-square = sum over terms of: (Ax - B)*x - b*x - y^2
                    // y^2 = \sum_stamps \sum_pixels input->kernel[y][x]^2
                    p_pmSubSolve_Ax (&Ax, kernelMatrix, Xsvd);
                    p_pmSubSolve_VdV (&Axx, Ax, Xsvd);
                    p_pmSubSolve_VdV (&Bx, kernelVector, Xsvd);
                    p_pmSubSolve_y2 (&y2, kernels, stamps);

                    // apparently, this works (compare with the brute force value below
                    double chiSquare = Axx - 2.0*Bx + y2;
                    double deltaChi = (k == 0) ? chiSquare : chiSquareLast - chiSquare;
                    chiSquareLast = chiSquare;

                    // fprintf (stderr, "chi square = %f, delta: %f\n", chiSquare, deltaChi);
                    if (k && !maxWeight && (deltaChi < 1.0)) {
                        maxWeight = k;
                    }
                }

                // keep all terms or we get extra ringing
                maxWeight = w->n;
                psVectorInit (wMask, 1);
                for (int k = 0; k < maxWeight; k++) {
                    wMask->data.U8[k] = 0;
                }
                p_pmSubSolve_SetWeights(wApply, w, wMask);

                // solve for x:
                // x = V * w * (U^T * B)
                p_pmSubSolve_UtB (&UtB, U, kernelVector);
                p_pmSubSolve_wUtB (&wUtB, wApply, UtB);
                p_pmSubSolve_VwUtB (&Xsvd, V, wUtB);

                // chi-square for this system of equations:
                // chi-square = sum over terms of: (Ax - B)*x - b*x - y^2
                // y^2 = \sum_stamps \sum_pixels input->kernel[y][x]^2
                p_pmSubSolve_Ax (&Ax, kernelMatrix, Xsvd);
                p_pmSubSolve_VdV (&Axx, Ax, Xsvd);
                p_pmSubSolve_VdV (&Bx, kernelVector, Xsvd);
                p_pmSubSolve_y2 (&y2, kernels, stamps);

                // apparently, this works (compare with the brute force value below
                double chiSquare = Axx - 2.0*Bx + y2;
                psLogMsg ("psModules.imcombine", PS_LOG_INFO, "model kernel with %d terms; chi square = %f\n", maxWeight, chiSquare);

                // re-calculate A^{-1} to get new variances:
                // Ainv = V * w * U^T
                // XXX since we keep all terms, this is identical to Ainv
                psImage *wUt  = p_pmSubSolve_wUt (wApply, U);
                Xvar = p_pmSubSolve_VwUt (V, wUt);
                psFree (wUt);

                psFree (Ax);
                psFree (UtB);
                psFree (wUtB);
                psFree (wApply);
                psFree (wMask);
            }

            // copy the kernel solutions to the full solution vector:
            solution = psVectorAlloc(sumVector->n, PS_TYPE_F64);
            solutionErr = psVectorAlloc(sumVector->n, PS_TYPE_F64);

            for (int ix = 0, kx = 0; ix < sumVector->n; ix++) {
                if (ix == bgIndex) {
                    solution->data.F64[ix] = 0;
                    solutionErr->data.F64[ix] = 0.001;
                    continue;
                }
                solutionErr->data.F64[ix] = sqrt(Ainv->data.F64[kx][kx]);
                solution->data.F64[ix] = Xsvd->data.F64[kx];
                kx++;
            }

            psFree (kernelMatrix);
            psFree (kernelVector);

            psFree (U);
            psFree (V);
            psFree (w);

            psFree (Ainv);
            psFree (Xsvd);
        } else {
            psVector *permutation = NULL;       // Permutation vector, required for LU decomposition
            psImage *luMatrix = psMatrixLUDecomposition(NULL, &permutation, sumMatrix);
            if (!luMatrix) {
                psError(PM_ERR_DATA, true, "LU Decomposition of least-squares matrix failed.\n");
                psFree(solution);
                psFree(sumVector);
                psFree(sumMatrix);
                psFree(luMatrix);
                psFree(permutation);
                return NULL;
            }

            solution = psMatrixLUSolution(NULL, luMatrix, sumVector, permutation);
            psFree(luMatrix);
            psFree(permutation);
            if (!solution) {
                psError(PM_ERR_DATA, true, "Failed to solve the least-squares system.\n");
                psFree(solution);
                psFree(sumVector);
                psFree(sumMatrix);
                return NULL;
            }

            // XXX LUD does not provide A^{-1}?  fake the error for now
            solutionErr = psVectorAlloc(sumVector->n, PS_TYPE_F64);
            for (int ix = 0; ix < sumVector->n; ix++) {
                solutionErr->data.F64[ix] = 0.1*solution->data.F64[ix];
            }
        }

        if (!kernels->solution1) {
            kernels->solution1 = psVectorAlloc (sumVector->n, PS_TYPE_F64);
            psVectorInit (kernels->solution1, 0.0);
        }

        // only update the solutions that we chose to calculate:
        if (mode & PM_SUBTRACTION_EQUATION_NORM) {
            int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
            kernels->solution1->data.F64[normIndex] = solution->data.F64[normIndex];
        }
        if (mode & PM_SUBTRACTION_EQUATION_BG) {
            int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index in matrix for background
            kernels->solution1->data.F64[bgIndex] = solution->data.F64[bgIndex];
        }
        if (mode & PM_SUBTRACTION_EQUATION_KERNELS) {
            int numKernels = kernels->num;
            int spatialOrder = kernels->spatialOrder;       // Order of spatial variation
            int numPoly = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of polynomial terms
            for (int i = 0; i < numKernels * numPoly; i++) {
                // XXX fprintf (stderr, "%f +/- %f (%f) -> ", solution->data.F64[i], solutionErr->data.F64[i], fabs(solution->data.F64[i]/solutionErr->data.F64[i]));
                if (fabs(solution->data.F64[i] / solutionErr->data.F64[i]) < COEFF_SIG) {
                    // XXX fprintf (stderr, "drop\n");
                    kernels->solution1->data.F64[i] = 0.0;
                } else {
                    // XXX fprintf (stderr, "keep\n");
                    kernels->solution1->data.F64[i] = solution->data.F64[i];
                }
            }
        }
        // double chiSquare = p_pmSubSolve_ChiSquare (kernels, stamps);
        // fprintf (stderr, "chi square Brute = %f\n", chiSquare);

        psFree(solution);
        psFree(sumVector);
        psFree(sumMatrix);
# endif

#ifdef TESTING
              // XXX double-check for NAN in data:
                for (int iy = 0; iy < stamp->matrix->numRows; iy++) {
                    for (int ix = 0; ix < stamp->matrix->numCols; ix++) {
                        if (!isfinite(stamp->matrix->data.F64[iy][ix])) {
                            fprintf (stderr, "WARNING: NAN in matrix\n");
                        }
                    }
                }
                for (int ix = 0; ix < stamp->vector->n; ix++) {
                    if (!isfinite(stamp->vector->data.F64[ix])) {
                        fprintf (stderr, "WARNING: NAN in vector\n");
                    }
                }
#endif

#ifdef TESTING
        for (int ix = 0; ix < sumVector->n; ix++) {
            if (!isfinite(sumVector->data.F64[ix])) {
                fprintf (stderr, "WARNING: NAN in vector\n");
            }
        }
#endif

#ifdef TESTING
        for (int ix = 0; ix < sumVector->n; ix++) {
            if (!isfinite(sumVector->data.F64[ix])) {
                fprintf (stderr, "WARNING: NAN in vector\n");
            }
        }
        {
            psImage *inverse = psMatrixInvert(NULL, sumMatrix, NULL);
            psFitsWriteImageSimple("matrixInv.fits", inverse, NULL);
            psFree(inverse);
        }
        {
            psImage *X = psMatrixInvert(NULL, sumMatrix, NULL);
            psImage *Xt = psMatrixTranspose(NULL, X);
            psImage *XtX = psMatrixMultiply(NULL, Xt, X);
            psFitsWriteImageSimple("matrixErr.fits", XtX, NULL);
            psFree(X);
            psFree(Xt);
            psFree(XtX);
        }
#endif

