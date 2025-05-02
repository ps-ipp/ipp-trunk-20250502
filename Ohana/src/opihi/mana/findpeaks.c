# include "mana.h"

int findpeaks (int argc, char **argv) {
  
  int i, j, n, N, Nx, Ny;
  int xo, yo;
  int Npeak, NPEAK, Npeaks;
  float *v;
  int *peaks, *keep, *xp, *yp, *zp;
  float threshold, vt, vo;
  Vector *vecx, *vecy, *vecz;
  Buffer *buf;

  if (argc < 3) goto usage;

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  threshold = atof (argv[2]);

  if ((vecx = SelectVector ("xp", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector ("yp", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecz = SelectVector ("zp", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];

  Npeak = 0;
  NPEAK = Nx;
  ALLOCATE (xp, int, NPEAK);
  ALLOCATE (yp, int, NPEAK);
  ALLOCATE (zp, int, NPEAK);

  /* find peaks for each row */
  v = (float *) buf[0].matrix.buffer;
  for (j = 0; j < Ny; j++) {
    peaks = findrowpeaks (&v[j*Nx], Nx, threshold, &Npeaks);

    if (Npeak + Npeaks >= NPEAK) {
      NPEAK = Npeak + Npeaks + Nx;
      REALLOCATE (xp, int, NPEAK);
      REALLOCATE (yp, int, NPEAK);
      REALLOCATE (zp, int, NPEAK);
    }
    for (i = 0; i < Npeaks; i++) {
      xp[Npeak + i] = peaks[i];
      yp[Npeak + i] = j;
      zp[Npeak + i] = v[peaks[i] + j*Nx];
    }
    Npeak += Npeaks;
    free (peaks);
  }
  
  /* identify non-local peaks */
  ALLOCATE (keep, int, MAX (Npeak, 1));
  v = (float *) buf[0].matrix.buffer;
  for (n = 0; n < Npeak; n++) {
    xo = xp[n];
    yo = yp[n];
    vo = v[xo + yo*Nx];
    keep[n] = TRUE;
    for (i = xo - 1; i <= xo + 1; i++) {
      if (i < 0) continue;
      if (i >= Nx) continue;
      for (j = yo - 1; j <= yo + 1; j++) {
	if ((i == xo) && (j == yo)) continue;
	if (j < 0) continue;
	if (j >= Ny) continue;
	vt = v[i + j*Nx];
	if (vt > vo) {
	  keep[n] = FALSE;
	  goto next_peak;
	}
      }
    }
  next_peak:
    continue;
  }

  ResetVector (vecx, OPIHI_FLT, Npeak);
  ResetVector (vecy, OPIHI_FLT, Npeak);
  ResetVector (vecz, OPIHI_FLT, Npeak);

  /* eliminate non-local peaks */
  for (N = n = 0; n < Npeak; n++) {
    if (!keep[n]) continue;
    vecx[0].elements.Flt[N] = xp[n];
    vecy[0].elements.Flt[N] = yp[n];
    vecz[0].elements.Flt[N] = zp[n];
    N ++;
  }
  free (xp);
  free (yp);
  free (zp);
  free (keep);

  REALLOCATE (vecx[0].elements.Flt, opihi_flt, MAX (N, 1));
  REALLOCATE (vecy[0].elements.Flt, opihi_flt, MAX (N, 1));
  REALLOCATE (vecz[0].elements.Flt, opihi_flt, MAX (N, 1));
  vecx[0].Nelements = vecy[0].Nelements = vecz[0].Nelements = N;

  return (TRUE);

 usage:
  gprint (GP_ERR, "findpeaks (buffer) (threshold)\n");
  return (FALSE);
}


