# include "gophot.h"

fillerup (int xo, int yo, int findstats) {

  float *bigval, *noiseval, ufactor2, snlim2;
  int i, j, ilo, ihi, jlo, jhi, ix, iy, idist, nsky, maxnpt, outer;
  int *x, *y;
  float *z, *d;
  float N, S, F, Sx, Sy, Sx2, Sy2, xv, yv;
  float value, SNR, sky;

  ufactor2 = SQ(ufactor);
  snlim2 = SQ(snlim);
  npt = 0;
  nsky = 0;

  ilo = MAX (xo - nrect[1]/2, 0);
  ihi = MIN (xo + nrect[1]/2, nfast-1);
  jlo = MAX (yo - nrect[2]/2, 0);
  jhi = MIN (yo + nrect[2]/2, nslow-1);
  
  outer = MIN (nrect[1]/3, nrect[2]/3);
  
  for (j = jlo; j <= jhi; j++) {
    bigval = &big[ilo + j*nfast];
    noiseval = &noise[ilo + j*nfast];
    for (i = ilo; i <= ihi; i++, bigval++, noiseval++) {
      if (!finite (*bigval)) continue;
      if (!finite (*noiseval)) continue;
      if (SQ(*bigval) < (*bigval + *noiseval) * snlim2) continue;
      if (*bigval < (*noiseval - rnoise)) continue;
      ix = i - xo;
      iy = j - yo;
      xs[npt] = ix;
      ys[npt] = iy;
      zs[npt] = *bigval/ufactor;
      dzs[npt] = (*bigval + *noiseval) / ufactor2;
      idist = MAX (abs(ix), abs(iy));
      if (idist > outer) {
	ts[nsky] = zs[npt];
	nsky ++;
      }
      npt ++;
    }
  }
  
  if (npt == 0) return (FALSE);
  if (!findstats) return (TRUE);

  if (nsky == 0) return (FALSE);

  fsort (ts, nsky);
  sky = ts[(int)(0.5*nsky)];
  sum2 = sky;

  z = zs;
  d = dzs;
  S = N = 0;
  for (i = 0; i < npt; i++, z++, d++) {
    if (*z < sky) continue;
    N += *d;
    S += *z - sky;
  }

  if (S < 0) return (FALSE);
  SNR = S*S / N;
  if (SNR < SQ(bumpcrit)) return (FALSE);

  if (SNR > 49) {
    x = xs;
    y = ys;
    z = zs;
    d = dzs;
    N = S = Sx = Sy = Sx2 = Sy2 = 0;
    for (i = 0; i < npt; i++, x++, y++, z++, d++) {
      idist = MAX (abs(*x), abs(*y));
      if (idist > outer) continue;
      if (*z < sky) continue;
      value = *z - sky;
      xv = *x * value;
      yv = *y * value;
      N += *d;
      S += value;
      Sx += xv;
      Sx2 += *x * xv;
      Sy += yv;
      Sy2 += *y * yv;
    }
    xmax = Sx / S;
    ymax = Sy / S;
    xmax2 = fabs(Sx2 / S - xmax*xmax);
    ymax2 = fabs(Sy2 / S - ymax*ymax);
    maxval = S / sqrt (xmax2*ymax2);
  } else {
    z = zs;
    maxval = *z;
    maxnpt = 0;
    for (i = 0; i < npt; i++, z++) {
      if (*z > maxval) {
	maxnpt = i;
	maxval = *z;
      }
    }
    xmax = xs[maxnpt];
    ymax = ys[maxnpt];
    xmax2 = ava[4];
    ymax2 = ava[6];
    maxval -= sum2; 
 }

  maxval *= ufactor;
  sum2 *= ufactor;

  if (!finite (maxval)) return (FALSE);
  return (TRUE);
  
}

/* adjust the errors by the deviation from the guess fit */
filladjust (float *star) {

  int i;
  float f, df;

  return (0);
  if (star[1] < cmax/(ufactor*10)) return (0); 

  for (i = 0; i < npt; i++) {

    f = onestar ((int)xs[i], (int)ys[i], star, (float *) NULL);
    
    df = 10 * fabs(f - zs[i]);
    dzs[i] += df/ufactor;
  }

}

/* check if weighted center is in OK location 
   weighted center is based on useable pixels, not
   on entire box */
centertest (float *star, int *x, int *y) {

  int i;
  float f, df, Sf, Sx, Sy, dx, dy;

  Sf = Sx = Sy = 0;

  for (i = 0; i < npt; i++) {

    f = onestar ((int)xs[i], (int)ys[i], star, (float *) NULL);
    
    Sf += f;
    Sx += f*xs[i];
    Sy += f*ys[i];

  }

  dx = star[2] - Sx/Sf;
  dy = star[3] - Sy/Sf;

  if ((fabs(dx) > 0.25*nrect[1]) || (fabs(dy) > 0.25*nrect[2])) {
    mprint (3, "center %f %f %f\n", dx, dy, Sf);
    return (TRUE);
  }

  return (FALSE);

}

