# include "gastro2.h"

# define A00 +500.00
# define A10 +4.00
# define A01 +1.00
# define A20 -0.001
# define A11 +0.000
# define A02 -0.001
# define A30 +0.00002
# define A21 +0.00001
# define A12 -0.00001
# define A03 -0.00002

# define B00 400.00
# define B10  -1.00
# define B01  4.00
# define B20 -0.001
# define B11  0.000
# define B02 -0.001
# define B30 -0.00002
# define B21 -0.00001
# define B12 +0.00001
# define B03 +0.00002

int main (int argc, char **argv) {

  /* generate a set of fake data and send to fitter */
  int i, N, order;
  double x, y, z;
  double L[50000], M[50000], X[50000], Y[50000];
  Coords coords;
  FILE *f;

  if (argc != 2) {
    fprintf (stderr, "USAGE: coordtest (order)\n");
    exit (2);
  }

  order = atoi (argv[1]);
  fit_init (order);
  N = 0;

  switch (order) {
    case 1:
      for (x = -10.0; x < 10.1; x += 0.1) {
	for (y = -10.0; y < 10.1; y += 0.1) {
      
	  L[N] = A00 + A10*x + A01*y;
	  M[N] = B00 + B10*x + B01*y;
	  X[N] = x;
	  Y[N] = y;
	  fit_add (x, y, L[N], M[N], 1.0);
	  N++;
	}
      }
      break;
    case 2:
      for (x = -10.0; x < 10.1; x += 0.1) {
	for (y = -10.0; y < 10.1; y += 0.1) {
      
	  L[N] = A00 + A10*x + A01*y + A20*x*x + A11*x*y + A02*y*y;
	  M[N] = B00 + B10*x + B01*y + B20*x*x + B11*x*y + B02*y*y;
	  X[N] = x;
	  Y[N] = y;
	  fit_add (x, y, L[N], M[N], 1.0);
	  N++;
	}
      }
      break;
    case 3:
      for (x = -10.0; x < 10.1; x += 0.1) {
	for (y = -10.0; y < 10.1; y += 0.1) {
      
	  L[N] = A00 + A10*x + A01*y + A20*x*x + A11*x*y + A02*y*y + A30*x*x*x + A21*x*x*y + A12*x*y*y + A03*y*y*y;
	  M[N] = B00 + B10*x + B01*y + B20*x*x + B11*x*y + B02*y*y + B30*x*x*x + B21*x*x*y + B12*x*y*y + B03*y*y*y;
	  X[N] = x;
	  Y[N] = y;
	  fit_add (x, y, L[N], M[N], 1.0);
	  N++;
	}
      }
      break;
  }

  fit_eval ();

  strcpy (coords.ctype, "DEC--PLY");
  coords.crval1 = 0.0;
  coords.crval2 = 0.0;

  fit_adjust (&coords);

  fprintf (stderr, "CTYPE: %s\n", coords.ctype);
  fprintf (stderr, "CRVAL: %f %f\n", coords.crval1, coords.crval2);
  fprintf (stderr, "CRPIX: %f %f\n", coords.crpix1, coords.crpix2);
  fprintf (stderr, "CDELT: %f %f\n", coords.cdelt1, coords.cdelt2);
  fprintf (stderr, "PC1_j: %f %f\n", coords.pc1_1,  coords.pc1_2);
  fprintf (stderr, "PC2_j: %f %f\n", coords.pc2_1,  coords.pc2_2);

  fprintf (stderr, "Npolyterms: %d\n", coords.Npolyterms);
  for (i = 0; i < 7; i++) {
    fprintf (stderr, "%f %f\n", coords.polyterms[i][0], coords.polyterms[i][1]);
  }

  f = fopen ("test.dat", "w");
  {
    double Lo, Mo, dL, dL2, dM, dM2;
    double Xo, Yo, dX, dX2, dY, dY2;
    double Lx, Mx;

    dL = dL2 = dM = dM2 = 0.0;
    dX = dX2 = dY = dY2 = 0.0;
    for (i = 0; i < N; i++) {
      XY_to_RD (&Lo, &Mo, X[i], Y[i], &coords);
      dL  += (L[i] - Lo);
      dM  += (M[i] - Mo);
      dL2 += SQ(L[i] - Lo);
      dM2 += SQ(M[i] - Mo);

      RD_to_XY (&Xo, &Yo, L[i], M[i], &coords);
      dX  += (X[i] - Xo);
      dY  += (Y[i] - Yo);
      dX2 += SQ(X[i] - Xo);
      dY2 += SQ(Y[i] - Yo);

      // fit_apply (&Lx, &Mx, X[i], Y[i]);
      // fprintf (stderr, "%f,%f -> %f,%f | %f,%f : %f, %f\n", X[i], Y[i], Lx, Mx, L[i], M[i], Lo, Mo);

      fprintf (f, "%f %f : %f %f :: %f %f : %f %f\n",
	       X[i], Y[i], L[i], M[i], Lo, Mo, Xo, Yo);
    }
    fclose (f);

    fprintf (stderr, "dL: %f\n", sqrt(fabs(dL2/N - SQ(dL/N))));
    fprintf (stderr, "dM: %f\n", sqrt(fabs(dM2/N - SQ(dM/N))));
    fprintf (stderr, "dX: %f\n", sqrt(fabs(dX2/N - SQ(dX/N))));
    fprintf (stderr, "dY: %f\n", sqrt(fabs(dY2/N - SQ(dY/N))));
  }

  exit (0);
}

