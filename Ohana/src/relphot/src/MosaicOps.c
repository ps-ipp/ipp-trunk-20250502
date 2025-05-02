# include "relphot.h"
off_t findMosaic (unsigned int *startTimes, off_t Nmosaic, unsigned int start);
int save_test_mosaic_measures (FILE *fout, int Nmos, Catalog *catalog);

// see discussion in ImagesOps.c re: IDX_T

// array of mosaic definition structures
static off_t  Nmosaic;
static Mosaic *mosaic;

// relationships between the mosaics and their associated images
static off_t   *MosaicN_Image; // number of images associated with the given mosaic
static off_t  **MosaicToImage; // list of images associated with the given mosaic

// mosaic index for given image : ImageToMosaic[ImageIndex] = MosaicIndex (ImageIndex : 0 < Nimage)
static off_t   *ImageToMosaic;

// elsewhere, we have loaded a set of catalogs with measures (catalog[cat].measure[meas])
// each mosaic has N_onMosaic[MosaicIndex] measurements
static off_t    *N_onMosaic;   // actual number of measurements on mosaic	 
static off_t    *N_ONMOSAIC;   // allocated number of measurements on mosaic   

// relationships between the measure,catalog set and the mosaics:
static off_t   **MeasureToMosaic; // Mosaic index from measure,catalog  : MeasureToMosaic[cat][meas] = MosaicIndex 
static off_t   **MosaicToCatalog; // catalog for given measure on mosaic : MosaicCatalog[MosaicIndex][i] = cat (i : 0 < NonMosaic[MosaicIndex])
static off_t   **MosaicToMeasure; // measure for given measure on mosaic : MosaicMeasure[MosaicIndex][i] = cat (i : 0 < NonMosaic[MosaicIndex])

// MeasureToMosaic was 'bin'
// MosaicToCatalog was 'clist'
// MosaicToMeasure was 'mlist'

// N_onMosaic was 'Nlist'
// N_ONMOSAIC was 'NLIST'

// ImageToMosaic was 'mosimage'

// MosaicN_image was 'Nimlist'
// MosaicToImage was 'imlist'

void sort_times (unsigned int *T, int N) {

# define SWAPFUNC(A,B){ unsigned int tmp; \
  tmp = T[A]; T[A] = T[B]; T[B] = tmp; \
}
# define COMPARE(A,B)(T[A] < T[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* find mosaic frames (unique time periods) (NOTE : we do NOT require matching photcodes...)
   this function will also identify the images NOT in the subset which belong to a selected mosaic
 */
void initMosaics (Image *subset, off_t Nsubset, Image *image, char *inSubset, off_t Nimage) {

  off_t i, j, found, NMOSAIC, *MosaicN_IMAGE;
  unsigned int start, stop, *startTimes, *startTimesMosaic;

  if (!MOSAIC_ZEROPT) return;

  INITTIME;

  /* a 'mosaic' in relphot is (unlike relastro) a virtual concept: there is no 
   * entry in the image table that represents this mosaic.  Instead, it is an
   * internal construct that defines a group of related images 
   */

  // generate a list of all subset image start times
  ALLOCATE (startTimes, unsigned int, Nsubset);
  int Nmoschip = 0;
  for (i = 0; i < Nsubset; i++) {
    if (!isMosaicChip(subset[i].photcode)) continue;
    startTimes[Nmoschip] = subset[i].tzero;
    Nmoschip ++;
  }
  sort_times (startTimes, Nmoschip);
  MARKTIME("create array of all image obstimes: %f sec\n", dtime);
  
  Nmosaic = 0;
  NMOSAIC = 1000;
  ALLOCATE (startTimesMosaic, unsigned int, NMOSAIC);
  startTimesMosaic[0] = startTimes[0];

  // generate a list of the unique start times (these define the mosaics)
  for (i = 0; i < Nmoschip; i++) {
    myAssert (startTimes[i] >= startTimesMosaic[Nmosaic], "times out of order?");
    if (startTimes[i] == startTimesMosaic[Nmosaic]) continue;
    Nmosaic ++;
    if (Nmosaic >= NMOSAIC) {
      NMOSAIC += 1000;
      REALLOCATE (startTimesMosaic, unsigned int, NMOSAIC);
    }
    startTimesMosaic[Nmosaic] = startTimes[i];
  }
  Nmosaic ++;
  MARKTIME("create subset array of mosaic obstimes: %f sec\n", dtime);

  // now I have a list of uniq start times, and they are in order
  // create the mosaic arrays for these times
  ALLOCATE (mosaic, Mosaic, Nmosaic);

  ALLOCATE (MosaicToImage, off_t *, Nmosaic);
  ALLOCATE (MosaicN_Image, off_t,   Nmosaic);
  ALLOCATE (MosaicN_IMAGE, off_t,   Nmosaic);

  for (i = 0; i < Nmosaic; i++) {
    /* a new mosaic, define ranges */
    mosaic[i].start     = startTimesMosaic[i];
    mosaic[i].stop      = 0;
    mosaic[i].McalPSF   = 0.0; // note : at the end, mosaic.Mcal is added back to the input images
    mosaic[i].McalAPER  = 0.0; // note : mosaic stores only offsets relative to the original image values
    mosaic[i].dMcal     = 0.0; // note : at the end, mosaic.Mcal is added back to the input images
    mosaic[i].dMsys     = 0.0;
    mosaic[i].McalChiSq = 0.0;// NAN or 0.0?
    mosaic[i].flags     = 0;
    mosaic[i].secz      = NAN;
    mosaic[i].photcode  = 0;
    mosaic[i].skipCal   = FALSE;
    mosaic[i].inTGroup  = FALSE; // not (yet?) assigned to a TGroup
    
    memset (&mosaic[i].coords, 0, sizeof(Coords));

    MosaicN_IMAGE[i] = 10;
    MosaicN_Image[i] = 0;
    ALLOCATE (MosaicToImage[i], off_t, MosaicN_IMAGE[i]);
    MosaicToImage[i][0] = -1;
  }

  int Nskip, Nmark;
  Nskip = Nmark = 0;
  // find any mosaics (startTimesMosaic) which match unselected images 
  for (i = 0; i < Nimage; i++) {
    if (inSubset[i]) {
      Nskip ++;
      continue;
    }
    
    if (!isMosaicChip(image[i].photcode)) continue;

    /* set image time range */
    start = image[i].tzero;
    stop  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);

    /* find a matching mosaic */
    j = findMosaic(startTimesMosaic, Nmosaic, start);
    if (j != -1) {
      // mark this mosaic as bad
      mosaic[j].skipCal = TRUE;
      Nmark ++;
    }
  }
  fprintf (stderr, "%d total images, %d overlap skyregion & match selection criteria, %d do not overlap, but match overlapping mosaics\n", (int) Nimage, (int) Nskip, (int) Nmark);
  MARKTIME("find unselected images matching selected mosaics: %f sec\n", dtime);

  ALLOCATE (ImageToMosaic, off_t, Nsubset); // mosaic to which image belongs

  // assign each image to a mosaic
  int Nsimple = 0;
  for (i = 0; i < Nsubset; i++) {
    ImageToMosaic[i] = -1;

    if (!isMosaicChip(subset[i].photcode)) {
      Nsimple ++;
      continue;
    }

    start = subset[i].tzero;
    stop  = subset[i].tzero + MAX(1.01*subset[i].trate*subset[i].NY, 1);

    j = findMosaic(startTimesMosaic, Nmosaic, start);
    if (j == -1) {
      fprintf (stderr, "programming error? all subset images should belong to a mosaic\n");
      abort();
    }

    // add reference from image to mosaic
    ImageToMosaic[i] = j;

    // have we already found this mosaic?
    found = (MosaicN_Image[j] > 0);

    /* add image to mosaic image list */
    MosaicToImage[j][MosaicN_Image[j]] = i;
    MosaicN_Image[j] ++;
    if (MosaicN_Image[j] == MosaicN_IMAGE[j]) {
      MosaicN_IMAGE[j] += 10;
      REALLOCATE (MosaicToImage[j], off_t, MosaicN_IMAGE[j]);
    }
    if (found) continue;
    
    /* a new mosaic, define ranges */
    if (mosaic[j].start != start) { 
      fprintf (stderr, "error?\n");
      abort();
    }
    mosaic[j].stop      = stop;

    mosaic[j].McalPSF   = 0.0;
    mosaic[j].McalAPER  = 0.0;
    mosaic[j].dMcal     = 0.0;
    mosaic[j].McalChiSq = 0.0;

    mosaic[j].nFitPhotom = 0;
    mosaic[j].nValPhotom = 0;

    mosaic[j].dMsys     = subset[i].dMagSys;
    mosaic[j].flags     = subset[i].flags;
    mosaic[j].secz      = subset[i].secz;
    mosaic[j].photcode  = GetPhotcodeEquivCodebyCode (subset[i].photcode);
  }

  // free this or not?
  free (MosaicN_IMAGE);
  free (startTimes);
  free (startTimesMosaic);

  initMosaicMcal (subset, Nsubset);

  fprintf (stderr, "matched %d images to %d mosaics, %d simple chips not matched to mosaics\n", (int) (Nsubset - Nsimple), (int) Nmosaic, (int) Nsimple);
  return;
}

/* find mosaic frames (unique time periods) (NOTE : require gpc1 chips, which is pretty limiting)
   if mergeMcal is TRUE, <image.Mcal> values will be saved on Mosaic.Mcal
 */
void makeMosaics (Image *image, off_t Nimage, int mergeMcal) {

  off_t i, j, found, NMOSAIC, *MosaicN_IMAGE;
  unsigned int start, stop, *startTimes, *startTimesMosaic;

  if (!MOSAIC_ZEROPT) return;

  INITTIME;

  /* a 'mosaic' in relphot is (unlike relastro) a virtual concept: there is no 
   * entry in the image table that represents this mosaic.  Instead, it is an
   * internal construct that defines a group of related images 
   */

  // generate a list of all image start times
  ALLOCATE (startTimes, unsigned int, Nimage);
  int Nmoschip = 0;
  for (i = 0; i < Nimage; i++) {
    if (!isMosaicChip(image[i].photcode)) continue;
    startTimes[Nmoschip] = image[i].tzero;
    Nmoschip ++;
  }
  sort_times (startTimes, Nmoschip);
  MARKTIME("create array of all image obstimes: %f sec\n", dtime);
  
  Nmosaic = 0;
  NMOSAIC = 1000;
  ALLOCATE (startTimesMosaic, unsigned int, NMOSAIC);
  startTimesMosaic[0] = startTimes[0];

  // generate a list of the unique start times (these define the mosaics)
  for (i = 0; i < Nmoschip; i++) {
    myAssert (startTimes[i] >= startTimesMosaic[Nmosaic], "times out of order?");
    if (startTimes[i] == startTimesMosaic[Nmosaic]) continue;
    Nmosaic ++;
    if (Nmosaic >= NMOSAIC) {
      NMOSAIC += 1000;
      REALLOCATE (startTimesMosaic, unsigned int, NMOSAIC);
    }
    startTimesMosaic[Nmosaic] = startTimes[i];
  }
  Nmosaic ++;
  MARKTIME("create array of mosaic obstimes: %f sec\n", dtime);

  // now I have a list of uniq start times, and they are in order
  // create the mosaic arrays for these times
  ALLOCATE (mosaic, Mosaic, Nmosaic);

  ALLOCATE (MosaicToImage, off_t *, Nmosaic);
  ALLOCATE (MosaicN_Image, off_t,   Nmosaic);
  ALLOCATE (MosaicN_IMAGE, off_t,   Nmosaic);

  // init the mosaic array values
  for (i = 0; i < Nmosaic; i++) {
    mosaic[i].start     = startTimesMosaic[i];
    mosaic[i].stop      = 0;
    mosaic[i].McalPSF   = 0.0; // note : at the end, mosaic.Mcal is added back to the input images
    mosaic[i].McalAPER  = 0.0; // note : mosaic stores only offsets relative to the original image values
    mosaic[i].dMcal     = 0.0; // note : at the end, mosaic.Mcal is added back to the input images
    mosaic[i].dMsys     = 0.0;
    mosaic[i].McalChiSq = 0.0;// NAN or 0.0?
    mosaic[i].flags     = 0;
    mosaic[i].secz      = NAN;
    mosaic[i].photcode  = 0;
    mosaic[i].skipCal   = FALSE;
    
    memset (&mosaic[i].coords, 0, sizeof(Coords));

    MosaicN_IMAGE[i] = 10;
    MosaicN_Image[i] = 0;
    ALLOCATE (MosaicToImage[i], off_t, MosaicN_IMAGE[i]);
    MosaicToImage[i][0] = -1;
  }

  ALLOCATE (ImageToMosaic, off_t, Nimage); // mosaic to which image belongs

  // assign each image to a mosaic
  int Nsimple = 0;
  for (i = 0; i < Nimage; i++) {
    ImageToMosaic[i] = -1;

    if (!isMosaicChip(image[i].photcode)) {
      Nsimple ++;
      continue;
    }

    start = image[i].tzero;
    stop  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);

    j = findMosaic(startTimesMosaic, Nmosaic, start);
    if (j == -1) {
      fprintf (stderr, "programming error? all image images should belong to a mosaic\n");
      abort();
    }

    // add reference from image to mosaic
    ImageToMosaic[i] = j;

    // have we already found this mosaic?
    found = (MosaicN_Image[j] > 0);

    /* add image to mosaic image list */
    MosaicToImage[j][MosaicN_Image[j]] = i;
    MosaicN_Image[j] ++;
    if (MosaicN_Image[j] == MosaicN_IMAGE[j]) {
      MosaicN_IMAGE[j] += 10;
      REALLOCATE (MosaicToImage[j], off_t, MosaicN_IMAGE[j]);
    }
    if (found) continue;
    
    /* a new mosaic, define ranges */
    if (mosaic[j].start != start) { 
      fprintf (stderr, "error?\n");
      abort();
    }
    mosaic[j].stop     = stop;
    mosaic[j].McalPSF   = 0.0;
    mosaic[j].McalAPER  = 0.0;
    mosaic[j].dMcal     = 0.0;
    mosaic[j].McalChiSq = 0.0;
    mosaic[j].dMsys     = image[i].dMagSys;
    mosaic[j].flags     = image[i].flags;
    mosaic[j].secz      = image[i].secz;
    mosaic[j].photcode  = GetPhotcodeEquivCodebyCode (image[i].photcode);
  }
  MARKTIME("assign images to mosaic: %f sec\n", dtime);

  // free this or not?
  free (MosaicN_IMAGE);
  free (startTimes);
  free (startTimesMosaic);

  if (mergeMcal) {
    initMosaicMcal (image, Nimage);
  }

  fprintf (stderr, "matched %d images to %d mosaics\n", (int) Nimage, (int) Nmosaic);
  return;
}

Mosaic *getMosaicForImage (off_t im) {

  if (im < 0) return NULL;
  if (!ImageToMosaic) return NULL;

  off_t m = ImageToMosaic[im];
  if (m < 0) return NULL;
  if (m >= Nmosaic) return NULL;

  return (&mosaic[m]);
}

// use bisection to find the overlapping mosaic (returns exact match)
// startTimes is a sorted, unique list of times
// start might not be in the list
off_t findMosaic (unsigned int *startTimes, off_t Nmosaic, unsigned int start) {

  off_t Nlo, Nhi, N;

  // find the last mosaic before start
  Nlo = 0; // first valid startTimes value
  Nhi = Nmosaic - 1; // last valid startTimes value

  // if start is not in this range, return -1
  if (start < startTimes[Nlo]) return (-1);
  if (start > startTimes[Nhi]) return (-1);

  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (startTimes[N] < start) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N, Nmosaic - 1);
    }
  }
  // we now have : startTimes[Nlo] < start <= starTimes[Nhi]

  // find a matched mosaic starting from Nlo, or return -1
  for (N = Nlo; N <= Nhi; N++) { 
    if (startTimes[N] == start) return N;
  }
  return (-1);
}

// this function sets mosaic->coords to the median of the individual chips.  This
// coordinate frame is used by the parallel region analysis to assign exposures (mosaics)
// to specific machines by a single center (rather than individual chips)
void setMosaicCenters (Image *image, off_t Nimage) {
  OHANA_UNUSED_PARAM(Nimage);

  /* find max dR, dD range for all mosaics */
  /* define mosaic.coords to cover dR, dD */
  /* send results to initGridBins */

  off_t i, j, m, NX, NY, NC, Nc;
  double R, D, Rmid, Dmid;
  double *Rc, *Dc;

  NC = 100;
  ALLOCATE (Rc, double, NC);
  ALLOCATE (Dc, double, NC);

  for (i = 0; i < Nmosaic; i++) {
    Nc = 0;
    Rmid = Dmid = NAN;
    for (j = 0; j < MosaicN_Image[i]; j++) {
      m = MosaicToImage[i][j];

      NX = image[m].NX;
      NY = image[m].NY;
      if (!XY_to_RD (&R, &D, 0.5*NX, 0.5*NY, &image[m].coords)) continue;
      R = ohana_normalize_angle_to_midpoint (R, 180.0);

      // Exclude images with crazy astrometry
      // XXX NOTE : this is gpc1-specific
      { 
	double dP1 = hypot(image[m].coords.pc1_1, image[m].coords.pc1_2);
	double dP2 = hypot(image[m].coords.pc2_1, image[m].coords.pc2_2);
	if (fabs(dP1 - 1.0) > 0.02) continue;
	if (fabs(dP2 - 1.0) > 0.02) continue;

	double X00, Y00, X10, Y10, X01, Y01;
	XY_to_LM (&X00, &Y00, 0.0, 0.0, &image[m].coords);
	XY_to_LM (&X10, &Y10, image[m].NX, 0.0, &image[m].coords);
	XY_to_LM (&X01, &Y01, 0.0, image[m].NY, &image[m].coords);
	double dS0 = hypot ((X00 - X10), (Y00 - Y10));
	double dS1 = hypot ((X00 - X01), (Y00 - Y01));
	if (dS0 > 6000) continue;
	if (dS1 > 6500) continue;
      }	

      Rc[Nc] = R;
      Dc[Nc] = D;
      Nc ++;
      if (Nc >= NC) {
	NC += 100;
	REALLOCATE (Rc, double, NC);
	REALLOCATE (Dc, double, NC);
      }
    }

    if (Nc > 0) {
      dsort (Rc, Nc);
      if (Rc[Nc-1] - Rc[0] > 180.0) {
	// in our list, Rc is in the range 0.0 to 360.0.  
	// any mosaic which is close to the 0.0, 360.0 boundary may have some on
	// one side or the other.  count how many have values more than Rc[0] + 180.
	// if more than half are at the large end, re-normalize to that range
	int Nbig = 0;
	for (j = 1; j < Nc; j++) {
	  if (Rc[j] - Rc[0] > 180.0) Nbig ++;
	}
	if (Nbig  > 0.5*Nc) {
	  for (j = 0; j < Nc; j++) {
	    Rc[j] = ohana_normalize_angle_to_midpoint (Rc[j], 360.0);
	  }
	  dsort (Rc, Nc);
	} else if (Nbig > 0) {
	  for (j = 0; j < Nc; j++) {
	    Rc[j] = ohana_normalize_angle_to_midpoint (Rc[j], 0.0);
	  }
	  dsort (Rc, Nc);
	}
      }
      dsort (Dc, Nc);

      Rmid = Rc[(int)(0.5*Nc)];
      Dmid = Dc[(int)(0.5*Nc)];
    }

    InitCoords (&mosaic[i].coords, "DEC--TAN");
    mosaic[i].coords.crval1 = Rmid;
    mosaic[i].coords.crval2 = Dmid;
    mosaic[i].coords.cdelt1 = 1.0 / 3600.0;
    mosaic[i].coords.cdelt2 = 1.0 / 3600.0;
  }
  return;
}

void initMosaicMcal (Image *image, off_t Nimage) {
  OHANA_UNUSED_PARAM(Nimage);

  double McalPSF, McalAPER, dMcal, McalChiSq;

  fprintf (stderr, "*** moving Mcal from image.Mcal to mosaic.Mcal ***\n");

  for (off_t i = 0; i < Nmosaic; i++) {
    McalPSF = McalAPER = dMcal = McalChiSq = 0;
    for (off_t j = 0; j < MosaicN_Image[i]; j++) {
      off_t m = MosaicToImage[i][j];

      if (!isfinite(image[m].McalPSF)) {
	image[m].McalPSF   = 0.0;
	image[m].McalAPER  = 0.0;
	image[m].dMcal     = 0.0;
	image[m].McalChiSq = 0.0;
	fprintf (stderr, "warning: resetting NAN value for Mcal %s\n", image[m].name);
      }

      McalPSF   += image[m].McalPSF;
      McalAPER  += image[m].McalAPER;
      dMcal     += image[m].dMcal;
      McalChiSq += image[m].McalChiSq;

      image[m].McalPSF   = 0.0;
      image[m].McalAPER  = 0.0;
      image[m].dMcal     = NAN;
      image[m].McalChiSq = NAN;
    }

    mosaic[i].McalPSF   = McalPSF / MosaicN_Image[i];
    mosaic[i].McalAPER  = McalAPER / MosaicN_Image[i];
    mosaic[i].dMcal     = dMcal / MosaicN_Image[i];
    mosaic[i].McalChiSq = McalChiSq / MosaicN_Image[i];
  }
  return;
}

void setMcalFromMosaics () {

  off_t i, j, im, Nimage;
  Image *image;

  if (!MOSAIC_ZEROPT) return;

  image = getimages (&Nimage, NULL);

  fprintf (stderr, "*** return Mcal from mosaic.Mcal to image.Mcal ***\n");

  // copy the mosaic results to the images.  set the mosaic Mcal to 0.0 since we have moved its
  // impact to the images
  for (i = 0; i < Nmosaic; i++) {
    for (j = 0; j < MosaicN_Image[i]; j++) {
      im = MosaicToImage[i][j];
      if (mosaic[i].flags & ID_IMAGE_MOSAIC_PHOTCAL) {
	image[im].McalPSF     = mosaic[i].McalPSF;
	image[im].McalAPER    = mosaic[i].McalAPER;
	image[im].dMcal       = mosaic[i].dMcal;
	image[im].McalChiSq   = mosaic[i].McalChiSq;
	image[im].ubercalDist = mosaic[i].ubercalDist;
	image[im].dMagSys     = mosaic[i].dMsys;
	image[im].nFitPhotom  = mosaic[i].nFitPhotom;
      }
      image[im].flags 	   |= (mosaic[i].flags & ID_IMAGE_PHOTOM_FEW);
      image[im].flags 	   |= (mosaic[i].flags & ID_IMAGE_PHOTOM_POOR);
      image[im].flags 	   |= (mosaic[i].flags & ID_IMAGE_MOSAIC_POOR);
      image[im].flags 	   |= (mosaic[i].flags & ID_IMAGE_TGROUP_PHOTCAL);
      image[im].flags 	   |= (mosaic[i].flags & ID_IMAGE_MOSAIC_PHOTCAL);
      image[im].flags 	   |= (mosaic[i].flags & ID_IMAGE_IMAGE_PHOTCAL);
    }
    mosaic[i].McalPSF  = 0.0;
    mosaic[i].McalAPER = 0.0;
  }      
}

void markBadMosaic (int myMosaic) {

  off_t Nimage;
  Image *image;

  // this mosaic has been identified as poor.
  mosaic[myMosaic].flags |= ID_IMAGE_MOSAIC_POOR;

  image = getimages (&Nimage, NULL);

  // all images should be marked as coming from a bad mosaic
  // these will be fitted independently.  Make the starting solution consistent by
  // setting the image McalPSF for the current best fit from the mosaic

  for (off_t i = 0; i < MosaicN_Image[myMosaic]; i++) {
    off_t im = MosaicToImage[myMosaic][i];
    image[im].flags |= ID_IMAGE_MOSAIC_POOR;
    image[im].McalPSF = mosaic[myMosaic].McalPSF;
    image[im].McalAPER = mosaic[myMosaic].McalAPER;
  }

  mosaic[myMosaic].McalPSF  = 0.0;
  mosaic[myMosaic].McalAPER = 0.0;
}

void markGoodMosaic (int myMosaic) {

  off_t Nimage;
  Image *image;

  // this mosaic has been identified as good.
  mosaic[myMosaic].flags &= ~ID_IMAGE_MOSAIC_POOR;

  image = getimages (&Nimage, NULL);

  // all images should be marked as NOT coming from a bad mosaic
  for (off_t i = 0; i < MosaicN_Image[myMosaic]; i++) {
    off_t im = MosaicToImage[myMosaic][i];
    image[im].flags &= ~ID_IMAGE_MOSAIC_POOR;
  }
}

void initMosaicBins (Catalog *catalog, int Ncatalog, int doMosaicList) {

  off_t i, j;

  /* measure -> mosaic */
  if (!MOSAIC_ZEROPT) return;

  ALLOCATE (MeasureToMosaic, off_t *, Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    ALLOCATE (MeasureToMosaic[i], off_t, MAX (catalog[i].Nmeasure, 1));
    for (j = 0; j < catalog[i].Nmeasure; j++) MeasureToMosaic[i][j] = -1;
  }

  if (doMosaicList) {
    /* mosaic -> measure */
    ALLOCATE (N_onMosaic, off_t,   Nmosaic);
    ALLOCATE (N_ONMOSAIC, off_t,   Nmosaic);
    ALLOCATE (MosaicToCatalog, off_t *, Nmosaic);
    ALLOCATE (MosaicToMeasure, off_t *, Nmosaic);

    for (i = 0; i < Nmosaic; i++) {
      N_onMosaic[i] = 0;
      N_ONMOSAIC[i] = 100;
      ALLOCATE (MosaicToCatalog[i], off_t, N_ONMOSAIC[i]);
      ALLOCATE (MosaicToMeasure[i], off_t, N_ONMOSAIC[i]);
    }
  }
}

void freeMosaicBins (int Ncatalog, int doMosaicList) {

  off_t i;

  /* measure -> mosaic */
  if (!MOSAIC_ZEROPT) return;

  for (i = 0; i < Ncatalog; i++) {
    free (MeasureToMosaic[i]);
  }
  free (MeasureToMosaic);

  if (doMosaicList) {
    /* mosaic -> measure */
    for (i = 0; i < Nmosaic; i++) {
      free (MosaicToCatalog[i]);
      free (MosaicToMeasure[i]);
    }
    free (N_onMosaic);
    free (N_ONMOSAIC);
    free (MosaicToCatalog);
    free (MosaicToMeasure);
  }
}

void freeMosaics () {
  
  if (!MOSAIC_ZEROPT) return;

  for (int i = 0; i < Nmosaic; i++) {
    FREE (MosaicToImage[i]);
  }
  
  FREE (mosaic);
  FREE (MosaicToImage);
  FREE (MosaicN_Image);
  FREE (ImageToMosaic); // mosaic to which image belongs
}

int findMosaics (Catalog *catalog, int Ncatalog, int doMosaicList) {

  if (!MOSAIC_ZEROPT) return (FALSE);
  // if we are calibrating by mosaic, redefine myDet == on one of my mosaics

  int Nmatch = 0;
  for (int i = 0; i < Ncatalog; i++) {
    for (off_t j = 0; j < catalog[i].Nmeasure; j++) {
      catalog[i].measureT[j].myDet = FALSE; // a detetion is not mine until proven otherwise

      if (TimeSelect) {
	if (catalog[i].measureT[j].t < TSTART) continue;
	if (catalog[i].measureT[j].t > TSTOP) continue;
      }

      int Ns = GetActivePhotcodeIndex (catalog[i].measureT[j].photcode);
      if (Ns < 0) continue;
      matchMosaics (catalog, j, i, doMosaicList);
      Nmatch ++;
    }
  }
  // fprintf (stderr, "matched %d detections to mosaics\n", Nmatch);
  return (TRUE);
}

void matchMosaics (Catalog *catalog, off_t meas, int cat, int doMosaicList) {

  off_t idx, ID, mosID;
  MeasureTiny *measure;

  measure = &catalog[cat].measureT[meas];

  ID = measure[0].imageID;
  idx = getImageByID (ID);
  if (idx == -1) {
    if (VERBOSE2) fprintf (stderr, "missed measurement "OFF_T_FMT", %d\n", meas, cat);
    return;
  }

  mosID = ImageToMosaic[idx];
  if (mosID < 0) {
    // Image *image = getimage(idx);
    // fprintf (stderr, "unmatched image %s\n", image[0].name);
    // skip measurements from simple chips (not matched to a mosaic by definition)
    return;
  }

  // test to check we got the right match:
  {
    Image *image = getimage(idx);
    // XXX we are now matching with just tzero.  be careful for cameras with drift
    // unsigned int imageStart = image[0].tzero - MAX(0.01*image[0].trate*image[0].NY, 1);
    unsigned int imageStart = image[0].tzero;
    if (imageStart != mosaic[mosID].start) {
      fprintf (stderr, "error in image to mosaic match\n");
      abort();
    }
  }

  // this measurement is on one of my mosaics, mark it as mine.
  catalog[cat].measureT[meas].myDet = TRUE;
  MeasureToMosaic[cat][meas] = mosID;

  if (doMosaicList) {
    MosaicToCatalog[mosID][N_onMosaic[mosID]] = cat;
    MosaicToMeasure[mosID][N_onMosaic[mosID]] = meas;
    N_onMosaic[mosID] ++;
    
    if (N_onMosaic[mosID] == N_ONMOSAIC[mosID]) {
      N_ONMOSAIC[mosID] += 100;
      REALLOCATE (MosaicToCatalog[mosID], off_t, N_ONMOSAIC[mosID]);
      REALLOCATE (MosaicToMeasure[mosID], off_t, N_ONMOSAIC[mosID]);
    }	
  }
  return;
}

float getMmos (off_t meas, int cat) {

  off_t i;
  float value;

  if (!MOSAIC_ZEROPT) return (0);

  // unassigned measurements belong to simple chips
  i = MeasureToMosaic[cat][meas];
  if (i == -1) return (0.0);

  // if the mosaic cannot be calibrated (too few measurements, skip it)
  // XXX: if the mosaic is bad (few) but the chips are OK, I can used them.
  // XXX: if (mosaic[i].flags & ID_IMAGE_PHOTOM_FEW) return (NAN);  
  value = mosaic[i].McalPSF;
  return (value);
}

int getMosaicFlags (off_t meas, int cat) {

  if (!MOSAIC_ZEROPT) return (0);

  // unassigned measurements belong to simple chips
  int i = MeasureToMosaic[cat][meas];
  if (i == -1) return (0);

  return (mosaic[i].flags);
}

typedef struct {
  int Nfew;
  int Nbad;
  int Ncal;
  int Ngrp;
  int Ngrid;
  int Nrel;
  int Nsys;
  int Nskip;
  off_t Nmax;
  FitDataSet psfStars;
  FitDataSet kronStars;
  FitDataSet brightStars;
} SetMmosInfo;

enum {THREAD_RUN, THREAD_DONE};

typedef struct {
  int entry;
  int state;
  Catalog *catalog;
  Image *image;
  SetMmosInfo info;
} ThreadInfo;

int setMmos_mosaic (Mosaic *mosaic, off_t Nmos, Image *image, Catalog *catalog, SetMmosInfo *info);
void *setMmos_worker (void *data);
int setMmos_threaded (Catalog *catalog);

void SetMmosInfoInit (SetMmosInfo *info, off_t Nmax, int allocLists) {
  info->Nfew = 0;
  info->Nbad = 0;
  info->Ncal = 0;
  info->Nrel = 0;
  info->Ngrid = 0;
  info->Nskip = 0;
  info->Nsys = 0;

  info->Nmax = Nmax;

  if (allocLists) {
    // we can only fit the zero point for individual exposures, not the airmass
    FitDataSetAlloc (&info->psfStars,    Nmax, 0, 0); 
    FitDataSetAlloc (&info->kronStars,   Nmax, 0, 0); 
    FitDataSetAlloc (&info->brightStars, Nmax, 0, 0); 

    // until the analysis has converged a bit, do not use the IRLS analysis
    if (UseStandardOLS(ZPT_MOSAIC)) {
      info->brightStars.MaxIterations = 0;
      info->kronStars.MaxIterations = 0;
      info->psfStars.MaxIterations = 0;
    }
  }
}

void SetMmosInfoFree (SetMmosInfo *info) {
  FitDataSetFree (&info->brightStars);
  FitDataSetFree (&info->kronStars);
  FitDataSetFree (&info->psfStars);
}

void SetMmosInfoAccum (SetMmosInfo *summary, SetMmosInfo *results) {
  summary->Nfew  += results->Nfew ;
  summary->Nsys  += results->Nsys ;
  summary->Nbad  += results->Nbad ;
  summary->Ncal  += results->Ncal ;
  summary->Nrel  += results->Nrel ;
  summary->Ngrid += results->Ngrid;
  summary->Nskip += results->Nskip;
}

static int npass_output = 0;

// mutex to lock setMmos_worker operations 
static pthread_mutex_t setMmos_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nextMosaic = 0;

// we have an array of mosaics (mosaic, Nmosaic).  we need to hand out mosaics one at a time to
// the worker threads as they need
off_t getNextMosaicForThread () {

  pthread_mutex_lock (&setMmos_mutex);
  if (nextMosaic >= Nmosaic) {
    pthread_mutex_unlock (&setMmos_mutex);
    return (-1);
  }
  int thisMosaic = nextMosaic;
  nextMosaic ++;

  pthread_mutex_unlock (&setMmos_mutex);
  return (thisMosaic);
}

int setMmos (Catalog *catalog) {

  off_t i, N, Nmax;
  Image *image;

  if (!MOSAIC_ZEROPT) return FALSE;
  if (FREEZE_MOSAICS) return FALSE;

  if (MOSAIC_ZPT_MODE == MOSAIC_ZPT_MODE_NONE) return FALSE;

  // plots cannot be done in a threaded context (the plot commands collide)
  // so do not run setMmos in threaded mode if PLOTSTUFF is set
  if (NTHREADS && !PLOTSTUFF) {
    int status = setMmos_threaded (catalog);
    return status;
  }

  image = getimages (&N, NULL);

  fprintf (stderr, "limiting negative clouds to %f\n", CLOUD_TOLERANCE);

  Nmax = 0;
  for (i = 0; i < Nmosaic; i++) {
    Nmax = MAX (Nmax, N_onMosaic[i]);
  }

  SetMmosInfo info;
  SetMmosInfoInit (&info, Nmax, TRUE);

  // int savePlotDelay = PLOTDELAY;
  // if (PLOTSTUFF) PLOTDELAY = 0.0;

  for (i = 0; i < Nmosaic; i++) {
    setMmos_mosaic (&mosaic[i], i, image, catalog, &info);
  }
  SetMmosInfoFree (&info);

  if (PLOTSTUFF) {
    fprintf (stdout, "press return\n"); 
    if (fscanf (stdin, "%*c") != 1) fprintf (stderr, "\n");
  }

  npass_output ++;

  fprintf (stderr, "%d mosaics marked having too few measurements (Nbad: %d, Ncal: %d, Ngrid: %d, Nrel: %d, Nsys: %d)\n", info.Nfew, info.Nbad, info.Ncal, info.Ngrid, info.Nrel, info.Nsys);

  return (TRUE);
}
  
// 'mosaic' is a pointer to the current mosaic of interest (Nmos)
int setMmos_mosaic (Mosaic *myMosaic, off_t Nmos, Image *image, Catalog *catalog, SetMmosInfo *info) {

  off_t j, NimageReal;

  FitDataSet *psfStars = &info->psfStars;
  FitDataSet *kronStars = &info->kronStars;
  FitDataSet *brightStars = &info->brightStars;

  assert (Nmos >= 0);
  assert (Nmos < Nmosaic);

  int Nsecfilt = GetPhotcodeNsecfilt ();

  int minUbercalDist = 1000;

  // unset this flag at start (set below if zeropoint is calculated)
  myMosaic->flags &= ~ID_IMAGE_MOSAIC_PHOTCAL;  // unset this flag

  // calculate the statistics for both good and bad exposures, but only set Mmos if the rules allow
  // we only skip the statistics for nights with too few measurements or exposures

  // if we are fitting TGroup zero points, when we identify the bad TGroups, we proceed
  // to fit the mosaics which are from the bad TGroups

  // if we are fitting Mosaic zero points, when we identify the bad nights, we proceed
  // to fit the images which are from the bad Mosaics

  int badNight  = (myMosaic[0].flags & ID_IMAGE_NIGHT_POOR);
  int badMosaic = (myMosaic[0].flags & ID_IMAGE_MOSAIC_POOR);
  int useMmos   = TRUE;

  // in BAD_NIGHT mode, we fit ONLY mosaics in bad nights (do not fit mosaics from good nights)
  if (MOSAIC_ZPT_MODE == MOSAIC_ZPT_MODE_BAD_NIGHT) {
    if (!badNight) useMmos = FALSE;
  }

  // in BAD_NIGHT_GOOD_MOSAIC mode, we fit ONLY good mosaics in bad nights
  if ((MOSAIC_ZPT_MODE == MOSAIC_ZPT_MODE_BAD_NIGHT_GOOD_MOSAIC)) {
    if (!badNight) useMmos = FALSE; // do not fit good nights
    if (badMosaic) useMmos = FALSE; // do not fit bad mosaics
  }

  // in GOOD_MOSAIC mode, we fit good mosaics ignoring night state
  // This case should only be used when not fitting nights / tgroups 
  if ((MOSAIC_ZPT_MODE == MOSAIC_ZPT_MODE_GOOD_MOSAIC)) {
    if (badMosaic) useMmos = FALSE; // do not fit bad mosaics
  }

  // Image *imageReal = getimages (&NimageReal, NULL); returned pointer is not used
  getimages (&NimageReal, NULL);

  // UBERCAL image: if this is an ubercal image, set minUbercalDist to 0:
  // we optionally do not recalibrate images with UBERCAL zero points 
  if (myMosaic[0].flags & ID_IMAGE_PHOTOM_UBERCAL) {
    myMosaic[0].ubercalDist = 0;
    // propagate ubercalDist to the images
    for (j = 0; j < MosaicN_Image[Nmos]; j++) {
      off_t im = MosaicToImage[Nmos][j];
      assert (im < NimageReal);
      assert (im >= 0);
      image[im].ubercalDist = myMosaic[0].ubercalDist;
      // fprintf (stderr, "%d %d %d\n", (int) i, (int) im, image[im].ubercalDist);
    }
    // XXX if KEEP_UBERCAL, calculate stats but do not change Mmos
    if (KEEP_UBERCAL) return TRUE;
  }

  // do not modify the calibration for mosaics with partial images loaded (skipCal TRUE)
  if (myMosaic[0].skipCal) {
    info->Nskip ++;
    return TRUE;
  }

  int testImage = FALSE;
  // testImage |= (abs(myMosaic[0].start - 1324104046) < 10);

  FILE *fout = NULL;
  if (testImage) {
    char filename[64];
    snprintf (filename, 64, "test.%05d.%02d.dat", (int) Nmos, npass_output);
    fout = fopen (filename, "w");
    save_test_mosaic_measures (fout, Nmos, catalog);
    fclose (fout);
  }

  // number of stars to measure the bright-end scatter
  int Nbright = 0;

  int N = 0;
  for (j = 0; j < N_onMosaic[Nmos]; j++) {
      
    off_t m = MosaicToMeasure[Nmos][j];
    off_t c = MosaicToCatalog[Nmos][j];
      
    if (catalog[c].measureT[m].dbFlags & MEAS_BAD) {
      info->Nbad ++;
      continue;
    }
    float Mcal = getMcal  (m, c, MAG_CLASS_PSF);
    if (isnan(Mcal)) {
      info->Ncal++;
      continue;
    }
    float Mgrp = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL);
    if (isnan(Mgrp)) {
      info->Ngrp++;
      continue;
    }
    float Mgrid = getMgridTiny (&catalog[c].measureT[m]);
    if (isnan(Mgrid)) {
      info->Ngrid ++;
      continue;
    }

    // Mrel* is the average magnitude for this star.  For PS1 stacks, we have too much
    // PSF variability.  We need to calibrate the PSF magnitudes separately from the
    // Aperture-like magnitues.  (We have an option to use the kron magnitudes or the
    // other apertures here).  I basically need to do this analysis separately for each
    // magnitude type
    
    float MrelPSF = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP);
    if (isnan(MrelPSF)) {
      info->Nrel ++;
      continue;
    }
    float MrelKron = getMrel  (catalog, m, c, MAG_CLASS_KRON, MAG_SRC_CHP);
      
    // image.Mcal is not supposed to include the flat-field correction, so we need to
    // apply that offset as well here for this image (in other words, each detection is
    // being compared to the model, excluding the zero point, Mcal.  The model includes
    // the flat-correction.  NOTE the sign of Mflat (Image.Mcal = Measure.Mcal + Mflat).
    // this was inconsistent pre r41606

    float Mflat = getMflat (m, c, catalog);

    off_t n = catalog[c].measureT[m].averef;
    float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);
    if (isnan(MsysPSF)) {
      info->Nsys++;
      continue;
    }
    float MsysKron = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_KRON);

    PhotCode *code = GetPhotcodebyCode (catalog[c].measureT[m].photcode);
    if (!code) goto skip;
    if (code->equiv < 1) goto skip;
    int Nsec = GetPhotcodeNsec (code->equiv);
    if (Nsec == -1) goto skip;
    minUbercalDist = MIN (catalog[c].secfilt[n*Nsecfilt + Nsec].ubercalDist, minUbercalDist);

  skip:
    assert (N < info->Nmax);
    assert (N >= 0);
    assert (Nbright < info->Nmax);
    assert (Nbright >= 0);

    float Moff =  Mcal + Mgrp + Mgrid + Mflat;

    psfStars->alldata-> yVector[N] = MsysPSF - MrelPSF - Moff;
    psfStars->alldata->dyVector[N] = MAX (catalog[c].measureT[m].dM, MIN_ERROR);

    kronStars->alldata-> yVector[N] = MsysKron - MrelKron - Moff;
    kronStars->alldata->dyVector[N] = psfStars->alldata->dyVector[N];

    if (catalog[c].measureT[m].dM < IMFIT_SYS_SIGMA_LIM) {
      brightStars->alldata-> yVector[Nbright] = psfStars->alldata-> yVector[N];
      brightStars->alldata->dyVector[Nbright] = psfStars->alldata->dyVector[N];
      Nbright ++;
    }
    N++;

  }
  /* N_onMosaic[Nmos] is all measurements, N is good measurements */

  /* skip mosaics with too few good measurements or too many bad measurements */
  int mark = (N < IMAGE_TOOFEW) || (N < IMAGE_GOOD_FRACTION*N_onMosaic[Nmos]);
  if (mark) {
    myMosaic->flags |= ID_IMAGE_PHOTOM_FEW;
    myMosaic->McalPSF    = 0.0;
    myMosaic->McalAPER   = 0.0;
    myMosaic->dMcal      = NAN;
    myMosaic->stdev      = NAN;
    myMosaic->dMmin      = NAN;
    myMosaic->dMmax      = NAN;
    myMosaic->McalChiSq  = NAN;
    myMosaic->nFitPhotom = 0;
    myMosaic->nValPhotom = N;
    info->Nfew ++;
    if (testImage) {
      fprintf (stderr, "NOTE: *** marked test image poor : %d %d %d***\n", (int) N, (int) IMAGE_TOOFEW, (int) (IMAGE_GOOD_FRACTION*N_onMosaic[Nmos]));
    }
    return TRUE; // skip mosaics with too few good measurements
  } else {
    myMosaic[0].flags &= ~ID_IMAGE_PHOTOM_FEW;
  }

  // soften the errors based on the scatter
  FitDataSetSoften (psfStars, N);
  
  fit1d_irls (psfStars, N);
  fit1d_irls (brightStars, Nbright);
  fit1d_irls (kronStars, N);

  if (useMmos) {
    myMosaic->McalPSF    = psfStars->bSaveArray[0][0];
    myMosaic->McalAPER   = kronStars->bSaveArray[0][0];
    myMosaic->flags |= ID_IMAGE_MOSAIC_PHOTCAL;  // set this flag
  } else {
    myMosaic->McalPSF    = 0.0;
    myMosaic->McalAPER   = 0.0;
  }

  // for now, I have no reason to measure these separately for camera-level images
  myMosaic->dMcal      = psfStars->bSigma[0];	    
  myMosaic->stdev      = psfStars->sigma;
  myMosaic->McalChiSq  = psfStars->chisq;	    
  myMosaic->nFitPhotom = psfStars->Nmeas;	    
  myMosaic->nValPhotom = N;	    

  // bright end scatter
  myMosaic->dMsys = brightStars->sigma; // stdev of bright star mags

  if (testImage) {
    fprintf (stderr, "test image %d (%d) %f %f %d ... ", (int) Nmos, myMosaic->start, myMosaic->McalPSF, myMosaic->dMcal, myMosaic->nFitPhotom);
    fprintf (stderr, "%f %f  :  %f\n", myMosaic[0].McalPSF, myMosaic[0].dMsys, myMosaic[0].McalChiSq);
  }

  if (PLOTSTUFF) {
    fprintf (stderr, "Mmos: %6.3f +/- %6.3f %5d %5d | %s\n", myMosaic->McalPSF, myMosaic->dMcal, myMosaic->nFitPhotom, N, image[MosaicToImage[Nmos][0]].name);
    plot_setMcal (psfStars->alldata->yVector, N);
  }

  // minUbercalDist calculated here is the min value for any star owned by this image
  // since this particular image is tied to that star, bump its distance by 1
  myMosaic[0].ubercalDist = minUbercalDist + 1;

  // propagate ubercalDist to the images
  for (j = 0; j < MosaicN_Image[Nmos]; j++) {
    off_t im = MosaicToImage[Nmos][j];
    image[im].ubercalDist = myMosaic[0].ubercalDist;
  }
  
  fprintf (stderr, "MOSAIC time %.6f photcode %d Mcal: %f, dMcal: %f, chisq: %f, %d of %d, useMmos: %d\n",
	   ohana_sec_to_mjd(myMosaic[0].start), myMosaic[0].photcode, psfStars->bSaveArray[0][0], myMosaic[0].dMcal, myMosaic[0].McalChiSq, myMosaic[0].nFitPhotom, myMosaic[0].nValPhotom, useMmos);

  return TRUE;
}
  
int save_test_mosaic_measures (FILE *fout, int Nmos, Catalog *catalog) {

  int Nsecfilt = GetPhotcodeNsecfilt ();

  for (int j = 0; j < N_onMosaic[Nmos]; j++) {
      
    off_t m = MosaicToMeasure[Nmos][j];
    off_t c = MosaicToCatalog[Nmos][j];
      
    // NOTE : we are only using Mcal == McalPSF in this function; we set McalAPER to McalPSF
    float Mcal     = getMcal  (m, c, MAG_CLASS_PSF);
    // float Mgrp     = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL);
    float Mgrid    = getMgridTiny (&catalog[c].measureT[m]);
    float MrelPSF  = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP);
    float Mflat    = getMflat (m, c, catalog);

    off_t n = catalog[c].measureT[m].averef;
    float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);
    
    float delta = MsysPSF - MrelPSF - Mcal - Mgrid - Mflat;

    int isBad = (catalog[c].measureT[m].dbFlags & MEAS_BAD);
    
    fprintf (fout, "%f %f : %f %f %f %f %f  : %f %d\n", catalog[c].averageT[n].R, catalog[c].averageT[n].D, MsysPSF, MrelPSF, Mcal, Mgrid, Mflat, delta, isBad);
  }
  return TRUE;
}

int setMmos_threaded (Catalog *catalog) {

  int i;
  off_t N;

  Image *image = getimages (&N, NULL);

  fprintf (stderr, "limiting negative clouds to %f\n", CLOUD_TOLERANCE);

  off_t Nmax = 0;
  for (i = 0; i < Nmosaic; i++) {
    Nmax = MAX (Nmax, N_onMosaic[i]);
  }

  SetMmosInfo summary;
  SetMmosInfoInit (&summary, Nmax, FALSE);

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
  
  pthread_t *threads;
  ALLOCATE (threads, pthread_t, NTHREADS);

  ThreadInfo *threadinfo;
  ALLOCATE (threadinfo, ThreadInfo, NTHREADS);

  // each time this function is called, we cycle through the available mosaics
  // make sure we start at 0
  nextMosaic = 0;;

  // launch N worker threads
  for (i = 0; i < NTHREADS; i++) {
    threadinfo[i].entry = i;
    threadinfo[i].state = THREAD_RUN;
    threadinfo[i].catalog  =  catalog;
    threadinfo[i].image    =    image;

    // we do NOT allocate the arrays here, we only supply basic info (Nmax, Nloop) used in
    // the threads to allocate the arrays and set the MaxIterations
    SetMmosInfoInit (&threadinfo[i].info, Nmax, FALSE);
    pthread_create (&threads[i], NULL, setMmos_worker, &threadinfo[i]);
  }
  pthread_attr_destroy (&attr);

  // wait until all threads have finished
  while (1) {
    int allDone = TRUE;
    for (i = 0; i < NTHREADS; i++) {
      if (threadinfo[i].state == THREAD_RUN) allDone = FALSE;
    }
    if (allDone) {
      break;
    }
    usleep (500000);
  }

  // all threads are done, free the threads array and grab the info
  free (threads);
  
  // report stats & summary from the threads
  for (i = 0; i < NTHREADS; i++) {
    fprintf (stderr, "setMmos thread %d : %d mosaics marked having too few measurements (Nbad: %d, Ncal: %d, Ngrid: %d, Nrel: %d, Nsys: %d), %d partials skipped\n", 
	     i, 
	     threadinfo[i].info.Nfew, 
	     threadinfo[i].info.Nbad, 
	     threadinfo[i].info.Ncal, 
	     threadinfo[i].info.Ngrid, 
	     threadinfo[i].info.Nrel, 
	     threadinfo[i].info.Nsys,
      	     threadinfo[i].info.Nskip);
    SetMmosInfoAccum (&summary, &threadinfo[i].info);
  }
  fprintf (stderr, "total : %d mosaics marked having too few measurements (Nbad: %d, Ncal: %d, Ngrid: %d, Nrel: %d, Nsys: %d), %d partials skipped\n", 
	   summary.Nfew, 
	   summary.Nbad, 
	   summary.Ncal, 
	   summary.Ngrid, 
	   summary.Nrel, 
	   summary.Nsys, 
	   summary.Nskip);
  free (threadinfo);
  // XXX SetMmosInfoFree (&summary);

  npass_output ++;

  return TRUE;
}

void *setMmos_worker (void *data) {

  ThreadInfo *threadinfo = data;

  SetMmosInfo results;
  SetMmosInfoInit (&results, threadinfo->info.Nmax, TRUE);

  while (1) {

    off_t i = getNextMosaicForThread();
    if (i == -1) {
      threadinfo->state = THREAD_DONE;
      return NULL;
    }

    Catalog *catalog = threadinfo->catalog;
    Image *image = threadinfo->image;

    setMmos_mosaic (&mosaic[i], i, image, catalog, &results);
    SetMmosInfoAccum (&threadinfo->info, &results);
  }

  SetMmosInfoFree (&results);
  return NULL;
}

StatType statsMosaicM (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  off_t i;
  int n;
  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  if (!MOSAIC_ZEROPT) return (stats);

  ALLOCATE (list, double, Nmosaic);
  ALLOCATE (dlist, double, Nmosaic);

  n = 0;
  for (i = 0; i < Nmosaic; i++) {
    if (mosaic[i].flags & (ID_IMAGE_MOSAIC_POOR | ID_IMAGE_PHOTOM_FEW)) continue;
    if (mosaic[i].skipCal) continue;
    if (KEEP_UBERCAL && (mosaic[i].flags & ID_IMAGE_PHOTOM_UBERCAL)) continue;

    list[n] = mosaic[i].McalPSF;
    dlist[n] = 1;
    n++;
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

StatType statsMosaicdM (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  off_t i, n;
  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  if (!MOSAIC_ZEROPT) return (stats);
  if (FREEZE_MOSAICS) return (stats); 

  ALLOCATE (list, double, Nmosaic);
  ALLOCATE (dlist, double, Nmosaic);

  n = 0;
  for (i = 0; i < Nmosaic; i++) {
    if (mosaic[i].flags & (ID_IMAGE_MOSAIC_POOR | ID_IMAGE_PHOTOM_FEW)) continue;
    if (mosaic[i].skipCal) continue;
    if (KEEP_UBERCAL && (mosaic[i].flags & ID_IMAGE_PHOTOM_UBERCAL)) continue;

    list[n] = mosaic[i].dMcal;
    dlist[n] = 1;
    n++;
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

StatType statsMosaicN (Catalog *catalog) {

  off_t i, j, m, c, n, N;
  double *list, *dlist;
  float Mcal, Mgrp, Mgrid, Mrel;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  if (!MOSAIC_ZEROPT) return (stats);
  if (FREEZE_MOSAICS) return (stats); 

  ALLOCATE (list, double, Nmosaic);
  ALLOCATE (dlist, double, Nmosaic);

  n = 0;
  for (i = 0; i < Nmosaic; i++) {
    if (mosaic[i].flags & (ID_IMAGE_MOSAIC_POOR | ID_IMAGE_PHOTOM_FEW))  continue;
    if (mosaic[i].skipCal) continue;
    if (KEEP_UBERCAL && (mosaic[i].flags & ID_IMAGE_PHOTOM_UBERCAL)) continue;

    N = 0;
    for (j = 0; j < N_onMosaic[i]; j++) {

      m = MosaicToMeasure[i][j];
      c = MosaicToCatalog[i][j];

      Mcal = getMcal  (m, c, MAG_CLASS_PSF);
      if (isnan(Mcal)) continue;
      Mgrp = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL);
      if (isnan(Mgrp)) continue;
      Mgrid = getMgridTiny (&catalog[c].measureT[m]);
      if (isnan(Mgrid)) continue;
      Mrel = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP);
      if (isnan(Mrel)) continue;
      N++;
    }
    list[n] = N;
    dlist[n] = 1;
    n++;
  }
  // fprintf (stderr, "Nmosaic: "OFF_T_FMT", n: "OFF_T_FMT"\n",  Nmosaic,  n);

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

StatType statsMosaicX (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  off_t i, n;
  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  if (!MOSAIC_ZEROPT) return (stats);
  if (FREEZE_MOSAICS) return (stats); 

  ALLOCATE (list, double, Nmosaic);
  ALLOCATE (dlist, double, Nmosaic);

  n = 0;
  for (i = 0; i < Nmosaic; i++) {
    if (mosaic[i].flags & (ID_IMAGE_MOSAIC_POOR | ID_IMAGE_PHOTOM_FEW)) continue;
    if (mosaic[i].skipCal) continue;
    if (KEEP_UBERCAL && (mosaic[i].flags & ID_IMAGE_PHOTOM_UBERCAL)) continue;

    list[n]  = mosaic[i].McalChiSq;
    dlist[n] = 1;
    n++;
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

/* mark mosaic if: abs(Mcal - <Mcal>) too large, dMcal too large */

static float MinChiSqLim = NAN;
static float MaxChiSqLim = NAN;
static float MinScatterLim = NAN;
static float MaxScatterLim = NAN;

void clean_mosaics () {

  double *mlist, *slist;

  if (!MOSAIC_ZEROPT) return;
  if (FREEZE_MOSAICS) return;

  if (VERBOSE) fprintf (stderr, "marking poor mosaics\n");

  if (isnan (MaxChiSqLim))   MaxChiSqLim   = MOSAIC_CHISQ;
  if (isnan (MaxScatterLim)) MaxScatterLim = MOSAIC_SCATTER;

  ALLOCATE (mlist, double, Nmosaic);
  ALLOCATE (slist, double, Nmosaic);

  // Measure the good/bad statistics using mosaics which are actually calibrated.
  // But they could be applied to all mosaics below (allowing bad mosaics to become good)
  // However, for now, we will NOT allow mosaics to be redeemed (bad -> good)

  int N = 0;
  for (int i = 0; i < Nmosaic; i++) {
    if (!(mosaic[i].flags & ID_IMAGE_MOSAIC_PHOTCAL)) continue;
    if (mosaic[i].skipCal) continue;
    mlist[N] = mosaic[i].McalChiSq;
    slist[N] = mosaic[i].stdev;
    N++;
  }

  StatType stats;
  liststats_setmode (&stats, "MEAN");

  liststats (mlist, NULL, NULL, N, &stats);
  float ChiSqUpper90 = stats.Upper90; 
  if (isnan (MinChiSqLim)) MinChiSqLim = 2.0*stats.median;             // chi-square cut cannot fall below this value (even if this is > MaxChiSqLim)
  float ChiSqLimit = MAX(MinChiSqLim, MIN(MaxChiSqLim, ChiSqUpper90)); // chi-square cut should be between MinChiSqLim and MaxChiSqLim

  liststats (slist, NULL, NULL, N, &stats);
  float ScatterUpper90 = stats.Upper90; 
  if (isnan (MinScatterLim)) MinScatterLim = 2.0*stats.median;         // scatter cut cannto fall below this value (even if this is > MaxScatterLim)
  float ScatterLimit = MAX(MinScatterLim, MIN(MaxScatterLim, ScatterUpper90));

  fprintf (stderr, "MOSAIC: ChiSqLimit: %f, ScatterLimit: %f | ChiSquare Upper 90: %f, Scatter Upper 90: %f\n", ChiSqLimit, ScatterLimit, ChiSqUpper90, ScatterUpper90);
  
  int Ntotal = 0, Npoor = 0, Nmark = 0, Nscatter = 0, Nfew = 0, Nchisq = 0;
  for (int i = 0; i < Nmosaic; i++) {
    // if we are keeping ubercal sacrosanct, then we should not be allowed to break them...
    if (KEEP_UBERCAL && (mosaic[i].flags & ID_IMAGE_PHOTOM_UBERCAL)) continue;

    Ntotal ++;

    if (mosaic[i].skipCal) continue;

    int mark = FALSE;

    // we do not allow bad nights to be redeemed
    if (mosaic[i].flags & ID_IMAGE_MOSAIC_POOR) {
      Npoor ++;
      continue;
    }

    if (mosaic[i].flags & ID_IMAGE_PHOTOM_FEW) {
      mark = TRUE;
      Nfew ++;
    }
    if (mosaic[i].stdev > ScatterLimit) {
      mark = TRUE;
      Nscatter ++;
    }
    if (mosaic[i].McalChiSq > ChiSqLimit) {
      mark = TRUE;
      Nchisq ++;
    }
    if (mark) { 
      Nmark ++;
      markBadMosaic(i); // mark the images associated with a bad night
    } else {
      markGoodMosaic(i); // mark the images associated with a good night
    }
  }

  fprintf (stderr, "%d + %d of %d mosaics marked poor (%d scatter, %d chisq, %d few)\n",  Nmark, Npoor, Ntotal, Nscatter, Nchisq, Nfew);

  free (mlist);
  free (slist);
}

TGroup *getTGroupForMosaic (int mos) {
  if (mos >= Nmosaic) return NULL;
  if (mos < 0) return NULL;

  if (!mosaic[mos].inTGroup) return NULL;

  int imageIdx = MosaicToImage[mos][0];

  TGroup *mygrp = getTGroupForImage (imageIdx);
  return mygrp;
}

double get_median_zpt_mosaics (short photcode) {

  double *mlist;

  if (!MOSAIC_ZEROPT) return NAN;

  ALLOCATE (mlist, double, Nmosaic);

  int N = 0;
  for (int i = 0; i < Nmosaic; i++) {
    if (mosaic[i].photcode != photcode) continue;
    if (!(mosaic[i].flags & ID_IMAGE_MOSAIC_PHOTCAL)) continue;
    mlist[N] = mosaic[i].McalPSF;
    N++;
  }

  if (N == 0) {
    free (mlist);
    return NAN;
  }

  StatType stats;
  liststats_setmode (&stats, "MEAN");

  liststats (mlist, NULL, NULL, N, &stats);
  free (mlist);

  fprintf (stderr, "rationalize by mosaic using %d pts\n", N);
  return stats.median; 
}

void set_median_zpt_mosaics (short photcode, double zpt) {

  if (!MOSAIC_ZEROPT) return;
  if (!isfinite(zpt)) return; // do not break the zero points

  for (int i = 0; i < Nmosaic; i++) {
    if (mosaic[i].photcode != photcode) continue;
    // fprintf (stderr, "MOSAIC %d zpt %f -> ", i, mosaic[i].McalPSF);

    int applyOffset = TRUE;
    TGroup *mygrp = getTGroupForMosaic (i);
    if (mygrp && (mygrp->flags & ID_IMAGE_TGROUP_PHOTCAL)) applyOffset = FALSE;
    if (!(mosaic[i].flags & ID_IMAGE_MOSAIC_PHOTCAL)) applyOffset = FALSE;

    if (applyOffset) {
      mosaic[i].McalPSF -= zpt;
      mosaic[i].McalAPER -= zpt;
    }
    // fprintf (stderr, "%f (%d)\n", mosaic[i].McalPSF, applyOffset);
  }

  return; 
}

void plot_mosaic_fields (Catalog *catalog) {

  off_t i, j, m, c, N, Nimage;
  double *xlist, *ylist;
  char string[64];
  Graphdata graphdata;

  if (!MOSAIC_ZEROPT) return;

  // Image *image = getimages (&Nimage, NULL); returned value ignored
  getimages (&Nimage, NULL);

  N = 0;
  for (i = 0; i < Nmosaic; i++) {
    N = MAX (N, N_onMosaic[i]);
  }

  ALLOCATE (xlist, double, N);
  ALLOCATE (ylist, double, N);

  for (i = 0; i < Nmosaic; i++) {
    N = 0;
    for (j = 0; j < N_onMosaic[i]; j++) {
      
      m = MosaicToMeasure[i][j];
      c = MosaicToCatalog[i][j];
      
      if (catalog[c].measureT[m].dbFlags & MEAS_BAD) continue;

      // ave = catalog[c].measureT[m].averef;
      xlist[N] = catalog[c].measureT[m].R;
      ylist[N] = catalog[c].measureT[m].D;
      N++;
    }
  
    sprintf (string, "Mosaic "OFF_T_FMT,  i);
    plot_defaults (&graphdata);
    plot_list (&graphdata, xlist, ylist, N, string, NULL);
  }

  free (ylist);
  free (xlist);
}

void plot_mosaics () {

  off_t i, bin;
  double *xlist, *Mlist, *dlist;
  Graphdata graphdata;

  if (!MOSAIC_ZEROPT) return;
  if (FREEZE_MOSAICS) return;

  ALLOCATE (xlist, double, Nmosaic);
  ALLOCATE (dlist, double, Nmosaic);
  ALLOCATE (Mlist, double, Nmosaic);

  for (i = 0; i < Nmosaic; i++) {
    Mlist[i] = mosaic[i].McalPSF;
    dlist[i] = mosaic[i].dMcal;
    xlist[i] = mosaic[i].secz;
  }

  plot_defaults (&graphdata);
  graphdata.xmin = 0.95;
  graphdata.xmax = 2.50;
  graphdata.ymin = PlotdMmin;
  graphdata.ymax = PlotdMmax;
  plot_list (&graphdata, xlist, Mlist, Nmosaic, "airmass vs Mcal", "%s.airmass.png", OUTROOT);
  plot_defaults (&graphdata);
  graphdata.size = 1.5;
  graphdata.ptype = 7;
  plot_list (&graphdata, Mlist, dlist, Nmosaic, "Mcal vs dMcal", "%s.MdM.png", OUTROOT);

# define NBIN 200
  REALLOCATE (xlist, double, NBIN);
  REALLOCATE (Mlist, double, NBIN);

  /**** dMcal histgram ****/
  for (i = 0; i < NBIN; i++) xlist[i] = 0.00005*i;
  bzero (Mlist, NBIN*sizeof(double));
  for (i = 0; i < Nmosaic; i++) {
    bin = mosaic[i].dMcal / 0.00005;
    bin = MAX (0, MIN (NBIN - 1, bin));
    Mlist[bin] += 1.0;
  }
  plot_defaults (&graphdata);
  graphdata.style = 1;
  plot_list (&graphdata, xlist, Mlist, NBIN, "dMcal hist", "%s.dMcalhist.png", OUTROOT);

  free (dlist);
  free (xlist);
  free (Mlist);
}

off_t *SelectRefMosaic (Mosaic **refmosaic, off_t *Nimage) {

  off_t i, Imax, Nmax;

  Imax = 0;
  Nmax = MosaicN_Image[0];
  for (i = 0; i < Nmosaic; i++) {
    if (MosaicN_Image[i] > Nmax) {
      Imax = i;
      Nmax = MosaicN_Image[i];
    }
  }

  *refmosaic = &mosaic[Imax];
  *Nimage = Nmax;
  return (MosaicToImage[Imax]);
}
