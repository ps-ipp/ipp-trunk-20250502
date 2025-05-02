/* @file  psSparse.h
 * @brief functions to manipulate sparse matrices equations
 *
 * $Revision: 1.9 $ $Name: not supported by cvs2svn $
 * $Date: 2007-08-09 01:40:07 $
 * Copyright 2004-2005 IfA, University of Hawaii
 */

#ifndef PS_SPARSE_H
#define PS_SPARSE_H

/// @addtogroup MathOps Mathematical Operations
/// @{

// constraints to limit the range of the matrix equation solution
typedef struct
{
    double paramDelta;
    double paramMin;
    double paramMax;
}
psSparseConstraint;

// A sparse matrix equation: A x = Bf
typedef struct
{
    psVector *Aij;                      // Aij contains the populated elements of the matrix
    psVector *Bfj;                      // Bfj contains the elements of the vector Bf
    psVector *Qii;                      // Qii contains the diagonal elements of Aij
    psVector *Si;                       // Si contains the i-index values of Aij
    psVector *Sj;                       // Sj contains the j-index values of Aij
    int Nelem;                         // Number of elements
    int Nrows;                         // Number of rows
}
psSparse;

// The border elements of a sparse matrix equation:
// A = |S B'| where T is a low-rank square matrix (N<20)
//     |B T | and B is a rectangular band (B' is B transpose)
typedef struct
{
    psSparse *sparse;   // corresponding sparse matrix equation
    psImage *Bij;   // Bij contains the border band (Nrow x Nborder)
    psImage *Tjj;   // Tjj contains the square border matrix (Nborder x Nborder)
    psVector *Gj;   // XXX lower dependent var drop??
    int Nrows;    // Number of rows (long dimension of Bij, 0-j)
    int Nborder;   // Number of border elements (size of Qii)
}
psSparseBorder;

// allocate a sparse matrix structure
psSparse *psSparseAlloc(int Nrows, int Nelem) PS_ATTR_MALLOC;

// add a new matrix element
// user should only add elements above the diagonal
bool psSparseMatrixElement(psSparse *sparse, // Matrix to which to add
                           int i, int j, // Matrix indices at which to add
                           float value  // Value to add
                          );

// define a new sparse matrix equation vector element
void psSparseVectorElement(psSparse *sparse, // Matrix to which to add
                           int i,      // Index to add
                           float value  // Value to add
                          );

// perform the operation matrix * vector on a sparse matrix and a vector
psVector *psSparseMatrixTimesVector(psVector *output, // Output vector, or NULL
                                    const psSparse *matrix, // Sparse matrix
                                    const psVector *vector // Corresponding vector
                                   );

// re-sort a sparse matrix to have all elements in index order rather than insertion order
// call this before solving, but after populating matrix and vector
bool psSparseResort(psSparse *sparse    // Matrix to re-sort
                   );

// solve the equation A x = Bf for the value of x
// a good starting guess is the vector Bf
psVector *psSparseSolve(psVector *output,// The output vector, or NULL
                        psSparseConstraint constraint, // Constraint to limit the range of the solution
                        const psSparse *sparse, // Sparse matrix
                        int Niter       // Number of iterations
                       );

// allocate a sparse matrix structure
psSparseBorder *psSparseBorderAlloc(psSparse *sparse, int Nborder) PS_ATTR_MALLOC;

bool psSparseBorderElementT(psSparseBorder *border, int i, int j, float value);

bool psSparseBorderElementB(psSparseBorder *border, int i, int j, float value);

bool psSparseBorderElementG(psSparseBorder *border, int i, float value);

psVector *psSparseBorderLowerProduct (psVector *dG, psSparseBorder *border, psVector *xVec);

psVector *psSparseBorderUpperProduct (psVector *dF, psSparseBorder *border, psVector *yVec);

psVector *psSparseBorderSquareProduct (psVector *dG, psSparseBorder *border, psVector *yVec);

bool psSparseBorderUpperDelta (psSparseBorder *border, psVector *dF);

psVector *psSparseBorderLowerDelta (psVector *Go, psSparseBorder *border, psVector *dG);

bool psSparseBorderMultiply (psVector **fIn, psVector **gIn, psSparseBorder *border, psVector *xVec, psVector *yVec);

bool psSparseBorderSolve(psVector **xFit, psVector **yFit, psSparseConstraint constraint, psSparseBorder *border, int Niter);

/// @}
#endif /* PS_SPARSE_H */
