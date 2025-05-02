# include "gophot.h"

int isearch (int first) {
  
  int jrect[3];
  int i, j, nsprev, nfound, nnew, ngoodpts, ngoodmask, ngoodfit, nfit;
  float dummy[2*NPMAX], star[2*NPMAX], err[NPMAX], pixt2, bestsky;
  float tthresh, thresh2, highsky, highthresh, tmp;
  float sky, Chi, dx, dy, rdnoise2;
  float *Bval, *Nval;
  bool iscosmic, isbright, isfaint, hole;
  float get_mediansky ();
  FILE *f;

  bzero (dummy, NPMAX*sizeof(float));
  
  pixt2 = SQ(pixthresh);

  /* use jrect as temp in isearch.c */
  for (i = 1; i < 3; i++) {
    jrect[i] = (first) ? krect[i] : irect[i]; 
  }
	
  nsprev = nstot;
  nnew = 0;
  ngoodpts = 0;
  ngoodmask = 0;
  ngoodfit = 0;
  thresh2 = SQ(thresh);
  nfit = (first) ? NFIT2 : NFIT1;
  /* on first pass or so, we use 7 par fit, not 4 par fit */

  Bval = big;
  Nval = noise;
  for (i = 0; i < nslow; i++) {
    for (j = 0; j < nfast; j++, Bval++, Nval++) {

      /* occasionally update sky guess based on median sky image */
      if (!(j % nthpix)) {
	sky = get_mediansky (j, i); 
	tthresh = sky + thresh;
      }

      imtype[nstot] = 1;

      /* skip bad pixels */
      if (*Bval < tthresh) continue;
      if (!finite (*Nval)) continue;
      if (*Bval < *Nval - rnoise) continue;

      /* test for significant peak */
      if (!transmask (j, i, sky)) continue;
      nnew ++;

      /* nrect is passed globally to fillerup or through - rather obscure */ 
      nrect[1] = jrect[1];
      nrect[2] = jrect[2];

      /* fills the vectors xs, ys, zs, dzs */
      if (!fillerup (j, i, TRUE)) continue;
      if ((fabs(xmax) > 0.33*nrect[1]) || (fabs(ymax) > 0.33*nrect[2])) {
	mprint (3, "centroid moved: %d %d  %f %f\n", j, i, xmax, ymax);
      }

      tmp = enuff4*jrect[1]*jrect[2];
      if (npt < tmp) {
	mprint (3, "skipping: npt = %d\n", npt);
	continue;
      }
      ngoodpts ++;

      /* if sky guess has changed significantly, retest for significant peak */
      if (fabs(sum2 - sky) / maxval > 0.2) {
	if (!transmask (j, i, sum2)) {
	  mprint (3, "failed transmask on local sky, %f\n", sum2);
	  continue;
	}
      }
      ngoodmask ++;

      /* fill in guess for this object */
      if (first) {
	newguess (star, dummy, j, i); 
      } else {
	guess1 (star, dummy, j, i);
      }
      filladjust (star); 

      /* fit star using 4 parameterfit */
      Chi = chisq (onestar, xs, ys, zs, dzs, npt, star, err, nfit, acc, parlim, nit);

      /* if (finite(Chi) && ((err[2] > 0.1) || (err[3] > 0.1))) fprintf (stderr, "errors: %d %d  %f %f   %f %f   %f\n", j, i, err[2], err[3], err[4], err[6], Chi/npt); */
      if (!finite(Chi)) {
	mprint (3, "failed to converge:  no entry in starlist\n");
	continue;
      }
      if ((fabs(star[2]) > 0.33*nrect[1]) || (fabs(star[3]) > 0.33*nrect[2])) {
	mprint (3, "fit center moved: %d %d  %f %f\n", j, i, star[2], star[3]);
	continue;
      }
      if (centertest (star, xs, ys, npt)) {
	mprint (3, "star %d at %d %d moved\n", nstot, j, i);
	continue;
      }
      if (offpic (star, j, i, &dx, &dy)) {
	mprint (3, "fitted star off image\n");
	continue;
      }

      /* save fit parameters */
      parupd (star, starpar[nstot], j, i);
      parupd (star, shadow[nstot], j, i);
      rchisq[nstot] = Chi / npt;
      apple[nstot][4] = MIN (1, 1.086*sqrt(err[1])/star[1]);
      mprint (2, "star %d: %f, %f  peak: %f chisq: %f\n", nstot, starpar[nstot][2], starpar[nstot][3], maxval, rchisq[nstot]);
      ngoodfit ++;

      if (verybright (starpar[nstot])) {
	imtype[nstot] = 10;
	goto finish;
      }	
      if (cosmic (starpar[nstot])) {
	imtype[nstot] = 8;
	hole = oblit (starpar[nstot]);
	goto finish_noadd;
      }	
      if (toofaint (starpar[nstot], err)) {
	imtype[nstot] = 7;
	mprint (3, "faint imtype %d\n", imtype[nstot]);
	goto finish;
      }

    finish:
      addstar (starpar[nstot], SUB, imtype[nstot]);

    finish_noadd:
      nstot ++;
      if (nstot >= NSMAX) {
	mprint (0, "too many stars!\n");
	return (0);
      }

    }
  }
  nfound = nstot - nsprev;
  mprint (0, "stars found %d, stars tested %d\n", nfound, nnew);
  mprint (0, "ngoodpts: %d, ngoodfit: %d, ngoodmask: %d\n", ngoodpts, ngoodfit, ngoodmask);

  return (nfound);
}


/* this function uses C 0,N-1 for a[], fa[] */

      /* before star subtraction:

       Nval contains just sq(RN) (e) 
       Bval contains counts (e)
       dBval = sqrt(Bval + Nval)

       after star subtraction:
       
       Nval contains fit + sq(RN)
       Bval contains (obs-fit)
       dBval = sqrt(Bval + Nval) [Bval' + Nval' = Bval + Nval] 
      */       

      /* note that the coordinates used in the fit are relative to j, i
	 and are adjusted back in parupd */

