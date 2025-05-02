# include "gophot.h"

# define ZERO_SHADOW {shadow[i][0] = 0; \
	              shadow[i][1] = 0; \
	              shadow[i][4] = 1; \
                      shadow[i][5] = 0; \
                      shadow[i][6] = 1; }

/* try 7 par fit for each object, decide on object types */
shape () {

  bool notnuff, gotfaint, verybig, offp, converge;

  /* watch twofpar - i think we only need 2 entries, for storage in this loop */
  float star[NPMAX], err[NPMAX], star1[NPMAX], star2[NPMAX];
  float sky, galchi, dx, dy, starchi;
  int i, j, n, k, ix, iy, jmtype, nsprev;

  test7 = TRUE;
  nsprev = nstot;

  for (i = 0; i < nsprev; i++) {
    mprint (3, " determining shape for object no. %d\n", i);

    gotfaint = FALSE;
    fixxy = imtype[i] >= 100;
    jmtype = (fixxy) ? imtype[i] - 100 : imtype[i];
	   
    switch (jmtype) {
    case 7:
      mprint (3, "too faint, skipping object %d at %f, %f\n", i, starpar[i][2], starpar[i][3]);
      continue;
      break;
    case 4:
    case 9:
      mprint (3, "nonconverge, skipping object %d at %f, %f\n", i, starpar[i][2], starpar[i][3]);
      continue;
      break;
    case 8:
    case 18:
    case 6:
    case 16:
      mprint (3, "poor obj, skipping object %d at %f, %f\n", i, starpar[i][2], starpar[i][3]);
      ZERO_SHADOW;
      continue;
      break;
    }

    if (shadow[i][0] == 0) {
      sky = guess2 (star, starpar[i], &ix, &iy);
    } else {
      sky = guess2 (star, shadow[i], &ix, &iy);
    }

    addstar (starpar[i], ADD, jmtype);

    nrect[1] = irect[1];
    nrect[2] = irect[2];
    fillerup (ix, iy, FALSE);
    notnuff = (npt < enuff7*irect[1]*irect[2]);

    if (notnuff) {
      mprint (3, "obj %d, npts %d, %d & %d - skipping star: not enough pixels for 7-parm fit \n", i, npt, ix, iy);
      if (jmtype != 2) jmtype = 5;
      if (jmtype == 12) beta4 = 0.01;
      addstar (starpar[i], SUB, jmtype);
      beta4 = 1.0;
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }

    if (fixxy) {
      gotfaint = !transmask (ix, iy, sky);
      if (gotfaint) { 
	if (jmtype != 3) jmtype = 7;
	mprint (3, "obj %d, npts %d, %d & %d - skipping star: too faint for 7 par fit\n", i, npt, ix, iy);
	addstar (starpar[i], SUB, jmtype);
	imtype[i] = fixxy ? jmtype + 100 : jmtype;
	continue;
      }
    }

    filladjust (star); 
    mprint (2, "obj %d, npts %d, %d & %d\n", i, npt, ix, iy);

    /* fit 'extended' to star (7 parameter fit) */
    galchi = chisq (onestar, xs, ys, zs, dzs, npt, star, err, NFIT2, acc, parlim, nit);
    /*
    if (finite(galchi) && ((err[4] > 0.25) || (err[6] > 0.25))) {
      fprintf (stderr, "7 errors: %d %d  %f %f   %f %f   %f\n", ix, iy, err[2], err[3], err[4], err[6], galchi/npt);
      fprintf (stderr, "                 %f %f   %f %f\n", star[2], star[3], star[4], star[6]);
    }
    */
    if (fabs(star[5]) >= 1.0 / sqrt(fabs(star[4]*star[6]))) {
      mprint (3, "hyperbola!  problem with %d at %f %f\n", i, starpar[i][2], starpar[i][3]);
      /* just a warning... */
    }
    if ((star[4] > SQ(0.25*nrect[1])) || (star[6] > SQ(0.25*nrect[1]))) {
     mprint (3, "warning - dubious fit for %d at %f %f\n", i, starpar[i][2], starpar[i][3]);
      /* just a warning... */
    } 
    if (!finite (galchi)) {
      /* shadow, starpar keep 4-par fit */
      mprint (3, "non-converge object %d at %f %f\n", i, starpar[i][2], starpar[i][3]);
      if (jmtype != 3) jmtype = 9;
      mprint (3, "obj %d, npts %d, %d & %d .... failed to converge\n", i, npt, ix, iy);
      addstar (starpar[i], SUB, jmtype);
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }
    
    /* is new fit position reasonable? */
    if (centertest (star, xs, ys, npt) || (fabs(star[2]) > 0.33*nrect[1]) || (fabs(star[3]) > 0.33*nrect[2])) {
      mprint (3, "fit center moved 2: %f %f  %f %f\n", starpar[i][2], starpar[i][3], star[2], star[3]);
      jmtype = 16;
      /* shadow, starpar keep 4-par fit */
      addstar (starpar[i], SUB, jmtype);
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }
      
    /* calculate error here for type 2, 10. redone in improve for 1, 3, 5, 7 */
    apple[i][4] = MIN (1, 1.086*sqrt(err[1])/star[1]);
    rchisq[i] = galchi / npt;

    /* save 7-par fit in shadow */
    parupd (star, shadow[i], ix, iy);
    for (j = 0; j < NPAR; j++) shaderr[i][j] = err[j];
	
    /* allow type 10 to go back to 1 and vice-versa */
    if ((jmtype == 10) && !verybright (starpar[i])) jmtype = 1;
    if (verybright (starpar[i])) jmtype = 10;

    if (jmtype == 10) {
      mprint (3, " very bright, keep 7 par fit only\n");

      if ((thresh < tmax / 4) && (thresh > tmax/16)) {
	/* test if type 10 can be safely split in two */
	starchi = twofit (star, starpar[i], star1, star2);
	/* has to be a really significant improvement */
	if (finite(starchi) && (starchi/galchi < 10*stograt) && (starchi < 10*npt)) {
	  mprint (3, " result-> a split star: gal-chi: %f, star-chi: %f\n", galchi, starchi);
	  jmtype = 3;
	  parupd (star1, shadow[i], ix, iy);
	  parupd (star1, starpar[i], ix, iy);
	  addstar (starpar[i], SUB, jmtype);
	  imtype[nstot] = 3;
	  parupd (star2, starpar[nstot], ix, iy);
	  parupd (star2, shadow[nstot], ix, iy);	
	  addstar (starpar[nstot], SUB, jmtype);
	  nstot ++;
	  continue;
	}
      }
      parupd (star, starpar[i], ix, iy);

      if ((thresh < tmax / 4) && (thresh > tmax/16)) {

	float err2[NPMAX], galchi2;

	beta4 = 0.01;
	for (n = 0; n < NPMAX; n++) star2[n] = star[n];
	galchi2 = chisq (onestar, xs, ys, zs, dzs, npt, star2, err2, NFIT2, acc, parlim, nit);
	if (1.2*galchi2 < galchi) {
	  jmtype = 12;
	  parupd (star2, starpar[i], ix, iy);
	  mprint (3, "extended: %f vs %f  %f %f  %f -   %d\n", galchi/npt, galchi2/npt, starpar[i][2], starpar[i][3], star[1], jmtype);
	}
	beta4 = 1.0;
      }

      addstar (starpar[i], SUB, jmtype);
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }
      
    verybig = galaxy (star, shaderr[i], starpar[i]);
    if (jmtype == 3) verybig = verybig && (chi[4] > xtra);

    /* shouldn't this test come earlier? */
    converge = finite(galchi);
    offp = offpic (star, ix, iy, &dx, &dy);
    verybig = verybig && !offp && converge;;

    if (!verybig || fixxy) {
      if (jmtype != 3) jmtype = 1;
      if (!converge) {
	if (jmtype != 3) jmtype = 9;
	mprint (3, "obj %d, npts %d, %d & %d .... failed to converge\n", i, npt, ix, iy);
	addstar (starpar[i], SUB, jmtype);
	imtype[i] = fixxy ? jmtype + 100 : jmtype;
	continue;
      }
      if (offp) {
	jmtype = 9;
	imtype[i] = fixxy ? jmtype + 100 : jmtype;
	mprint (3, "obj %d, npts %d, %d & %d .... fit center outside fit subraster\n", i, npt, ix, iy);
      }
      addstar (starpar[i], SUB, jmtype);
      imtype[i] = fixxy ? jmtype + 100 : jmtype;
      continue;
    }
	   
    mprint (3, "obj %d, npts %d, %d & %d .... is very big....\n", i, npt, ix, iy);
    mprint (3, ".... testing galaxy vs. double-star \n");
    
    starchi = twofit (star, starpar[i], star1, star2);

    if (finite(starchi) && (starchi/galchi < stograt)) {
      mprint (3, " result-> a split star: gal-chi: %f, star-chi: %f\n", galchi, starchi);
      jmtype = 3;
      parupd (star1, shadow[i], ix, iy);
      parupd (star1, starpar[i], ix, iy);
      addstar (starpar[i], SUB, jmtype);
      imtype[nstot] = 3;
      parupd (star2, starpar[nstot], ix, iy);
      parupd (star2, shadow[nstot], ix, iy);	
      addstar (starpar[nstot], SUB, jmtype);
      nstot ++;
    } else {
      mprint (3, " result-> a galaxy: gal-chi: %f, star-chi: %f\n", galchi, starchi);
      jmtype = 2;
      parupd (star, starpar[i], ix, iy);
      if (star[1]*star[4]*star[6]*ufactor > 20000) {
	float err2[NPMAX], galchi2;

	beta4 = 0.01;
	for (n = 0; n < NPMAX; n++) star2[n] = star[n];
	galchi2 = chisq (onestar, xs, ys, zs, dzs, npt, star2, err2, NFIT2, acc, parlim, nit);
	if (1.2*galchi2 < galchi) {
	  jmtype = 12;
	  parupd (star2, starpar[i], ix, iy);
	}
	/* fprintf (stderr, "extended: %f vs %f  %f %f  %f -   %d\n", galchi/npt, galchi2/npt, starpar[i][2], starpar[i][3], star[1], jmtype);
	 */
	beta4 = 1.0;
      }
      addstar (starpar[i], SUB, jmtype);
    }
    imtype[i] = fixxy ? jmtype + 100 : jmtype;
  }
  test7 = FALSE;
}



/*****************

  This routine seems really poorly written.  there is too much obfuscation.

  we just need to do a few things:

  1) do we try this object with 7-par fit? (if no, continue)

  2) fit the object with 7-par fit

  3) did it succeed? (if no, set some types)

  4) should it be a type 10? (ie, very bright)

  5) should it be a type 2? (ie, significantly extended)

  6) should if be a type 3? (ie, split in two)

  7) should it be a type 12? (ie, hubble fit)

******************/
