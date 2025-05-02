# include "relastro.h"

# define USE_IMAGE_ID 1

// we have an array of images: image[ImageIndex] (ImageIndex : 0 < Nimage)
static Image        *image;   // list of available images
static off_t        Nimage;   // number of available images
static int         isImageSubset;

// if we read only a subset of the rows from the Image FITS, LineNumber tells us to which row
// each image belongs
static off_t       *LineNumber = NULL; // match of subset to full image table

static off_t        *N_onImage = NULL;   // number of measurements on image
static off_t        *N_ONIMAGE = NULL;   // allocated number of measurements on image   

static int          *Ncatlist = NULL;  // catalogs associated with each image
static int          *NCATLIST = NULL;  // catalogs associated with each image
static int         **catlist  = NULL;  // catalogs associated with each image

static IDX_T       **MeasureToImage = NULL;     // link from catalog,measure to image
static IDX_T       **ImageToCatalog = NULL;   // catalog which supplied measurement on image
static IDX_T       **ImageToMeasure = NULL;   // measure reference for measurement on image

// if we search by image ID, we sort (imageIDs, imageIdx) by imageIDs to get a sorted
// index

# if USE_IMAGE_ID
static off_t        *imageIDs = NULL; // list of all image IDs
static off_t        *imageIdx = NULL; // list of index for image IDs 

// as an alternative, we generate imageSeq, which directly maps imageID -> seq
static off_t        *imageSeq = NULL; // list of index for image IDs 

// MAX_ID requires 512M to store the image index
# define MAX_ID      0x8000000
static off_t         minImageID = MAX_ID;
static off_t         maxImageID = 0;

# else /* not using IMAGE_ID */
static unsigned int *start;
static unsigned int *stop;
# endif

// MeasureToImage was 'bin'
// ImageToCatalog was 'clist'
// ImageToMeasure was 'mlist'

// N_onImage was 'Nlist'
// N_ONIMAGE was 'NLIST'

int areImagesLoaded () {

  if (image) return TRUE;
  return FALSE;
}

int areImagesMatched () {

  if (MeasureToImage) return TRUE;
  return FALSE;
}

Image *getimages (off_t *N, off_t **line_number) {

  *N = Nimage;
  if (line_number) *line_number = LineNumber;
  return (image);
}

Image *getimage (off_t N) {
  return (&image[N]);
}

int *getCatlist (int *N, off_t im) {

  *N = Ncatlist[im];
  return (catlist[im]);
}

void initImages (Image *input, off_t *line_number, off_t N, int isSubset) {

  off_t i;

  isImageSubset = isSubset;
  image = input;
  LineNumber = line_number;
  Nimage = N;

# if USE_IMAGE_ID
  ALLOCATE (imageIDs, off_t, Nimage);
  ALLOCATE (imageIdx, off_t, Nimage);

  for (i = 0; i < Nimage; i++) {
    imageIdx[i] = i;
    imageIDs[i] = image[i].imageID;
    minImageID = MIN(minImageID, image[i].imageID);
    maxImageID = MAX(maxImageID, image[i].imageID);
    myAssert (image[i].imageID < MAX_ID, "image IDs too large for index memory");
  }
  llsortpair (imageIDs, imageIdx, Nimage);

  ALLOCATE (imageSeq, off_t, maxImageID + 1);
  for (i = 0; i < maxImageID + 1; i++)  {
    imageSeq[i] = -1; // not yet assigned
  }

  for (i = 0; i < Nimage; i++) {
    off_t N = image[i].imageID;
    myAssert (imageSeq[N] == -1, "previously assigned");
    imageSeq[N] = i;
  }

# else
  ALLOCATE (start, unsigned, Nimage);
  ALLOCATE (stop, unsigned, Nimage);

  for (i = 0; i < Nimage; i++) {
    start[i] = image[i].tzero - MAX(0.01*image[i].trate*image[i].NY, 1);
    stop[i]  = image[i].tzero + MAX(1.01*image[i].trate*image[i].NY, 1);
  }
# endif
}

void freeImages (char *dbImagePtr) {

  FREE (LineNumber);

# if USE_IMAGE_ID
  FREE (imageIDs);
  FREE (imageIdx);
  FREE (imageSeq);
# else
  FREE (start);
  FREE (stop);
# endif

  // we call gfits_db_free as well as this function.  sometimes those point at the same
  // memory location, in which case we should only do the free once.
  if (((void *) dbImagePtr != (void *) image) && isImageSubset) free (image);
  free_astrom_table();
}

off_t getImageByID (off_t ID) {
# if USE_IMAGE_ID

  if (imageSeq) {
    if (ID < minImageID) return (-1);
    if (ID > maxImageID) return (-1);
    off_t N = imageSeq[ID];
    return N;
  }

  // we have a pair of vectors (imageIDs, imageIdx) sorted by imageIDs
  // use bisection to find the specified image ID

  off_t Nlo, Nhi, N;

  Nlo = 0; Nhi = Nimage;
  while (Nhi - Nlo > 10) {
    N = 0.5*(Nlo + Nhi);
    if (imageIDs[N] < ID) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nimage);
    }
  }

  for (N = Nlo; N < Nhi; N++) {
    if (imageIDs[N] == ID)
      return (imageIdx[N]);
  }
# endif

  return (-1);
}

// these are really image & catalog indexes
void initImageBins (Catalog *catalog, int Ncatalog, int FULLINIT) {

  off_t i, j;

  ALLOCATE (MeasureToImage, IDX_T *, Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    ALLOCATE (MeasureToImage[i], IDX_T, MAX (catalog[i].Nmeasure, 1));
    for (j = 0; j < catalog[i].Nmeasure; j++) MeasureToImage[i][j] = -1;
  }

  ALLOCATE (N_onImage, off_t,   Nimage);
  ALLOCATE (N_ONIMAGE, off_t,   Nimage);
  ALLOCATE (ImageToCatalog, IDX_T *,   Nimage);
  ALLOCATE (ImageToMeasure, IDX_T *, Nimage);

  for (i = 0; i < Nimage; i++) {
    N_onImage[i] =   0;
    N_ONIMAGE[i] =  30;
    ImageToCatalog[i] = NULL;  // we allocate these iff they are needed in matchImage
    ImageToMeasure[i] = NULL;  // we allocate these iff they are needed in matchImage
  }

  if (FULLINIT) {
    ALLOCATE (Ncatlist, int,  Nimage);
    ALLOCATE (NCATLIST, int,  Nimage);
    ALLOCATE (catlist, int *, Nimage);

    for (i = 0; i < Nimage; i++) {
      Ncatlist[i] =  0;
      NCATLIST[i] = 32;
      ALLOCATE (catlist[i], int, NCATLIST[i]);
    }
  }
}

void freeImageBins (int Ncatalog) {

  off_t i;

  for (i = 0; i < Ncatalog; i++) {
    FREE (MeasureToImage[i]);
  }
  FREE (MeasureToImage);
  for (i = 0; i < Nimage; i++) {
    FREE (ImageToCatalog[i]); 
    FREE (ImageToMeasure[i]); 
    if (catlist) { FREE (catlist[i]); }
  }
  FREE (Ncatlist);
  FREE (NCATLIST);
  FREE (catlist);
  Ncatlist = NULL;
  NCATLIST = NULL;
  catlist  = NULL;

  FREE (ImageToCatalog);
  FREE (ImageToMeasure);
  FREE (N_onImage);
  FREE (N_ONIMAGE);
}

/* match measurements to images */
void findImages (Catalog *catalog, int Ncatalog, int MATCHCAT) {

  off_t i, j;
  char *name;

  for (i = 0; i < Ncatalog; i++) {
    for (j = 0; j < catalog[i].Nmeasure; j++) {
      if (NphotcodesSkip > 0) {
	int k;
	int found = FALSE;
	for (k = 0; (k < NphotcodesSkip) && !found; k++) {
	  if (photcodesSkip[k][0].code == catalog[i].measureT[j].photcode) found = TRUE;
	  if (photcodesSkip[k][0].code == GetPhotcodeEquivCodebyCode(catalog[i].measureT[j].photcode)) found = TRUE;
	}
	if (found) continue;
      }  
      matchImage (catalog, j, i, MATCHCAT);
    }
  }

  // watch for chips with few stars
  int Nfew = 0;
  int Nbad = 0;
  for (i = 0; i < Nimage; i++) {
    // ignore the PHU entries
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) continue;
    if (VERBOSE2 || (MATCHCAT && (N_onImage[i] < 20))) {
      name = GetPhotcodeNamebyCode (image[i].photcode);
      int showExample = (Nfew < 30);
      if (showExample) {
	char *myDate = ohana_sec_to_date(image[i].tzero);
	fprintf (stderr, "image "OFF_T_FMT" (%d, %s) has "OFF_T_FMT" of %d measures (%s, %s) ",  i,  image[i].imageID, image[i].name, N_onImage[i], image[i].nstar, myDate, name);
	free (myDate);
      }
      if (N_onImage[i] < 20) {
	if (showExample) fprintf (stderr, "*");
	Nfew ++;
      } 
      if (N_onImage[i] < 15) {
	if (showExample) fprintf (stderr, "*** warning ***");
	Nbad ++;
      } 
      if (showExample) fprintf (stderr, "\n");
    }
  }
  if (MATCHCAT) fprintf (stderr, OFF_T_FMT" total images, %d with < 20 measurements, %d with < 15\n", Nimage, Nfew, Nbad);
}

# if USE_IMAGE_ID
// this is the imageID-based match
void matchImage (Catalog *catalog, off_t meas, int cat, int MATCHCAT) {

  off_t idx, ID;
  MeasureTiny *measure;
  int i, found;

  measure = &catalog[cat].measureT[meas];

  ID = measure[0].imageID;

  if (catalog[cat].measure) {
    Measure *measureBig = &catalog[cat].measure[meas];
    int TESTPT = FALSE;
    TESTPT |= CAT_ID_SRC && OBJ_ID_SRC && (measureBig->imageID == CAT_ID_SRC) && (measureBig->detID == OBJ_ID_SRC);
    TESTPT |= CAT_ID_DST && OBJ_ID_DST && (measureBig->imageID == CAT_ID_DST) && (measureBig->detID == OBJ_ID_DST);
    if (TESTPT) {
      fprintf (stderr, "got test det\n");
    }
  }

  if (!ID) return; // detection not associated with an image

  idx = getImageByID (ID);
  if (idx == -1) {
    if (VERBOSE2) fprintf (stderr, "can't match detection to image?\n");
    return;
  }
  measure->myDet = TRUE; // I 'own' this detection (I am calibrating its image)

  // index for (catalog, measure) -> image
  MeasureToImage[cat][meas] = idx;

  // if we need to allocate an image index table, do so here
  if (!ImageToCatalog[idx]) {
      ALLOCATE (ImageToCatalog[idx], IDX_T, N_ONIMAGE[idx]);
  }
  if (!ImageToMeasure[idx]) {
      ALLOCATE (ImageToMeasure[idx], IDX_T, N_ONIMAGE[idx]);
  }

  // index for image, Nentry -> catalog
  ImageToCatalog[idx][N_onImage[idx]] = cat;

  // index for image, Nentry -> measure
  ImageToMeasure[idx][N_onImage[idx]] = meas;
  N_onImage[idx] ++;

  if (N_onImage[idx] == N_ONIMAGE[idx]) {
    N_ONIMAGE[idx] += 30;
    REALLOCATE (ImageToCatalog[idx], IDX_T, N_ONIMAGE[idx]);
    REALLOCATE (ImageToMeasure[idx], IDX_T, N_ONIMAGE[idx]);
  }

  if (MATCHCAT) {
    // index for image -> catalog list
    found = FALSE;
    for (i = 0; !found && (i < Ncatlist[idx]); i++) {
      if (catlist[idx][i] == cat) found = TRUE;
    }
    if (!found) {
      catlist[idx][Ncatlist[idx]] = cat;
      Ncatlist[idx] ++;
      if (Ncatlist[idx] == NCATLIST[idx]) {
	NCATLIST[idx] += 32;
	REALLOCATE (catlist[idx], int, NCATLIST[idx]);
      }
    }
  }

  return;
}

# else
// this is the time-based match
void matchImage (Catalog *catalog, off_t meas, int cat, int MATCHCAT) {

  off_t i;
  MeasureTiny *measure;

  measure = &catalog[cat].measureT[meas];

  /* find the image that supplied this measurement */
  for (i = 0; i < Nimage; i++) {
    // let's try the very slow method first before adding a bisection search
    // if (image[i].imageID != measure[0].imageID) continue;
    if (image[0].photcode == -1) continue;
    if (measure[0].photcode != image[i].photcode) continue;
    if (measure[0].t < start[i]) continue;
    if (measure[0].t > stop[i]) continue;

    // index for (catalog, measure) -> image
    MeasureToImage[cat][meas] = i;

    // index for image, Nentry -> catalog
    ImageToCatalog[i][N_onImage[i]] = cat;

    // index for image, Nentry -> measure
    ImageToMeasure[i][N_onImage[i]] = meas;
    N_onImage[i] ++;

    if (N_onImage[i] == N_ONIMAGE[i]) {
      N_ONIMAGE[i] += 30;
      REALLOCATE (ImageToCatalog[i], IDX_T, N_ONIMAGE[i]);
      REALLOCATE (ImageToMeasure[i], IDX_T, N_ONIMAGE[i]);
    }
    return;
  }
  if (VERBOSE2) fprintf (stderr, "can't match detection to image?\n");
  return;
}
# endif

float getColorBlue (off_t meas, int cat) {

  off_t i;

  i = MeasureToImage[cat][meas];
  if (i == -1) return (NAN);
  return (image[i].refColorBlue);
}

float getColorRed (off_t meas, int cat) {

  off_t i;

  i = MeasureToImage[cat][meas];
  if (i == -1) return (NAN);
  return (image[i].refColorRed);
}

void plot_images () {

  off_t i, bin;
  double *xlist, *Mlist, *dlist;
  Graphdata graphdata;

  ALLOCATE (xlist, double, Nimage);
  ALLOCATE (dlist, double, Nimage);
  ALLOCATE (Mlist, double, Nimage);

  /**** dMcal vs airmass ****/
  for (i = 0; i < Nimage; i++) {
    Mlist[i] = image[i].McalPSF;
    dlist[i] = image[i].dMcal;
    xlist[i] = image[i].secz;
  }

  plot_defaults (&graphdata);
  graphdata.ymin = PlotdMmin;
  graphdata.ymax = PlotdMmax;
  plot_list (&graphdata, xlist, Mlist, Nimage, "airmass vs Mcal", "airmass.png");
  plot_defaults (&graphdata);
  plot_list (&graphdata, Mlist, dlist, Nimage, "Mcal vs dMcal", NULL);

# define NBIN 20
  REALLOCATE (xlist, double, NBIN);
  REALLOCATE (Mlist, double, NBIN);

  /**** dMcal histgram ****/
  for (i = 0; i < NBIN; i++) xlist[i] = 0.00025*i;
  bzero (Mlist, NBIN*sizeof(double));
  for (i = 0; i < Nimage; i++) {
    bin = image[i].dMcal / 0.00025;
    bin = MAX (0, MIN (NBIN - 1, bin));
    Mlist[bin] += 1.0;
  }

  plot_defaults (&graphdata);
  graphdata.style = 1;
  plot_list (&graphdata, xlist, Mlist, NBIN, "dMcal hist", "dMcalhist.png");

  free (dlist);
  free (xlist);
  free (Mlist);
}

void dump_measures (Average *average, Measure *measure) {

  off_t j, off;

  for (j = 0; j < average[0].Nmeasure; j++) {
    off = average[0].measureOffset + j;
    fprintf (stderr, "R, D, mag, dMag: %lf, %lf, %f, %f\n", measure[off].R, measure[off].D, measure[off].M, measure[off].dM);
  }
  return;
}

static int NcatTotal = 0;

// return StarData values for detections in the specified image, converting coordinates from the
// chip positions: X,Y -> L,M -> P,Q -> R,D
void fixImageRaw (Catalog *catalog, int Ncatalog, off_t im) {

  off_t i, m, c, n, nPos;
  double X, Y, L, M, P, Q, R, D, dR, dD;
  double dPos, dPosSys, DPOS_MAX_ASEC;

  Mosaic *mosaic;
  Coords *moscoords, *imcoords;

  // WRP images need to have an associated mosaic
  moscoords = NULL;
  if (!strcmp(&image[im].coords.ctype[4], "-WRP")) {
    mosaic = getMosaicForImage (im);
    if (mosaic == NULL) return;  // if we cannot find the associated image, skip it
    moscoords = &mosaic[0].coords;
  }
  imcoords = &image[im].coords;

  if (moscoords) {
    DPOS_MAX_ASEC = 3600.0*DPOS_MAX*hypot(moscoords[0].cdelt1, moscoords[0].cdelt2);
  } else {
    DPOS_MAX_ASEC = 3600.0*DPOS_MAX*hypot(imcoords[0].cdelt1, imcoords[0].cdelt2);
  }

  // these are used to accumulate the rms position offsets.  if this value, or any
  // specific entry, is too large, we will reset the image to the original coords at the
  // end of the analysis
  dPos = 0.0;
  nPos = 0;

  // convert the image systematic error in pixels to a value in arcsec
  { 
    double dLsig, dMsig;
    double Ro, Do, Rx, Dx, dP0, dP1;
    Coords *coords;

    // these values are in pixels, but we to convert to arcsec
    dLsig = image[0].dXpixSys;
    dMsig = image[0].dYpixSys;

    if (moscoords == NULL) {
      coords = imcoords;
    } else {
      coords = moscoords;
    }
    XY_to_LM (&Ro, &Do, 0.0, 0.0, coords);
    XY_to_LM (&Rx, &Dx, dLsig, 0.0, coords);
    dP0 = 3600.0 * hypot(Rx - Ro, Dx - Do); // convert to arcsec
    XY_to_LM (&Rx, &Dx, 0.0, dMsig, coords);
    dP1 = 3600.0 * hypot(Rx - Ro, Dx - Do); // convert to arcsec
    dPosSys = 0.5 * (dP0 + dP1);
  }      

  int NoffRAave = 0;  int NoffRAori = 0; 
  int NoffDECave = 0; int NoffDECori = 0;

  for (i = 0; i < N_onImage[im]; i++) {
    m = ImageToMeasure[im][i];
    c = ImageToCatalog[im][i];
    myAssert (c < Ncatalog, "oops");

    Measure *measure = &catalog[c].measure[m];
    MeasureTiny *measureT = &catalog[c].measureT[m];

    int TESTPT = FALSE;
    TESTPT |= CAT_ID_SRC && OBJ_ID_SRC && (measure->catID == CAT_ID_SRC) && (measure->objID == OBJ_ID_SRC);
    TESTPT |= CAT_ID_DST && OBJ_ID_DST && (measure->catID == CAT_ID_DST) && (measure->objID == OBJ_ID_DST);
    if (TESTPT) {
      fprintf (stderr, "got test det\n");
    }

    X = measure[0].Xccd;
    Y = measure[0].Yccd;
    if (USE_FIXED_PIXCOORDS) {
      if (isfinite(measure[0].Xfix) && isfinite(measure[0].Yfix)) {
	float dX = measure[0].Xfix - measure[0].Xccd;
	float dY = measure[0].Yfix - measure[0].Yccd;
	if (hypot(dX,dY) < 2.0) {
	  X = measure[0].Xfix;
	  Y = measure[0].Yfix;
	} 
      }
    }
    n = measure[0].averef;
    Average *average = &catalog[c].average[n];

    if (moscoords == NULL) {
      // this is a Simple image (not a mosaic)
      // note that for a Simple image, L,M = P,Q
      XY_to_LM (&L, &M, X, Y, imcoords);
      LM_to_RD (&R, &D, L, M, imcoords);
    } else {
      XY_to_LM (&L, &M, X, Y, imcoords);
      XY_to_LM (&P, &Q, L, M, moscoords);
      LM_to_RD (&R, &D, P, Q, moscoords);
    }

    // new dR, dD : test
    dR = 3600.0*(average[0].R - R);
    dD = 3600.0*(average[0].D - D);

    // make sure detection is on the same side of the 0,360 boundary as the average
    // this will give some funny results withing ~1 arcsec of the pol
    if (dR > +180.0*3600.0) {
      // average on high end of boundary, move star up
      R += 360.0;
      dR = 3600.0*(average[0].R - R);
    }
    if (dR < -180.0*3600.0) {
      // average on low end of boundary, move star down
      R -= 360.0;
      dR = 3600.0*(average[0].R - R);
    }

    float csdec = cos(average[0].D * RAD_DEG);

    // complain if the new location is far from the average location
    // NOTE: This should never happen, or our StarMap tests are not working
    if (fabs(dR*csdec) > 3.0*ADDSTAR_RADIUS) {
      NoffRAave ++;
      if (VERBOSE2) {
	fprintf (stderr, "measurement is far from average location (R): %f %f (%f %f %f)\n", average[0].R, average[0].D, dR, dR*csdec, dD);
	dump_measures (&average[0], catalog[c].measure);
      }
      // abort ();
    }
    if (fabs(dD) > 3.0*ADDSTAR_RADIUS) {
      NoffDECave ++;
      if (VERBOSE2) {
	fprintf (stderr, "measurement is far from average location (D): %f %f (%f %f)\n", average[0].R, average[0].D, dR, dD);
	dump_measures (&average[0], catalog[c].measure);
      }
      // abort ();
    }

    // complain if the new location is far from the old location
    if (fabs(csdec*(measure[0].R - R)) > DPOS_MAX_ASEC) {
      NoffRAori ++;
      if (VERBOSE2) {
	fprintf (stderr, "measurement moves far from original location (R): %f %f (%f %f %f %f)\n", average[0].R, average[0].D, measure[0].R, dR, csdec*(measure[0].R - R), dD);
	dump_measures (&average[0], catalog[c].measure);
      }
      // abort();
    }
    if (fabs(measure[0].D - D) > DPOS_MAX_ASEC) {
      NoffDECori ++;
      if (VERBOSE2) {
	fprintf (stderr, "measurement moves far from original location (D): %f %f (%f %f %f)\n", average[0].R, average[0].D, dR, measure[0].D, dD);
	dump_measures (&average[0], catalog[c].measure);
      }      // abort();
    }

    // XXX NOTE : apply csdec:
    dPos += SQ(measure[0].R - R) + SQ(measure[0].D - D);
    nPos ++;

    measure[0].R = R;
    measure[0].D = D;
    measureT[0].R = R;
    measureT[0].D = D;
    
    // set the systematic error for this image:
    measure[0].dRsys = ToShortPixels(dPosSys);
    measureT[0].dRsys = ToShortPixels(dPosSys);
  }

  NcatTotal += nPos;

  int Noff = NoffRAave + NoffDECave + NoffRAori + NoffDECori;
  if (VERBOSE2 && (Noff > 0)) fprintf (stderr, "Noff ave RA %d, Noff ave DEC %d, Noff ori RA %d, Noff ori DEC %d\n", NoffRAave, NoffDECave, NoffRAori, NoffDECori);

  return;
}

void printNcatTotal () {
  fprintf (stderr, "NcatTotal: %d\n", NcatTotal);
}

// return StarData values for detections in the specified image, converting coordinates from the
// chip positions: X,Y -> L,M -> P,Q -> R,D.  This function is used by the image fitting steps, for
// which the detections have already been filtered when they were loaded (bcatalog)
StarData *getImageRaw (Catalog *catalog, int Ncatalog, off_t im, off_t *Nstars, CoordMode mode) {

  off_t i, m, c, n;

  Mosaic *mosaic;
  Coords *moscoords;
  StarData *raw;

  mosaic = NULL;
  moscoords = NULL;
  if (mode == MODE_MOSAIC) {
    mosaic = getMosaicForImage (im);
    if (mosaic == NULL) {
      fprintf (stderr, "mosaic not found for image %s\n", image[im].name);
      return NULL;
    }
    moscoords = &mosaic[0].coords;
  }

  ALLOCATE (raw, StarData, N_onImage[im]);

  for (i = 0; i < N_onImage[im]; i++) {
    m = ImageToMeasure[im][i];
    c = ImageToCatalog[im][i];
    myAssert (c < Ncatalog, "oops");

    MeasureTiny *measure = &catalog[c].measureT[m];

    /* apply the current image transformation or use the current value of R+dR, D+dD? */
    raw[i].X = measure[0].Xccd;
    raw[i].Y = measure[0].Yccd;
    if (USE_FIXED_PIXCOORDS) {
      if (isfinite(measure[0].Xfix) && isfinite(measure[0].Yfix)) {
	float dX = measure[0].Xfix - measure[0].Xccd;
	float dY = measure[0].Yfix - measure[0].Yccd;
	if (hypot(dX,dY) < 2.0) {
	  raw[i].X = measure[0].Xfix;
	  raw[i].Y = measure[0].Yfix;
	} 
      }
    }
    raw[i].Mag  = measure[0].M;
    raw[i].dMag = measure[0].dM;
    raw[i].dPos = GetAstromErrorTiny (&measure[0], ERROR_MODE_POS);

    n = measure[0].averef;

    int TESTPT = FALSE;
    TESTPT |= CAT_ID_SRC && OBJ_ID_SRC && (catalog[c].average[n].catID == CAT_ID_SRC) && (catalog[c].average[n].objID == OBJ_ID_SRC);
    TESTPT |= CAT_ID_DST && OBJ_ID_DST && (catalog[c].average[n].catID == CAT_ID_DST) && (catalog[c].average[n].objID == OBJ_ID_DST);
    if (TESTPT) {
      fprintf (stderr, "got test det\n");
    }

    // an object with only one detection provides no information about the image calibration
    // XXX this is already taken care of in bcatalog
    raw[i].mask = MARK_MEAS_DEFAULT;
    if (catalog[c].average[n].Nmeasure <= SRC_MEAS_TOOFEW) {
      raw[i].mask |= MARK_TOO_FEW_MEAS;
    }
    if (!finite(measure[0].R) || !finite(measure[0].D)) {
      raw[i].mask |= MARK_NAN_POS_ERROR;
    }

    // XXX A TEST: can we use only 2MASS measurements to fit the images?
    // XXX Do NOT apply this for the real calibration
    if (FALSE && !(measure[0].dbFlags & ID_MEAS_OBJECT_HAS_2MASS)) {
      fprintf (stderr, "@");
      raw[i].mask |= MARK_BIG_OFFSET;
    }

    raw[i].Nmeas = catalog[c].average[n].Nmeasure; // record so we can check how well connected an image is

    switch (mode) {
      case MODE_SIMPLE:
        /* note that for a Simple image, L,M = P,Q */
        XY_to_LM (&raw[i].L, &raw[i].M, raw[i].X, raw[i].Y, &image[im].coords);
        raw[i].P = raw[i].L;
        raw[i].Q = raw[i].M;
        LM_to_RD (&raw[i].R, &raw[i].D, raw[i].P, raw[i].Q, &image[im].coords);
        break;
      case MODE_MOSAIC:
        XY_to_LM (&raw[i].L, &raw[i].M, raw[i].X, raw[i].Y, &image[im].coords);
        XY_to_LM (&raw[i].P, &raw[i].Q, raw[i].L, raw[i].M, moscoords);
        LM_to_RD (&raw[i].R, &raw[i].D, raw[i].P, raw[i].Q, moscoords);
        break;
      default:
	fprintf (stderr, "error: invalid mode in getImageRaw");
	abort ();
    }
  }

  *Nstars = N_onImage[im];
  return (raw);
}

// return StarData values for averages positions in the specified image, converting coordinates from
// the sky positions: R,D -> P,Q -> L,M -> X,Y

StarData *getImageRef (Catalog *catalog, int Ncatalog, off_t im, off_t *Nstars, CoordMode mode) {

  off_t i, m, c, n;

  Mosaic *mosaic;
  Coords *moscoords;
  StarData *ref;

  int Nsecfilt = GetPhotcodeNsecfilt();

  mosaic = NULL;
  moscoords = NULL;
  if (mode == MODE_MOSAIC) {
    mosaic = getMosaicForImage (im);
    if (mosaic == NULL) {
      fprintf (stderr, "mosaic not found for image %s\n", image[im].name);
      return NULL;
    }
    moscoords = &mosaic[0].coords;
  }

  ALLOCATE (ref, StarData, N_onImage[im]);

  for (i = 0; i < N_onImage[im]; i++) {
    m = ImageToMeasure[im][i];
    c = ImageToCatalog[im][i];
    myAssert (c < Ncatalog, "oops");

    MeasureTiny *measure = &catalog[c].measureT[m];
    n = measure[0].averef;

    /* apply the current image transformation or use the current value of R+dR, D+dD? */
    ref[i].R = catalog[c].average[n].R;
    ref[i].D = catalog[c].average[n].D;
    
    int XVERB = FALSE;
    XVERB |= (catalog[c].average[n].objID == OBJ_ID_SRC) && (catalog[c].average[n].catID == CAT_ID_SRC);
    XVERB |= (catalog[c].average[n].objID == OBJ_ID_DST) && (catalog[c].average[n].catID == CAT_ID_DST);
    if (XVERB) {
      fprintf (stderr, "found test object\n");
    }

    // if we are applying the galaxy model, move the reference position...
    if (APPLY_PROPER_MOTION) {
      // apply proper-motion from average position to measure epoch:
      float dTime = (measure[0].t - catalog[c].average[n].Tmean) / (86400*365.25) ; // time relative to Tmean in years

      // XXX do this in a better way? (my main concern is that this is wrong near the pole : should be done in a local projection
      ref[i].R += dTime * catalog[c].average[n].uR / 3600.0 / cos(ref[i].D*RAD_DEG);
      ref[i].D += dTime * catalog[c].average[n].uD / 3600.0;
    }

    ref[i].Mag  = measure[0].M;
    ref[i].dMag = measure[0].dM;
    ref[i].dPos = GetAstromErrorTiny (&measure[0], ERROR_MODE_POS);
    ref[i].ColorBlue = NAN;
    ref[i].ColorRed = NAN;

    if ((DCR_BLUE_NSEC_POS >= 0) && (DCR_BLUE_NSEC_NEG >= -1)) {
      ref[i].ColorBlue = catalog[c].secfilt[n*Nsecfilt + DCR_BLUE_NSEC_POS].MpsfChp - catalog[c].secfilt[n*Nsecfilt + DCR_BLUE_NSEC_NEG].MpsfChp;
    }
    if ((DCR_RED_NSEC_POS >= 0) && (DCR_RED_NSEC_NEG >= -1)) {
      ref[i].ColorRed = catalog[c].secfilt[n*Nsecfilt + DCR_RED_NSEC_POS].MpsfChp - catalog[c].secfilt[n*Nsecfilt + DCR_RED_NSEC_NEG].MpsfChp;
    }

    ref[i].mask = FALSE;

    /* note that for a Simple image, L,M = P,Q */
    switch (mode) {
      case MODE_SIMPLE:
	RD_to_LM (&ref[i].P, &ref[i].Q, ref[i].R, ref[i].D, &image[im].coords);
	ref[i].L = ref[i].P;
	ref[i].M = ref[i].Q;
	LM_to_XY (&ref[i].X, &ref[i].Y, ref[i].L, ref[i].M, &image[im].coords);
	break;
      case MODE_MOSAIC:
	RD_to_LM (&ref[i].P, &ref[i].Q, ref[i].R, ref[i].D, moscoords);
	LM_to_XY (&ref[i].L, &ref[i].M, ref[i].P, ref[i].Q, moscoords);
	LM_to_XY (&ref[i].X, &ref[i].Y, ref[i].L, ref[i].M, &image[im].coords);
	break;
      default:
        fprintf (stderr, "invalid case");
        abort();
    }
  }

  *Nstars = N_onImage[im];
  return (ref);
}

// return StarData values for detections in the specified image, converting coordinates from the
// chip positions: X,Y -> L,M -> P,Q -> R,D.  This function is used by the image fitting steps, for
// which the detections have already been filtered when they were loaded (bcatalog)
int setImageRaw (Catalog *catalog, int Ncatalog, off_t im, StarData *raw, off_t Nraw, CoordMode mode) {

  off_t i, m, c;

  Coords *moscoords;

  moscoords = NULL;
  if (mode == MODE_MOSAIC) {
    moscoords = image[im].coords.mosaic;
    myAssert (moscoords, "coords.mosaic not defined for image %s (%d)", image[im].name, (int) im);
  }

  myAssert (Nraw == N_onImage[im], "impossible!");

  for (i = 0; i < N_onImage[im]; i++) {
    m = ImageToMeasure[im][i];
    c = ImageToCatalog[im][i];
    myAssert (c < Ncatalog, "oops");

    // XXX should I use the raw coords or just measure.X,Y -> R,D?

    switch (mode) {
      case MODE_SIMPLE:
        /* note that for a Simple image, L,M = P,Q */
        XY_to_LM (&raw[i].L, &raw[i].M, raw[i].X, raw[i].Y, &image[im].coords);
        raw[i].P = raw[i].L;
        raw[i].Q = raw[i].M;
        LM_to_RD (&raw[i].R, &raw[i].D, raw[i].P, raw[i].Q, &image[im].coords);
        break;
      case MODE_MOSAIC:
        XY_to_LM (&raw[i].L, &raw[i].M, raw[i].X, raw[i].Y, &image[im].coords);
        XY_to_LM (&raw[i].P, &raw[i].Q, raw[i].L, raw[i].M, moscoords);
        LM_to_RD (&raw[i].R, &raw[i].D, raw[i].P, raw[i].Q, moscoords);
        break;
      default:
	fprintf (stderr, "error: invalid mode in getImageRaw");
	abort ();
    }

    MeasureTiny *measure = &catalog[c].measureT[m];
    measure->R = raw[i].R;
    measure->D = raw[i].D;
    if (catalog[c].measure) {
      catalog[c].measure[m].R = raw[i].R;
      catalog[c].measure[m].D = raw[i].D;
    }      
  }
  return TRUE;
}

// return StarData values for detections in the specified image, converting coordinates from the
// chip positions: X,Y -> L,M -> P,Q -> R,D.  This function is used by the image fitting steps, for
// which the detections have already been filtered when they were loaded (bcatalog)
int updateImageRaw (Catalog *catalog, int Ncatalog, off_t im) {

  off_t i, m, c;

  for (i = 0; i < N_onImage[im]; i++) {
    m = ImageToMeasure[im][i];
    c = ImageToCatalog[im][i];
    myAssert (c < Ncatalog, "oops");

    MeasureTiny *measure = &catalog[c].measureT[m];

    /* apply the current image transformation or use the current value of R+dR, D+dD? */
    double X = measure[0].Xccd;
    double Y = measure[0].Yccd;
    if (USE_FIXED_PIXCOORDS) {
      if (isfinite(measure[0].Xfix) && isfinite(measure[0].Yfix)) {
	float dX = measure[0].Xfix - measure[0].Xccd;
	float dY = measure[0].Yfix - measure[0].Yccd;
	if (hypot(dX,dY) < 2.0) {
	  X = measure[0].Xfix;
	  Y = measure[0].Yfix;
	} 
      }
    }

    double R, D;
    XY_to_RD (&R, &D, X, Y, &image[im].coords);
    measure->R = R;
    measure->D = D;
    if (catalog[c].measure) {
      catalog[c].measure[m].R = R;
      catalog[c].measure[m].D = D;
    }      
  }
  return TRUE;
}

