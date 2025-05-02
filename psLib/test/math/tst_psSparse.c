
void psSparseMatrixTest ()
{

    // build a sparse matrix
    psSparse *sparse = psSparseAlloc (3, 9);

    psSparseMatrixElement (sparse, 0, 0, 3.0);
    psSparseMatrixElement (sparse, 1, 1, 2.0);
    psSparseMatrixElement (sparse, 2, 2, 1.0);

    psSparseMatrixElement (sparse, 1, 0, 0.1);
    psSparseMatrixElement (sparse, 2, 0, -0.1);

    psSparseResort (sparse);
    for (int i = 0; i < sparse->Nelem; i++) {
        fprintf (stderr, "%d %d %f\n",
                 sparse->Si->data.S32[i],
                 sparse->Sj->data.S32[i],
                 sparse->Aij->data.F32[i]);
    }

    psVector *x = psVectorAlloc (3, PS_DATA_F32);
    x->data.F32[0] = 3;
    x->data.F32[1] = 5;
    x->data.F32[2] = 7;
    x->n = 3;
    fprintf (stderr, "x: %f %f %f\n", x->data.F32[0], x->data.F32[1], x->data.F32[2]);


    psVector *B = psSparseMatrixTimesVector (NULL, sparse, x);
    fprintf (stderr, "B: %f %f %f\n", B->data.F32[0], B->data.F32[1], B->data.F32[2]);

    sparse->Bfj->data.F32[0] = B->data.F32[0];
    sparse->Bfj->data.F32[1] = B->data.F32[1];
    sparse->Bfj->data.F32[2] = B->data.F32[2];

    psSparseConstraint constraint;
    constraint.paramMin   = -1e8;
    constraint.paramMax   = +1e8;
    constraint.paramDelta = +1e8;

    x = psSparseSolve (x, constraint, sparse, 0);
    fprintf (stderr, "x: %f %f %f\n", x->data.F32[0], x->data.F32[1], x->data.F32[2]);

    x = psSparseSolve (x, constraint, sparse, 1);
    fprintf (stderr, "x: %f %f %f\n", x->data.F32[0], x->data.F32[1], x->data.F32[2]);

    x = psSparseSolve (x, constraint, sparse, 2);
    fprintf (stderr, "x: %f %f %f\n", x->data.F32[0], x->data.F32[1], x->data.F32[2]);

    x = psSparseSolve (x, constraint, sparse, 3);
    fprintf (stderr, "x: %f %f %f\n", x->data.F32[0], x->data.F32[1], x->data.F32[2]);

    x = psSparseSolve (x, constraint, sparse, 4);
    fprintf (stderr, "x: %f %f %f\n", x->data.F32[0], x->data.F32[1], x->data.F32[2]);
    return;
}

