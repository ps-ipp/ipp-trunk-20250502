/* @file  psPolynomial.h
 * @brief Standard Mathematical Functions.
 *
 * This file will hold the prototypes for procedures which allocate, free,
 * and evaluate various polynomials.  Those polynomial structures are also
 * defined here.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.69 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-10-09 19:24:46 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_POLYNOMIAL_H
#define PS_POLYNOMIAL_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include <stdio.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>

#include "psVector.h"
#include "psScalar.h"

typedef enum {
    PS_POLY_MASK_NONE = 0,
    PS_POLY_MASK_SET  = 1, // the coefficient should not be used (implies MASK_FIT)
    PS_POLY_MASK_FIT  = 2, // the coefficient should not be fitted 
    PS_POLY_MASK_BOTH = 3, // the coefficient should not be fitted 
} psPolynomialMaskValues;

/** Evaluate a non-normalized Gaussian with the given mean and sigma at the
 *  given coordianate.
 *
 *  Note that this is not a Gaussian deviate.  The evaluated Gaussian is:
 *        \f[ exp(-\frac{(x-mean)^2}{2\sigma^2}) \f]
 *
 *  @return float      value on the gaussian curve given the input parameters
 */
float psGaussian(
    float x,                           ///< Value at which to evaluate
    float mean,                        ///< Mean for the Gaussian
    float sigma,                       ///< Standard deviation for the Gaussian
    bool normal                        ///< Indicates whether result should be normalized
);

/** Polynomial Type.
 *
 *  Enumeration for Polynomial types.
 */
typedef enum {
    PS_POLYNOMIAL_ORD,                  ///< Ordinary Polynomial
    PS_POLYNOMIAL_CHEB                  ///< Chebyshev Polynomial
}
psPolynomialType;

/** One-dimensional polynomial */
typedef struct
{
    psPolynomialType type;              ///< Polynomial type
    unsigned int nX;                    ///< Polynomial order
    psF64 *coeff;                       ///< Coefficients
    psF64 *coeffErr;                    ///< Error in coefficients
    psMaskType *coeffMask;		///< Coefficient mask
    double scale[1];			///< Chebyshev scale factor
    double zero[1];			///< Chebyshev zero point
}
psPolynomial1D;

/** Two-dimensional polynomial */
typedef struct
{
    psPolynomialType type;              ///< Polynomial type
    unsigned int nX;            ///< Polynomial order in x
    unsigned int nY;            ///< Polynomial order in y
    psF64 **coeff;                      ///< Coefficients
    psF64 **coeffErr;                   ///< Error in coefficients
    psMaskType **coeffMask;                  ///< Coefficients mask
    double scale[2];			///< Chebyshev scale factor
    double zero[2];			///< Chebyshev zero point
}
psPolynomial2D;

/** Three-dimensional polynomial */
typedef struct
{
    psPolynomialType type;              ///< Polynomial type
    unsigned int nX;            ///< Polynomial order in x
    unsigned int nY;            ///< Polynomial order in y
    unsigned int nZ;            ///< Polynomial order in z
    psF64 ***coeff;                     ///< Coefficients
    psF64 ***coeffErr;                  ///< Error in coefficients
    psMaskType ***coeffMask;                 ///< Coefficients mask
    double scale[3];			///< Chebyshev scale factor
    double zero[3];			///< Chebyshev zero point
}
psPolynomial3D;

/** Four-dimensional polynomial */
typedef struct
{
    psPolynomialType type;              ///< Polynomial type
    unsigned int nX;			///< Polynomial order in x
    unsigned int nY;			///< Polynomial order in y
    unsigned int nZ;			///< Polynomial order in z
    unsigned int nT;			///< Polynomial order in t
    psF64 ****coeff;                    ///< Coefficients
    psF64 ****coeffErr;                 ///< Error in coefficients
    psMaskType ****coeffMask;		///< Coefficients mask
    double scale[4];			///< Chebyshev scale factor
    double zero[4];			///< Chebyshev zero point
}
psPolynomial4D;


/** Allocates a psPolynomial1D structure with n terms
 *
 *  @return  psPolynomial1D*    new 1-D polynomial struct
 */
psPolynomial1D* psPolynomial1DAlloc(
    psPolynomialType type,             ///< Polynomial Type
    unsigned int nX                    ///< Number of terms
) PS_ATTR_MALLOC;

/** Allocates a 2-D polynomial structure
 *
 *  @return  psPolynomial2D*    new 2-D polynomial struct
 */
psPolynomial2D* psPolynomial2DAlloc(
    psPolynomialType type,             ///< Polynomial Type
    unsigned int nX,                   ///< Number of terms in x
    unsigned int nY                    ///< Number of terms in y
) PS_ATTR_MALLOC;

/** Allocates a 3-D polynomial structure
 *
 *  @return  psPolynomial3D*    new 3-D polynomial struct
 */
psPolynomial3D* psPolynomial3DAlloc(
    psPolynomialType type,             ///< Polynomial Type
    unsigned int nX,                   ///< Number of terms in x
    unsigned int nY,                   ///< Number of terms in y
    unsigned int nZ                    ///< Number of terms in z
) PS_ATTR_MALLOC;

/** Allocates a 4-D polynomial structure
 *
 *  @return  psPolynomial4D*    new 4-D polynomial struct
 */
psPolynomial4D* psPolynomial4DAlloc(
    psPolynomialType type,             ///< Polynomial Type
    unsigned int nX,                   ///< Number of terms in x
    unsigned int nY,                   ///< Number of terms in y
    unsigned int nZ,                   ///< Number of terms in z
    unsigned int nT                    ///< Number of terms in t
) PS_ATTR_MALLOC;

bool psPolynomial2DRecycle(psPolynomial2D *poly,
                           psPolynomialType type,
                           unsigned int nX,
                           unsigned int nY);

psPolynomial2D *psPolynomial2DCopy(psPolynomial2D *out,
                                   psPolynomial2D *poly);

/** Evaluates a 1-D polynomial at specific coordinates.
 *
 *  @return psF64    result of polynomial at given location
 */
psF64 psPolynomial1DEval(
    const psPolynomial1D* poly,        ///< Coefficients for the polynomial
    psF64 x                            ///< location at which to evaluate
);

/** Evaluates a 2-D polynomial at specific coordinates.
 *
 *  @return psF64    result of polynomial at given location
 */
psF64 psPolynomial2DEval(
    const psPolynomial2D* poly,        ///< Coefficients for the polynomial
    psF64 x,                           ///< x location at which to evaluate
    psF64 y                            ///< y location at which to evaluate
);

/** Evaluates a 3-D polynomial at specific coordinates.
 *
 *  @return psF64    result of polynomial at given location
 */
psF64 psPolynomial3DEval(
    const psPolynomial3D* poly,        ///< Coefficients for the polynomial
    psF64 x,                           ///< x location at which to evaluate
    psF64 y,                           ///< y location at which to evaluate
    psF64 z                            ///< z location at which to evaluate
);

/** Evaluates a 4-D polynomial at specific coordinates.
 *
 *  @return psF64    result of polynomial at given location
 */
psF64 psPolynomial4DEval(
    const psPolynomial4D* poly,        ///< Coefficients for the polynomial
    psF64 x,                           ///< x location at which to evaluate
    psF64 y,                           ///< y location at which to evaluate
    psF64 z,                           ///< z location at which to evaluate
    psF64 t                            ///< t location at which to evaluate
);

/** Evaluates a 1-D polynomial at specific sets of coordinates
 *
 *  @return psVector*    results of polynomials at given locations
 */
psVector *psPolynomial1DEvalVector(
    const psPolynomial1D *poly,        ///< Coefficients for the polynomial
    const psVector *x                  ///< x locations at which to evaluate
);

/** Evaluates a 2-D polynomial at specific sets of coordinates
 *
 *  @return psVector*    results of polynomial at given locations
 */
psVector *psPolynomial2DEvalVector(
    const psPolynomial2D *poly,        ///< Coefficients for the polynomial
    const psVector *x,                 ///< x locations at which to evaluate
    const psVector *y                  ///< y locations at which to evaluate
);

/** Evaluates a 3-D polynomial at specific sets of coordinates
 *
 *  @return psVector*    results of polynomial at given locations
 */
psVector *psPolynomial3DEvalVector(
    const psPolynomial3D *poly,        ///< Coefficients for the polynomial
    const psVector *x,                 ///< x locations at which to evaluate
    const psVector *y,                 ///< y locations at which to evaluate
    const psVector *z                  ///< z locations at which to evaluate
);

/** Evaluates a 4-D polynomial at specific sets of coordinates
 *
 *  @return psVector*    results of polynomial at given locations
 */
psVector *psPolynomial4DEvalVector(
    const psPolynomial4D *poly,        ///< Coefficients for the polynomial
    const psVector *x,                 ///< x locations at which to evaluate
    const psVector *y,                 ///< y locations at which to evaluate
    const psVector *z,                 ///< z locations at which to evaluate
    const psVector *t                  ///< t locations at which to evaluate
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psPolynomial1D structure, false otherwise.
 */
bool psMemCheckPolynomial1D(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psPolynomial2D structure, false otherwise.
 */
bool psMemCheckPolynomial2D(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psPolynomial3D structure, false otherwise.
 */
bool psMemCheckPolynomial3D(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psPolynomial4D structure, false otherwise.
 */
bool psMemCheckPolynomial4D(
    psPtr ptr                          ///< the pointer whose type to check
);




/** Creates the specified number of chebyshev polys.
 *
 *  @return psPolynomial1D** The chebyshev polys.
 *
 */
psPolynomial1D **p_psCreateChebyshevPolys(
    psS32 numPolys
);

typedef struct
{
    int n;                              ///< The number of Chebyshev polys.
    psPolynomial1D **chebyPolys;        ///< THe chebyshev polys

}
p_chebyPolys;


// chebyshev support functions:
bool psChebyshevSetScale (psPolynomial2D* myPoly, const psVector *vec, int dir);
psVector *psChebyshevNormVector (const psPolynomial2D* myPoly, const psVector *vec, int dir);
psVector *psChebyshevPolyVector (const psVector *vec, int order);

/*****************************************************************************
    PS_POLY macros:
*****************************************************************************/
#define PS_ASSERT_POLY1D(NAME, RVAL) \
if (false == psMemCheckPolynomial1D(NAME)) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: argument %s is not a psPolynomial1D struct.\n",\
            #NAME); \
    return(RVAL); \
} \

#define PS_ASSERT_POLY_NON_NULL(NAME, RVAL) \
if ((NAME) == NULL || (NAME)->coeff == NULL) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: polynomial %s or its coeffs is NULL.", \
            #NAME); \
    return(RVAL); \
} \

#define PS_ASSERT_POLY_TYPE(NAME, TYPE, RVAL) \
if ((NAME)->type != TYPE) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: polynomial %s has wrong type.", #NAME); \
    return(RVAL); \
} \

#define PS_ASSERT_POLY_VALID_TYPE(TYPE, RVAL) \
if ((TYPE != PS_POLYNOMIAL_ORD) && \
        (TYPE != PS_POLYNOMIAL_CHEB)) { \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: invalid type %d for polynomial", TYPE); \
    return(RVAL); \
} \

// XXX warning: this is fragile if NAME contains an external 'i'
#define PS_POLY_PRINT_1D(NAME) \
printf("Poly %s: (nX) is (%d)\n", #NAME, NAME->nX);\
for (psS32 i = 0 ; i < NAME->nX+1 ; i++) {\
    printf("%s->coeff[%d] is %f\n", #NAME, i, NAME->coeff[i]); \
}\

// XXX warning: this is fragile if NAME contains an external 'i' or 'j'
#define PS_POLY_PRINT_2D(NAME) \
printf("Poly %s: (nX, nY) is (%d, %d)\n", #NAME, NAME->nX, NAME->nY);\
for (psS32 i = 0 ; i < NAME->nX+1 ; i++) {\
    for (psS32 j = 0 ; j < NAME->nY+1 ; j++) {\
        printf("%s->coeff[%d][%d] is %f\n", #NAME, i, j, NAME->coeff[i][j]); \
    }\
}\

/// @}
#endif // #ifndef PS_POLYNOMIAL_H
