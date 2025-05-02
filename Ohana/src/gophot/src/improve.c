# include "gophot.h"

# define ADD +1
# define SUB -1

improve (int last) {

  float err[NPMAX], star[NPMAX], sky, dx, dy, starchi;
  int i, k, ix, iy, jmtype, niter, nfit;
  bool skip, snok;

  /* imtype may be 1-9 for regular objects, 101-109 for fixed objects */

  for (i = 0; i < nstot; i++) {

    /* set up flags for fixed objects */
    fixxy = fixpos && (imtype[i] >= 100);
    jmtype = fixxy ? imtype[i] - 100 : imtype[i];
    /* inverse: imtype[i] = fixxy ? jmtype + 100 : jmtype; */

    if (jmtype == 0) continue;
    if (jmtype == 6) continue;
    if (jmtype == 16) continue;
    if (jmtype == 8) continue;
    if (jmtype == 18) continue;

    /* if (jmtype == 3) continue; */
    if (jmtype == 2) continue;
    if (jmtype == 12) continue;
    if (jmtype == 10) continue;
	   
    /* add star back to frame, get sky, get subraster */
    addstar (starpar[i], ADD, jmtype);
    sky = guess3 (star, starpar[i], &ix, &iy);
    mprint (3, " improving star %d at %d, %d\n", i, ix, iy);
    nrect[1] = irect[1];
    nrect[2] = irect[2];
    fillerup (ix, iy, FALSE);  /* we don't use the center information here */

    /* skip star if off picture, has negative flux, or if s/n too low for non-fixed objects */
    skip = offpic (star, ix, iy, &dx, &dy);
    if (star[1] <= 0) {
      skip = TRUE;
      starpar[i][1] = 0;
    }
    if (!skip && !fixxy) {
      snok = transmask (ix,iy,sky);
      skip = skip && !snok;
    }
    if (skip) {
      jmtype = 6;
      mprint (3, " deactivating star %d at %d, %d\n", i, ix, iy);
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }

    /* fixed objects only vary x,y */
    if (fixxy) {
      nfit = NFIT0;
      niter = 2;
    } else {
      nfit = NFIT1;
      niter = nit;
    }

    filladjust (star); 

    /* fit for magnitudes -- 4 param fit (or 2 for fixed obj) (initial guesses in are the previous fits) */
    starchi = chisq (onestar, xs, ys, zs, dzs, npt, star, err, nfit, acc, parlim, niter);

    /* set non-converge objects to type 4, don't bother with other calcs */
    if (!finite(starchi)) {
      if (jmtype != 3) jmtype = 4;
      for (k = 0; k < NPMAX; k++) galpar[i][k] = starpar[i][k];
      for (k = 0; k < NPMAX; k++) shadow[i][k] = starpar[i][k];
      apple[i][4] = MIN (1.086*sqrt(err[1])/star[1], 1);
      addstar (starpar[i], SUB, jmtype);
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }

    /* things which get negative flux, turn in to type 6 and skip */
    if (star[1] <= 0) {
      starpar[i][1] = 0;
      jmtype = 6;
      mprint (3, " deactivating star %d at %d, %d\n", i, ix, iy);
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }

    /* this section applies to type 1 3 5 7 */
    if ((jmtype != 2) && (jmtype != 10)) {
      /* store the fitted values */
      parupd (star, starpar[i], ix, iy);
    }

    if ((jmtype != 2) && (jmtype != 3) && toofaint(starpar[i], err)) jmtype = 7;
	   
    apple[i][4] = MIN (1, 1.086*sqrt(err[1])/star[1]);

    if (last) {
      nrect[1] = arect[1];
      nrect[2] = arect[2];
      fillerup (ix, iy, FALSE);
      impaper2 (i);
    }
	      
    addstar (starpar[i], SUB, jmtype);
    imtype[i] = fixxy ? jmtype + 100 : jmtype;

  }

  return (0);

}

/* this function uses C 0,N-1 for a[], fa[] */
