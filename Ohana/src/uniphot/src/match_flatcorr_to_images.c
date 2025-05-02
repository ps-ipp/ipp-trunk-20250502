# include "setphot.h"

// the date/time of the image is used to find the 'season'
// the photcode is used to find the actual flat correction

int match_flatcorr_to_images (Image *image, off_t Nimage, FlatCorrectionTable *flatcorrTable) {

  int i, j;

  // make 4 lookup tables for the photcodes (one for each season)

  short **index;

  short maxCode = 0;
  for (i = 0; i < flatcorrTable->Nimage; i++) {
    maxCode = MAX (maxCode, flatcorrTable->image[i].photcode);
  }
  maxCode ++; // we want the outer bound, not the last value

  ALLOCATE (index, short *, flatcorrTable->Nseason);
  for (i = 0; i < flatcorrTable->Nseason; i++) {
      ALLOCATE (index[i], short, maxCode);
      for (j = 0; j < maxCode; j++) {
	index[i][j] = -1;
      }
  }

  for (i = 0; i < flatcorrTable->Nimage; i++) {
    // the flat-field correction is defined for non-existent images (eg, XY00)
    // these have photcode == 0, so skip them
    if (!flatcorrTable->image[i].photcode) continue;

    // which season?
    for (j = 0; j < flatcorrTable->Nseason; j++) {
      if (flatcorrTable->image[i].tstart == flatcorrTable->tstart[j]) {
	assert (index[j][flatcorrTable->image[i].photcode] == -1);
	index[j][flatcorrTable->image[i].photcode] = i;
	break;
      }
    }
  }
  
  for (i = 0; i < Nimage; i++) {
    if (!image[i].photcode) continue; // skip PHU images
    if (image[i].photcode > maxCode) continue; // skip PHU images

    int found = FALSE;
    for (j = 0; !found && (j < flatcorrTable->Nseason); j++) {
      if (image[i].tzero < flatcorrTable->tstart[j]) continue;
      if (image[i].tzero > flatcorrTable->tstop[j]) continue;

      int seq = index[j][image[i].photcode];
      if (seq == -1) break;

      image[i].photom_map_id = flatcorrTable->image[seq].ID;
      found = TRUE;
    }
    if (!found) {
      fprintf (stderr, "image does not match flatcorr Table ranges: %s\n", image[i].name);
    }
  }
  return TRUE;
}
