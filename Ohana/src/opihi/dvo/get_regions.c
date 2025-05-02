# include "dvoshell.h"

/* return region files containing given image */
GSCRegion *get_regions (Image *image, int *Nregions) {
  
  GSCRegion *region;
  FILE *f;
  double x, y;
  double dec, ra;
  int j, done, nregion, NREGION;
  char filename[256], path[256];
  
  VarConfig ("CATDIR", "%s", path);
  VarConfig ("GSCFILE", "%s", filename);
  f = fopen (filename, "r");
  if (f == NULL) {
    gprint (GP_ERR, "ERROR: can't find GSC region file %s\n", filename);
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
      aregion (&region[nregion], f, ra, dec, path);
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
  *Nregions = nregion;
  
  fclose (f);
  return (region);
  
}
