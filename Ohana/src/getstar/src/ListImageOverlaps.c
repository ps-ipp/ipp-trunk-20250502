# include "dvoImageOverlaps.h"

/* given image, find catalog images which overlap it */
int ListImageOverlaps (Image *dbImages, Image *image, off_t *matches, off_t Nmatches) {
  
  off_t i, N;

  for (i = 0; i < Nmatches; i++) {
    N = matches[i];
    fprintf (stdout, "%s  :  %s\n", image[0].name, dbImages[N].name);
  }
  
  return (TRUE);
}
