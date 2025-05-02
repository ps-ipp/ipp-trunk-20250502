# include "markstar.h"

GSCRegion *gregions1 (catstats, Nregions)
CatStats catstats[];
int *Nregions;
{
  
  GSCRegion *region;
  int i, j, k, x, y, done, nregion, nregion2, NREGION;
  double ra, dec, dx, dy, Xo[4], Yo[4];
  FILE *f;

  f = fopen (GSCFILE, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: could not open GSC region file %s\n", GSCFILE);
    exit (0);
  }
  nregion = 0;
  NREGION = 10;
  ALLOCATE (region, GSCRegion, NREGION);

  if (catstats[0].DEC[0] == 86.25) { /* pole region */
    dec = 86.0;
    for (ra = 0.1; ra < 370; ra+= 30.0) {
      aregion (&region[nregion], f, ra, dec);
      done = FALSE;
      for (j = 0; (j < nregion - 1) && !done; j++) {
	if (!strcmp (region[nregion].filename, region[j].filename)) {
	  nregion --;
	  done = TRUE;
	}
      }
      nregion ++;
      if (nregion == NREGION) {
	NREGION += 10;
	REALLOCATE (region, GSCRegion, NREGION);
      }
    }
  } else {
    for (x = 0; x < 2; x++) {
      for (y = 0; y < 2; y++) {
	for (dx = -0.1; dx <= 0.1; dx += 0.2) {
	  for (dy = -0.1; dy <= 0.1; dy += 0.2) {
	    ra  = catstats[0].RA[x] + dx;
	    dec = catstats[0].DEC[y] + dy;
	    aregion (&region[nregion], f, ra, dec);
	    done = FALSE;
	    for (j = 0; (j < nregion) && !done; j++) {
	      if (!strcmp (region[nregion].filename, region[j].filename)) {
		done = TRUE;
	      }
	    }
	    if (!done) {
	      nregion ++;
	    } 
	    if (nregion == NREGION) {
	      NREGION += 10;
	      REALLOCATE (region, GSCRegion, NREGION);
	    }
	  }
	}
      }
    }
  }

  if (VERBOSE) {
    fprintf (stderr, "using %d regions\n", nregion);
    for (i = 0; i < nregion; i++) {
      fprintf (stderr, "region %d: %f %f  %f %f\n", i, region[i].RA[0], region[i].RA[1], region[i].DEC[0], region[i].DEC[1]);
    } 
  }

  REALLOCATE (region, GSCRegion, MAX (nregion, 1));
  *Nregions = nregion;
  
  fclose (f);
  return (region);
  
}

GSCRegion *gregions2 (image, Nregions)
Image *image;
int *Nregions;
{
  
  GSCRegion *region;
  FILE *f;
  double x, y;
  double dr, dd, dec, ra;
  int i, j, done, nregion, NREGION;
  
  f = fopen (GSCFILE, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't find GSC region file %s\n", GSCFILE);
    exit (0);
  }
  
  /* find regions at image corners */
  NREGION = 10;
  ALLOCATE (region, GSCRegion, NREGION);
  nregion = 0;

  /* look for new regions on grid across image */ 
  for (x = 0.0; x <= 1.0; x+=0.25) {
    for (y = 0.0; y <= 1.0; y+=0.25) {
      XY_to_RD (&ra, &dec, image[0].NX*(1.1*x - 0.05), image[0].NY*(1.1*y - 0.05), &image[0].coords);
      aregion (&region[nregion], f, ra, dec);
      done = FALSE;
      for (j = 0; (j < nregion) && !done; j++) {
	if (!strcmp (region[nregion].filename, region[j].filename)) {
	  nregion --;
	  done = TRUE;
	}
      }
      nregion ++;
      if (nregion == NREGION) {
	NREGION += 10;
	REALLOCATE (region, GSCRegion, NREGION);
      }
    }
  }

  if (VERBOSE) {
    fprintf (stderr, "found %d region files:\n", nregion);
    for (i = 0; i < nregion; i++) {
      fprintf (stderr, "  %d %s\n", i, region[i].filename);
    }
  }
  *Nregions = nregion;
  
  fclose (f);
  return (region);
  
}
