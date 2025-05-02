# include "dvoImageExtract.h"

/* given image, find catalog images which overlap it */
off_t *SelectImages (char *filename, Image *images, off_t Nimages, off_t *Nmatch) {
  
  int Nchar;
  off_t i, NMATCH, nmatch, *match;

  /* matches here are only based on string comparisons */

  /* match represents the subset of matched images */
  nmatch = 0;
  NMATCH = 20;
  ALLOCATE (match, off_t, NMATCH);

  /* setup links for mosaic WRP and DIS entries */
  BuildChipMatch (images, Nimages);

  Nchar = strlen (filename);

  for (i = 0; i < Nimages; i++) {

    if (strncmp (images[i].name, filename, Nchar)) continue;

    match[nmatch] = i;
    nmatch ++;
    if (nmatch == NMATCH) {
      NMATCH += 20;
      REALLOCATE (match, off_t, NMATCH);
    }
  }  

  if (VERBOSE) fprintf (stderr, "found "OFF_T_FMT" matching images\n",  nmatch);

  *Nmatch = nmatch;
  return (match);
}
