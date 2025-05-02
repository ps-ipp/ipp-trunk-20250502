# include "relastro.h"

/* do we want an init function which does the alloc and a clear function to free? */
int FitPar (PMFit *fit, double *X, double *dX, double *Y, double *dY, double *pR, double *pD, int Npts) {

  int i;

  double **A, **B;
  double wx, wy, Wx, Wy, Xs, Ys;
  double PR, PD, PRX, PDY, PR2, PD2;

  A = array_init (3, 3);
  B = array_init (3, 1);

  Wx = Wy = Xs = Ys = 0.0;
  PR = PD = PRX = PDY = PR2 = PD2 = 0.0;
  for (i = 0; i < Npts; i++) {
    /* handle case where dX or dY = 0.0 */
    wx = 1.0 / SQ(dX[i]);
    wy = 1.0 / SQ(dY[i]);

    Wx += wx;
    Wy += wy;

    PR += pR[i]*wx;
    PD += pD[i]*wy;
    
    PRX += pR[i]*X[i]*wx;
    PDY += pD[i]*Y[i]*wy;
    
    PR2 += SQ(pR[i])*wx;
    PD2 += SQ(pD[i])*wy;

    Xs += X[i]*wx;
    Ys += Y[i]*wy;
  }

  A[0][0] = Wx;
  A[0][2] = PR;

  A[1][1] = Wy;
  A[1][2] = PD;

  A[2][0] = PR;
  A[2][1] = PD;
  A[2][2] = PR2 + PD2;

  B[0][0] = Xs;
  B[1][0] = Ys;
  B[2][0] = PRX + PDY;

  dgaussjordan (A, B, 3, 1);

  fit[0].Ro = B[0][0];
  fit[0].Do = B[1][0];
  fit[0].p  = B[2][0];

  fit[0].uR = 0.0;
  fit[0].uD = 0.0;
  
  fit[0].dRo = sqrt(A[0][0]);
  fit[0].dDo = sqrt(A[2][2]);
  fit[0].dp  = sqrt(A[4][4]);
  
  fit[0].duR = 0.0;
  fit[0].duD = 0.0;

  array_free (A, 3);
  array_free (B, 3);

  /* get the chisq from the matrix values */

  return (TRUE);
}
