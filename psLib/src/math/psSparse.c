#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "psMemory.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psVector.h"
#include "psAssert.h"
#include "psConstants.h"
#include "psImageStructManip.h"
#include "psImage.h"
#include "psMatrix.h"
#include "psSparse.h"
#include "psAbort.h"

#define BUFFER 100                      // Size to increment at each go

static void sparseFree(psSparse *sparse)
{
    if (!sparse) {
        return;
    }
    psFree(sparse->Aij);
    psFree(sparse->Bfj);
    psFree(sparse->Qii);
    psFree(sparse->Si);
    psFree(sparse->Sj);
    return;
}

// allocate a sparse matrix container for Nrows, with Nelem slots allocated
psSparse *psSparseAlloc(int Nrows, int Nelem)
{
    psSparse *sparse = (psSparse *)psAlloc(sizeof(psSparse));
    psMemSetDeallocator(sparse, (psFreeFunc)sparseFree);

    sparse->Aij = psVectorAllocEmpty(Nelem, PS_DATA_F32);
    sparse->Si  = psVectorAllocEmpty(Nelem, PS_DATA_S32);
    sparse->Sj  = psVectorAllocEmpty(Nelem, PS_DATA_S32);

    sparse->Nelem = 0;

    sparse->Bfj = psVectorAlloc(Nrows, PS_DATA_F32);
    sparse->Qii = psVectorAlloc(Nrows, PS_DATA_F32);

    sparse->Nrows = Nrows;

    return sparse;
}

// user should only add elements above the diagonal, but we don't check this
bool psSparseMatrixElement(psSparse *sparse, int i, int j, float value)
{
    PS_ASSERT_PTR_NON_NULL(sparse, false);
    PS_ASSERT_INT_NONNEGATIVE(i, false);
    PS_ASSERT_INT_NONNEGATIVE(j, false);

    // EAM : this is now a fatal error
    if (i < j) {
        // psError(PS_ERR_UNKNOWN, true, "i=%d, j=%d refers to a sub-diagonal element. not allowed!");
        psAbort("i=%d, j=%d refers to a sub-diagonal element. not allowed!", i, j);
        return false;
    }

    if (i == j) {
        // add to the diagonal
        sparse->Qii->data.F32[i] = value;

        // check vectors lengths and extend if needed
        if (sparse->Nelem >= sparse->Aij->nalloc) {
            psVectorRealloc(sparse->Aij, sparse->Aij->nalloc + BUFFER);
            psVectorRealloc(sparse->Si,  sparse->Si->nalloc + BUFFER);
            psVectorRealloc(sparse->Sj,  sparse->Sj->nalloc + BUFFER);
        }

        int k = sparse->Nelem;         // Index at which to add
        sparse->Aij->data.F32[k] = value;
        sparse->Si->data.S32[k]  = i;
        sparse->Sj->data.S32[k]  = j;

        sparse->Nelem ++;
        sparse->Aij->n ++;
        sparse->Si->n ++;
        sparse->Sj->n ++;
    } else {
        // check vectors lengths and extend if needed
        if (sparse->Nelem >= sparse->Aij->nalloc - 1) {
            psVectorRealloc(sparse->Aij, sparse->Aij->nalloc + BUFFER);
            psVectorRealloc(sparse->Si,  sparse->Si->nalloc + BUFFER);
            psVectorRealloc(sparse->Sj,  sparse->Sj->nalloc + BUFFER);
        }

        int k = sparse->Nelem;         // Index at which to add
        sparse->Aij->data.F32[k] = value;
        sparse->Si->data.S32[k]  = i;
        sparse->Sj->data.S32[k]  = j;
        k++;

        sparse->Aij->data.F32[k] = value;
        sparse->Si->data.S32[k]  = j;
        sparse->Sj->data.S32[k]  = i;

        sparse->Nelem  += 2;
        sparse->Aij->n += 2;
        sparse->Si->n  += 2;
        sparse->Sj->n  += 2;
    }

    return true;
}

void psSparseVectorElement(psSparse *sparse, int i, float value)
{

    sparse->Bfj->data.F32[i] = value;
    return;
}

// multiply A * x
psVector *psSparseMatrixTimesVector(psVector *output, const psSparse *matrix, const psVector *vector)
{
    PS_ASSERT_PTR_NON_NULL(matrix, NULL);
    PS_ASSERT_VECTOR_NON_NULL(vector, NULL);

    output = psVectorRecycle(output, vector->n, PS_TYPE_F32);

    int Nelem = 0;                     // Number of elements
    for (int j = 0; j < vector->n; j++) {
        double F = 0;                    // Running total
        while (matrix->Sj->data.S32[Nelem] == j) {
            int i = matrix->Si->data.S32[Nelem];
            F += vector->data.F32[i] * matrix->Aij->data.F32[Nelem];
            Nelem++;
        }
        output->data.F32[j] = F;
    }
    return output;
}

// solve Ax = B
psVector *psSparseSolve(psVector *output, psSparseConstraint constraint, const psSparse *sparse, int Niter)
{
    PS_ASSERT_PTR_NON_NULL(sparse, NULL);
    PS_ASSERT_INT_POSITIVE(Niter, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(constraint.paramDelta, 0.0, NULL);

    // Dereference some vectors
    psVector *Qii = sparse->Qii;
    psVector *Bfj = sparse->Bfj;

    // initial guess is B / Qii
    output = psVectorCopy(output, Bfj, PS_DATA_F32);
    for (int i = 0; i < output->n; i++) {
        output->data.F32[i] /= Qii->data.F32[i];
    }

    // temporary storage for intermediate results
    psVector *dQ = psVectorAlloc(output->n, PS_DATA_F32);

    for (int j = 0; j < Niter; j++) {
        dQ = psSparseMatrixTimesVector(dQ, sparse, output);
        for (int i = 0; i < dQ->n; i++) {
            psF32 dG = (dQ->data.F32[i] - Bfj->data.F32[i]) / Qii->data.F32[i];
            if (fabs (dG) > constraint.paramDelta) {
                if (dG > 0) {
                    dG = +constraint.paramDelta;
                } else {
                    dG = -constraint.paramDelta;
                }
            }
            output->data.F32[i] -= dG;
            output->data.F32[i] = PS_MAX(output->data.F32[i], constraint.paramMin);
            output->data.F32[i] = PS_MIN(output->data.F32[i], constraint.paramMax);
        }
    }
    psFree(dQ);

    return output;
}

bool psSparseResort(psSparse *sparse)
{
    int Nelem = sparse->Nelem;

    psVector *index = psVectorSortIndex(NULL, sparse->Sj); // Index key for sorting
    if (!index) {
        psError(PS_ERR_UNKNOWN, false, "Unable to sort sparse matrix.\n");
        return false;
    }
    psVector *Aij = sparse->Aij;
    psVector *Si = sparse->Si;
    psVector *Sj = sparse->Sj;

    // allocate new temporary vectors
    psVector *tAij = psVectorAlloc(Nelem, PS_DATA_F32);
    psVector *tSi  = psVectorAlloc(Nelem, PS_DATA_S32);
    psVector *tSj  = psVectorAlloc(Nelem, PS_DATA_S32);

    for (int i = 0; i < Nelem; i++) {
        int j = index->data.U32[i];
        tAij->data.F32[i] = Aij->data.F32[j];
        tSi->data.S32[i]  = Si->data.S32[j];
        tSj->data.S32[i]  = Sj->data.S32[j];
    }
    psFree(index);
    psFree(Aij);
    psFree(Si);
    psFree(Sj);

    sparse->Aij = tAij;
    sparse->Si = tSi;
    sparse->Sj = tSj;

    return true;
}

/*** psSparseBorder Functions : these are used to solve a matrix equation of the form:
     A x = f  where A is partitioned into:
     A = |S B| where Q is a low-rank square matrix (N<20)
     |B Q| and B is a rectangular band (technically this is B and B^T)
     and S is a sparse matrix.
*/

static void psSparseBorderFree(psSparseBorder *border)
{
    if (!border) {
        return;
    }
    psFree(border->sparse);
    psFree(border->Bij);
    psFree(border->Tjj);
    psFree(border->Gj);
    return;
}

// allocate a sparse matrix border container for a system with Nrows + Nborder elements
// the supplied psSparse has a size of Nrows
psSparseBorder *psSparseBorderAlloc(psSparse *sparse, int Nborder)
{
    psAssert(sparse->Nrows > 0, "Require positive size: nrows = %d", sparse->Nrows);
    psAssert(Nborder > 0, "Require positive size: nborder = %d", Nborder);

    psSparseBorder *border = (psSparseBorder *)psAlloc(sizeof(psSparseBorder));
    psMemSetDeallocator(border, (psFreeFunc) psSparseBorderFree);

    border->sparse = psMemIncrRefCounter (sparse); // XXX increment ref counter or not?

    int Nrows = sparse->Nrows;
    border->Nrows = Nrows;
    border->Nborder = Nborder;

    border->Bij = psImageAlloc(Nrows, Nborder, PS_DATA_F32);
    psImageInit (border->Bij, 0.0);

    border->Tjj = psImageAlloc(Nborder, Nborder, PS_DATA_F32);
    psImageInit (border->Tjj, 0.0);

    border->Gj = psVectorAlloc(Nborder, PS_DATA_F32);
    psVectorInit (border->Gj, 0.0);

    return border;
}

// add elements to border->Tjj
bool psSparseBorderElementT(psSparseBorder *border, int i, int j, float value)
{
    PS_ASSERT_PTR_NON_NULL(border, false);
    PS_ASSERT_PTR_NON_NULL(border->Tjj, false);
    PS_ASSERT_INT_NONNEGATIVE(i, false);
    PS_ASSERT_INT_NONNEGATIVE(j, false);

    // check i,j against border->Tjj->nX,nY
    border->Tjj->data.F32[j][i] = value;
    return true;
}

// add elements to border->Bij
bool psSparseBorderElementB(psSparseBorder *border, int i, int j, float value)
{
    PS_ASSERT_PTR_NON_NULL(border, false);
    PS_ASSERT_PTR_NON_NULL(border->Bij, false);
    PS_ASSERT_INT_NONNEGATIVE(i, false);
    PS_ASSERT_INT_NONNEGATIVE(j, false);

    border->Bij->data.F32[j][i] = value;
    return true;
}

// add elements to border->Gj
bool psSparseBorderElementG(psSparseBorder *border, int i, float value)
{
    PS_ASSERT_PTR_NON_NULL(border, false);
    PS_ASSERT_PTR_NON_NULL(border->Gj, false);
    PS_ASSERT_INT_NONNEGATIVE(i, false);

    border->Gj->data.F32[i] = value;
    return true;
}

// perform the operation dG = B*x
psVector *psSparseBorderLowerProduct (psVector *dG, psSparseBorder *border, psVector *xVec)
{
    // XXX assert xVec->n == border->Nrows

    int Nborder = border->Nborder;
    int Nrows = border->Nrows;

    dG = psVectorRecycle (dG, Nborder, PS_TYPE_F32);
    psVectorInit (dG, 0.0);

    for (int j = 0; j < Nborder; j++) {
        double value = 0;
        for (int i = 0; i < Nrows; i++) {
            value += border->Bij->data.F32[j][i] * xVec->data.F32[i];
        }
        dG->data.F32[j] = value;
    }

    return dG;
}

// perform the operation dF = B^T*y
psVector *psSparseBorderUpperProduct (psVector *dF, psSparseBorder *border, psVector *yVec)
{
    // XXX assert yVec->n == border->Nborder

    int Nborder = border->Nborder;
    int Nrows = border->Nrows;

    dF = psVectorRecycle (dF, Nrows, PS_TYPE_F32);
    psVectorInit (dF, 0.0);

    for (int i = 0; i < Nrows; i++) {
        double value = 0;
        for (int j = 0; j < Nborder; j++) {
            value += border->Bij->data.F32[j][i] * yVec->data.F32[j];
        }
        dF->data.F32[i] = value;
    }

    return dF;
}

// perform the operation dG = T*y
psVector *psSparseBorderSquareProduct (psVector *dG, psSparseBorder *border, psVector *yVec)
{
    // XXX assert yVec->n == border->Nborder

    int Nborder = border->Nborder;

    dG = psVectorRecycle (dG, Nborder, PS_TYPE_F32);
    psVectorInit (dG, 0.0);

    for (int i = 0; i < Nborder; i++) {
        double value = 0;
        for (int j = 0; j < Nborder; j++) {
            value += border->Tjj->data.F32[i][j] * yVec->data.F32[j];
        }
        dG->data.F32[i] = value;
    }

    return dG;
}

// perform the operation dF = B^T*y
bool psSparseBorderUpperDelta (psSparseBorder *border, psVector *dF)
{

    int Nrows = border->Nrows;

    for (int i = 0; i < Nrows; i++) {
        border->sparse->Bfj->data.F32[i] -= dF->data.F32[i];
    }

    return true;
}

// perform the operation dF = B^T*y
psVector *psSparseBorderLowerDelta (psVector *Go, psSparseBorder *border, psVector *dG)
{
    int Nborder = border->Nborder;

    Go = psVectorRecycle (Go, Nborder, PS_TYPE_F64);
    psVectorInit (Go, 0.0);

    for (int i = 0; i < Nborder; i++) {
        Go->data.F64[i] = border->Gj->data.F32[i] - dG->data.F32[i];
    }
    return Go;
}

// multiply A*x = b (where x is (x,y) and b is (f,g))
bool psSparseBorderMultiply (psVector **fIn, psVector **gIn, psSparseBorder *border, psVector *xVec, psVector *yVec)
{
    psVector *fVec = *fIn;
    fVec = psSparseMatrixTimesVector (fVec, border->sparse, xVec);
    psVector *dF = psSparseBorderUpperProduct (NULL, border, yVec);
    for (int i = 0; i < border->Nrows; i++) {
        fVec->data.F32[i] += dF->data.F32[i];
    }
    psFree (dF);
    *fIn = fVec;

    psVector *gVec = *gIn;
    gVec = psSparseBorderSquareProduct (gVec, border, yVec);
    psVector *dG = psSparseBorderLowerProduct (NULL, border, xVec);
    for (int i = 0; i < border->Nborder; i++) {
        gVec->data.F32[i] += dG->data.F32[i];
    }
    psFree (dG);
    *gIn = gVec;

    return true;
}

bool psSparseBorderSolve(psVector **xFit, psVector **yFit, psSparseConstraint constraint, psSparseBorder *border, int Niter)
{
    // PS_ASSERT_PTR_NON_NULL(sparse, NULL);
    // PS_ASSERT_INT_POSITIVE(Niter, NULL);
    // PS_ASSERT_FLOAT_LARGER_THAN(constraint.paramDelta, 0.0, NULL);

    psVector *xVec = *xFit;
    psVector *yVec = *yFit;

    // save the original value of Bfj, alloc other temp vectors
    // XXX be careful about TYPE requirements of support functions...
    psVector *Fo = psVectorCopy (NULL, border->sparse->Bfj, PS_TYPE_F32);
    psVector *dF = psVectorAlloc (border->Nrows, PS_TYPE_F32);
    psVector *Go = psVectorAlloc (border->Nborder, PS_TYPE_F64);
    psVector *dG = psVectorAlloc (border->Nborder, PS_TYPE_F32);
    psImage *square = psImageAlloc (border->Nborder, border->Nborder, PS_TYPE_F64);

    for (int j = 0; j < Niter; j++) {

        // solve Sx = f
        xVec = psSparseSolve(xVec, constraint, border->sparse, Niter);

        dG = psSparseBorderLowerProduct (dG, border, xVec);

        // XXX i have an error here: i'm not
        // I need to reset to the original value of g before subtracting
        // XXX this function returns an psVector:F64 so we can us it in GJ
        Go = psSparseBorderLowerDelta (Go, border, dG);

        // use gauss-jordan to solve the lower square
        // square is modified by GJSolve, so re-copy each time
        square = psImageCopy (square, border->Tjj, PS_TYPE_F64);
        if (!psMatrixGJSolve (square, Go)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to solve for lower square.");
            psFree (dG);
            psFree (Go);
            psFree (dF);
            psFree (Fo);
            psFree (square);
            return false;
        }
        yVec = psVectorCopy (yVec, Go, PS_TYPE_F32);

        // calculate the delta relative to the original Bfj:
        border->sparse->Bfj = psVectorCopy (border->sparse->Bfj, Fo, PS_TYPE_F32);
        dF = psSparseBorderUpperProduct (dF, border, yVec);
        psSparseBorderUpperDelta (border, dF);
    }

    psFree (dG);
    psFree (Go);
    psFree (dF);
    psFree (Fo);
    psFree (square);

    *xFit = xVec;
    *yFit = yVec;
    return true;
}
