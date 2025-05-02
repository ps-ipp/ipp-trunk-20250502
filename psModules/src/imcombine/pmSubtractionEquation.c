#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmVisual.h"
#include "pmFPA.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionThreads.h"

#include "pmSubtractionEquation.h"
#include "pmSubtractionVisual.h"

//# define TESTING                         // TESTING output for debugging; may not work with threads!
# define USE_WEIGHT                      // Include weight (1/variance) in equation?
# define USE_WINDOW                      // window to avoid neighbor contamination

/* I believe we want to apply the WEIGHT to the chisq portions of the calculation (but not the WINDOW),
 * and the WINDOW to the moments portiosn of the calculations (but not the WEIGHT)
 *
 */

# define PENALTY false
# define MOMENTS (!PENALTY)
# define MOMENTS_PENALTY_SCALE 20 // up-weight the moments somewhat

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Calculate the least-squares matrix and vector
static bool calculateMatrixVector(psImage *matrix,	 // Least-squares matrix, updated
                                  psVector *vector,	 // Least-squares vector, updated
                                  double normValue,	 // Normalisation, supplied
                                  const psKernel *input, // Input image (target)
                                  const psKernel *reference, // Reference image (convolution source)
                                  const psKernel *weight,  // Weight image
                                  const psKernel *window,  // Window image
                                  const psArray *convolutions,         // Convolutions for each kernel
                                  const pmSubtractionKernels *kernels, // Kernels
                                  const psImage *polyValues, // Spatial polynomial values
                                  int footprint // (Half-)Size of stamp
                                  )
{
    // (I - R * sum_i a_i k_i - g) (R * k_j) = 0
    // I C_j = sum_i C_i C_j

    // Background: C_i = 1.0
    // Normalisation: C_i = R

    int numKernels = kernels->num;                      // Number of kernels
    int spatialOrder = kernels->spatialOrder;       // Order of spatial variation
    int numPoly = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of polynomial terms
    double poly[numPoly];                                 // Polynomial terms
    double poly2[numPoly][numPoly];                       // Polynomial-polynomial values
    int numParams = numKernels * numPoly;

    psAssert(matrix &&
             matrix->type.type == PS_TYPE_F64 &&
             matrix->numCols == numParams &&
             matrix->numRows == numParams,
             "Least-squares matrix is bad.");
    psAssert(vector &&
             vector->type.type == PS_TYPE_F64 &&
             vector->n == numParams,
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

    // the order of the elements in the matrix and vector is:
    // [kernel 0, x^0 y^0][kernel 1 x^0 y^0]...[kernel N, x^0 y^0]
    // [kernel 0, x^1 y^0][kernel 1 x^1 y^0]...[kernel N, x^1 y^0]
    // [kernel 0, x^n y^m][kernel 1 x^n y^m]...[kernel N, x^n y^m]

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
		    // XXX NOTE: do NOT apply the window to the chisq portions of the calculation
                    if (false && window) {
                        cc *= window->kernel[y][x];
                    }
                    sumCC += cc;
                }
            }

            // Spatial variation of kernel coeffs
	    for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
		for (int jTerm = 0, jIndex = j; jTerm < numPoly; jTerm++, jIndex += numKernels) {
		    double value = sumCC * poly2[iTerm][jTerm];
		    matrix->data.F64[iIndex][jIndex] = value;
		    matrix->data.F64[jIndex][iIndex] = value;
		}
	    }
        }

        double sumRC = 0.0;             // Sum of the reference-convolution products
        double sumIC = 0.0;             // Sum of the input-convolution products
        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                float conv = iConv->kernel[y][x];
                float in = input->kernel[y][x];
                float ref = reference->kernel[y][x];
                double ic = in * conv;
                double rc = ref * conv;
                if (weight) {
                    float wtVal = weight->kernel[y][x];
                    ic *= wtVal;
                    rc *= wtVal;
                }
		// XXX NOTE: do NOT apply the window to the chisq portions of the calculation
                if (false && window) {
                    float winVal = window->kernel[y][x];
                    ic *= winVal;
                    rc *= winVal;
                }
                sumIC += ic;
                sumRC += rc;
            }
        }

        // Spatial variation
        for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
	    vector->data.F64[iIndex] = (sumIC - normValue*sumRC) * poly[iTerm];
        }
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

# define RENORM_BY_FLUX 0

// Calculate the least-squares matrix and vector for dual convolution
// XXX we could avoid calculating these values on successive passes *if* the stamp has not changed.
static bool calculateDualMatrixVector(pmSubtractionStamp *stamp,	      // stamp of interest
                                      double normValue,	      // Normalisation, updated
                                      double normValue2,      // Normalisation, updated
                                      const psKernel *weight,  // Weight image
                                      const psKernel *window,  // Window image
                                      const pmSubtractionKernels *kernels, // Kernels
                                      const psImage *polyValues, // Spatial polynomial values
                                      int footprint // (Half-)Size of stamp
                                      )
{
    int numKernels = kernels->num;			  // Number of kernels
    int spatialOrder = kernels->spatialOrder;		  // Order of spatial variation
    int numPoly = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of polynomial terms
    double poly[numPoly];                                 // Polynomial terms
    double poly2[numPoly][numPoly];                       // Polynomial-polynomial values

    int numParams = numKernels * numPoly;		  // Number of regular parameters
    int numParams2 = numKernels * numPoly;	   // Number of additional parameters for dual
    int numDual = numParams + numParams2;	   // Total number of parameters for dual

    psImage *matrix = stamp->matrix;		   // Least-squares matrix, updated
    psVector *vector = stamp->vector;		   // Least-squares vector, updated
    psKernel *image1 = stamp->image1;		   // Image 1
    psKernel *image2 = stamp->image2;		   // Image 2
    psArray *convolutions1 = stamp->convolutions1; // Convolutions of image 1 for each kernel
    psArray *convolutions2 = stamp->convolutions2; // Convolutions of image 2 for each kernel

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

    // the order of the elements in the matrix and vector is:
    // [kernel 0, x^0 y^0][kernel 1 x^0 y^0]...[kernel N, x^0 y^0]
    // [kernel 0, x^1 y^0][kernel 1 x^1 y^0]...[kernel N, x^1 y^0]
    // [kernel 0, x^n y^m][kernel 1 x^n y^m]...[kernel N, x^n y^m]

    // for DUAL convolution analysis, we apply the normalization to I1 as follows:
    // norm = I2 / I1
    // 
    // I1c = norm I1 + \Sum_i a_i norm I1 \cross k_i
    // I2c =      I2 + \Sum_i b_i      I2 \cross k_i

    // we cannot absorb the normalization into a_i until the analysis is complete, or the
    // second moment terms are incorrectly calculated.

    for (int i = 0; i < numKernels; i++) {
        psKernel *iConv1 = convolutions1->data[i]; // Convolution 1 for index i
        psKernel *iConv2 = convolutions2->data[i]; // Convolution 2 for index i
        for (int j = i; j < numKernels; j++) {
            psKernel *jConv1 = convolutions1->data[j]; // Convolution 1 for index j
            psKernel *jConv2 = convolutions2->data[j]; // Convolution 2 for index j

            double sumAA = 0.0;         // Sum of convolution products between image 1
            double sumBB = 0.0;         // Sum of convolution products between image 2
            double sumAB = 0.0;         // Sum of convolution products across images 1 and 2

	    double MxxAA = 0.0;
	    double MyyAA = 0.0;
	    double MxxBB = 0.0;
	    double MyyBB = 0.0;
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {

		    // XXX NOTE: clipping low S/N pixels does not seem to work very well
		    if (false && weight) {
			float i1 = image1->kernel[y][x];
			float i2 = image2->kernel[y][x];
			float sn = (i1 + i2) / sqrt (weight->kernel[y][x]);
			if (sn < 0.5) continue;
		    }

                    double aa = iConv1->kernel[y][x] * jConv1->kernel[y][x] * PS_SQR(normValue);
                    double bb = iConv2->kernel[y][x] * jConv2->kernel[y][x];
                    double ab = iConv1->kernel[y][x] * jConv2->kernel[y][x] * normValue;

		    float wtVal = (weight) ? weight->kernel[y][x] : 1.0;
                    sumAA += wtVal*aa;
                    sumBB += wtVal*bb;
                    sumAB += wtVal*ab;

		    if (MOMENTS) {
			float winVal = (window) ? window->kernel[y][x] : 1.0;
			MxxAA += winVal*x*x*aa;
			MyyAA += winVal*y*y*aa;
			MxxBB += winVal*x*x*bb;
			MyyBB += winVal*y*y*bb;
		    }
                }
            }

	    // XXX does normSquare1,2 mess up the relative scaling?
	    // XXX no: normSquare1,2 is the sum of the flux^2 for the source
	    if (MOMENTS) {
		MxxAA /= stamp->normSquare1 * PS_SQR(normValue);
		MyyAA /= stamp->normSquare1 * PS_SQR(normValue);
		MxxBB /= stamp->normSquare2;
		MyyBB /= stamp->normSquare2;
	    }

	    // XXX this makes the Chisq portion independent of the normalization and star flux
	    // but may be mis-scaling between stars of different fluxes
# if (RENORM_BY_FLUX)	    
	    sumAA /= PS_SQR(stamp->normI2);
	    sumAB /= PS_SQR(stamp->normI2);
	    sumBB /= PS_SQR(stamp->normI2);
# endif

	    // fprintf (stderr, "i,j : %d %d : M(xx,yy)(AA,BB) : %f %f %f %f\n", i, j, MxxAA, MyyAA, MxxBB, MyyBB);

            // Spatial variation of kernel coeffs
	    for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
		for (int jTerm = 0, jIndex = j; jTerm < numPoly; jTerm++, jIndex += numKernels) {
		    double aa = sumAA * poly2[iTerm][jTerm];
		    double bb = sumBB * poly2[iTerm][jTerm];
		    double ab = sumAB * poly2[iTerm][jTerm];

		    matrix->data.F64[iIndex][jIndex] = aa;
		    matrix->data.F64[jIndex][iIndex] = aa;

		    matrix->data.F64[iIndex + numParams][jIndex + numParams] = bb;
		    matrix->data.F64[jIndex + numParams][iIndex + numParams] = bb;

		    // add in second moments
		    if (MOMENTS) {
			matrix->data.F64[iIndex][jIndex] += kernels->penalty * MxxAA * MOMENTS_PENALTY_SCALE;
			matrix->data.F64[iIndex][jIndex] += kernels->penalty * MyyAA * MOMENTS_PENALTY_SCALE;
			matrix->data.F64[jIndex][iIndex] += kernels->penalty * MxxAA * MOMENTS_PENALTY_SCALE;
			matrix->data.F64[jIndex][iIndex] += kernels->penalty * MyyAA * MOMENTS_PENALTY_SCALE;

			matrix->data.F64[iIndex + numParams][jIndex + numParams] += kernels->penalty * MxxBB * MOMENTS_PENALTY_SCALE;
			matrix->data.F64[iIndex + numParams][jIndex + numParams] += kernels->penalty * MyyBB * MOMENTS_PENALTY_SCALE;
			matrix->data.F64[jIndex + numParams][iIndex + numParams] += kernels->penalty * MxxBB * MOMENTS_PENALTY_SCALE;
			matrix->data.F64[jIndex + numParams][iIndex + numParams] += kernels->penalty * MyyBB * MOMENTS_PENALTY_SCALE;
		    }
		    matrix->data.F64[iIndex][jIndex + numParams] = ab;
		    matrix->data.F64[jIndex + numParams][iIndex] = ab;
		}
	    }
        }

	// we need to calculate the lower-diagonal AB elements since they are not symmetric for A <-> B
        for (int j = 0; j < i; j++) {
            psKernel *jConv2 = convolutions2->data[j]; // Convolution 2 for index j
            double sumAB = 0.0;         // Sum of convolution products for matrix C
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    double ab = iConv1->kernel[y][x] * jConv2->kernel[y][x] * normValue;
                    if (weight) {
                        ab *= weight->kernel[y][x];
                    }
		    // XXX NOTE: do NOT apply the window to the chisq portions of the calculation
                    if (false && window) {
                        ab *= window->kernel[y][x];
                    }
                    sumAB += ab;
                }
            }

	    // XXX this makes the Chisq portion independent of the normalization and star flux
	    // but may be mis-scaling between stars of different fluxes
# if (RENORM_BY_FLUX)
	    sumAB /= PS_SQR(stamp->normI2);
# endif

            // Spatial variation of kernel coeffs
	    for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
		for (int jTerm = 0, jIndex = j; jTerm < numPoly; jTerm++, jIndex += numKernels) {
		    double ab = sumAB * poly2[iTerm][jTerm];
		    matrix->data.F64[iIndex][jIndex + numParams] = ab;
		    matrix->data.F64[jIndex + numParams][iIndex] = ab;
		}
            }
        }

        double sumAI2 = 0.0;            // Sum of A.I_2 products (for vector)
        double sumBI2 = 0.0;            // Sum of B.I_2 products (for vector)
        double sumAI1 = 0.0;            // Sum of A.I_1 products (for matrix, normalisation)
        double sumBI1 = 0.0;            // Sum of B.I_1 products (for matrix, normalisation)

	double MxxAI1 = 0.0;
	double MyyAI1 = 0.0;
	double MxxBI2 = 0.0;
	double MyyBI2 = 0.0;
        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                double a = iConv1->kernel[y][x];
                double b = iConv2->kernel[y][x];
                float i1 = image1->kernel[y][x];
                float i2 = image2->kernel[y][x];

		// XXX NOTE: clipping low S/N pixels does not seem to work very well
		if (false && weight) {
		    float sn = (i1 + i2) / sqrt (weight->kernel[y][x]);
		    if (sn < 0.5) continue;
		}

                double ai2 = a * i2 * normValue;
                double bi2 = b * i2;
                double ai1 = a * i1 * PS_SQR(normValue);
                double bi1 = b * i1 * normValue;

		float wtVal = (weight) ? weight->kernel[y][x] : 1.0;
                sumAI2 += wtVal*ai2;
                sumBI2 += wtVal*bi2;
                sumAI1 += wtVal*ai1;
                sumBI1 += wtVal*bi1;

		if (MOMENTS) {
		    float winVal = (window) ? window->kernel[y][x] : 1.0;
		    MxxAI1 += winVal*x*x*ai1;
		    MyyAI1 += winVal*y*y*ai1;
		    MxxBI2 += winVal*x*x*bi2;
		    MyyBI2 += winVal*y*y*bi2;
		}
            }
        }

	if (MOMENTS) {
	    MxxAI1 /= stamp->normSquare1 * PS_SQR(normValue);
	    MyyAI1 /= stamp->normSquare1 * PS_SQR(normValue);
	    MxxBI2 /= stamp->normSquare2;
	    MyyBI2 /= stamp->normSquare2;
	}

	// fprintf (stderr, "i : %d : M(xx,yy)(AI1,BI2) : %f %f %f %f\n", i, MxxAI1, MyyAI1, MxxBI2, MyyBI2);

	// XXX this makes the Chisq portion independent of the normalization and star flux
	// but may be mis-scaling between stars of different fluxes
# if (RENORM_BY_FLUX)
	sumAI1 /= PS_SQR(stamp->normI2);
	sumBI1 /= PS_SQR(stamp->normI2);
	sumAI2 /= PS_SQR(stamp->normI2);
	sumBI2 /= PS_SQR(stamp->normI2);
# endif

        // Spatial variation
        for (int iTerm = 0, iIndex = i; iTerm < numPoly; iTerm++, iIndex += numKernels) {
            double ai2 = sumAI2 * poly[iTerm];
            double bi2 = sumBI2 * poly[iTerm];
            double ai1 = sumAI1 * poly[iTerm];
            double bi1 = sumBI1 * poly[iTerm];
	    vector->data.F64[iIndex]             = ai2 - ai1;
	    vector->data.F64[iIndex + numParams] = bi2 - bi1;

	    // fprintf (stderr, "i : %d : V(I1,I2) : %f %f\n", i, vector->data.F64[iIndex], vector->data.F64[iIndex + numParams]);

	    // add in second moments
	    if (MOMENTS) {
		vector->data.F64[iIndex]             -= kernels->penalty * MxxAI1 * MOMENTS_PENALTY_SCALE;
		vector->data.F64[iIndex]             -= kernels->penalty * MyyAI1 * MOMENTS_PENALTY_SCALE;

		vector->data.F64[iIndex + numParams] -= kernels->penalty * MxxBI2 * MOMENTS_PENALTY_SCALE;
		vector->data.F64[iIndex + numParams] -= kernels->penalty * MyyBI2 * MOMENTS_PENALTY_SCALE;
	    }
        }
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

// Add in penalty term to least-squares vector
bool calculatePenalty(psImage *matrix,                     // Matrix to which to add in penalty term
		      psVector *vector,                    // Vector to which to add in penalty term
		      const pmSubtractionKernels *kernels, // Kernel parameters
		      float normSquare1,		   // Normalisation for image 1
		      float normSquare2		   // Normalisation for image 2
  )
{
    psAssert (kernels->mode == PM_SUBTRACTION_MODE_DUAL, "only use penalties for dual convolution");

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

    // [q_0,x_0,y_0 q_1,x_0,y_0, q_2,x_0,y_0]
    // [q_0,x_1,y_0 q_1,x_1,y_0, q_2,x_1,y_0]
    // [q_0,x_0,y_1 q_1,x_0,y_1, q_2,x_0,y_1]

    for (int i = 0; i < numKernels; i++) {
        for (int yOrder = 0, index = i; yOrder <= spatialOrder; yOrder++) {
            for (int xOrder = 0; xOrder <= spatialOrder - yOrder; xOrder++, index += numKernels) {
                // Contribution to chi^2: a_i^2 P_i
                psAssert(isfinite(penalties1->data.F32[i]), "Invalid penalty");
		// fprintf (stderr, "penalty: %f + %f (%f * %f)\n", matrix->data.F64[index][index], normSquare1 * penalties1->data.F32[i], normSquare1, penalties1->data.F32[i]);
                matrix->data.F64[index][index] += normSquare1 * penalties1->data.F32[i];

		// fprintf (stderr, "penalty: (x^%d y^%d fwhm %f) : %f + %f (%f * %f)\n", kernels->u->data.S32[index], kernels->v->data.S32[index], kernels->widths->data.F32[index], 
		// matrix->data.F64[index + numParams][index + numParams], normSquare2 * penalties2->data.F32[i], normSquare2, penalties2->data.F32[i]);
		matrix->data.F64[index + numParams][index + numParams] += normSquare2 * penalties2->data.F32[i];			     
		// matrix[i][i] is ~ (k_i * I_1)(k_i * I_1)
		// penalties scale with second moments
		//
            }
        }
    }

    return true;
}

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
    // This is probably in a tight loop, so don't check inputs
    psVector *solution = wantDual ? kernels->solution2 : kernels->solution1; // Solution vector
    return p_pmSubtractionCalculatePolynomial(solution, polyValues, kernels->spatialOrder, index,
                                              kernels->num);
}

double p_pmSubtractionSolutionNorm(const pmSubtractionKernels *kernels)
{
    // This is probably in a tight loop, so don't check inputs
    int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
    return kernels->solution1->data.F64[normIndex];
}

double p_pmSubtractionSolutionBackground(const pmSubtractionKernels *kernels,
                                                const psImage *polyValues)
{
    // This is probably in a tight loop, so don't check inputs
    int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background
    return p_pmSubtractionCalculatePolynomial(kernels->solution1, polyValues, kernels->bgOrder, bgIndex, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmSubtractionCalculateMoments(
    pmSubtractionKernels *kernels, // Kernels
    pmSubtractionStampList *stamps)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);

    // XXX skip this, right?
    return true;

    // these are only used by DUAL mode
    if (kernels->mode != PM_SUBTRACTION_MODE_DUAL) return true;

    psTimerStart("pmSubtractionCalculateMoments");

    int footprint = stamps->footprint;  // Half-size of stamps

    // Loop over each stamp and calculate its normalization factor 
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (stamp->status == PM_SUBTRACTION_STAMP_REJECTED) continue;
        if (stamp->status == PM_SUBTRACTION_STAMP_NONE) continue;

	pmSubtractionCalculateMomentsStamp(kernels, stamp, footprint, stamps->normWindow2, stamps->normWindow1);
    }

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Calculate moments: %f sec", psTimerClear("pmSubtractionCalculateMoments"));

    return true;
}

bool pmSubtractionCalculateMomentsStamp(
    pmSubtractionKernels *kernels, // Kernels
    pmSubtractionStamp *stamp,		// stamp on which to save normalization)
    int footprint,			// (Half-)Size of stamp
    int normWindow1,			// Window (half-)size for normalisation measurement
    int normWindow2			// Window (half-)size for normalisation measurement
    )
{
    double Mxx, Myy;

    int numKernels = kernels->num;

    // Generate convolutions: these are generated once and saved
    if (!pmSubtractionConvolveStamp(stamp, kernels, footprint)) {
        psError(psErrorCodeLast(), false, "Unable to convolve stamp");
        return false;
    }

    if (!stamp->MxxI1) {
	stamp->MxxI1 = psVectorAlloc (numKernels, PS_TYPE_F32);
    }
    if (!stamp->MyyI1) {
	stamp->MyyI1 = psVectorAlloc (numKernels, PS_TYPE_F32);
    }
    if (!stamp->MxxI2) {
	stamp->MxxI2 = psVectorAlloc (numKernels, PS_TYPE_F32);
    }
    if (!stamp->MyyI2) {
	stamp->MyyI2 = psVectorAlloc (numKernels, PS_TYPE_F32);
    }

    for (int i = 0; i < numKernels; i++) {
        pmSubtractionCalculateMomentsKernel(&Mxx, &Myy, stamp->convolutions1->data[i], footprint, normWindow1);
	stamp->MxxI1->data.F32[i] = Mxx / stamp->normI1;
	stamp->MyyI1->data.F32[i] = Myy / stamp->normI1;
        pmSubtractionCalculateMomentsKernel(&Mxx, &Myy, stamp->convolutions2->data[i], footprint, normWindow2);
	stamp->MxxI2->data.F32[i] = Mxx / stamp->normI2;
	stamp->MyyI2->data.F32[i] = Myy / stamp->normI2;
    }

    pmSubtractionCalculateMomentsKernel(&Mxx, &Myy, stamp->image1, footprint, normWindow1);
    stamp->MxxI1raw = Mxx / stamp->normI1;
    stamp->MyyI1raw = Myy / stamp->normI1;

    pmSubtractionCalculateMomentsKernel(&Mxx, &Myy, stamp->image2, footprint, normWindow2);
    stamp->MxxI2raw = Mxx / stamp->normI2;
    stamp->MyyI2raw = Myy / stamp->normI2;

    // fprintf (stderr, "Mxx I1: %f, Myy I1: %f, Mxx I2: %f, Myy I2: %f\n", stamp->MxxI1raw, stamp->MyyI1raw, stamp->MxxI2raw, stamp->MyyI2raw);

    return true;
}

bool pmSubtractionCalculateMomentsKernel(double *Mxx, double *Myy, psKernel *image, int footprint, int window) {

    double Sxx = 0.0;
    double Syy = 0.0;
    for (int y = - footprint; y <= footprint; y++) {
        for (int x = - footprint; x <= footprint; x++) {
            if (PS_SQR(x) + PS_SQR(y) > PS_SQR(window)) continue;

            double flux = image->kernel[y][x];

	    Sxx += PS_SQR(x) * flux;
	    Syy += PS_SQR(y) * flux;
        }
    }
    *Mxx = Sxx;
    *Myy = Syy;
    return true;
}

///---------

bool pmSubtractionCalculateNormalization(
    pmSubtractionStampList *stamps,
    const pmSubtractionMode mode)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);

    psTimerStart("pmSubtractionCalculateNormalization");

    psVector *norms = psVectorAllocEmpty(stamps->num, PS_TYPE_F64); // Normalisations
    psVector *norm2 = psVectorAllocEmpty(stamps->num, PS_TYPE_F64); // Normalisations

    int footprint = stamps->footprint;  // Half-size of stamps

    // Loop over each stamp and calculate its normalization factor 
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (stamp->status == PM_SUBTRACTION_STAMP_REJECTED) continue;
        if (stamp->status == PM_SUBTRACTION_STAMP_NONE) continue;

	// XXX skip this if we have already calculated it? (stamp->norm does not change, just the median statistic)
	// XXX maybe not: the star may have changed for a given stamp -- only if norm is reset to NAN can we do this
	if (mode == PM_SUBTRACTION_MODE_2) {
	    pmSubtractionCalculateNormalizationStamp(stamp, stamp->image2, stamp->image1, footprint, stamps->normWindow2, stamps->normWindow1);
	} else {
	    pmSubtractionCalculateNormalizationStamp(stamp, stamp->image1, stamp->image2, footprint, stamps->normWindow1, stamps->normWindow2);
	}
	psVectorAppend(norms, stamp->norm);
	psVectorAppend(norm2, stamp->normSquare2 / stamp->normSquare1);
    }

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN); // Statistics for norm
    if (!psVectorStats(stats, norms, NULL, NULL, 0)) {
	psError(PM_ERR_DATA, false, "Unable to determine median normalisation");
	psFree(stats);
	psFree(norms);
	psFree(norm2);
	return false;
    }
    stamps->normValue = stats->robustMedian;

    psStatsInit(stats);
    if (!psVectorStats(stats, norm2, NULL, NULL, 0)) {
	psError(PM_ERR_DATA, false, "Unable to determine median normalisation");
	psFree(stats);
	psFree(norms);
	psFree(norm2);
	return false;
    }
    stamps->normValue2 = stats->robustMedian;

    psLogMsg ("psModules.imcombine", PS_LOG_INFO, "norm (1): %f (%f) %ld\n", stamps->normValue, stamps->normValue2,norms->n);

    psFree(stats);
    psFree(norms);
    psFree(norm2);

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Calculate normalization: %f sec", psTimerClear("pmSubtractionCalculateNormalization"));

    return true;
}

bool pmSubtractionCalculateNormalizationStamp(
    pmSubtractionStamp *stamp,		// stamp on which to save normalization)
    const psKernel *image1,		// Input image (target)
    const psKernel *image2,		// Reference image (convolution source)
    int footprint,			// (Half-)Size of stamp
    int normWindow1,			// Window (half-)size for normalisation measurement
    int normWindow2			// Window (half-)size for normalisation measurement
    )
{
    double normI1 = 0.0;  // Sum of I_1 within the normalisation window (aperture)
    double normI2 = 0.0;  // Sum of I_2 within the normalisation window (aperture)
    double normSquare1 = 0.0;  // Sum of (I_1)^2 within the normalisation window (aperture)
    double normSquare2 = 0.0;  // Sum of (I_2)^2 within the normalisation window (aperture)
    for (int y = - footprint; y <= footprint; y++) {
        for (int x = - footprint; x <= footprint; x++) {
            double im1 = image1->kernel[y][x];
            double im2 = image2->kernel[y][x];

            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(normWindow1)) {
                normI1 += im1;
		normSquare1 += PS_SQR(im1);
            }
            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(normWindow2)) {
                normI2 += im2;
		normSquare2 += PS_SQR(im2);
            }
        }
    }
    //    psLogMsg("psModules.imcombine",PS_LOG_INFO, "stampNorm: %f %f %d %d %d %f %f",
    //	     stamp->x,stamp->y,footprint,normWindow1,normWindow2,
    //	     normI1,normI2);
    stamp->norm = normI2 / normI1;
    stamp->normI1 = normI1;
    stamp->normI2 = normI2;
    stamp->normSquare1 = normSquare1;
    stamp->normSquare2 = normSquare2;

    // psLogMsg ("psModules.imcombine", PS_LOG_DETAIL, "normValue: %f %f %f  (%f %f)\n", normI1, normI2, stamp->norm, normSquare1, normSquare2);

    return true;
}

bool pmSubtractionCalculateEquationThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    pmSubtractionStampList *stamps = job->args->data[0]; // List of stamps
    pmSubtractionKernels *kernels = job->args->data[1]; // Kernels
    int index = PS_SCALAR_VALUE(job->args->data[2], S32); // Stamp index

    return pmSubtractionCalculateEquationStamp(stamps, kernels, index);
}

bool pmSubtractionCalculateEquationStamp(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels, int index)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PS_ASSERT_INT_NONNEGATIVE(index, false);
    PS_ASSERT_INT_LESS_THAN(index, stamps->num, false);

    int footprint = stamps->footprint;  // Half-size of stamps
    int spatialOrder = kernels->spatialOrder; // Maximum order of spatial variation
    int numKernels = kernels->num;      // Number of kernel basis functions
    int numSpatial = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of spatial variations

    // numKernels is the number of unique kernel images (one for each Gaussian modified by a specific polynomial).
    // = \sum_i^N_Gaussians [(order + 1) * (order + 2) / 2], eg for 1 Gauss and 1st order, numKernels = 3

    // Total number of parameters to solve for: coefficient of each kernel basis function, multipled by the
    // number of coefficients for the spatial polynomial, normalisation and a constant background offset.
    // XXX we no longer solve for the normalization and background in the matrix inversion
    int numParams = numKernels * numSpatial;
    if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
        // An additional image is convolved
        numParams += numKernels * numSpatial;
    }

    pmSubtractionStamp *stamp = stamps->stamps->data[index]; // Stamp of interest
    psAssert(stamp->status == PM_SUBTRACTION_STAMP_CALCULATE, "We only operate on stamps with this state.");

    psImage *polyValues = p_pmSubtractionPolynomial(NULL, spatialOrder, stamp->xNorm, stamp->yNorm); // Polynomial terms

    // Is this a new run? Have we allocated the correct sized vector/matrix?
    bool new = stamp->vector ? false : true;
    if (!new && (stamp->vector->n != numParams)) {
	psFree (stamp->vector);
	psFree (stamp->matrix);
	new = true;
    }

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
        status = calculateMatrixVector(stamp->matrix, stamp->vector, stamps->normValue, stamp->image2, stamp->image1,
                                       weight, window, stamp->convolutions1, kernels, polyValues, footprint);
        break;
      case PM_SUBTRACTION_MODE_2:
        status = calculateMatrixVector(stamp->matrix, stamp->vector, stamps->normValue, stamp->image1, stamp->image2,
                                       weight, window, stamp->convolutions2, kernels, polyValues, footprint);
        break;
      case PM_SUBTRACTION_MODE_DUAL:
        status = calculateDualMatrixVector(stamp, stamps->normValue, stamps->normValue2, weight, window, kernels, polyValues, footprint);
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

bool pmSubtractionCalculateEquation(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels)
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
            if (!psThreadJobAddPending(job)) {
                return false;
            }
        } else {
            pmSubtractionCalculateEquationStamp(stamps, kernels, i);
        }
    }

    if (!psThreadPoolWait(true, true)) {
        psError(psErrorCodeLast(), false, "Error waiting for threads.");
        return false;
    }

    pmSubtractionVisualShowKernels((pmSubtractionKernels  *)kernels);
    pmSubtractionVisualShowBasis(stamps);
    pmSubtractionVisualPlotLeastSquares(stamps);

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Calculate equation: %f sec", psTimerClear("pmSubtractionCalculateEquation"));

    return true;
}

// private functions used on pmSubtractionSolveEquation
bool psVectorWriteFile (char *filename, const psVector *vector);
bool psFitsWriteImageSimple (char *filename, psImage *image, psMetadata *header);

bool pmSubtractionSolveEquation(pmSubtractionKernels *kernels, const pmSubtractionStampList *stamps)
{
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, false);
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);

    psTimerStart("pmSubtractionSolveEquation");

    // Check inputs
    int numKernels = kernels->num;      // Number of kernel basis functions
    int numSpatial = PM_SUBTRACTION_POLYTERMS(kernels->spatialOrder); // Number of spatial variations
    // XXX int numBackground = PM_SUBTRACTION_POLYTERMS(kernels->bgOrder); // Number of background terms
    // XXX int numParams = numKernels * numSpatial + 1 + numBackground;    // Number of parameters being solved for
    int numParams = numKernels * numSpatial;	    // Number of parameters being solved for
    int numSolution1 = numParams, numSolution2 = 0; // Number of parameters for each solution
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

	if (stamp->vector->n != numParams) {
	    fprintf (stderr, "mismatch length\n");
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
        // Accumulate the least-squares matrices and vectors.  These are generated for the
	// kernel elements, excluding the background and normalization.
        psImage *sumMatrix = psImageAlloc(numParams, numParams, PS_TYPE_F64); // Combined matrix
        psVector *sumVector = psVectorAlloc(numParams, PS_TYPE_F64); // Combined vector
        psVectorInit(sumVector, 0.0);
        psImageInit(sumMatrix, 0.0);

        int numStamps = 0;              // Number of good stamps
        for (int i = 0; i < stamps->num; i++) {
            pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
            if (stamp->status == PM_SUBTRACTION_STAMP_USED) {
		
                (void)psBinaryOp(sumMatrix, sumMatrix, "+", stamp->matrix);
                (void)psBinaryOp(sumVector, sumVector, "+", stamp->vector);

                pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "green");
                numStamps++;
            } else if (stamp->status == PM_SUBTRACTION_STAMP_REJECTED) {
                pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "red");
            }
        }

	pmSubtractionVisualPlotLeastSquaresResid(stamps, sumMatrix, numStamps);

#if 0
	psImage *save = psImageCopy(NULL, sumMatrix, PS_TYPE_F32);
        psFitsWriteImageSimple ("sumMatrix.fits", save, NULL);
        psVectorWriteFile("sumVector.dat", sumVector);
	psFree (save);
#endif

	// XXX TEST : print the matrix & vector
	if (0) {
	    for (int iy = 0; iy < sumMatrix->numRows; iy++) {
		for (int ix = 0; ix < sumMatrix->numCols; ix++) {
		    fprintf (stderr, "%e  ", sumMatrix->data.F64[iy][ix]);
		}
		fprintf (stderr, " : %e\n", sumVector->data.F64[iy]);
	    }
	}

	psImage *invMatrix = NULL;
        psVector *solution = NULL;                       // Solution to equation!
        solution = psVectorAlloc(numParams, PS_TYPE_F64);
        psVectorInit(solution, 0);

	// XXX TEST: try some constraint on the svd solution
	// solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, NAN);
	// SINGLE solution
# if (1)
	solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, 1e-10);
	invMatrix = psMatrixInvert(NULL, sumMatrix, NULL);
# endif
# if (0)
	psMatrixLUSolve(sumMatrixLU, sumVector);
	solution = psMemIncrRefCounter(sumVector);
	invMatrix = psMemIncrRefCounter(sumMatrix);
# endif
# if (0)
	psMatrixGJSolve(sumMatrix, sumVector);
	invMatrix = psMemIncrRefCounter(sumMatrix);
	solution = psMemIncrRefCounter(sumVector);
# endif

# if (0)
        for (int i = 0; i < solution->n; i++) {
	    psLogMsg("psModules.imcombine", PS_LOG_DETAIL, "Single solution %d: %lf +/- %lf\n", i, solution->data.F64[i], sqrt(fabs(invMatrix->data.F64[i][i])));
        }
# endif

	// ensure we have a solution vector of the right size
	kernels->solution1    = psVectorRecycle(kernels->solution1,    sumVector->n + 2, PS_TYPE_F64); // 1 for norm, 1 for bg
	kernels->solution1err = psVectorRecycle(kernels->solution1err, sumVector->n + 2, PS_TYPE_F64); // 1 for norm, 1 for bg
	psVectorInit(kernels->solution1, 0.0);
	psVectorInit(kernels->solution1err, 0.0);

	int numKernels = kernels->num;
	int spatialOrder = kernels->spatialOrder;       // Order of spatial variation
	int numPoly = PM_SUBTRACTION_POLYTERMS(spatialOrder); // Number of polynomial terms

	for (int i = 0; i < numKernels * numPoly; i++) {
	    kernels->solution1->data.F64[i] = solution->data.F64[i];
	    kernels->solution1err->data.F64[i] = sqrt(invMatrix->data.F64[i][i]);
	}

	// Apply the normalisation and background separately
	int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
	int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background
	kernels->solution1->data.F64[normIndex] = stamps->normValue;
	kernels->solution1->data.F64[bgIndex] = 0.0;

        psFree(solution);
        psFree(sumVector);
        psFree(sumMatrix);
        psFree(invMatrix);

    } else {
        // Dual convolution solution
        // Accumulate the least-squares matrices and vectors.  These are generated for the
	// kernel elements, excluding the background and normalization.
        psImage *sumMatrix = psImageAlloc(numParams, numParams, PS_TYPE_F64);
        psVector *sumVector = psVectorAlloc(numParams, PS_TYPE_F64);
        psImageInit(sumMatrix, 0.0);
        psVectorInit(sumVector, 0.0);

        int numStamps = 0;	   // Number of good stamps
	double normSquare1 = 0.0; // Sum of (I_1)^2 over stamps
	double normSquare2 = 0.0; // Sum of (I_2)^2 over stamps
        for (int i = 0; i < stamps->num; i++) {
            pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
            if (stamp->status == PM_SUBTRACTION_STAMP_USED) {
                (void)psBinaryOp(sumMatrix, sumMatrix, "+", stamp->matrix);
                (void)psBinaryOp(sumVector, sumVector, "+", stamp->vector);

		normSquare1 += stamp->normSquare1;
		normSquare2 += stamp->normSquare2;

                pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "green");
                numStamps++;
            } else if (stamp->status == PM_SUBTRACTION_STAMP_REJECTED) {
                pmSubtractionStampPrint(ds9, stamp->x, stamp->y, stamps->footprint, "red");
            }
        }

	pmSubtractionVisualPlotLeastSquaresResid(stamps, sumMatrix, numStamps);

#if 0
	psImage *save = psImageCopy(NULL, sumMatrix, PS_TYPE_F32);
        psFitsWriteImageSimple ("sumMatrix.fits", save, NULL);
        psVectorWriteFile("sumVector.dat", sumVector);
	psFree (save);
#endif

	if (PENALTY) {
	    calculatePenalty(sumMatrix, sumVector, kernels, normSquare1, normSquare2);
	}

	// XXX TEST : print the matrix & vector
	if (0) {
	    for (int iy = 0; iy < sumMatrix->numRows; iy++) {
		for (int ix = 0; ix < sumMatrix->numCols; ix++) {
		    fprintf (stderr, "%e  ", sumMatrix->data.F64[iy][ix]);
		}
		fprintf (stderr, " : %e\n", sumVector->data.F64[iy]);
	    }
	}

	psImage *invMatrix = NULL;
        psVector *solution = NULL;                       // Solution to equation!
        solution = psVectorAlloc(numParams, PS_TYPE_F64);
        psVectorInit(solution, 0);

	// DUAL solution
# if (1)
	solution = psMatrixSolveSVD(solution, sumMatrix, sumVector, 1e-10);
	invMatrix = psMatrixInvert(NULL, sumMatrix, NULL);
# endif
# if (0)
	psMatrixLUSolve(sumMatrix, sumVector);
	solution = psMemIncrRefCounter(sumVector);
	invMatrix = psMemIncrRefCounter(sumMatrix);
# endif

#if (0)
        for (int i = 0; i < solution->n; i++) {
            fprintf(stderr, "Dual solution %d: %lf +/- %lf\n", i, solution->data.F64[i], sqrt(invMatrix->data.F64[i][i]));
        }
#endif

	// XXX TEST: manually set the coeffs to a desired solution
	// solution->data.F64[0] = +1.826;
	// solution->data.F64[1] = -0.115;
	// solution->data.F64[2] =  0.0;
	// solution->data.F64[3] =  0.0;

	// ensure we have solution vectors of the right size
	kernels->solution1    = psVectorRecycle(kernels->solution1,    numSolution1 + 2, PS_TYPE_F64); // 1 for norm, 1 for bg
	kernels->solution1err = psVectorRecycle(kernels->solution1err, numSolution1 + 2, PS_TYPE_F64); // 1 for norm, 1 for bg
	kernels->solution2    = psVectorRecycle(kernels->solution2,    numSolution2, 	 PS_TYPE_F64); // 1 for norm, 1 for bg
	kernels->solution2err = psVectorRecycle(kernels->solution2err, numSolution2, 	 PS_TYPE_F64); // 1 for norm, 1 for bg

	psVectorInit(kernels->solution1, 0.0);
	psVectorInit(kernels->solution1err, 0.0);
	psVectorInit(kernels->solution2, 0.0);
	psVectorInit(kernels->solution2err, 0.0);

	// for DUAL convolution analysis, we apply the normalization to I1 as follows:
	// I1c = norm I1 + \Sum_i a_i norm I1 \cross k_i
	// I2c =      I2 + \Sum_i b_i      I2 \cross k_i

	// We absorb the normalization into a_i after the analysis is complete to be consistent
	// with the SINGLE definitions of the convolutions

	int numKernels = kernels->num;
	for (int i = 0; i < numKernels * numSpatial; i++) {
	    // we solve for coefficients 
	    kernels->solution1->data.F64[i] = solution->data.F64[i] * stamps->normValue;
	    kernels->solution2->data.F64[i] = solution->data.F64[i + numSolution1];

	    kernels->solution1err->data.F64[i] = sqrt(invMatrix->data.F64[i][i]) * stamps->normValue;
	    int i2 = i + numSolution1;
	    kernels->solution2err->data.F64[i] = sqrt(invMatrix->data.F64[i2][i2]);
	}

	// Apply the normalisation and background separately
	int normIndex = PM_SUBTRACTION_INDEX_NORM(kernels); // Index for normalisation
	int bgIndex = PM_SUBTRACTION_INDEX_BG(kernels); // Index for background
	kernels->solution1->data.F64[normIndex] = stamps->normValue;
	kernels->solution1->data.F64[bgIndex] = 0.0;

        psFree(solution);
        psFree(sumVector);
        psFree(sumMatrix);
        psFree(invMatrix);
    }

    if (ds9) {
        fclose(ds9);
    }

    if (psTraceGetLevel("psModules.imcombine") >= 7) {
        for (int i = 0; i < kernels->solution1->n; i++) {
            psTrace("psModules.imcombine", 7, "Solution 1 %d: %f +/- %f\n", i, kernels->solution1->data.F64[i], kernels->solution1err->data.F64[i]);
        }
        if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
            for (int i = 0; i < kernels->solution2->n; i++) {
                psTrace("psModules.imcombine", 7, "Solution 2 %d: %f +/- %f\n", i, kernels->solution2->data.F64[i], kernels->solution2err->data.F64[i]);
            }
        }
     }

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Solve equation: %f sec", psTimerClear("pmSubtractionSolveEquation"));

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

    // fprintf (stderr, "sum: %f, peak: %f, sigma: %f, fsigma: %f, fmax: %f, fmin: %f\n", sum, peak, sigma, sigma/sum, dOuter/sum, dTotal/sum);
    psVectorAppend(fResSigma, sigma/sum);
    psVectorAppend(fResOuter, dOuter/sum);
    psVectorAppend(fResTotal, dTotal/sum);
    return true;
}

// given the convolved image(s) and the residual image, calculate the second moment(s) and the chisq
bool pmSubtractionChisqStats(psVector *fluxesVector, psVector *chisqDVector, psVector *chisqRVector, psVector *momentVector, psVector *stampMask, psKernel *convolved1, psKernel *convolved2, psKernel *difference, psKernel *residual, psKernel *weight, psKernel *window) {

# ifndef USE_WEIGHT
    psAssert(weight == NULL, "impossible!");
# endif
# ifndef USE_WINDOW
    psAssert(window == NULL, "impossible!");
# endif

    int npix = 0;
    float chisqR = 0;
    float chisqD = 0;

    // get the chisq
    for (int y = residual->yMin; y <= residual->yMax; y++) {
        for (int x = residual->xMin; x <= residual->xMax; x++) {
            float valueR = PS_SQR(residual->kernel[y][x]);
	    if (weight) {
	     	valueR *= weight->kernel[y][x];
	    }
	    // XXX NOTE: do NOT apply the window to the chisq portions of the calculation (that would bias the chisq)
            chisqR += valueR;

            float valueD = PS_SQR(difference->kernel[y][x]);
	    if (weight) {
	     	valueD *= weight->kernel[y][x];
	    }
            chisqD += valueD;
	    npix ++;
        }
    }
    psVectorAppend(chisqRVector, chisqR / npix);
    psVectorAppend(chisqDVector, chisqD / npix);

    float value1 = 0;
    float value2 = 0;
    float flux2 = 0;
    float fluxX = 0;
    float fluxY = 0;
    float fluxX2 = 0;
    float fluxY2 = 0;

    float fluxC1 = 0;
    float fluxC2 = 0;

    float moment = 0;

    // get the moments from convolved1
    if (convolved1) {
	for (int y = residual->yMin; y <= residual->yMax; y++) {
	    for (int x = residual->xMin; x <= residual->xMax; x++) {
		value1  = convolved1->kernel[y][x];
		value2  = PS_SQR(value1);

		if (window) {
		    value1 *= window->kernel[y][x];
		    value2 *= window->kernel[y][x];
		}

		fluxC1 += value1;
		flux2  += value2;
		fluxX  += x * value2;
		fluxY  += y * value2;
		// fluxX2 += PS_SQR(x) * value2;
		// fluxY2 += PS_SQR(y) * value2;
		fluxX2 += PS_SQR(x) * value1;
		fluxY2 += PS_SQR(y) * value1;
	    }
	}
	// float Mx = fluxX / flux2;
	// float My = fluxY / flux2;
	// float Mxx = fluxX2 / flux2;
	// float Myy = fluxY2 / flux2;
	float Mxx = fluxX2 / fluxC1;
	float Myy = fluxY2 / fluxC1;

	// fprintf (stderr, "conv1, flux2: %f, Mx: %f, My: %f, Mxx: %f, Myy: %f, chisq: %f, npix: %d\n", flux2, Mx, My, Mxx, Myy, chisq, npix);
	moment += Mxx + Myy;
    }

    // get the moments from convolved1
    if (convolved2) {
	for (int y = residual->yMin; y <= residual->yMax; y++) {
	    for (int x = residual->xMin; x <= residual->xMax; x++) {
		value1  = convolved2->kernel[y][x];
		value2  = PS_SQR(value1);

		// XXX NOTE: do NOT apply the weight to the moments calculation
		if (false && weight) {
		    value2 *= weight->kernel[y][x];
		}
		if (window) {
		    value1 *= window->kernel[y][x];
		    value2 *= window->kernel[y][x];
		}

		fluxC2 += value1;
		flux2  += value2;
		fluxX  += x * value2;
		fluxY  += y * value2;
		// fluxX2 += PS_SQR(x) * value2;
		// fluxY2 += PS_SQR(y) * value2;
		fluxX2 += PS_SQR(x) * value1;
		fluxY2 += PS_SQR(y) * value1;
	    }
	}
	// float Mx = fluxX / flux2;
	// float My = fluxY / flux2;
	// float Mxx = fluxX2 / flux2;
	// float Myy = fluxY2 / flux2;
	float Mxx = fluxX2 / fluxC2;
	float Myy = fluxY2 / fluxC2;

	// fprintf (stderr, "conv2, flux2: %f, Mx: %f, My: %f, Mxx: %f, Myy: %f, chisq: %f, npix: %d\n", flux2, Mx, My, Mxx, Myy, chisq, npix);
	moment += Mxx + Myy;
    }

    float flux = fluxC1 + fluxC2;
    
    if (convolved1 && convolved2) {
	moment *= 0.5;
	flux *= 0.5;
    }
    psVectorAppend(momentVector, moment);
    psVectorAppend(fluxesVector, flux);

    // check that the last appended values are ok:
    int Nelem = fluxesVector->n - 1;
    bool valid = true;
    valid &= isfinite(chisqRVector->data.F32[Nelem]);
    valid &= isfinite(fluxesVector->data.F32[Nelem]);
    valid &= isfinite(momentVector->data.F32[Nelem]);
    if (valid) {
      psVectorAppend(stampMask, 0);
    } else {
      psVectorAppend(stampMask, 0x02);
    }
    return true;
}

bool pmSubtractionCalculateChisqAndMoments(pmSubtractionQuality **bestMatch, 
					   pmSubtractionStampList *stamps,
					   pmSubtractionKernels *kernels)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NULL);

    psTimerStart("pmSubtractionCalculateChisqAndMoments");

    // XXX need to save these somewhere
    psVector *fluxes = psVectorAllocEmpty(stamps->num, PS_TYPE_F32);
    psVector *chisqD = psVectorAllocEmpty(stamps->num, PS_TYPE_F32);
    psVector *chisqR = psVectorAllocEmpty(stamps->num, PS_TYPE_F32);
    psVector *moments = psVectorAllocEmpty(stamps->num, PS_TYPE_F32);
    psVector *stampMask = psVectorAllocEmpty(stamps->num, PS_TYPE_VECTOR_MASK);

    int footprint = stamps->footprint; // Half-size of stamps
    int numKernels = kernels->num;      // Number of kernels

    psImage *polyValues = NULL;         // Polynomial values

    // storage for the image (convolved2 is not used in SINGLE mode)
    psKernel *residual = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Residual image
    psKernel *difference = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Residual image
    psKernel *convolved1 = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Residual image
    psKernel *convolved2 = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Residual image

    int nGood = 0;
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // The stamp of interest
        if (stamp->status != PM_SUBTRACTION_STAMP_USED) {
	    // mark this stamp as unused (note that we have to append NANs to the other vectors to keep the lengths in sync)
	    psVectorAppend(moments, NAN);
	    psVectorAppend(fluxes, NAN);
	    psVectorAppend(chisqD, NAN);
	    psVectorAppend(chisqR, NAN);
	    psVectorAppend(stampMask, 0x01);
            continue;
        }
	nGood ++;

        // Calculate coefficients of the kernel basis functions
        polyValues = p_pmSubtractionPolynomial(polyValues, kernels->spatialOrder, stamp->xNorm, stamp->yNorm);
        double norm = p_pmSubtractionSolutionNorm(kernels); // Normalisation
        double background = p_pmSubtractionSolutionBackground(kernels, polyValues);// Difference in background

        // Calculate residuals
        psImageInit(residual->image, 0.0);
        psImageInit(difference->image, 0.0);

	psKernel *weight = NULL;
	psKernel *window = NULL;
    
#ifdef USE_WEIGHT
    weight = stamp->weight;
#endif
#ifdef USE_WINDOW
    window = stamps->window;
#endif

        if (kernels->mode != PM_SUBTRACTION_MODE_DUAL) {

	    // the single-direction psf match code attempts to find the kernel such that:
	    // source * kernel = target.  we need to assign 'source' and 'target' correctly
	    // depending on which of image1 or image2 we asked to be convolved.

            psKernel *target;           // Target postage stamp (convolve source to match the target)
            psKernel *source;           // Source postage stamp (convolve source to match the target)
            psArray *convolutions;      // Convolution postage stamps for each kernel basis function

	    // init the accumulation image
	    psImageInit(convolved1->image, 0.0);

            switch (kernels->mode) {
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
                psAbort("Unsupported subtraction mode: %x", kernels->mode);
            }

	    // generate the convolved source image (sum over kernels)
            for (int j = 0; j < numKernels; j++) {
                psKernel *convolution = convolutions->data[j]; // Convolution
                double coefficient = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, false); // Coefficient
                for (int y = - footprint; y <= footprint; y++) {
                    for (int x = - footprint; x <= footprint; x++) {
                        convolved1->kernel[y][x] += convolution->kernel[y][x] * coefficient;
                    }
                }
            }

	    // Generate the difference, residual, and convolved source images.  Note the we
	    // accumulate the convolution of (A-B), so we need to replace it to generate the
	    // images of the convolved source image.
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    difference->kernel[y][x] = target->kernel[y][x] - source->kernel[y][x] * norm - background;
                    residual->kernel[y][x] = difference->kernel[y][x] - convolved1->kernel[y][x];
		    convolved1->kernel[y][x] += source->kernel[y][x] * norm;
                }
            }

	    // XXX if we want to have a weight and window, we'll need to pass through to here
            pmSubtractionChisqStats(fluxes, chisqD, chisqR, moments, stampMask, convolved1, NULL, difference, residual, weight, window);

        } else {

            // Dual convolution
            psArray *convolutions1 = stamp->convolutions1; // Convolutions of the first image
            psArray *convolutions2 = stamp->convolutions2; // Convolutions of the second image
            psKernel *image1 = stamp->image1; // The first image
            psKernel *image2 = stamp->image2; // The second image

	    // init the accumulation images
	    psImageInit(convolved1->image, 0.0);
	    psImageInit(convolved2->image, 0.0);

            for (int j = 0; j < numKernels; j++) {
                psKernel *conv1 = convolutions1->data[j]; // Convolution of first image
                psKernel *conv2 = convolutions2->data[j]; // Convolution of second image
                double coeff1 = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, false); // Coefficient 1
                double coeff2 = p_pmSubtractionSolutionCoeff(kernels, polyValues, j, true); // Coefficient 2

                for (int y = - footprint; y <= footprint; y++) {
                    for (int x = - footprint; x <= footprint; x++) {
			// NOTE sign for coeff2
                        convolved1->kernel[y][x] += +conv1->kernel[y][x] * coeff1;
                        convolved2->kernel[y][x] += -conv2->kernel[y][x] * coeff2;
                    }
                }
            }

	    // Generate the difference, residual, and convolved source images.  Note the we
	    // accumulate the convolutions of (A-B), so we need to replace (A or B) to generate
	    // the images of the convolved source images.
            for (int y = - footprint; y <= footprint; y++) {
                for (int x = - footprint; x <= footprint; x++) {
                    difference->kernel[y][x] = image2->kernel[y][x] - image1->kernel[y][x] * norm - background;
                    residual->kernel[y][x] = difference->kernel[y][x] + convolved2->kernel[y][x] - convolved1->kernel[y][x];
		    convolved1->kernel[y][x] += image1->kernel[y][x] * norm;
		    convolved2->kernel[y][x] += image2->kernel[y][x];
                }
            }

	    if (0) {
		psFitsWriteImageSimple("conv1.fits", convolved1->image, NULL);
		psFitsWriteImageSimple("conv2.fits", convolved2->image, NULL);
		psFitsWriteImageSimple("resid.fits", residual->image,   NULL);
		pmVisualAskUser(NULL);
	    } 

            pmSubtractionChisqStats(fluxes, chisqD, chisqR, moments, stampMask, convolved1, convolved2, difference, residual, weight, window);
        }
    }

    // find the mean chisq and mean moment
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    psVectorStats (stats, chisqD, NULL, stampMask, 0xff);
    float chisqDValue = stats->sampleMean;

    psStatsInit(stats);
    psVectorStats (stats, chisqR, NULL, stampMask, 0xff);
    float chisqRValue = stats->sampleMean;

    psStatsInit(stats);
    psVectorStats (stats, moments, NULL, stampMask, 0xff);
    float momentValue = stats->sampleMean;

    double sumKernel1 = 0.0, sumKernel2 = 0.0; // Sum of the kernel

    // calculate the variance contribution from this smoothing kernel
    psKernel *modelKernel = pmSubtractionKernel(kernels, 0.0, 0.0, false);
    for (int y = modelKernel->yMin; y <= modelKernel->yMax; y++) {
        for (int x = modelKernel->xMin; x <= modelKernel->xMax; x++) {
            if (!isfinite(modelKernel->kernel[y][x])) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Non-finite covariance matrix element in kernel at %d,%d", x, y);
                return NULL;
            }
            sumKernel1 += PS_SQR(modelKernel->kernel[y][x]);
        }
    }
    psFree (modelKernel);

    if (kernels->mode == PM_SUBTRACTION_MODE_DUAL) {
	psKernel *modelKernel = pmSubtractionKernel(kernels, 0.0, 0.0, true);
	for (int y = modelKernel->yMin; y <= modelKernel->yMax; y++) {
	    for (int x = modelKernel->xMin; x <= modelKernel->xMax; x++) {
		if (!isfinite(modelKernel->kernel[y][x])) {
		    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Non-finite covariance matrix element in kernel at %d,%d", x, y);
		    return NULL;
		}
		sumKernel2 += PS_SQR(modelKernel->kernel[y][x]);
	    }
	}
	psFree (modelKernel);
    } else {
	sumKernel2 = 1.0;
    }

    // if we modify the chisq value by the (sumKernel1 + sumKernel2), we account for the
    // smoothing coming from larger kernels adding additional spatial fit terms should be
    // penalized by increasing the score somewhat.  the 0.01 value is not well-chosen.
    float orderFactor = 0.01 * kernels->spatialOrder;
    float score = 2.0 * chisqRValue / (sumKernel1 + sumKernel2) + orderFactor;
    psLogMsg("psModules.imcombine", PS_LOG_INFO, "chisq: %6.3f, chisqD: %6.3f, moment: %6.3f, sumKernel_1: %6.3f, sumKernel_2, score: %6.3f: %6.3f\n", chisqRValue, chisqDValue, momentValue, sumKernel1, sumKernel2, score);

    // save this result if it is the first or the best (skip if bestMatch is NULL)
    if (bestMatch) {
	pmSubtractionQuality *match = *bestMatch;
	bool keep = false;
	if (match == NULL) {
	    *bestMatch = match = pmSubtractionQualityAlloc();
	    keep = true;
	} else {
	    if (score < match->score) {
		psFree(match->fluxes);
		psFree(match->chisq);
		psFree(match->moments);
		psFree(match->stampMask);
		keep = true;
	    }
	}
	if (keep) {
	    psLogMsg("psModules.imcombine", PS_LOG_INFO, "keeping order: %d, mode: %d, score: %f\n", kernels->spatialOrder, kernels->mode, score);
	    match->score        = score;
	    match->spatialOrder = kernels->spatialOrder;
	    match->mode         = kernels->mode;
	    match->nGood        = nGood;
	    match->fluxes       = psMemIncrRefCounter(fluxes);
	    match->chisq        = psMemIncrRefCounter(chisqR);
	    match->moments      = psMemIncrRefCounter(moments);
	    match->stampMask    = psMemIncrRefCounter(stampMask);
	}	    
    }

    pmSubtractionVisualPlotChisqAndMoments(fluxes, chisqR, moments);

    psFree(stats);
    psFree(chisqR);
    psFree(chisqD);
    psFree(fluxes);
    psFree(moments);
    psFree(stampMask);

    psFree(residual);
    psFree(difference);
    psFree(convolved1);
    psFree(convolved2);
    psFree(polyValues);

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Calculate Chisq and Moments: %f sec", psTimerClear("pmSubtractionCalculateChisqAndMoments"));

    return true;
}

// XXX for now, let's not use this, and let's instead just use values from pmSubtractionCalculateChisqAndMoments
psVector *pmSubtractionCalculateDeviations(pmSubtractionStampList *stamps, pmSubtractionKernels *kernels)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernels, NULL);
    PM_ASSERT_SUBTRACTION_KERNELS_SOLUTION(kernels, NULL);

    psTimerStart("pmSubtractionCalculateDeviations");

    psVector *deviations = psVectorAlloc(stamps->num, PS_TYPE_F32); // Mean deviation for stamps
    int footprint = stamps->footprint; // Half-size of stamps
    long numPixels = PS_SQR(2 * footprint + 1); // Number of pixels in footprint
    double devNorm = 1.0 / (double)numPixels; // Normalisation for deviations
    int numKernels = kernels->num;      // Number of kernels

    psImage *polyValues = NULL;         // Polynomial values
    psKernel *residual = psKernelAlloc(-footprint, footprint, -footprint, footprint); // Residual image

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

	double flux = 0.0;
        double deviation = 0.0;         // Sum of differences
        for (int y = - footprint; y <= footprint; y++) {
            for (int x = - footprint; x <= footprint; x++) {
                double dev = PS_SQR(residual->kernel[y][x]) * weight->kernel[y][x];
                deviation += dev;
		flux += stamp->image1->kernel[y][x] + stamp->image2->kernel[y][x];
            }
        }
        deviations->data.F32[i] = devNorm * deviation;
        psTrace("psModules.imcombine", 5, "Deviation and Flux for stamp %d (%d,%d): %f %f\n",
                i, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5), deviations->data.F32[i], flux);
        psStringAppend(&log, "Stamp %d (%d,%d): %f\n",
                       i, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5), deviations->data.F32[i]);
        if (!isfinite(deviations->data.F32[i])) {
            stamp->status = PM_SUBTRACTION_STAMP_REJECTED;
            psTrace("psModules.imcombine", 5, "Rejecting stamp %d (%d,%d) because of non-finite deviation\n", i, (int)(stamp->x - 0.5), (int)(stamp->y - 0.5));
            continue;
        }
    }

    psFree(keepStamps);

    psLogMsg("psModules.imcombine", PS_LOG_MINUTIA, "%s", log);
    psFree(log);

    // calculate and report the normalization and background for the image center
    {
        polyValues = p_pmSubtractionPolynomial(polyValues, kernels->spatialOrder, 0.0, 0.0);
        double norm = p_pmSubtractionSolutionNorm(kernels); // Normalisation
        double background = p_pmSubtractionSolutionBackground(kernels, polyValues);// Difference in background
        psLogMsg("psModules.imcombine", PS_LOG_INFO, "normalization: %f, background: %f", norm, background);

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

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Calculate Deviations: %f sec", psTimerClear("pmSubtractionCalculateDeviations"));

    return deviations;
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
