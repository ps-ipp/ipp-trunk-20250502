/* @file  psMatrix.h
 *
 * @brief Provides functions for linear algebra operations on psImages and psVectors.
 *
 * Functions are provided to:
 *     Transpose a psImage
 *     Compute LUD
 *     Solve LUD
 *     Matrix inversion
 *     Calculate determinant
 *     Matrix multiplication
 *     Calculate Eigenvectors
 *     Convert matrix to vector
 *     Convert vector to matrix
 *
 * These functions treat psImages as if they were matrices, therefore there is no psMatrix. These functions
 * operate only with the psF64 data type.
 *
 * @author Ross Harman, MHPCC
 *
 * @version $Revision: 1.28 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-12-14 00:41:17 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PSMATRIX_H
#define PSMATRIX_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/** LU Decomposition of psImage matrix.
 *
 *  Performs a LU decomposition on a psImage matrix and returns the LU matrix. If the user specifies NULL for
 *  the outImage or outPerm arguments, then they will be automatically created. The input image must
 *  be square. This function operates only with the psF64 data type. Input and output arguments should not be
 *  the same. GSL indexes the top row as the zero row, not the bottom.
 *
 *  @return  psImage* : Pointer to LU decomposed psImage.
 */
psImage *psMatrixLUDecomposition(
    psImage* out,                      ///< Image to return, or NULL.
    psVector** perm,                   ///< Output permutation vector used by psMatrixLUSolve.
    const psImage* in                  ///< Image to decompose.
);

/** LU Solution of psImage matrix.
 *
 *  Solves for and returns the psVector, {x} in the equation [A]{x} = {b}. If the user specifies NULL as the
 *  outVector argument, then it will automatically be created. The input image must be square. This function
 *  operates only with the psF64 data type. Input and output arguments should not be the same. GSL indexes
 *  the top row as the zero row, not the bottom.
 *
 *  @return  psVector* : Pointer to psVector solution of matrix equation.
 */
psVector *psMatrixLUSolution(
    psVector* out,                     ///< Vector to return, or NULL.
    const psImage* LU,                 ///< LU-decomposed matrix.
    const psVector* RHS,               ///< Vector right-hand-side of equation.
    const psVector* perm               ///< Permutation vector resulting from psMatrixLUD function.
);

/** LU Decomposition-based Matrix inversion
 *
 *  @return  psImage * : Pointer to psImage inverse of matrix
 */
psImage *psMatrixLUInvert(
    psImage *out,                  ///< place result here if not NULL
    const psImage* LU,             ///< LU-decomposed matrix.
    const psVector* perm           ///< Permutation vector resulting from psMatrixLUD function.
);

/** LU Decomposition-based solver for Ax = B.
 *
 *  @return bool:   True if successful.
 */
bool psMatrixLUSolve(
    psImage *A,                   ///< Matrix to be solved
    psVector *b                   ///< Vector of values
);

/** Gauss-Jordan-based solver for Ax = B.
 *
 *  @return bool:   True if successful.
 */
bool psMatrixGJSolve(
    psImage *A,                   ///< Matrix to be solved
    psVector *b                   ///< Vector of values
);

/** Invert psImage matrix.
 *
 *  Inverts a psImage matrix and returns the determinant as an option through the argument list. If the user
 *  specifies NULL as the outImage argument, then it will automatically be created. The input image must be
 *  square. This function operates only with the psF64 data type. Input and output arguments should not be
 *  the same. GSL indexes the top row as the zero row, not the bottom.
 *
 *  @return  psImage* : Pointer to inverted psImage.
 */
psImage* psMatrixInvert(
    psImage* out,                      ///< Image to return, or NULL for in-place substitution.
    const psImage* in,                 ///< Image to be inverted
    float *determinant                 ///< Determinant to return, or NULL
);

/** Calculate psImage matrix determinant.
 *
 *  Calculates the determinant of a psImage matrix and returns the single precision floating point result. The
 *  input image must be square. This function operates only with the psF64 data type. GSL indexes the top row
 *  as the zero row, not the bottom.
 *
 *  @return  float: Determinant from psImage.
 */
float psMatrixDeterminant(
    const psImage* in                  ///< Image used to calculate determinant.
);

/** Performs psImage matrix multiplication.
 *
 *  Performs a classical matrix multiplication involving row and column operations. Input images must be square
 *  and the same size. If the user specifies NULL as the outImage argument, then it will automatically be
 *  created. This function operates only with the psF64 data type. GSL indexes the top row as the
 *  zero row, not the bottom.
 *
 *  @return  psImage* : Pointer to resulting psImage.
 */
psImage* psMatrixMultiply(
    psImage* out,                      ///< Matrix to return, or NULL.
    const psImage* in1,                ///< First input image.
    const psImage* in2                 ///< Second input image.
);

/** Transpose matrix.
 *
 *  Performs psImage matrix transpose by substituting existing rows for columns. The input image must be
 *  square. If the user specifies NULL as the outImage argument, then it will automaticallty be created.
 *  This function operates only with the psF64 data type. GSL indexes the top row as the zero
 *  row, not the bottom.
 *
 *  @return  psImage* : Pointer to transposed psImage.
 */
psImage* psMatrixTranspose(
    psImage* out,                      ///< Image to return, or NULL
    const psImage* in                  ///< Image to transpose
);

/** Calculate matrix eigenvectors.
 *
 *  Calculates the eigenvectors for a matrix. The input image must be symmetric and square. If the user
 *  specifies NULL as the outImage argument, then it will automatically be created. This function operates
 *  only with the psF64 data type. GSL indexes the top row as the zero row, not the bottom.
 *
 *  @return  psImage* : Pointer to matrix of Eigenvectors.
 */
psImage* psMatrixEigenvectors(
    psImage* out,                      ///< Eigenvectors to return, or NULL.
    const psImage* in                  ///< Input image.
);

/** Convert matrix to vector.
 *
 *  Converts a 1-d psImage matrix into a vector. If the user specifies NULL as the outVector argument, then it
 *  will automatically be created based on the input image (PS_DIMEN_VECTOR for an input image with 1 col or
 *  PS_DIMENT_TRANSV for an input image with 1 row). Either the number of rows or the number of colums of the
 *  input matrix must be 1. This function operates only  with the psF64 data type.
 *
 *  @return  psVector* : Pointer to psVector.
 */
psVector* psMatrixToVector(
    psVector* outVector,               ///< Vector to return, or NULL.
    const psImage* inImage             ///< Image to convert.
);

/** Convert vector to matrix.
 *
 *  Converts a vector into a psImage matrix. If the dimensionality of the vector is PS_DIMEN_VECTOR, then the
 *  resulting psImage is a 1d column. If the dimensionality of the vector is PS_DIMEN_TRANSV, then the
 *  resulting psImage is a 1d row. If the user specifies NULL as the outImage argument,  then it will
 *  automatically be created. This function operates only with the psF64 data type.
 *
 *  @return  psVector* : Pointer to psIamge.
 */
psImage* psVectorToMatrix(
    psImage* outImage,                 ///< Matrix to return, or NULL.
    const psVector* inVector           ///< Vector to convert.
);

/// Solve a matrix equation using Singular Value Decomposition
///
/// Solves Ax = b for x
psVector *psMatrixSolveSVD(
    psVector *solution,                 ///< Solution to output, or NULL
    const psImage *matrix,              ///< Matrix to be solved
    const psVector *vector,             ///< Vector of values
    float thresh                        ///< Threshold relative to maximum for trimming singular values
    );

/// Single value decomposition (original by Andy Becker, updated by EAM)
bool psMatrixSVD(psImage **U, psVector **w, psImage **V, const psImage *A);

/// @}
#endif // #ifndef PSMATRIX_H
