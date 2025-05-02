# include "relastro.h"

// array of mosaic definition structures
static off_t   Nmosaic;
static Mosaic *mosaic = NULL;

// list of all images associated with a mosaic
static off_t   *Nmosaic_own_images; // number of images for this mosaic
static off_t   *Amosaic_own_images; // size of allocated array
static off_t   **mosaic_own_images; // array of arrays: mosaic -> images

// list of mosaic associated with each image  
static off_t    Nmosaic_for_images; // number of images (for internal checks)
static off_t    *mosaic_for_images; // array of: image -> mosaic

Mosaic *getmosaics (off_t *N) {
  *N = Nmosaic;
  return (mosaic);
}

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

// first, let's continue to use the time to make the match, but use bracketing to make it faster:

// find mosaic frames (unique time periods & photcode name matches mosaic) 
void initMosaics (Image *image, off_t Nimage) {

  off_t i, Nmos, NMOSAIC;
  unsigned int start, stop;

  unsigned int *startMos, *stopMos;
  off_t *indexMos;

  Nmosaic = 0;
  NMOSAIC = 10;
  ALLOCATE (mosaic, Mosaic, NMOSAIC);

  ALLOCATE (Nmosaic_own_images, off_t, NMOSAIC);
  ALLOCATE (Amosaic_own_images, off_t, NMOSAIC);
  ALLOCATE (mosaic_own_images, off_t *, NMOSAIC);
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
    mosaic[Nmosaic].start     = start;
    mosaic[Nmosaic].stop      = stop;
    mosaic[Nmosaic].McalPSF   = 0.0;
    mosaic[Nmosaic].McalAPER  = 0.0;
    mosaic[Nmosaic].dMcal     = 0.0;
    mosaic[Nmosaic].McalChiSq = 0.0;
    mosaic[Nmosaic].flags     = image[i].flags;
    mosaic[Nmosaic].secz      = image[i].secz;
    mosaic[Nmosaic].coords    = image[i].coords;
    mosaic[Nmosaic].myImage   = i;

    // init the mosaic_own_images array data
    Nmosaic_own_images[Nmosaic] = 0;
    Amosaic_own_images[Nmosaic] = 10;
    ALLOCATE (mosaic_own_images[Nmosaic], off_t, Amosaic_own_images[Nmosaic]);

    startMos[Nmosaic] = start;
    stopMos[Nmosaic] = stop;
    indexMos[Nmosaic] = Nmosaic;

    Nmosaic ++;
    if (Nmosaic == NMOSAIC) {
      NMOSAIC += 10;
      REALLOCATE (mosaic, Mosaic, NMOSAIC);
      REALLOCATE (mosaic_own_images, off_t *, NMOSAIC);
      REALLOCATE (Nmosaic_own_images, off_t, NMOSAIC);
      REALLOCATE (Amosaic_own_images, off_t, NMOSAIC);
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

  // emit an error if we miss mosaics, but stop if we miss too many
  int NmissMosaic = 0;
  int NtestMosaic = 0;

  /* now assign the WRP images to these mosaics */
  for (i = 0; i < Nimage; i++) {
    mosaic_for_images[i] = -1; // default value for no mosaic found

    if (strcmp(&image[i].coords.ctype[4], "-WRP")) continue;
    NtestMosaic ++;

    /* set image time range */
    start = image[i].tzero - MAX(0.01*image[i].trate*image[i].NY, 1);
    stop  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);

    Nmos = getMosaicByTimes (start, stop, startMos, stopMos, indexMos);
    if (Nmos == -1) {
      if (NmissMosaic < 1000) {
	fprintf (stderr, "cannot match mosaic for %s\n", image[i].name);
      }
      NmissMosaic ++;
      continue;
    }

    // mosaic corresponding to this image
    mosaic_for_images[i] = Nmos;

    // add image to mosaic_own_image list 
    mosaic_own_images[Nmos][Nmosaic_own_images[Nmos]] = i;
    Nmosaic_own_images[Nmos] ++;
    if (Nmosaic_own_images[Nmos] == Amosaic_own_images[Nmos]) {
      Amosaic_own_images[Nmos] += 10;
      REALLOCATE (mosaic_own_images[Nmos], off_t, Amosaic_own_images[Nmos]);
    }
    assert (Nmosaic_own_images[Nmos] < Amosaic_own_images[Nmos]);
  }

  fprintf (stderr, "mosaic matching : %d of possible %d failed to match\n", NmissMosaic, NtestMosaic);
  if (NmissMosaic > 0.5*NtestMosaic) {
    fprintf (stderr, "serious problem with mosaic matching\n");
    exit (5);
  }

  free (startMos);
  free (stopMos);
  free (indexMos);
  return;
}

void freeMosaics () {

  off_t i;

  if (!mosaic) return;

  for (i = 0; i < Nmosaic; i++) {
    FREE (mosaic_own_images[i]);
  }

  FREE (mosaic);
  FREE (Nmosaic_own_images);
  FREE (Amosaic_own_images);
  FREE (mosaic_own_images);
  FREE (mosaic_for_images);
}

// return StarData values for detections in the specified image, converting coordinates from the
// chip positions: X,Y -> L,M -> P,Q -> R,D
StarData *getMosaicRaw (Catalog *catalog, int Ncatalog, off_t mos, off_t *Nstars) {

  off_t i, j, im, Nraw, Nnew;
  StarData *raw, *new;

  Nraw = 0;
  ALLOCATE (raw, StarData, 1);

  // loop over the images owned by this mosaic
  for (i = 0; i < Nmosaic_own_images[mos]; i++) {

    im = mosaic_own_images[mos][i];
    
    // retrieve stars for this chip, applying chip and mosaic astrometry
    // this function does the reverse-lookup for the mosaic corresponding to this image
    new = getImageRaw (catalog, Ncatalog, im, &Nnew, MODE_MOSAIC);
    if (!new) {
      fprintf (stderr, "inconsistent: missing mosaic for image already associated with a mosaic? (1)\n");
      abort();
    }
    
    // merge new and raw
    REALLOCATE (raw, StarData, Nraw + Nnew);
    for (j = 0; j < Nnew; j++) {
      raw[Nraw+j] = new[j];
    }
    Nraw += Nnew;

    free (new);
  }

  *Nstars = Nraw;
  return (raw);
}

// return StarData values for averages positions in the specified image, converting coordinates from
// the sky positions: R,D -> P,Q -> L,M -> X,Y
StarData *getMosaicRef (Catalog *catalog, int Ncatalog, off_t mos, off_t *Nstars) {

  off_t i, j, im, Nref, Nnew;
  StarData *ref, *new;
  
  Nref = 0;
  ALLOCATE (ref, StarData, 1);

  for (i = 0; i < Nmosaic_own_images[mos]; i++) {

    im = mosaic_own_images[mos][i];
    
    // retrieve stars for this chip, applying chip and mosaic astrometry
    // this function does the reverse-lookup for the mosaic corresponding to this image
    new = getImageRef (catalog, Ncatalog, im, &Nnew, MODE_MOSAIC);
    if (!new) {
      fprintf (stderr, "inconsistent: missing mosaic for image already associated with a mosaic? (2)\n");
      abort();
    }
    
    // merge new and ref
    REALLOCATE (ref, StarData, Nref + Nnew);
    for (j = 0; j < Nnew; j++) {
      ref[Nref+j] = new[j];
    }
    Nref += Nnew;

    free (new);
  }

  *Nstars = Nref;
  return (ref);
}

Mosaic *getMosaicForImage (off_t im) {

  off_t mos;

  if (im < 0) abort();
  if (im >= Nmosaic_for_images) abort();

  // search for the mosaic that matches this image
  mos = mosaic_for_images[im];
  if (mos < 0) return NULL;

  return &mosaic[mos];
}

// extend each host image table to include the mosaic 'images' needed by the host
int select_mosaics_hostregion (RegionHostTable *regionHosts, Image *image, off_t Nimage) {
  OHANA_UNUSED_PARAM(Nimage);

  int i;
  off_t j;
  char *mosaicUsed;

  ALLOCATE (mosaicUsed, char, Nmosaic);

  // we need to add the mosaics to each of the region hosts lists of images
  for (i = 0; i < regionHosts->Nhosts; i++) {

    int Nadd = 0;
    int NADD = 100;
    off_t *addMosaic = NULL;
    ALLOCATE (addMosaic, off_t, NADD);

    // reset the mosaicUsed flags (valid only for this host)
    memset (mosaicUsed, 0, Nmosaic * sizeof(char));

    RegionHostInfo *host = &regionHosts->hosts[i];

    // find the mosaics associated with a given 
    for (j = 0; j < host->Nimage; j++) {

      int im = host->imseq[j];
      
      if (im < 0) abort();
      if (im >= Nmosaic_for_images) abort();

      // search for the mosaic that matches this image (skip unmatched images)
      off_t mos = mosaic_for_images[im];
      if (mos < 0) continue; 

      if (mosaicUsed[mos]) continue;

      mosaicUsed[mos] = TRUE;
      addMosaic[Nadd] = mos;
      Nadd ++;
      
      CHECK_REALLOCATE (addMosaic, off_t, NADD, Nadd, 100);
    }

    REALLOCATE (host->image, Image, host->Nimage + Nadd);

    for (j = 0; j < Nadd; j++) {
      off_t mos = addMosaic[j];
      off_t mos_im = mosaic[mos].myImage;

      host->image[host->Nimage + j] = image[mos_im];
    }
    
    free (addMosaic);

    host->Nimage += Nadd;
  }
  free (mosaicUsed);

  return TRUE;
}

