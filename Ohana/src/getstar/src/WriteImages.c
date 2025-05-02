# include "dvoImageExtract.h"

/* given image, find catalog images which overlap it */
int WriteImages (char *filename, Image *images, off_t Nimages, off_t *matches, off_t Nmatches) {
  OHANA_UNUSED_PARAM(Nimages);
  
  int i, N;
  int isWRP;
  FILE *f;

  /* matches here are only based on string comparisons */

  f = fopen (filename, "w");
  if (f == NULL) {
    fprintf (stderr, "failed to open output file %s\n", filename);
    exit (1);
  }

  Image *mosaic = NULL;
  isWRP = FALSE;
  for (i = 0; i < Nmatches; i++) {
    N = matches[i];
    if (!strcmp (&images[N].coords.ctype[4], "-WRP")) {
      if (isWRP) {
	if (mosaic != images[N].parent) {
	  fprintf (stderr, "only one mosaic allowed in an output file\n");
	  exit (1);
	}
      } else {
	mosaic = images[N].parent;
      }
    }      
  }

  if (isWRP) {
    WriteImageFITS (f, mosaic);
  } else {
    // write a blank
    if (Nmatches > 1) {
      WriteImageFITS (f, NULL);
    }
  }

  for (i = 0; i < Nmatches; i++) {
    N = matches[i];
    WriteImageFITS (f, &images[N]);
  }  

  fclose (f);
  return TRUE;
}
