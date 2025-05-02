# include "data.h"

int svd (int argc, char **argv) {
  
  int i, Nx, Ny, status;
  float *in, *out, *A, *U, *V;
  Buffer *Ma, *Mu, *Mv;
  Vector *Vw;
  opihi_flt *W;

  if (argc != 6) goto usage;
  if (strcmp (argv[2], "=")) goto usage;

  if ((Ma = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Mu = SelectBuffer (argv[3], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Vw = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Mv = SelectBuffer (argv[5], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  Nx = Ma[0].header.Naxis[0];
  Ny = Ma[0].header.Naxis[1];

  /* U is Nx, Ny */
  gfits_free_matrix (&Mu[0].matrix);
  gfits_free_header (&Mu[0].header);
  Mu[0].bitpix = Ma[0].bitpix;
  Mu[0].unsign = Ma[0].unsign;
  Mu[0].bscale = Ma[0].bscale;
  Mu[0].bzero  = Ma[0].bzero;
  gfits_copy_header (&Ma[0].header, &Mu[0].header);
  gfits_create_matrix (&Mu[0].header, &Mu[0].matrix);
  
  /* V is Nx, Nx */
  gfits_free_matrix (&Mv[0].matrix);
  gfits_free_header (&Mv[0].header);
  Mv[0].bitpix = Ma[0].bitpix;
  Mv[0].unsign = Ma[0].unsign;
  Mv[0].bscale = Ma[0].bscale;
  Mv[0].bzero  = Ma[0].bzero;
  gfits_copy_header (&Ma[0].header, &Mv[0].header);
  gfits_modify (&Mv[0].header, "NAXIS2", "%d", 1, Nx);
  Mv[0].header.Naxis[1] = Nx;
  gfits_create_matrix (&Mv[0].header, &Mv[0].matrix);

  /* w is Nx */
  ResetVector (Vw, OPIHI_FLT, Nx);

  /* pointers to the various arrays */
  A = (float *) Ma[0].matrix.buffer;
  U = (float *) Mu[0].matrix.buffer;
  V = (float *) Mv[0].matrix.buffer;
  W = Vw[0].elements.Flt;

  /* copy A to U (svdcmp replaces A with U) */
  in  = A;
  out = U;
  for (i = 0; i < Nx*Ny; i++, in++, out++) *out = *in;
  /* use a bcopy instead? */

  // try C.R. Bond's version -- requires matrices in the form A[row][col] not A[row*Ncol + col]
  if (0) { 
      int j;
      double **a, **u, **v, *q;
      ALLOCATE (a, double *, Ny);
      ALLOCATE (u, double *, Ny);
      ALLOCATE (v, double *, Ny);
      for (i = 0; i < Ny; i++) {
	  ALLOCATE (a[i], double, Nx);
	  ALLOCATE (u[i], double, Nx);
	  ALLOCATE (v[i], double, Nx);
      }	  
      ALLOCATE (q, double, Nx);

      for (j = 0; j < Ny; j++) {
	  for (i = 0; i < Nx; i++) {
	      a[j][i] = A[j*Nx + i];
	      u[j][i] = 0;
	      v[j][i] = 0;
	  }
      }
      for (i = 0; i < Nx; i++) {
	  q[i] = 0;
      }

# if 1     
      status = svdcmp_bond_new (Ny, Nx, 1, 1, FLT_EPSILON, 1e-6, a, q, u, v);
      fprintf (stderr, "status: %d\n", status);
# else
      fprintf (stderr, "fix bond svdcmp\n");
      return FALSE;
# endif

      // copy u q v back to U W V:
      for (j = 0; j < Ny; j++) {
	  for (i = 0; i < Nx; i++) {
	      U[j*Nx + i] = u[j][i];
	      V[j*Nx + i] = v[j][i];
	  }
      }
      for (i = 0; i < Nx; i++) {
	  W[i] = q[i];
      }

      for (j = 0; j < Ny; j++) {
	  free(a[j]);
	  free(u[j]);
	  free(v[j]);
      }
      free (a);
      free (u);
      free (v);
      free (q);

      return TRUE;
  }

  status = svdcmp (U, W, V, Nx, Ny);
  if (!status) {
    gprint (GP_ERR, "error running svdcmp\n");
    return (FALSE);
  }
  return (TRUE);

 usage:
  gprint (GP_ERR, "USAGE: svd A = U w Vt\n");
  return (FALSE);
  
}

