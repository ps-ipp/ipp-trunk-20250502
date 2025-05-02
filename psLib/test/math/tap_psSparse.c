    #include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(40);


    // test psSparseAlloc()
    {
        psMemId id = psMemGetId();
        psSparse *matrix = psSparseAlloc(10, 20);
        ok(matrix != NULL, "psSparse successfully allocated");
        skip_start(matrix == NULL, 12, "skipping tests because psSparseAlloc() returned NULL");
        ok(matrix->Aij != NULL, "psSparseAlloc() set ->Aij correctly");
        ok(matrix->Aij->n == 0, "psSparseAlloc() set ->Aij->n correctly");
        ok(matrix->Si != NULL, "psSparseAlloc() set ->Si correctly");
        ok(matrix->Si->n == 0, "psSparseAlloc() set ->Si->n correctly");
        ok(matrix->Sj != NULL, "psSparseAlloc() set ->Sj correctly");
        ok(matrix->Sj->n == 0, "psSparseAlloc() set ->Sj->n correctly");
        ok(matrix->Bfj != NULL, "psSparseAlloc() set ->Bfj correctly");
        ok(matrix->Bfj->n == 10, "psSparseAlloc() set ->Bfj->n correctly");
        ok(matrix->Qii != NULL, "psSparseAlloc() set ->Qii correctly");
        ok(matrix->Qii->n == 10, "psSparseAlloc() set ->Qii->n correctly");
        ok(matrix->Nelem == 0, "psSparseAlloc() set ->Nelem correctly");
        ok(matrix->Nrows == 10, "psSparseAlloc() set ->Nrows correctly");
        skip_end();
        psFree(matrix);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // test psSparseSolve for a simple normal example matrix
    {
        psMemId id = psMemGetId();
        // the basic equation is Ax = b

        // create a matrix A with diagonals of 1 and a small number of off diagonal elements.
        // construct a vector x, construct the corresponding vector b by multiplication. solve
        // Ax = b for x using psSparseSolve.  compare with the input values for x.

        psSparse *matrix = psSparseAlloc(100, 100);
        ok(matrix != NULL, "psSparse successfully allocated");
        skip_start(matrix == NULL, 5, "Skipping tests because psSparseAlloc() failed");

        for(int i = 0; i < 100; i++) {
            psSparseMatrixElement(matrix, i, i, 1.0);
            if (i + 1 < 100) {
                psSparseMatrixElement(matrix, i + 1, i, 0.1);
            }
        }

        // incoming matrix elements do not need to be in order; sort before
        // applying sparse matrix
        psSparseResort(matrix);

        psVector *xRef = psVectorAlloc(100, PS_TYPE_F32);
        for (int i = 0; i < 100; i++) {
            xRef->data.F32[i] = 1.0;
        }

        psVector *bVec = psSparseMatrixTimesVector(NULL, matrix, xRef);

        for (int i = 0; i < 100; i++) {
            psSparseVectorElement(matrix, i, bVec->data.F32[i]);
        }

        psSparseConstraint constraint;
        constraint.paramMin   = 0.0;
        constraint.paramMax   = 1e8;
        constraint.paramDelta = 1e8;

        // solve for normalization terms (need include local sky?)
        psVector *xFit = psSparseSolve(NULL, constraint, matrix, 4);

        // measure stdev between xFit and xRes
        float dS = 0;
        float dS2 = 0;
        for (int i = 0; i < 100; i++) {
            float dX = xRef->data.F32[i] - xFit->data.F32[i];
            // fprintf(stderr, "%5.3f %5.3f %5.3f %5.3f\n", bVec->data.F32[i], xRef->data.F32[i], xFit->data.F32[i], dX);
            dS2 += PS_SQR(dX);
            dS += dX;
        }
        dS /= 100.0;
        dS2 = sqrt(dS2/100.0 - dS*dS);

        is_float_tol(dS2, 0.0, 1e-4, "scatter: %.20f", dS2);

        psFree(matrix);
        psFree(xRef);
        psFree(bVec);
        psFree(xFit);

        skip_end();
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // test psSparseSolve for a simple non-normal example matrix
    {
        psMemId id = psMemGetId();
        // the basic equation is Ax = b

        // create a matrix A with diagonals of 1 and a small number of off diagonal elements.
        // construct a vector x, construct the corresponding vector b by multiplication. solve
        // Ax = b for x using psSparseSolve.  compare with the input values for x.

        psSparse *matrix = psSparseAlloc(100, 100);
        ok(matrix != NULL, "psSparse successfully allocated");
        skip_start(matrix == NULL, 5, "Skipping tests because psSparseAlloc() failed");

        for (int i = 0; i < 100; i++) {
            psSparseMatrixElement(matrix, i, i, 5.0);
            if (i + 1 < 100) {
                psSparseMatrixElement(matrix, i + 1, i, 0.1);
            }
        }

        // incoming matrix elements do not need to be in order; sort before
        // applying sparse matrix
        psSparseResort(matrix);

        psVector *xRef = psVectorAlloc(100, PS_TYPE_F32);
        for (int i = 0; i < 100; i++) {
            xRef->data.F32[i] = 1.0;
        }

        psVector *bVec = psSparseMatrixTimesVector(NULL, matrix, xRef);

        for (int i = 0; i < 100; i++) {
            psSparseVectorElement(matrix, i, bVec->data.F32[i]);
        }

        psSparseConstraint constraint;
        constraint.paramMin   = 0.0;
        constraint.paramMax   = 1e8;
        constraint.paramDelta = 1e8;

        // solve for normalization terms(need include local sky?)
        psVector *xFit = psSparseSolve(NULL, constraint, matrix, 4);

        // measure stdev between xFit and xRes
        float dS = 0;
        float dS2 = 0;
        for (int i = 0; i < 100; i++) {
            float dX = xRef->data.F32[i] - xFit->data.F32[i];
            // fprintf(stderr, "%5.3f %5.3f %5.3f %5.3f\n", bVec->data.F32[i], xRef->data.F32[i], xFit->data.F32[i], dX);
            dS2 += PS_SQR(dX);
            dS += dX;
        }
        dS /= 100.0;
        dS2 = sqrt(dS2/100.0 - dS*dS);

        is_float_tol(dS2, 0.0, 1e-4, "scatter: %.20f", dS2);

        psFree(matrix);
        psFree(xRef);
        psFree(bVec);
        psFree(xFit);

        skip_end();
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    /* sample matrix equation:

    1.0 0.1 0.1 | 1.0 |   |1.15|
    0.1 1.0 0.2 | 1.0 | = |1.20|
    0.1 0.2 0.5 | 0.5 |   |0.55|

    lower product : 1.0*0.1 + 1.0*0.2 = 0.3
    upper product : 0.1*0.5 = 0.05
              : 0.2*0.5 = 0.10
    */

    // test psSparseBorderMultiply
    {
        psMemId id = psMemGetId();
        // the basic equation(Ax = b) is:
        // |S B'||x| = |f|
        // |B Q ||y| = |g|

        // construct a matrix S with diagonals of 1 and a small number of off diagonal elements.
        // construct a border with finite values and a corresponding Q matrix
        // construct a vector x, construct the corresponding vector b by multiplication. solve
        // Ax = b for x using psSparseSolve.  compare with the input values for x.

        int Nrows = 2;
        int Nborder = 1;

        // construct the sparse matrix
        psSparse *matrix = psSparseAlloc(Nrows, Nrows);
        ok(matrix != NULL, "psSparse successfully allocated");
        skip_start(matrix == NULL, 5, "Skipping tests because psSparseAlloc() failed");

        psSparseMatrixElement(matrix, 0, 0, 1.0);
        psSparseMatrixElement(matrix, 1, 1, 1.0);
        psSparseMatrixElement(matrix, 1, 0, 0.1);
        psSparseResort(matrix);

        // border region has a width of 1:
        psSparseBorder *border = psSparseBorderAlloc(matrix, Nborder);

        // construct the B component:
        psSparseBorderElementB(border, 0, 0, 0.1);
        psSparseBorderElementB(border, 1, 0, 0.2);

        // construct the T component:
        psSparseBorderElementT(border, 0, 0, 0.5);

        // construct the X and Y vectors:
        psVector *xRef = psVectorAlloc(Nrows, PS_TYPE_F32);
        xRef->data.F32[0] = 1.0;
        xRef->data.F32[1] = 1.0;
        psVector *yRef = psVectorAlloc(Nborder, PS_TYPE_F32);
        yRef->data.F32[0] = 0.5;

        // construct the f and g vectors by multiplication:
        psVector *fVec;

        // test the support functions: LowerProduct
        fVec = psSparseBorderLowerProduct(NULL, border, xRef);
        is_float(fVec->n, 1.0, "f dimen: %d", fVec->n);
        is_float(fVec->data.F32[0], 0.3, "f(0): %f", fVec->data.F32[0]);
        psFree(fVec);

        // test the support functions: Upper Product
        fVec = psSparseBorderUpperProduct(NULL, border, yRef);
        is_float(fVec->n, 2.0, "f dimen: %d", fVec->n);
        is_float(fVec->data.F32[0], 0.05, "f(0): %f", fVec->data.F32[0]);
        is_float(fVec->data.F32[1], 0.10, "f(0): %f", fVec->data.F32[1]);
        psFree(fVec);

        // test the support functions: Square Product
        fVec = psSparseBorderSquareProduct(NULL, border, yRef);
        is_float(fVec->n, 1.0, "f dimen: %d", fVec->n);
        is_float(fVec->data.F32[0], 0.25, "f(0): %f", fVec->data.F32[0]);
        psFree(fVec);

        fVec = NULL;
        psVector *gVec = NULL;
        psSparseBorderMultiply(&fVec, &gVec, border, xRef, yRef);
        is_float(fVec->data.F32[0], 1.15, "f(0): %f", fVec->data.F32[0]);
        is_float(fVec->data.F32[1], 1.20, "f(1): %f", fVec->data.F32[1]);
        is_float(gVec->data.F32[0], 0.55, "g(0): %f", gVec->data.F32[0]);

        // supply the fVec and gVec data to the border
        for (int i = 0; i < Nrows; i++) {
            psSparseVectorElement(border->sparse, i, fVec->data.F32[i]);
        }
        for (int i = 0; i < Nborder; i++) {
            psSparseBorderElementG(border, i, gVec->data.F32[i]);
        }

        // solve the border equation
        psSparseConstraint constraint;
        constraint.paramMin   = 0.0;
        constraint.paramMax   = 1e8;
        constraint.paramDelta = 1e8;

        // solve for normalization terms (need include local sky?)
        psVector *xFit = NULL;
        psVector *yFit = NULL;
        psSparseBorderSolve(&xFit, &yFit, constraint, border, 4);
        is_float_tol(xFit->data.F32[0], 1.0, 1e-4, "f(0): %f", xFit->data.F32[0]);
        is_float_tol(xFit->data.F32[1], 1.0, 1e-4, "f(1): %f", xFit->data.F32[1]);
        is_float_tol(yFit->data.F32[0], 0.5, 1e-4, "g(0): %f", yFit->data.F32[0]);

        psFree(xFit);
        psFree(yFit);
        psFree(fVec);
        psFree(gVec);
        psFree(xRef);
        psFree(yRef);
        psFree(border);
        psFree(matrix);
        skip_end();
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // test psSparseBorderSolve for a simple example matrix
    {
        psMemId id = psMemGetId();
        // the basic equation(Ax = b) is:
        // |S B'||x| = |f|
        // |B Q ||y| = |g|

        // construct a matrix S with diagonals of 1 and a small number of off diagonal elements.
        // construct a border with finite values and a corresponding Q matrix
        // construct a vector x, construct the corresponding vector b by multiplication. solve
        // Ax = b for x using psSparseSolve.  compare with the input values for x.

        int Nrows = 10;
        int Nborder = 1;

        // construct the sparse matrix
        psSparse *matrix = psSparseAlloc(Nrows, Nrows);
        ok(matrix != NULL, "psSparse successfully allocated");
        skip_start(matrix == NULL, 5, "Skipping tests because psSparseAlloc() failed");

        for (int i = 0; i < Nrows; i++) {
            psSparseMatrixElement(matrix, i, i, 1.0);
            if (i + 1 < Nrows) {
                psSparseMatrixElement(matrix, i + 1, i, 0.1);
            }
        }
        psSparseResort(matrix);

        // border region has a width of 1:
        psSparseBorder *border = psSparseBorderAlloc(matrix, Nborder);

        // construct the B component:
        for (int i = 0; i < Nrows; i++) {
            for (int j = 0; j < Nborder; j++) {
                psSparseBorderElementB(border, i, j, 0.1);
            }
        }
        is_float(border->Bij->data.F32[0][0], 0.1, "set matrix element %d,%d", 0, 0);

        // construct the Q component:
        for (int i = 0; i < Nborder; i++) {
            for (int j = 0; j < Nborder; j++) {
                psSparseBorderElementT(border, i, j, i+j+2);
            }
        }

        // construct the X and Y vectors:
        psVector *xRef = psVectorAlloc(Nrows, PS_TYPE_F32);
        for (int i = 0; i < Nrows; i++) {
            xRef->data.F32[i] = 1.0;
        }
        psVector *yRef = psVectorAlloc(Nborder, PS_TYPE_F32);
        for (int i = 0; i < Nborder; i++) {
            yRef->data.F32[i] = 1.0;
        }

        // construct the f and g vectors by multiplication:
        psVector *fVec = NULL;
        psVector *gVec = NULL;
        psSparseBorderMultiply(&fVec, &gVec, border, xRef, yRef);

        // supply the fVec and gVec data to the border
        for (int i = 0; i < Nrows; i++) {
            psSparseVectorElement(border->sparse, i, fVec->data.F32[i]);
        }
        for (int i = 0; i < Nborder; i++) {
            psSparseBorderElementG(border, i, gVec->data.F32[i]);
        }

        // solve the border equation
        psSparseConstraint constraint;
        constraint.paramMin   = 0.0;
        constraint.paramMax   = 1e8;
        constraint.paramDelta = 1e8;

        // solve for normalization terms(need include local sky?)
        psVector *xFit = NULL;
        psVector *yFit = NULL;
        psSparseBorderSolve(&xFit, &yFit, constraint, border, 4);

        // measure stdev between xFit and xRef
        float dS = 0;
        float dS2 = 0;
        for (int i = 0; i < Nrows; i++) {
            float dX = xRef->data.F32[i] - xFit->data.F32[i];
            // fprintf(stderr, "%5.3f %5.3f %5.3f %5.3f\n", fVec->data.F32[i], xRef->data.F32[i], xFit->data.F32[i], dX);
            dS2 += PS_SQR(dX);
            dS += dX;
        }
        dS /= Nrows;
        dS2 = sqrt(dS2/Nrows - dS*dS);
        is_float_tol(dS2, 0.0, 1e-4, "scatter: %.20f", dS2);

        // measure stdev between yFit and yRef
        float dY = yRef->data.F32[0] - yFit->data.F32[0];
        // fprintf(stderr, "%5.3f %5.3f %5.3f %5.3f\n", gVec->data.F32[0], yRef->data.F32[0], yFit->data.F32[0], dS);
        is_float_tol(dY, 0.0, 2e-4, "scatter: %.20f", dY);

        psFree(xRef);
        psFree(yRef);
        psFree(xFit);
        psFree(yFit);
        psFree(fVec);
        psFree(gVec);
        psFree(matrix);
        psFree(border);

        skip_end();
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
