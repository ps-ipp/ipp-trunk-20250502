# include "gophot.h"

addstar (float *instar, int iadd, int type) {

  float star0[NPMAX], star1[NPMAX], *bigval, *noiseval;
  float sky, bfactor, bsky, cfactor, csky, val;
  int ix, iy, jrect[5];
  int i, j, ihi, ilo, jhi, jlo, ixin, iyin;
	
  if (type == 12) beta4 = 0.01;

  needit = FALSE;
	
  sky = guess2 (star0, instar, &ixin, &iyin);
  sky = guess2 (star1, instar, &ixin, &iyin) / ufactor;
	
  addlims (instar, jrect);
  star1[4] = star0[4]*SQ(xpnd);
  star1[5] = star0[5]/SQ(xpnd);			
  star1[6] = star0[6]*SQ(xpnd);
	
  bfactor = ufactor*iadd;
  bsky = iadd*(0.5 - sky);  /* why the 0.5? */
  cfactor = fac*ufactor;
  csky = 0.5 - fac*sky;

  ilo = MAX (jrect[1], 0);
  ihi = MIN (jrect[2], nfast-1);
  jlo = MAX (jrect[3], 0);
  jhi = MIN (jrect[4], nslow-1);
	
  for (j = jlo; j <= jhi; j++) {
    iy = j - iyin;
    bigval = &big[ilo + j*nfast];
    noiseval = &noise[ilo + j*nfast];
    for (i = ilo; i <= ihi; i++, bigval++, noiseval++) {
      if (!finite (*noiseval)) continue;
      ix = i - ixin;
      /* shouldn't bfactor multiply both onestar and bsky? */
      val = ufactor*(onestar (ix, iy, star0, (float *) NULL) - sky);
      *bigval += iadd*val;

      val = fac*ufactor*fabs(onestar (ix, iy, star1, (float *) NULL) - sky);
      *noiseval -= iadd*val;

      if (*noiseval <= -1000) {
	mprint (2, "i,j,noise = %d, %d, %f, %f %f %f %d, negative noise! obliterating\n", i, j, *noiseval, val, *bigval, cfactor, iadd);
	/*
	*bigval = MAGIC;
	*noiseval = MAGIC;
	*/
      }
    }
  }
  needit = TRUE;
  beta4 = 1.0;
}

/* star0 is the nominial star region, star1 is the expanded are for the noise array */

