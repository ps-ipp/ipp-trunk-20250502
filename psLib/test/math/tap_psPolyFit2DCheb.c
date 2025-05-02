#include <stdio.h>
#include <string.h>
#include <pslib.h>
//#include "tap.h"
//#include "pstap.h"

int main(int argc, char **argv)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    // plan_tests(88);

    if (argc != 4) {
      fprintf (stderr, "USAGE: tap_psPolyFit2DCheb (input) (Nx) (Ny)\n");
      exit (2);
    }
    
    FILE *f = fopen (argv[1], "r");
    int Nx = atoi(argv[2]);
    int Ny = atoi(argv[3]);
    
    // data file should contain x y p
    // determine the best fit 2D polynomial from x y to p
    psVector *x = psVectorAllocEmpty(1000, PS_TYPE_F64);
    psVector *y = psVectorAllocEmpty(1000, PS_TYPE_F64);
    psVector *p = psVectorAllocEmpty(1000, PS_TYPE_F64);
    
    double xv, yv, pv;
    
    while (fscanf (f, "%lf %lf %lf", &xv, &yv, &pv) == 3) {
      psVectorAppend (x, xv);
      psVectorAppend (y, yv);
      psVectorAppend (p, pv);
    }

    // Fit an ordinary polynomial
    {
      psPolynomial2D *polyOrd = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, Nx, Ny);
      int result = psVectorFitPolynomial2D(polyOrd, NULL, 0, p, NULL, x, y);

      fprintf (stdout, " ---- Ord ---- \n");

      for (int ix = 0; ix < polyOrd->nX + 1; ix++) {
	for (int iy = 0; iy < polyOrd->nY + 1; iy++) {
	  fprintf (stdout, "%18.12e ", polyOrd->coeff[ix][iy]);
	}
	fprintf (stdout, "\n");
      }
      fprintf (stdout, "\n");

      psVector *P = psPolynomial2DEvalVector (polyOrd, x, y);

      FILE *g1 = fopen ("ord.dat", "w");
      for (int i = 0; i < x->n; i++) {
	fprintf (g1, "%lf %lf %lf %lf\n", x->data.F64[i], y->data.F64[i], p->data.F64[i], P->data.F64[i]);
      }
      fclose (g1);
    }

    // Fit a chebychev polynomial
    { 
      psPolynomial2D *polyCheb = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, Nx, Ny);

      int result = psVectorFitPolynomial2D(polyCheb, NULL, 0, p, NULL, x, y);

      fprintf (stdout, " ---- Cheb ---- \n");

      for (int ix = 0; ix < polyCheb->nX + 1; ix++) {
	for (int iy = 0; iy < polyCheb->nY + 1; iy++) {
	  fprintf (stdout, "%18.12e ", polyCheb->coeff[ix][iy]);
	}
	fprintf (stdout, "\n");
      }

      psVector *P = psPolynomial2DEvalVector (polyCheb, x, y);

      FILE *g1 = fopen ("cheb.dat", "w");
      for (int i = 0; i < x->n; i++) {
	fprintf (g1, "%lf %lf %lf %lf\n", x->data.F64[i], y->data.F64[i], p->data.F64[i], P->data.F64[i]);
      }
      fclose (g1);
    }
}
/// gcc -o tap_psPolyFit2DCheb tap_psPolyFit2DCheb.c -lm -lpslib -I /home/real/eugene/src/psconfig/ipp-trunk-20210518.lin64/include/pslib -L /home/real/eugene/src/psconfig/ipp-trunk-20210518.lin64/lib
