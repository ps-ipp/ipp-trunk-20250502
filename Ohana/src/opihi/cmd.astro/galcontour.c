# include "astro.h"

XXX probably not a great algorithm...


int galcontour (int argc, char **argv) {
  
  int i, j, Nx, Ny, N, Nsec, NELEMENTS, Npt;
  float *in;
  double theta, dtheta, x, y, Xo, Yo;
  Buffer *buf;
  Vector *xvec, *yvec, *tvec;
  char name[128];

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: galcontour (buffer) (Xo) (Yo) (flux)\n");
    gprint (GP_ERR, "  generate contour points (Cx) (Cy) (Ct)\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];
  
  Xo = atof(argv[2]);
  Yo = atof(argv[3]);
  flux = atof(argv[4]);

  if ((xvec = SelectVector ("Cx", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector ("Cy", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((tvec = SelectVector ("Ct", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  N = 0;
  NELEMENTS = 100;
  ResetVector (xvec, OPIHI_FLT, NELEMENTS);
  ResetVector (yvec, OPIHI_FLT, NELEMENTS);
  ResetVector (tvec, OPIHI_FLT, NELEMENTS);

  in = (float *) buf[0].matrix.buffer;

  // find the flux transition for each row until not found

  // first go up:
  for (j = Yo; j < Ny; j++) {
    
    // first go left
    for (i = Xo; i >= 0; i--) {
      F = in[i + j*Nx];
      if (F < flux) {
	// interpolate to flux value between f[i,j] and f[i+1,j]
      }
    }

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
