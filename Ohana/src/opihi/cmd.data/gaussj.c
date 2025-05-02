# include "data.h"

int gaussjordan (int argc, char **argv) {

  float *m;
  opihi_flt *vf;
  opihi_int *vi;
  double **a, **b;
  int i, j, N, status, QUIET, isFloat;
  Vector *B;
  Buffer *A;

  QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) goto usage;

  if ((A = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);    
  if ((B = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  N = B[0].Nelements;
  if (A[0].matrix.Naxis[0] != N) goto usage;
  if (A[0].matrix.Naxis[1] != N) goto usage;
  
  ALLOCATE (a, double *, N);
  ALLOCATE (b, double *, N);
  for (i = 0; i < N; i++) {
    ALLOCATE (a[i], double, N);
    ALLOCATE (b[i], double, 1);
  }

  isFloat = (B[0].type == OPIHI_FLT);
  vf = B[0].elements.Flt;
  vi = B[0].elements.Int;

  m = (float *) A[0].matrix.buffer;
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
      a[i][j] = m[i+j*N];
    }
    b[i][0] = isFloat ? vf[i] : vi[i]; 
  }

  status = dgaussjordan (a, b, N, 1);

  // if dgaussjordan succeeds, replace the input values with the results
  if (status) {
    // output vector needs to be float, so re-cast it
    ResetVector (B, OPIHI_FLT, N);
    vf = B[0].elements.Flt;

    for (i = 0; i < N; i++) {
      for (j = 0; j < N; j++) {
	m[i+j*N] = a[i][j];
      }
      vf[i] = b[i][0]; 
    }
  }

  for (i = 0; i < N; i++) {
    free (a[i]);
    free (b[i]);
  }
  free (a);
  free (b);

  if (!status && !QUIET) {
      gprint (GP_ERR, "gaussjordan: ill-conditioned matrix; input values are retained\n");
  }
  return (status);

 usage:
  gprint (GP_ERR, "USAGE: gaussj A B\n");
  gprint (GP_ERR, "  solves Ax = B, returns 1/A in A and x in B\n");
  gprint (GP_ERR, "  A must be square, B same dimensions\n");
  return (FALSE);
    
}
