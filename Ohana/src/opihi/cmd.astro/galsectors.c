# include "astro.h"

int galsectors (int argc, char **argv) {
  
  int i, j, Nx, Ny, N, Nsec, NELEMENTS, Npt;
  float *in;
  double theta, dtheta, x, y, Xo, Yo;
  Buffer *buf;
  Vector **fvec, **rvec, *tvec, *Rv, *Tv, *Fv;
  char name[128];

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: galsectors (buffer) (Xo) (Yo) Nsec\n");
    gprint (GP_ERR, "  generates Nsec sectors around the circle; the first is centered on 0.0 degrees\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];
  
  Xo = atof(argv[2]);
  Yo = atof(argv[3]);
  Nsec = atof(argv[4]);

  // for a test, generate R, theta vectors for the whole image:
  if ((Rv = SelectVector ("Rvec", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Tv = SelectVector ("Tvec", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((Fv = SelectVector ("Fvec", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  ResetVector (Rv, OPIHI_FLT, Nx*Ny);
  ResetVector (Tv, OPIHI_FLT, Nx*Ny);
  ResetVector (Fv, OPIHI_FLT, Nx*Ny);

  in = (float *) buf[0].matrix.buffer;
  Npt = 0;
  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++, in++) {
      x = i - Xo;
      y = j - Yo;
      Rv[0].elements.Flt[Npt] = hypot(x, y);
      Tv[0].elements.Flt[Npt] = DEG_RAD*atan2(y, x);
      Fv[0].elements.Flt[Npt] = *in;
      Npt ++;
    }
  }
  return (TRUE);

  dtheta = 360.0 / Nsec;
  // sector i ranges from (i-0.5)*dtheta to (i+0.5)*dtheta
  // i = theta / dtheta + 0.5;

  ALLOCATE (fvec, Vector *, Nsec);
  ALLOCATE (rvec, Vector *, Nsec);
  if ((tvec = SelectVector ("theta", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  ResetVector (tvec, OPIHI_FLT, Nsec);

  // generate the output vector names
  NELEMENTS = 1000;
  for (i = 0; i < Nsec; i++) {
    sprintf (name, "flux_%d", i);
    if ((fvec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) return (FALSE);
    sprintf (name, "rad_%d", i);
    if ((rvec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) return (FALSE);
    ResetVector (fvec[i], OPIHI_FLT, NELEMENTS);
    ResetVector (rvec[i], OPIHI_FLT, NELEMENTS);
    fvec[i][0].Nelements = 0;
    rvec[i][0].Nelements = 0;
    tvec[0].elements.Flt[i] = i*dtheta;
  }

  in = (float *) buf[0].matrix.buffer;
  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++, in++) {
      x = i - Xo;
      y = j - Yo;
      theta = DEG_RAD*atan2(y, x);
      if (theta < -0.5*dtheta) theta += 360.0; // need to allow -0.5dtheta for first bin
      if (theta > 360 - 0.5*dtheta) theta -= 360.0; // need to allow -0.5dtheta for first bin
      N = (int)(theta / dtheta + 0.5);
      if (N < 0) {
	fprintf (stderr, "?");
	continue;
      }
      if (N >= Nsec) {
	fprintf (stderr, "!");
	continue;
      }
      
      fvec[N][0].elements.Flt[fvec[N][0].Nelements] = *in;
      rvec[N][0].elements.Flt[rvec[N][0].Nelements] = hypot(x, y);
      fvec[N][0].Nelements ++;
      rvec[N][0].Nelements ++;

      *in = N;

      if (fvec[N][0].Nelements >= NELEMENTS) {
	NELEMENTS += 1000;
	for (N = 0; N < Nsec; N++) {
	  REALLOCATE (fvec[N][0].elements.Flt, opihi_flt, NELEMENTS);
	  REALLOCATE (rvec[N][0].elements.Flt, opihi_flt, NELEMENTS);
	}
      }
    }
  }
  return (TRUE);
}
