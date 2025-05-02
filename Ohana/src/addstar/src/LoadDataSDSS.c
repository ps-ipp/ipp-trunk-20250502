# include "addstar.h"

// examine the header sets and set the Image entries for the the valid images
// there should only be a single data set (phu + table) in this file
// each SDSS data set corresponds to 5 images (ugriz)
Catalog *LoadDataSDSS (FILE *f, char *imagename, Image **images, off_t *nvalid, Header **headers, off_t *extsize, HeaderSet *headerSets, off_t Nimages) {
  OHANA_UNUSED_PARAM(Nimages);

  off_t Nskip, Nvalid, NVALID;
  int j, Nhead, Ndata;

  if (images[0] == NULL) {
    Nvalid = 0;
    NVALID = 5;
    ALLOCATE (images[0], Image, NVALID);
  } else {
    Nvalid = *nvalid;
    NVALID = Nvalid + 5;
    REALLOCATE (images[0], Image, NVALID);
  }    

  // there is only one SDSS image per file (TRUE?)
  Nhead = headerSets[0].extnum_head;

  // XXX parse the information needed from the PHU header
  if (VERBOSE) fprintf (stderr, "reading header for %s (%s)\n", headerSets[0].exthead, headerSets[0].extdata);

  // advance the pointer to the start of the corresponding table block
  Ndata = headerSets[0].extnum_data;
  Nskip = 0;
  for (j = 0; j < Ndata; j++) {
    Nskip += extsize[j];
  }
  fseeko (f, Nskip, SEEK_SET); 
	 
  // we pass in images[0] so the function updates the end of the array
  Catalog *catalog = ReadStarsSDSS (f, imagename, headers[Nhead], headers[Ndata], images[0], &Nvalid);

  *nvalid = Nvalid;
  return (catalog);
}

