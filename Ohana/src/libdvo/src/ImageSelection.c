# include "dvo.h"

/* db image table */
static Image *image = NULL;
static off_t *subset = NULL;
static off_t Nimage = 0;
static off_t Nsubset = 0;
static Coords mosaic;

/* load images based on parameters and region, etc */
// the mosaic defined below is a short cut to generate FPA / Camera level coords 
// relative to the exposure center.  
int SetImageSelection (int mosaicMode, SkyRegionSelection *selection) {

  int TimeSelect;
  time_t tzero, tend;

  image = NULL;
  subset = NULL;
  
  TimeSelect = GetTimeSelection (&tzero, &tend);

  if (mosaicMode) {
    /* mosaic defines a frame with 0,0 at the mosaic center, and 1 arcsec / pixel */
    InitCoords (&mosaic, "DEC--SIN");
    mosaic.cdelt1 = mosaic.cdelt2 = 1.0 / 3600;
  }

  if ((image = LoadImagesDVO (&Nimage)) == NULL) return (FALSE);
  BuildChipMatch (image, Nimage);
  image_subset (image, Nimage, &subset, &Nsubset, selection, tzero, (double) tend - tzero, TimeSelect);
  sort_image_subset (image, subset, Nsubset);
  return (TRUE);
}

/* free loaded images */
void FreeImageSelection () {
  if (image != NULL) FreeImagesDVO(image);
  if (subset != NULL) free (subset);
  image = NULL;
  subset = NULL;
  return;
}

Image *MatchImageDVO (unsigned int time, short int source, unsigned int imageID) { 

  int m = -1;

  if ((imageID != 0) && (imageID < Nimage)) {
    // imageID is in range for the array of images. If the table is still in order and
    // no images have been deleted the index of the image we are looking for will be imageID - 1
    // If this is the case, we have it. Otherwise we'll have to go search for it below
    int guess = (int) imageID - 1;
    if (image[guess].imageID == imageID) {
        m = guess;
    }
  } 
  if (m == -1) {
    m = match_image_subset (image, subset, Nsubset, time, source);
  }
  if (m == -1) return (NULL);
  return (&image[m]);
}

Image *MatchImageDVO_old (unsigned int time, short int source, unsigned int imageID) { 

  int m = -1;

  if ((imageID != 0) && (imageID < Nimage)) {
    // imageID is in range for the array of images. If the table is still in order and
    // no images have been deleted the index of the image we are looking for will be imageID - 1
    // If this is the case, we have it. Otherwise we'll have to go search for it below
    int guess = (int) imageID - 1;
    if (image[guess].imageID == imageID) {
        m = guess;
    }
  } 
  if (m == -1) {
    m = match_image_subset (image, subset, Nsubset, time, source);
  }
  if (m == -1) return (NULL);
  return (&image[m]);
}

Coords *MatchMosaic (unsigned int time, short int source) { 

  int m;

  // mosaic.crval1 = 0;
  // mosaic.crval2 = 0;
  m = match_image_subset (image, subset, Nsubset, time, source);
  if (m == -1) return (NULL);
  // mosaic = image[m].coords.crval1;
  // mosaic = image[m].coords.crval2;

  // if WRP, return the image, otherwise return NULL
  if (strcmp(&image[m].coords.ctype[4], "-WRP")) return NULL;
  return (&image[m].coords);
}
