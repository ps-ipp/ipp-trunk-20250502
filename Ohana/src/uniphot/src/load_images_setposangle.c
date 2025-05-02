# include "setposangle.h"

// array of mosaic definition structures
static off_t   Nmosaic;
static Mosaic *mosaic;

// list of mosaic associated with each image  
static off_t    Nmosaic_for_images; // number of images (for off_ternal checks)
static off_t    *mosaic_for_images; // array of: image -> mosaic

Image *load_images_setposangle (FITS_DB *db, off_t *Nimage) {

  Image *image;

  if (VERBOSE) fprintf (stderr, "finding images\n");

  /* read entire db table */
  if (!dvo_image_load (db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db[0].filename);

  /* use a vtable to keep the images to be calibrated */
  image = gfits_table_get_Image (&db[0].ftable, Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }

  fprintf (stderr, "loaded "OFF_T_FMT" images\n", *Nimage);

  return (image);
}

// I need an index to go from image to associated mosaic (if existant)
// This code is copied from relastro/src/MosaicOps.c

off_t getMosaicByTimes (unsigned int start, unsigned int stop, unsigned int *startMos, unsigned int *stopMos, off_t *indexMos) {

  // use bisection to find the overlapping mosaic

  off_t Nlo, Nhi, N;

  // find the last mosaic before start
  Nlo = 0; Nhi = Nmosaic;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (startMos[N] < start) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nmosaic);
    }
  }

  // check for the matched mosaic starting from Nlo 
  // we may have to go much beyond Nlo since stop is not sorted
  // can we use a sorted version of stop to check when we are beyond the valid range??
  for (N = Nlo; N < Nmosaic; N++) { 
    if (stop  < stopMos[N]) continue;
    if (start > startMos[N])  continue;
    if (stop  < startMos[N]) return (-1);
    return (indexMos[N]);
  }

  return (-1);
}

// sort two times vectors and an index by first time vector
void sort_mosaic_times (unsigned int *S, unsigned int *E, off_t *I, off_t N) {

# define SWAPFUNC(A,B){ unsigned int tmp_t; off_t tmp_i; \
  tmp_t = S[A]; S[A] = S[B]; S[B] = tmp_t; \
  tmp_t = E[A]; E[A] = E[B]; E[B] = tmp_t; \
  tmp_i = I[A]; I[A] = I[B]; I[B] = tmp_i; \
}
# define COMPARE(A,B)(S[A] < S[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

// use the time to make the match, but use bracketing to make it faster:

// find mosaic frames (unique time periods & photcode name matches mosaic) 
void initMosaics (ImageSubset *image, off_t Nimage) {

  off_t i, Nmos, NMOSAIC;
  unsigned int start, stop;

  unsigned int *startMos, *stopMos;
  off_t *indexMos;

  Nmosaic = 0;
  NMOSAIC = 10;
  ALLOCATE (mosaic, Mosaic, NMOSAIC);

  ALLOCATE (startMos, unsigned int, NMOSAIC);
  ALLOCATE (stopMos, unsigned int, NMOSAIC);
  ALLOCATE (indexMos, off_t, NMOSAIC);

  /* find the mosaic images (coords.ctype = DIS); generate list of unique mosaics */
  for (i = 0; i < Nimage; i++) {
    if (strcmp(&image[i].coords.ctype[4], "-DIS")) continue;

    /* set image time range */
    start = image[i].tzero - MAX(0.01*image[i].trate*image[i].NY, 1);
    stop  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);

    /* a new mosaic, define ranges */
    mosaic[Nmosaic].start = start;
    mosaic[Nmosaic].stop  = stop;
    mosaic[Nmosaic].coords = image[i].coords;

    startMos[Nmosaic] = start;
    stopMos[Nmosaic] = stop;
    indexMos[Nmosaic] = Nmosaic;

    Nmosaic ++;
    if (Nmosaic == NMOSAIC) {
      NMOSAIC += 10;
      REALLOCATE (mosaic, Mosaic, NMOSAIC);
      REALLOCATE (startMos, unsigned int, NMOSAIC);
      REALLOCATE (stopMos, unsigned int, NMOSAIC);
      REALLOCATE (indexMos, off_t, NMOSAIC);
    }
  }

  // sort the index, start, and stop by the start times:
  sort_mosaic_times (startMos, stopMos, indexMos, Nmosaic);
  
  // array to store image->mosaic index
  Nmosaic_for_images = Nimage;
  ALLOCATE (mosaic_for_images, off_t, Nmosaic_for_images);

  /* now assign the WRP images to these mosaics */
  for (i = 0; i < Nimage; i++) {
    mosaic_for_images[i] = -1; // default value for no mosaic found

    if (strcmp(&image[i].coords.ctype[4], "-WRP")) continue;

    /* set image time range */
    start = image[i].tzero - MAX(0.01*image[i].trate*image[i].NY, 1);
    stop  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);

    Nmos = getMosaicByTimes (start, stop, startMos, stopMos, indexMos);
    if (Nmos == -1) {
      fprintf (stderr, "cannot match mosaic for %d (ID = %d)\n", (int) i, (int) image[i].imageID);
      continue;
    }

    // mosaic corresponding to this image
    mosaic_for_images[i] = Nmos;
  }

  free (startMos);
  free (stopMos);
  free (indexMos);
  return;
}

Mosaic *getMosaicForImage (off_t im) {

  off_t mos;

  if (im < 0) abort();
  if (im >= Nmosaic_for_images) abort();

  // search for the mosaic that 
  mos = mosaic_for_images[im];
  if (mos < 0) return NULL;

  return &mosaic[mos];
}

