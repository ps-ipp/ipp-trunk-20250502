# include "relastro.h"
# define NPTS 64
# define J2000 2451545.       /* Julian date at standard epoch */
# define T1970 2440587.500000 /* JD at UNIX ref time */

int main (int argc, char **argv) {
   
  int i, Npts;
  char line[1024];
  double R[NPTS], D[NPTS], Time[NPTS], JD[NPTS];
  double X[NPTS], Y[NPTS], dX[NPTS], dY[NPTS], pX[NPTS], pY[NPTS];
  double Tref[NPTS], Tjyrs[NPTS], TrefS, TjyrsS, TrefMean, Ro, Do;
  Coords coords;
  PMFit fitPAR;

  if (argc != 2) {
    fprintf (stderr, "USAGE: testparallax (file.dat)\n");
    exit (1);
  }

  // test parallax program
  FILE *f = fopen (argv[1], "r");
  if (!f) { fprintf (stderr, "failed to open %s\n", argv[1]); exit (2); }

  // XXX uncomment to skip first line scan_line(f, line);
    
  Npts = 0;
  for (i = 0; scan_line(f, line) != EOF; i++) {
    if (Npts == NPTS) {
      fprintf (stderr, "too many point: use dynamic alloc\n");
      exit (2);
    }
    if (line[0] == '#') continue;
    dparse (&R[Npts],    1, line);
    dparse (&D[Npts],    2, line);
    dparse (&JD[Npts],   3, line);
    dparse (&Time[Npts], 3, line);

    dparse (&dX[Npts],   4, line);
    dparse (&dY[Npts],   4, line);

    // trent's file
    // dparse (&JD[Npts],   1, line);
    // dparse (&Time[Npts], 2, line);
    // dparse (&R[Npts],    3, line);
    // dparse (&D[Npts],    4, line);
    Npts ++;
  }

  /* project coordinates to a plane centered on the object with units of arcsec */
  InitCoords (&coords, "DEC--SIN");
  coords.cdelt1 = coords.cdelt2 = 1.0 / 3600.0;

  // use one point as a local reference
  coords.crval1 = R[0];
  coords.crval2 = D[0];

  TrefS = TjyrsS = 0.0;

  // project to local coords
  for (i = 0; i < Npts; i++) {
    RD_to_XY (&X[i], &Y[i], R[i], D[i], &coords);

    Tjyrs[i] = Time[i] / 365.25;
    Tref[i] = Time[i] / 365.25;
    // Tunix[i] = (JD[i] - T1970) / 365.25; // time relative to T1970 in years
    // Tjyrs[i] = (JD[i] - J2000) / 365.25; // time relative to J2000 in years

    TrefS   += Tref[i];
    TjyrsS   += Tjyrs[i];

    // nominal for PS1
    // dX[i] = 0.020;
    // dY[i] = 0.020;
  }
  TrefMean = TrefS / Npts;
  // double TjyrsMean = TjyrsS / Npts;

  for (i = 0; i < Npts; i++) {
    Tref[i] -= TrefMean;
    ParFactor (&pX[i], &pY[i], R[i], D[i], Tjyrs[i], 0.0);
    fprintf (stderr, "%f %f : %f %f : %f %f : %f %f\n", R[i], D[i], X[i], Y[i], Tref[i], Tjyrs[i], pX[i], pY[i]);
  }

  // run fitter
  // FitPMandPar (&fitPAR, X, dX, Y, dY, Tref, pX, pY, Npts, TRUE);
  FitPMandPar (&fitPAR, X, dX, Y, dY, Tjyrs, pX, pY, Npts, TRUE);
  XY_to_RD (&Ro, &Do, fitPAR.Ro, fitPAR.Do, &coords);

  fprintf (stderr, "Rx   : %f\n", fitPAR.Ro);
  fprintf (stderr, "Dx   : %f\n", fitPAR.Do);
  fprintf (stderr, "Ro   : %f\n", Ro);
  fprintf (stderr, "Do   : %f\n", Do);
  fprintf (stderr, "dRo  : %f\n", fitPAR.dRo);
  fprintf (stderr, "dDo  : %f\n", fitPAR.dDo);
  fprintf (stderr, "uR   : %f\n", fitPAR.uR);
  fprintf (stderr, "uD   : %f\n", fitPAR.uD);
  fprintf (stderr, "duR  : %f\n", fitPAR.duR);
  fprintf (stderr, "duD  : %f\n", fitPAR.duD);
  fprintf (stderr, "p    : %f\n", fitPAR.p);
  fprintf (stderr, "dp   : %f\n", fitPAR.dp);
  fprintf (stderr, "uTot : %f\n", hypot(fitPAR.uR,fitPAR.uD));
  fprintf (stderr, "PA   : %f\n", DEG_RAD*atan2(fitPAR.uR,fitPAR.uD) + 360.0);
  
  exit (0);
}

