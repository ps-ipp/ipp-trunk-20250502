# include "delstar.h"

void match_images (Catalog *catalog, Image *image, off_t Nimage) {
  
  off_t j, k, found;
  e_time *start, *stop;

  /* this must be allocated so future free will not fail */
  ALLOCATE (catalog[0].image, int, MAX (catalog[0].Nmeasure, 1));
  if (catalog[0].Naverage == 0) {
    if (VERBOSE) fprintf (stderr, "no stars in catalog, skipping\n");
    return (FALSE);
  }

  ALLOCATE (start, unsigned int, Nimage);
  ALLOCATE (stop,  unsigned int, Nimage);
  for (j = 0; j < Nimage; j++) {
    start[j] = image[j].tzero - MAX(0.01*image[j].trate*image[j].NY, 1);
    stop[j]  = image[j].tzero + MAX(1.01*image[j].trate*image[j].NY, 1);
  }

  for (j = 0; j < catalog[0].Nmeasure; j++) {
    found = FALSE;
    if (catalog[0].measure[j].t == 0) {
      catalog[0].image[j] = -1;
      found = TRUE;
    }
    for (k = 0; (k < Nimage) && !found; k++) {
      if ((catalog[0].measure[j].t >= start[k]) && 
	  (catalog[0].measure[j].t <= stop[k])) {
	catalog[0].image[j] = k;
	found = TRUE;
      }
    }
    if (!found) {
      catalog[0].image[j] = -1;
      /* fprintf (stderr, "missing: %d %d\n", catalog[0].image[j], catalog[0].measure[j].t); */
    }
  }
  free (start);
  free (stop);
}

  /* this routine uses the time of each measurement to match the
measurement with an image.  Since the measurement is only store to 1
sec accuracy, which corresponds to roughly 30 rows at nominal speed,
we can't tell exactly which image the star come from.  However, this
doesn't matter, and in fact this helps a bit: a measurement from the
top of one image is the same as from the bottom of the next.
Therefore, we intentionally blur the edges of the images by 5%, which
will help to tie together neighboring images... */

