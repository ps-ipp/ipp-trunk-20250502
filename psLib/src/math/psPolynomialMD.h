#ifndef PS_POLYNOMIAL_MD_H
#define PS_POLYNOMIAL_MD_H

#include <psVector.h>
#include <psPolynomial.h>
#include <psArray.h>

/// Multi-dimensional polynomial
typedef struct {
    int dim;                            ///< Dimensions
    psVector *orders;                   ///< Polynomial orders for each dimension
    psVector *coeff;                    ///< Coefficients
    int numFit;                         ///< Number of values in fit
    float stdevFit;                     ///< Standard deviation of fit

    psImage  *fitMatrix;
    psVector *fitVector;

    psImage  *tmpMatrix;
    psVector *tmpVector;

    psVector *fitBuffer;

    psVector *ownMask;
    psVector *deviations;

    psVector *LUperm;
    psImage  *LU;

} psPolynomialMD;

/// Assertion for valid polynomial
#define PS_ASSERT_POLYNOMIALMD_NON_NULL(POLY, RETURN) \
    if (!(POLY) || (POLY)->dim < 0 || !(POLY)->orders || (POLY)->orders->n != (POLY)->dim || \
        !(POLY)->coeff) { \
        psError(PS_ERR_UNEXPECTED_NULL, true, "Invalid polynomial."); \
        return RETURN; \
    }

/// Constructor
psPolynomialMD *psPolynomialMDAlloc(const psVector *orders ///< Orders for each dimension
    ) PS_ATTR_MALLOC;

/// Evaluate a polynomial
double psPolynomialMDEval(const psPolynomialMD *poly, ///< Polynomial
                          const psVector *coords ///< Coordinates
    );

/// Fit a polynomial
bool psPolynomialMDFit(psPolynomialMD *poly, ///< Polynomial to fit
                       const psVector *values, ///< Values
                       const psVector *errors, ///< Errors
                       const psVector *mask, ///< Mask
                       psVectorMaskType maskVal, ///< Value to mask
                       const psArray *coordsArray ///< Array of coordinates
    );

/// Fit a polynomial, with clipping
bool psPolynomialMDClipFit(psPolynomialMD *poly, ///< Polynomial to fit
                           const psVector *values, ///< Values
                           const psVector *errors, ///< Errors
                           const psVector *mask, ///< Mask
                           psVectorMaskType maskVal, ///< Value to mask
                           const psArray *coordsArray, ///< Array of coordinates
                           int numIter,    ///< Number of rejection iterations
                           float rej    ///< Rejection limit, standard deviations
    );

#endif
