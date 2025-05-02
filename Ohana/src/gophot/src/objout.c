# include "gophot.h"
# define NCHAR 104

completeout () {

  FILE *f;
  int i, Nchar;
  float gcorr, gmag;
  float area, amajor, aminor, tilt, fmag, xc, yc, apmag, tmp;
  char line[NCHAR];
	
  f = fopen (files[4], "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "error, can't save data in file %s\n", files[4]);
    exit (1);
  }

  ellipse (ava[4], ava[5], ava[6], &area, &amajor, &aminor, &tilt);
  fprintf (f, "# Average Star: %f %f %f\n", amajor, aminor, tilt);

  for (i = 0; i < nregion; i++) {
    
    fmag = -2.5*log10 (region[i][1]);
    gmag = -2.5*log10 (region[i][7]);
    apmag = 99.999;

    Nchar = snprintf (line, NCHAR, "%3d %8.2f %8.2f %8.3f %6.3f %9.2f %9.3f %9.3f %7.2f %8.3f %8.3f  %8.2f",
		      20, region[i][2], region[i][3], fmag, 0.01, region[i][0], region[i][4], region[i][5], region[i][6], 
		      gmag, apmag, 10.0);
    fprintf (f, "%s\n", line);
  }

  for (i = 0; i < nstot; i++) {
         
    fmag = 99.999;
    gmag = 99.999;
    apmag = 99.999;

    if (imtype[i] != 8) {
      /* pure gaussian fit mags */
      ellipse (starpar[i][4], starpar[i][5], starpar[i][6], &area, &amajor, &aminor, &tilt);
      tmp = area*starpar[i][1]/eperdn;
      if (tmp > 0.0) fmag = -2.5 * log10 (tmp);
      /* galaxy-non-gauss fit mags */
      ellipse (shadow[i][4], shadow[i][5], shadow[i][6], &area, &amajor, &aminor, &tilt);
      gcorr = 1.0;
      tmp = area*gcorr*shadow[i][1]/eperdn;
      if (tmp > 0.0) gmag = -2.5 * log10 (tmp);
      tilt = 57.29578 * tilt;
    } else {
      /* get correct orientation for oblit boxes */
      if (starpar[i][5] != -1) fmag = -99.999;
      if (starpar[i][4] >= starpar[i][6]) {
	amajor = starpar[i][4];
	aminor = starpar[i][6];
	tilt = 0.0;
      } else {
	amajor = starpar[i][6];
	aminor = starpar[i][4];
	tilt = 90.0;
      }
    }
         
    /* what is the value of the center of a pixel? */
    /* this assumes the pixel center is at 0,0, not 0.5, 0.5 */
    xc = starpar[i][2];
    yc = starpar[i][3];

    if (apple[i][1] > 0.0) apmag = -2.5 * log10 (apple[i][1]/eperdn);
         
    Nchar = snprintf (line, NCHAR, "%3d %8.2f %8.2f %8.3f %6.3f %9.2f %9.3f %9.3f %7.2f %8.3f %8.3f  %8.2f",
	     imtype[i], xc, yc, fmag, apple[i][4], shadow[i][0], amajor, aminor, tilt, 
	     gmag, apmag, rchisq[i]);
    if (Nchar != NCHAR - 1) {
      mprint (1, "funny line %d\n", i);
    }
    fprintf (f, "%s\n", line);
  }         

  /*
  1    37.51  1193.91  -15.246   .034    594.48     6.657     5.654   -7.06  -15.611  -15.822
  */  
  fclose (f);

  return (0);
}

/* this function uses C 0,N-1 for a[], fa[] */
