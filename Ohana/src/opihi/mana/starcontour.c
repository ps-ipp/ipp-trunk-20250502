# include "mana.h"

int starcontour (int argc, char **argv) {
  
  int x, y, xs, xp, yp;
  int Nx, Ny, N, Npts, min, max;
  float *v;
  float zt, zo, xmin, xmax;
  Vector *vecx, *vecy;
  Buffer *buf;

  if (argc < 5) goto usage;

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  xp = atof (argv[2]);
  yp = atof (argv[3]);
  zo = atof (argv[4]);

  if ((vecx = SelectVector ("xo", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector ("yo", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  N = 0;
  Npts = 100;
  ResetVector (vecx, OPIHI_FLT, Npts);
  ResetVector (vecy, OPIHI_FLT, Npts);

  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];
  v = (float *)buf[0].matrix.buffer;

  /* find transition below (limit range?) */
  xmin = xmax = 0;
  xs = xp;
  for (y = yp; (y >= 0) && (v[xs + y*Nx] > zo); y--) {
    /* find transition below (limit range?) */
    min = max = FALSE;
    for (x = xs; (x >= 0) && !min; x--) {
      min = FALSE;
      zt = v[x + y*Nx];
      if (zt < zo) {
	min = TRUE;
	xmin = x + (zo - zt)/(v[x + 1 + y*Nx] - zt);
	vecx[0].elements.Flt[N] = xmin;
	vecy[0].elements.Flt[N] = y;
	N ++;
	if (N >= Npts) {
	  Npts += 100;
	  REALLOCATE (vecx[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	  REALLOCATE (vecy[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	}
      }
      /* ignore edge cases? */
    }
    /* find transition above (limit range?) */
    for (x = xs; (x < Nx) && !max; x++) {
      max = FALSE;
      zt = v[x + y*Nx];
      if (zt < zo) {
	max = TRUE;
	xmax = x - (zo - zt)/(v[x - 1 + y*Nx] - zt);
	vecx[0].elements.Flt[N] = xmax;
	vecy[0].elements.Flt[N] = y;
	N ++;
	if (N >= Npts) {
	  Npts += 100;
	  REALLOCATE (vecx[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	  REALLOCATE (vecy[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	}
      }
      /* ignore edge cases? */
    }
    if (min && max) {
      xs = 0.5*(xmin + xmax);
    }
  }

  /* find transition above (limit range?) */
  xs = xp;
  for (y = yp; (y < Ny) && (v[xs + y*Nx] > zo); y++) {
    /* find transition below (limit range?) */
    min = max = FALSE;
    for (x = xs; (x >= 0) && !min; x--) {
      min = FALSE;
      zt = v[x + y*Nx];
      if (zt < zo) {
	min = TRUE;
	xmin = x + (zo - zt)/(v[x + 1 + y*Nx] - zt);
	vecx[0].elements.Flt[N] = xmin;
	vecy[0].elements.Flt[N] = y;
	N ++;
	if (N >= Npts) {
	  Npts += 100;
	  REALLOCATE (vecx[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	  REALLOCATE (vecy[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	}
      }
      /* ignore edge cases? */
    }
    /* find transition above (limit range?) */
    for (x = xs; (x < Nx) && !max; x++) {
      max = FALSE;
      zt = v[x + y*Nx];
      if (zt < zo) {
	max = TRUE;
	xmax = x - (zo - zt)/(v[x - 1 + y*Nx] - zt);
	vecx[0].elements.Flt[N] = xmax;
	vecy[0].elements.Flt[N] = y;
	N ++;
	if (N >= Npts) {
	  Npts += 100;
	  REALLOCATE (vecx[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	  REALLOCATE (vecy[0].elements.Flt, opihi_flt, MAX (Npts, 1));
	}
      }
      /* ignore edge cases? */
    }
    if (min && max) {
      xs = 0.5*(xmin + xmax);
    }
  }
  vecx[0].Nelements = N;
  vecy[0].Nelements = N;

  return (TRUE);

 usage:
  gprint (GP_ERR, "starcontour (buffer) (xpeak) (ypeak) (level)\n");
  return (FALSE);
}

