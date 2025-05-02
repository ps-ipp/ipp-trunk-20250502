# include "gophot.h"

float chisq (function, ix, iy, z, dz, npts, pars, dpars, npars, Acc, Parlim, niter) 
     float (function)(int, int, float *, float *);
     int *ix;
     int *iy;
     float *z, *dz;
     float *pars, *dpars, *Acc, *Parlim;
     int npts, npars, niter;
{

  float **covmatr, **tmpmatr;
  float *v, *tmpvec, *fpars;
  int *indx;
  bool conv, marq, limit, islimit;
  int i, j, k, jj, kk;
  float ifact, fact, chinew, chiold, f, dz1, fakk, d, tmpval, value;
  float perdeg, save;

  if (npars >= NPMAX) return (MAGIC);

  ALLOCATE (v, float, npars);
  ALLOCATE (indx, int, npars);
  ALLOCATE (fpars, float, NPMAX);
  ALLOCATE (tmpvec, float, npars);
  ALLOCATE (covmatr, float *, npars);
  ALLOCATE (tmpmatr, float *, npars);
  for (i = 0; i < npars; i++) {
    ALLOCATE (covmatr[i], float, npars);
    ALLOCATE (tmpmatr[i], float, npars + 1);
  }

  conv = FALSE;
  limit = FALSE;

  /* check if parameters exceeds limits at the start. */
  for (j = 0; j < npars; j++) {
    if (Parlim[j] < 0.0) {
      limit = fabs(pars[j]) > fabs(Parlim[j]);
      if (limit) mprint (4, "self-deception has occured: initial limits\n");
    }
  }

  /*
  for (j = 0; j < npars; j++) {
    mprint (3, "%d %f %f\n", j, pars[j], fpars[j]);
  }
  */

  ifact = 0;

  for (i = 0; (i < niter) && !conv && !limit; i++) {
    chinew = 0.0;
    for (j = 0; j < npars; j++) {
      for (kk = 0; kk < npars + 1; kk++) {
	tmpmatr[j][kk] = 0;
      }
    }

    for (j = 0; j < npts; j++) {
      f = function (ix[j], iy[j], pars, fpars) - z[j];
      /* fprintf (stderr, "%d  %d %d  %f %f %f\n", j, ix[j], iy[j], f, z[j], dz[j]); */
      dz1 = 1.0 / dz[j];
      chinew += SQ(f)*dz1;
      for (kk = 0; kk < npars; kk++) {
	if (fabs(fpars[kk]) > 1e-12) {
	  fakk = fpars[kk]*dz1;
	  tmpmatr[kk][npars] += fakk*f;
	  for (jj = 0; jj <= kk; jj++) {
	    if (fabs(fpars[kk]) > 1e-12) tmpmatr[kk][jj] += fakk*fpars[jj];	
	  }
	}
      }
    } 

    chiold = chinew;
    marq = FALSE;
    for (k = 1; (k <= 10) && !marq && !limit; k++) {
      conv = (k == 1);
      fact = (k == 1) ? 0.0 : pow (2.0, ifact);
      for (j = 0; j < npars; j++) {
	for (jj = 0; jj < j; jj++) {
	  covmatr[j][jj] = tmpmatr[j][jj];
	  covmatr[jj][j] = tmpmatr[j][jj];
	}
	covmatr[j][j] = (1+fact)*tmpmatr[j][j];
	v[j] = tmpmatr[j][npars];
      }
      ludcmp (covmatr, npars, indx, &d);

      /* if d = 0, the matrix was singular; no convergence. */
      if (d == 0) {
	mprint (4, "singular matrix!\n");
	/* need to free arrays */
	return (MAGIC);
      }
      lubksb (covmatr, npars, indx, v);

      /* 
	 check if change in parameters exceeds limits.  if Parlim(j) > 0, then
	 consider fractional changes.  if Parlim(j) < 0, consider absolute
	 changes.  if Parlim(j) = 0, ignore this test.
      */
      for (j = 0; j < npars; j++) {
	pars[j] -= chipar*v[j];
	if (Parlim[j] > 0.0) {
	  tmpval = fabs (v[j]/pars[j]);
	  islimit = (tmpval > Parlim[j]);
	  limit = limit || islimit;
	  if (islimit) mprint (4, "self-deception has occured: frac limits: %d  %f %f %f\n", j, pars[j], v[j], Parlim[j]);
	}
	if (Parlim[j] < 0.0) {
	  islimit = (fabs (pars[j]) > fabs (Parlim[j]));
	  limit = limit || islimit;
	  if (islimit) mprint (4, "self-deception has occured: abs limits: %d  %f %f %f\n", j, pars[j], v[j], Parlim[j]);
	}
	/* check convergence */
	if (Acc[j] > 0) {
	  tmpval = fabs (v[j]/pars[j]);
	  conv = conv && (tmpval <= Acc[j]);
	} else {
	  conv = conv && (fabs (v[j]) <= fabs (Acc[j]));
	}
      }

      if (conv) {
	marq = TRUE;
      } else {
	if (!limit) {
	  chinew = 0.0;
	  for (j = 0; j < npts; j++) {
	    f = function(ix[j], iy[j], pars, (float *) NULL) - z[j];
	    chinew += SQ(f)/dz[j];
	  }
	  if (k == 2) ifact --;
	  if (chinew < 1.0001*chiold) {
	    marq = TRUE;
	  } else {
	    if (k == 2) ifact += 2;
	    if (k > 2)  ifact ++;
	    if (ifact > 10) goto escape;
	    for (j = 0; j < npars; j++) pars[j] += chipar*v[j];
	  }
	}
      }

      mprint (4, "%d, %d, ", i, k);
      for (kk = 0; kk < npars; kk++) mprint (4, "%f ", pars[kk]);
      mprint (4, "  %f\n", chinew);

    }
  }
  escape:
      
  if (!limit) {
    for (j = 0; j < npars; j++) {
      for (i = 0; i < npars; i++) tmpvec[i] = 0;
      tmpvec[j] = 1;
      lubksb (covmatr, npars, indx, tmpvec);
      for (i = 0; i < npars; i++) tmpmatr[i][j] = tmpvec[i];
    }
  }
      
  if (conv && !limit) {
    perdeg = sqrt (chinew / MAX (npts - npars, 1));
    for (i = 0; i < npars; i++) {
      if (tmpmatr[i][i] > 0) {
	save = sqrt (tmpmatr[i][i]);
      } else {
	mprint (4, "trouble: negative autovariance for tmpmatr[%d][%d] = %f\n", i, i, tmpmatr[i][i]);
	save = 1e10;
      }
      for (j = 0; j < npars; j++) {
	covmatr[i][j] = tmpmatr[i][j]/save;
	covmatr[j][i] = tmpmatr[j][i]/save;
      }
      covmatr[i][i] = save*perdeg;
    }
    value = chiold;
  } else {
    value = MAGIC;
  }
  
  for (i = 0; i < npars; i++) dpars[i] = SQ(covmatr[i][i]);

  for (kk = 0; kk < npars; kk++) mprint (4, "%f ", pars[kk]);
  mprint (4, "     %f\n", chiold);
  
  for (i = 0; i < npars; i++) {
    free (covmatr[i]);
    free (tmpmatr[i]);
  }
  free (v);
  free (indx);
  free (fpars);
  free (tmpvec);
  free (covmatr);
  free (tmpmatr);

  return (value);

}
