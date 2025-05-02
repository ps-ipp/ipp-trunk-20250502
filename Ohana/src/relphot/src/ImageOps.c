# include "relphot.h"

// the MeasureToImage, ImageToCatalog, and ImageToMeasure arrays are a substantial part of the memory footprint.
// total data volume is currently:
// a) Naverage*sizeof(AverageTiny)      [averages]       : 32
// b) Naverage*sizeof(Secfilt)*Nsecfilt [secfilt values] : 8 * 32
// b) Nmeasure*sizeof(Measure)          [measurements]   : 72
// c) Nmeasure*sizeof(IDX_T)*3          [image idx]      : 3 * 16
// d) Nmeasure*sizeof(IDX_T)*3          [mosaic idx]     : 3 * 16
// e) Nimage*sizeof(Image)              [image data]     : 360

// for 3pi analysis in 2012, we have Nmeasure ~ 20 x Naverage
// so, each measurement costs ~14.5B (Ave + Sec) + 72B (Meas) + 96B (idx)!
// we are using off_t (64bit) to avoid the 32bit limit of an int, but
// if we really had 2^31 measurements in a single analysis, we would be using > 350GB of ram...
// until we reach that point, it is sort of silly to use IDX_T = off_t here.

// in fact, we are safer than this, because the number of detections per table is rarely so large.

// with IDX_T = int, each measurement costs ~14.5B (Ave + Sec) + 72B (Meas) + 48B (idx)!

// elsewhere (not in relphot_images), we need to use off_t because a single catalog 

// we have an array of images: image[ImageIndex] (ImageIndex : 0 < Nimage)
static off_t        Nimage = 0;   // number of available images
static Image        *image = NULL;   // array of available images  

// if we read only a subset of the rows from the Image FITS, LineNumber tells us to which row
// each image belongs
static off_t       *LineNumber; // match of subset to full image table

// to search by image ID, we sort (imageIDs, imageIdx) by imageIDs to get a sorted index
static off_t        *imageIDs; // list of all image IDs
static off_t        *imageIdx; // list of index for image IDs 

// QQ // as an alternative, we generate imageSeq, which directly maps imageID -> seq
// QQ static off_t        *imageSeq; // list of index for image IDs 
// QQ 
// QQ // MAX_ID requires 512M to store the image index
// QQ # define MAX_ID 0x8000000
// QQ static off_t         minImageID = MAX_ID;
// QQ static off_t         maxImageID = 0;

// elsewhere, we have loaded a set of catalogs with measures (catalog[cat].measure[meas])
// each image has N_onImage[ImageIndex] measurements
static off_t        *N_onImage;   // actual number of measurements on image	     
static off_t        *N_ONIMAGE;   // allocated number of measurements on image   

// relationships between the measure,catalog set and the images:
static IDX_T       **MeasureToImage = NULL; // image index from measure,catalog   : MeasureToImage[cat][meas] = ImageIndex
static IDX_T       **ImageToCatalog = NULL; // catalog for given measure on image : ImageCatalog[ImageIndex][i] = cat (i : 0 < NonImage[ImageIndex])
static IDX_T       **ImageToMeasure = NULL; // measure for given measure on image : ImageMeasure[ImageIndex][i] = cat (i : 0 < NonImage[ImageIndex])

// Projection Cell / SkyCell ID : for stack primary detection, we need to know the projection cell and skycell ID for each
// stack image.  for now, we generate these ID arrays based on the image names when we load in the image table (initImages).
// When we pass data to the remote clients via the ImageSubset, the projID/skycellID values are carried directly in the table.
int *tessID    = NULL;
int *projectID = NULL;
int *skycellID = NULL;

// MeasureToImage was 'bin'
// ImageToCatalog was 'clist'
// ImageToMeasure was 'mlist'

// N_onImage was 'Nlist'
// N_ONIMAGE was 'NLIST'

Image *getimages (off_t *N, off_t **line_number) {

  *N = Nimage;
  if (line_number) *line_number = LineNumber;
  return (image);
}

Image *getimage (off_t N) {
  return (&image[N]);
}

off_t *get_N_onImage() {
  return N_onImage;
}

IDX_T **get_ImageToCatalog() {
  return ImageToCatalog;
}

IDX_T **get_ImageToMeasure() {
  return ImageToMeasure;
}

void initImages (Image *input, off_t *line_number, off_t N) {

  off_t i;

  image = input;
  LineNumber = line_number;
  Nimage = N;

  ALLOCATE (imageIDs, off_t, Nimage);
  ALLOCATE (imageIdx, off_t, Nimage);

  // for stack images, assign projection cell ID and skycell ID based on filenames
  ALLOCATE (tessID,      int, Nimage);
  ALLOCATE (projectID,   int, Nimage);
  ALLOCATE (skycellID,   int, Nimage);

  for (i = 0; i < Nimage; i++) {
    imageIdx[i] = i;
    imageIDs[i] = image[i].imageID;

    tessID[i]    = -1;
    projectID[i] = -1;
    skycellID[i] = -1;

    TessellationIDsByImageName (&tessID[i], &projectID[i], &skycellID[i], image[i].name);

    // QQ minImageID = MIN(minImageID, image[i].imageID);
    // QQ maxImageID = MAX(maxImageID, image[i].imageID);
    // QQ myAssert (image[i].imageID < MAX_ID, "image IDs too large for index memory");
  }

  // sort the image index by the IDs
  // XXX does this break the imageID <-> projectID, etc match?
  llsortpair (imageIDs, imageIdx, Nimage);

  // QQ ALLOCATE (imageSeq, off_t, maxImageID + 1);
  // QQ for (i = 0; i < maxImageID + 1; i++)  {
  // QQ   imageSeq[i] = -1; // not yet assigned
  // QQ }
  // QQ 
  // QQ for (i = 0; i < Nimage; i++) {
  // QQ   off_t N = image[i].imageID;
  // QQ   myAssert (imageSeq[N] == -1, "previously assigned");
  // QQ   imageSeq[N] = i;
  // QQ }
}

void initImagesSubset (ImageSubset *input, off_t *line_number, off_t N) {

  off_t i;

  // we have been given an ImageSubset array, containing a reduced set of image fields
  // create full a Image array and save the needed values
  ALLOCATE (image, Image, N);

  ALLOCATE (tessID,    int, N);
  ALLOCATE (projectID, int, N);
  ALLOCATE (skycellID, int, N);

  for (i = 0; i < N; i++) {
    image[i].imageID       = input[i].imageID      ;
    image[i].photom_map_id = input[i].photom_map_id;
    image[i].flags         = input[i].flags        ;
    image[i].McalPSF       = input[i].McalPSF      ;
    image[i].McalAPER      = input[i].McalAPER     ;
    image[i].dMcal         = input[i].dMcal        ;
    image[i].tzero         = input[i].tzero        ;
    image[i].trate         = input[i].trate        ;
    image[i].ubercalDist   = input[i].ubercalDist  ;
    tessID[i]              = input[i].tessID       ;
    projectID[i]           = input[i].projID       ;
    skycellID[i]           = input[i].skycellID    ;
  }
  LineNumber = line_number;
  Nimage = N;

  ALLOCATE (imageIDs, off_t, Nimage);
  ALLOCATE (imageIdx, off_t, Nimage);

  for (i = 0; i < Nimage; i++) {
    imageIdx[i] = i;
    imageIDs[i] = image[i].imageID;
  }
  llsortpair (imageIDs, imageIdx, Nimage);
}

ImageSubset *getimages_subset (off_t *N) {

  *N = Nimage;

  off_t i;

  // we have been given an ImageSubset array, containing a reduced set of image fields
  // create full a Image array and save the needed values
  ImageSubset *subset = NULL;
  ALLOCATE (subset, ImageSubset, Nimage);
  for (i = 0; i < Nimage; i++) {
    subset[i].imageID       = image[i].imageID      ;
    subset[i].photom_map_id = image[i].photom_map_id;
    subset[i].flags         = image[i].flags        ;
    subset[i].McalPSF       = image[i].McalPSF      ;
    subset[i].McalAPER      = image[i].McalAPER     ;
    subset[i].dMcal         = image[i].dMcal        ;
    subset[i].tzero         = image[i].tzero        ;
    subset[i].trate         = image[i].trate        ;
    subset[i].ubercalDist   = image[i].ubercalDist  ;
    subset[i].tessID        = tessID[i];
    subset[i].projID        = projectID[i];
    subset[i].skycellID     = skycellID[i];
  }
  return subset;
}

off_t getImageByID (off_t ID) {

  // QQ  if (imageSeq) {
  // QQ    myAssert (ID >= minImageID, "oops");
  // QQ    myAssert (ID <= maxImageID, "oops");
  // QQ    off_t N = imageSeq[ID];
  // QQ    return N;
  // QQ  }

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

  return (-1);
}

void initImageBins (Catalog *catalog, int Ncatalog, int doImageList) {

  IDX_T i, j;

  ALLOCATE (MeasureToImage, IDX_T *, Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    ALLOCATE (MeasureToImage[i], IDX_T, MAX (catalog[i].Nmeasure, 1));
    for (j = 0; j < catalog[i].Nmeasure; j++) MeasureToImage[i][j] = -1;
  }

  if (doImageList) {
    ALLOCATE (N_onImage, off_t, Nimage);
    ALLOCATE (N_ONIMAGE, off_t, Nimage);
    ALLOCATE (ImageToCatalog, IDX_T *, Nimage);
    ALLOCATE (ImageToMeasure, IDX_T *, Nimage);

    for (i = 0; i < Nimage; i++) {
      N_onImage[i] = 0;
      N_ONIMAGE[i] = 30;
      ALLOCATE (ImageToCatalog[i], IDX_T, N_ONIMAGE[i]);
      ALLOCATE (ImageToMeasure[i], IDX_T, N_ONIMAGE[i]);
    }
  }
}

void freeImageBins (int Ncatalog, int doImageList) {

  off_t i;

  for (i = 0; i < Ncatalog; i++) {
    free (MeasureToImage[i]);
  }
  free (MeasureToImage);

  if (doImageList) {
    for (i = 0; i < Nimage; i++) {
      free (ImageToCatalog[i]);
      free (ImageToMeasure[i]);
    }
    free (ImageToCatalog);
    free (ImageToMeasure);
    free (N_onImage);
    free (N_ONIMAGE);
  }
}

void freeImages (char *dbImagePtr) {

  FREE (LineNumber);

  FREE (imageIDs);
  FREE (imageIdx);

  FREE (tessID);
  FREE (projectID);
  FREE (skycellID);

  // we call gfits_db_free as well as this function.  sometimes those point at the same
  // memory location, in which case we should only do the free once.
  if ((void *) dbImagePtr != (void *) image) free (image);

  free_astrom_table();
}

/* select all images equivalent to the active photcode set */
void findImages (Catalog *catalog, int Ncatalog, int doImageList) {

  int Nmatch = 0;
  for (int i = 0; i < Ncatalog; i++) {
    for (off_t j = 0; j < catalog[i].Nmeasure; j++) {
      catalog[i].measureT[j].myDet = FALSE; // a detetion is not mine until proven otherwise

      // skip measurements which do not match one of the requested photcodes (
      // (do we not already exclude in bcatalog -- maybe needed for reload_objects?
      int Ns = GetActivePhotcodeIndex (catalog[i].measureT[j].photcode);
      if (Ns < 0) continue;

      // if we match one of our images, myDet gets set to TRUE
      matchImage (catalog, j, i, doImageList);
      Nmatch ++;
    }
  }
 // fprintf (stderr, "matched %d detections to images\n", Nmatch);
}

void matchImage (Catalog *catalog, off_t meas, int cat, int doImageList) {

  off_t idx, ID;
  MeasureTiny *measure;
  
  measure = &catalog[cat].measureT[meas];

  ID = measure[0].imageID;
  idx = getImageByID (ID);
  if (idx == -1) {
    if (VERBOSE2) fprintf (stderr, "can't match detection to image?\n");
    return;
  }
  catalog[cat].measureT[meas].myDet = TRUE;

  // index for (catalog, measure) -> image
  MeasureToImage[cat][meas] = idx;

  if (doImageList) {
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
  }

  return;
}

off_t getImageEntry (off_t meas, int cat) {

  off_t i;

  if (!MeasureToImage) return -1;

  i = MeasureToImage[cat][meas];
  return (i);
}

int getImageFlags (off_t meas, int cat) {

  off_t i;

  if (!MeasureToImage) return 0;

  i = MeasureToImage[cat][meas];

  return (image[i].flags);
}

// returns image.McalPSF or image.McalAPER
// NOTE: static flat-field component is included in measure.Mflat
float getMcal (off_t meas, int cat, dvoMagClassType class) {

  off_t i;

  i = MeasureToImage[cat][meas];
  if (i == -1) return NAN;

  // allow context dependence?: in relphot_images, we want to exclude detections from poor images
  // but in the final pass, we want to allow even bad detections 
  if (image[i].flags & (ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_FEW)) return NAN;  

  switch (class) {
    case MAG_CLASS_PSF:
      return image[i].McalPSF;
    case MAG_CLASS_APER:
    case MAG_CLASS_KRON:
      return image[i].McalAPER;
    default:
      return NAN;
  }
  return NAN; // should not be able to reach here
}

short getUbercalDist (off_t meas, int cat) {

  off_t i;
  short distance;

  if (!MeasureToImage) return -1;

  i = MeasureToImage[cat][meas];
  if (i == -1) return (1000);

  distance = image[i].ubercalDist; // was dummy3 in structure
  return (distance);
}

float getCenterOffset (off_t meas, int cat, Measure *measure, unsigned int *myID) {

  off_t i;
  float distance;

  if (!MeasureToImage) return -1;

  i = MeasureToImage[cat][meas];
  if (i == -1) return (1000);

  float Xcenter = 0.5*image[i].NX;
  float Ycenter = 0.5*image[i].NY;

  *myID = image[i].imageID;

  distance = hypot (measure[0].Xccd - Xcenter, measure[0].Yccd - Ycenter);
  return (distance);
}

int MatchImageName (off_t meas, int cat, char *name) {

  off_t i;

  if (!name) return FALSE;
  if (!name[0]) return FALSE;

  if (!MeasureToImage) return FALSE;

  i = MeasureToImage[cat][meas];
  if (i == -1) return FALSE;

  // this is a bit crude: stack image names are of the form:
  // RINGS.V3.skycell.1495.027.sky.191211.stk.988232.cmf.

  // the primaryCell has a name of the form RINGS.V3.skycell.1495 or RINGS.V3.skycell.1495.027
  // (if we use projection or skycell as the primary)

  if (!strncmp(image[i].name, name, strlen(name))) return TRUE;
  return FALSE;
}

int MatchImageSkycellID (off_t meas, int cat, int myTessID, int myProjectionID, int mySkycellID) {

  off_t i;

  if (!MeasureToImage) return FALSE;

  i = MeasureToImage[cat][meas];
  if (i == -1) return FALSE;

  if (tessID[i]    == -1) return FALSE;
  if (projectID[i] == -1) return FALSE;
  if (skycellID[i] == -1) return FALSE;

  if (tessID[i]    != myTessID) return FALSE;
  if (projectID[i] != myProjectionID) return FALSE;
  if (skycellID[i] != mySkycellID) return FALSE;
  
  return TRUE;
}

int FindImageSkycellID (off_t meas, int cat, int *myTessID, int *myProjectionID, int *mySkycellID) {

  off_t i;

  if (!MeasureToImage) return FALSE;

  if (meas < 0) return FALSE;

  i = MeasureToImage[cat][meas];
  if (i == -1) return FALSE;

  if (tessID[i]    == -1) return FALSE;
  if (projectID[i] == -1) return FALSE;
  if (skycellID[i] == -1) return FALSE;

  *myTessID = tessID[i];
  *myProjectionID = projectID[i];
  *mySkycellID =  skycellID[i];
  
  return TRUE;
}

Coords *getCoords (off_t meas, int cat) {

  off_t i;

  i = MeasureToImage[cat][meas];
  if (i == -1) return (NULL);
  return (&image[i].coords);
}

/* determine Mcal values for all images */
void setMcal (Catalog *catalog) {

  off_t i, j, m, c, n;
  int mark, Nfew, Nbad, Nmos, Ngrp, Nrel, Ngrid, Nsys;

  if (IMAGE_ZPT_MODE == IMAGE_ZPT_MODE_NONE) return;

  fprintf (stderr, "limiting negative clouds to %f\n", CLOUD_TOLERANCE);

  int Nsecfilt = GetPhotcodeNsecfilt ();

  off_t Nmax = 0;
  for (i = 0; i < Nimage; i++) {
    Nmax = MAX (Nmax, N_onImage[i]);
  }

  // we are making a 0-order fit and not doing bootstrap analysis:
  FitDataSet psfStars, kronStars, brightStars;
  FitDataSetAlloc (&psfStars,    Nmax, 0, 0);
  FitDataSetAlloc (&kronStars,   Nmax, 0, 0);
  FitDataSetAlloc (&brightStars, Nmax, 0, 0);

  // until the analysis has converged a bit, do not use the IRLS analysis
  // default is MaxIterations = 10
  if (UseStandardOLS(ZPT_IMAGES)) {
    brightStars.MaxIterations = 0;
    kronStars.MaxIterations = 0;
    psfStars.MaxIterations = 0;
  }

  Nfew = Nbad = Nmos = Ngrp = Ngrid = Nrel = Nsys = 0;

  int Ncalibrated = 0;
  for (i = 0; i < Nimage; i++) {
    
    if (image[i].imageID == TEST_IMAGE1) {
      fprintf (stderr, "test image 1\n");
    }
    if (image[i].imageID == TEST_IMAGE2) {
      fprintf (stderr, "test image 1\n");
    }

    if (image[i].photcode == 0) continue; // skip the PHU images

    int badNight  = (image[i].flags & ID_IMAGE_NIGHT_POOR);
    int badMosaic = (image[i].flags & ID_IMAGE_MOSAIC_POOR);
    int isMosaic  = isMosaicChip(image[i].photcode);
    int useMcal   = TRUE;

    // unset at start by default (set if actually used below)
    image[i].flags &= ~ID_IMAGE_IMAGE_PHOTCAL;

    // if requested, freeze mosaic chips:
    if (FREEZE_IMAGES && isMosaic) continue;

    // in BAD_NIGHT mode, we fit ONLY images in bad nights
    if ((IMAGE_ZPT_MODE == IMAGE_ZPT_MODE_BAD_NIGHT)) {
      if (!badNight) useMcal = FALSE; // do not fit images from good nights
    }

    // in BAD_NIGHT_BAD_MOSAIC mode, we fit ONLY bad mosaics in bad nights
    if ((IMAGE_ZPT_MODE == IMAGE_ZPT_MODE_BAD_NIGHT_BAD_MOSAIC)) {
      if (!badNight) useMcal = FALSE; // do not fit images from good nights
      if (!badMosaic && isMosaic) useMcal = FALSE; // skip good mosaics (but not good images)
    }

    // in BAD_MOSAIC mode, we fit bad mosaics ignoring night state
    if ((IMAGE_ZPT_MODE == IMAGE_ZPT_MODE_BAD_MOSAIC)) {
      if (!badMosaic && isMosaic) useMcal = FALSE; // skip good mosaics (but not bad images)
    }

    // UBERCAL image: if this is an ubercal image, set minUbercalDist to 0:
    // we optionally do not recalibrate images with UBERCAL zero points 
    if (image[i].flags & ID_IMAGE_PHOTOM_UBERCAL) {
      image[i].ubercalDist = 0; // was dummy3
      if (KEEP_UBERCAL) continue;
    }

    int minUbercalDist = 1000;
    
    off_t Nref = 0;    // number of stars used to measure McalPSF
    int   Nkron = 0;   // number of stars to measure McalAPER
    int   Nbright = 0; // number of stars to measure the bright-end scatter

    if (N_onImage[i] == 0) {
      fprintf (stderr, "image missing detections? %s %d "OFF_T_FMT"\n", image[i].name, image[i].nstar, N_onImage[i]);
      continue;
    }

    for (j = 0; j < N_onImage[i]; j++) {
      
      m = ImageToMeasure[i][j];
      c = ImageToCatalog[i][j];
      
      if (catalog[c].measureT[m].dbFlags & MEAS_BAD) {
	  Nbad++;
	  continue;
      }
      float Mmos  = getMmos  (m, c);
      if (isnan(Mmos)) {
	  Nmos ++;
	  continue;
      }
      float Mgrp  = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL); // ignore error for now?
      if (isnan(Mgrp)) {
	  Ngrp ++;
	  continue;
      }
      float Mgrid = getMgridTiny (&catalog[c].measureT[m]);
      if (isnan(Mgrid)) {
	  Ngrid++;
	  continue;
      }

      // Mrel* is the average magnitude for this star.  For PS1 stacks, we have too much
      // PSF variability.  We need to calibrate the PSF magnitudes separately from the
      // Aperture-like magnitues.  (We have an option to use the kron magnitudes or the
      // other apertures here).  I basically need to do this analysis separately for each
      // magnitude type

      float MrelPSF  = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP);
      if (isnan(MrelPSF)) {
	  Nrel ++;
	  continue;
      }
      
      // image.Mcal is not supposed to include the flat-field correction, so we need to
      // apply that offset as well here for this image (in other words, each detection is
      // being compared to the model, excluding the zero point, Mcal.  The model includes
      // the flat-correction.  NOTE the sign of Mflat (Image.Mcal = Measure.Mcal + Mflat)
      // this was inconsistent w.r.t. PhotRel pre-r41606

      float Mflat = getMflat (m, c, catalog);

      n = catalog[c].measureT[m].averef;
      float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);
      if (isnan(MsysPSF)) {
	Nsys++;
	continue;
      }

      float Moff = Mmos + Mgrp + Mgrid + Mflat;

      PhotCode *code = GetPhotcodebyCode (catalog[c].measureT[m].photcode);
      if (!code) goto skip;
      if (code->equiv < 1) goto skip;
      int Nsec = GetPhotcodeNsec (code->equiv);
      if (Nsec == -1) goto skip;
      minUbercalDist = MIN (catalog[c].secfilt[n*Nsecfilt + Nsec].ubercalDist, minUbercalDist);

    skip:
      psfStars.alldata-> yVector[Nref] = MsysPSF - MrelPSF - Moff;
      psfStars.alldata->dyVector[Nref] = MAX (catalog[c].measureT[m].dM, MIN_ERROR);

      float MrelKron = getMrel  (catalog, m, c, MAG_CLASS_KRON, MAG_SRC_CHP);
      float MsysKron = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_KRON);

      if (isfinite(MrelKron) && isfinite(MsysKron)) {
	kronStars.alldata-> yVector[Nkron] = MsysKron - MrelKron - Moff;
	kronStars.alldata->dyVector[Nkron] = psfStars.alldata->dyVector[Nref];
	Nkron ++;
      }

      if ((image[i].imageID == TEST_IMAGE1) || (image[i].imageID == TEST_IMAGE2)) {
	fprintf (stderr, "%1d, %3d : %3d, %3d : %10.6f %10.6f : %6.3f  %6.3f  %6.3f  %6.3f  %6.3f : %6.3f\n", (int) i, (int) j, (int) c, (int) m, catalog[c].averageT[n].R, catalog[c].averageT[n].D, MsysKron, MrelKron, Mmos, Mgrid, Mflat, kronStars.alldata->yVector[Nkron]);
      }

      if (catalog[c].measureT[m].dM < IMFIT_SYS_SIGMA_LIM) {
	brightStars.alldata-> yVector[Nbright] = psfStars.alldata-> yVector[Nref];
	brightStars.alldata->dyVector[Nbright] = psfStars.alldata->dyVector[Nref];
	Nbright ++;
      }
      Nref++;
    }
    /* N_onImage[i] is all measurements, N is good measurements */

    // if (VERBOSE2) fprintf (stderr, "meas skipped: (Nbad: %d, Nmos: %d, Ngrid: %d, Nrel: %d, Nsys: %d)\n", Nbad, Nmos, Ngrid, Nrel, Nsys);

    /* too few good measurements or too many bad measurements */
    mark = (Nref < IMAGE_TOOFEW) || (Nref < IMAGE_GOOD_FRACTION*N_onImage[i]);
    if (mark) {
      image[i].flags |= ID_IMAGE_PHOTOM_FEW;
      image[i].McalPSF    = 0.0;
      image[i].McalAPER   = 0.0;
      image[i].McalChiSq  = NAN;
      image[i].dMcal      = NAN;
      image[i].dMagSys    = NAN;
      image[i].nFitPhotom = 0;
      Nfew ++;
    } else {
      image[i].flags &= ~ID_IMAGE_PHOTOM_FEW;
    }      
    if (mark) continue;

    // use liststats to find the 20-pct, median, 80-pct points
    StatType stats;
    liststats_setmode (&stats, "MEDIAN");
    liststats (psfStars.alldata->yVector, NULL, NULL, Nref, &stats);
    double altSigma = (stats.Upper80 - stats.Lower20) / 1.6;  // 20% to 80% encompasses 60% of the values, corresponds to the range (-0.85 sigma : +0.85 sigma)

    // soften the individual errors with 10% of the scatter above
    for (j = 0; j < Nref; j++) {
      double newSigma = hypot(psfStars.alldata->dyVector[j], 0.1*altSigma);
      psfStars.alldata->dyVector[j] = newSigma;
    }

    // soften the errors based on the scatter
    FitDataSetSoften (&psfStars, Nref);

    fit1d_irls (&psfStars, Nref);
    fit1d_irls (&kronStars, Nkron);
    fit1d_irls (&brightStars, Nbright);

    // no additional weight modification (we treat all stars on an image equally -- note an image is either ubercal-tied or not)
    if (useMcal) {
      image[i].McalPSF    = psfStars.bSaveArray[0][0];
      image[i].McalAPER   = kronStars.bSaveArray[0][0];
      image[i].flags |= ID_IMAGE_IMAGE_PHOTCAL;  // set this flag (unset by default)
    } else {
      image[i].McalPSF    = 0.0;
      image[i].McalAPER   = 0.0;
    }

    // record statistics on the fit regardless if it is used or not
    image[i].dMcal      = psfStars.bSigma[0];
    image[i].nFitPhotom = psfStars.Nmeas;
    image[i].McalChiSq  = psfStars.chisq;
    Ncalibrated ++;

    // bright end scatter
    image[i].dMagSys = brightStars.sigma;

    if ((image[i].imageID == TEST_IMAGE1) || (image[i].imageID == TEST_IMAGE2)) {
      fprintf (stderr, "Mcal for : %s : %7.4f %7.4f\n", image[i].name, image[i].McalAPER, image[i].dMcal);
    }

    if (PLOTSTUFF) {
      fprintf (stderr, "Mcal for : %s : %7.4f %7.4f\n", image[i].name, image[i].McalAPER, image[i].dMcal);
      plot_setMcal (psfStars.alldata-> yVector, Nref);
    }

    // minUbercalDist calculated here is the min value for any star owned by this image
    // since this particular image is tied to that star, bump its distance by 1
    image[i].ubercalDist = minUbercalDist + 1;
  }

  fprintf (stderr, "%d images calibrated\n", Ncalibrated);
  fprintf (stderr, "%d images marked having too few measurements (Nbad: %d, Nmos: %d, Ngrid: %d, Nrel: %d, Nsys: %d)\n", Nfew, Nbad, Nmos, Ngrid, Nrel, Nsys);

  FitDataSetFree (&brightStars);
  FitDataSetFree (&kronStars);
  FitDataSetFree (&psfStars);

  return;
}

/* determine McalTEST values for all images -- this is not used to set measure.Mcal, but only to test */
void setMcalTest (Catalog *catalog) {

  off_t i, j, m, c, n;
  int Nfew, Nbad, Nmos, Ngrp, Nrel, Ngrid, Nsys;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  off_t Nmax = 0;
  for (i = 0; i < Nimage; i++) {
    Nmax = MAX (Nmax, N_onImage[i]);
  }

  // we are making a 0-order fit and not doing bootstrap analysis:
  FitDataSet psfStars;
  FitDataSetAlloc (&psfStars, Nmax, 0, 0);

  // for testing, need to allow this to be optional
  if (FALSE && UseStandardOLS(ZPT_IMAGES)) {
    psfStars.MaxIterations = 0;
  }
  psfStars.MaxIterations = 0;

  Nfew = Nbad = Nmos = Ngrp = Ngrid = Nrel = Nsys = 0;

  int Ncalibrated = 0;
  for (i = 0; i < Nimage; i++) {
    if (image[i].photcode == 0) continue; // skip the PHU images

    off_t Nref = 0;    // number of stars used to measure McalPSF

    if (N_onImage[i] == 0) {
      // fprintf (stderr, "image missing detections? %s %d "OFF_T_FMT"\n", image[i].name, image[i].nstar, N_onImage[i]);
      continue;
    }

    for (j = 0; j < N_onImage[i]; j++) {
      
      m = ImageToMeasure[i][j];
      c = ImageToCatalog[i][j];
      
      if (catalog[c].measureT[m].dbFlags & MEAS_BAD) {
	  Nbad++;
	  continue;
      }
      float Mmos  = getMmos  (m, c);
      if (isnan(Mmos)) {
	  Nmos ++;
	  continue;
      }
      float Mgrp  = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL); // ignore error for now?
      if (isnan(Mgrp)) {
	  Ngrp ++;
	  continue;
      }
      float Mgrid = getMgridTiny (&catalog[c].measureT[m]);
      if (isnan(Mgrid)) {
	  Ngrid++;
	  continue;
      }

      // MrelPSF is the average magnitude for this star.  
      float MrelPSF  = getMrel  (catalog, m, c, MAG_CLASS_PSF, MAG_SRC_CHP);
      if (isnan(MrelPSF)) {
	  Nrel ++;
	  continue;
      }
      
      // image.Mcal is not supposed to include the flat-field correction, so we need to
      // apply that offset as well here for this image (in other words, each detection is
      // being compared to the model, excluding the zero point, Mcal.  The model includes
      // the flat-correction.  NOTE the sign of Mflat (Image.Mcal = Measure.Mcal + Mflat)

      float Mflat = getMflat (m, c, catalog);

      // get the PSF magnitude for thie measurement, with airmass slope applied
      n = catalog[c].measureT[m].averef;
      float MsysPSF = PhotSysTiny (&catalog[c].measureT[m], &catalog[c].averageT[n], &catalog[c].secfilt[n*Nsecfilt], MAG_CLASS_PSF);
      if (isnan(MsysPSF)) {
	Nsys++;
	continue;
      }

      float Moff = Mmos + Mgrp + Mgrid + Mflat;

      psfStars.alldata-> yVector[Nref] = MsysPSF - MrelPSF - Moff;
      psfStars.alldata->dyVector[Nref] = MAX (catalog[c].measureT[m].dM, MIN_ERROR);
      Nref++;
    }

    if (Nref < 5) {
      // fprintf (stderr, "very few detections? %s %d "OFF_T_FMT"\n", image[i].name, image[i].nstar, Nref);
      // continue;
      image[i].McalAPER   = NAN;
      image[i].dMcal      = NAN;
      image[i].nFitPhotom = Nref;
      image[i].McalChiSq  = NAN;
      continue;
    }

    // no additional weight modification (we treat all stars on an image equally -- note an image is either ubercal-tied or not)
    fit1d_irls (&psfStars, Nref);
    image[i].McalAPER   = psfStars.bSaveArray[0][0];
    image[i].dMcal      = psfStars.bSigma[0];
    image[i].nFitPhotom = psfStars.Nmeas;
    image[i].McalChiSq  = psfStars.chisq;
    Ncalibrated ++;
  }

  fprintf (stderr, "%d images calibrated\n", Ncalibrated);
  fprintf (stderr, "%d images marked having too few measurements (Nbad: %d, Nmos: %d, Ngrid: %d, Nrel: %d, Nsys: %d)\n", Nfew, Nbad, Nmos, Ngrid, Nrel, Nsys);

  FitDataSetFree (&psfStars);
  return;
}

/* mark image if: abs(Mcal) too large, dMcal too large */
void clean_images () {

  int mark, Nmark;
  off_t i, N;
  double *mlist, *slist, *dlist;
  double MaxOffset, MaxScatter, MedOffset;

  // if (FREEZE_IMAGES) return;

  if (VERBOSE) fprintf (stderr, "marking poor images\n");

  ALLOCATE (mlist, double, Nimage);
  ALLOCATE (slist, double, Nimage);
  ALLOCATE (dlist, double, Nimage);

  // measure stats for Mcal and dMcal
  for (i = N = 0; i < Nimage; i++) {
    if (image[i].flags & (ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_FEW)) continue;

    if (FREEZE_IMAGES && isMosaicChip(image[i].photcode)) continue;

    mlist[N] = image[i].McalPSF;
    slist[N] = image[i].dMcal;
    dlist[N] = 1;
    N++;
  }

  // use a straight mean to find the global image statistics (no weighting)
  StatType stats;
  liststats_setmode (&stats, "MEAN");

  liststats (mlist, dlist, NULL, N, &stats);
  MaxOffset = MAX (IMAGE_OFFSET, 3*stats.sigma);
  MedOffset = stats.median;

  liststats (slist, dlist, NULL, N, &stats);
  MaxScatter = MAX (IMAGE_SCATTER, 2*stats.median);
  fprintf (stderr, "Mrel: %f, dMrel: %f, Max Scatter: %f, Max Offset: %f\n", MedOffset, stats.median, MaxScatter, MaxOffset);
  
  Nmark = 0;
  for (i = 0; i < Nimage; i++) {
    // if we are keeping ubercal sacrosanct, then we should not be allowed to break them...
    if (KEEP_UBERCAL && (image[i].flags & ID_IMAGE_PHOTOM_UBERCAL)) continue;

    mark = FALSE;

    if (image[i].flags & ID_IMAGE_PHOTOM_FEW) {
      mark = TRUE;
    }
    if (image[i].dMcal > MaxScatter) {
      mark = TRUE;
    }
    if (fabs(image[i].McalPSF - MedOffset) > MaxOffset) {
      mark = TRUE;
    }

    if (mark) { 
      Nmark ++;
      image[i].flags |= ID_IMAGE_PHOTOM_POOR;
    } else {
      image[i].flags &= ~ID_IMAGE_PHOTOM_POOR;
    }
  }

  fprintf (stderr, "%d images marked poor\n", Nmark);
  free (mlist);
  free (slist);
  free (dlist);
}

// find the median zero point and subtract it (for each active average photcode)
void rationalize_zeropoints (int Niter) {

  if (VERBOSE) fprintf (stderr, "rationalize zero points\n");

  // if we are calculating zero points for tgroups,
  // rationalize based on the tgroups
  if (TGROUP_ZEROPT) {
    for (int ic = 0; ic < Nphotcodes; ic++) {
      double zpt = get_median_zpt_tgroups (photcodes[ic][0].code);
      fprintf (stderr, "rationalize zero points by tgroup for %s (%d) : %f\n", photcodes[ic][0].name, photcodes[ic][0].code, zpt);
      if (isnan(zpt)) continue;
      set_median_zpt_tgroups (photcodes[ic][0].code, zpt);
      set_median_zpt_mosaics (photcodes[ic][0].code, zpt);
      set_median_zpt_images  (photcodes[ic][0].code, zpt);
    }
    return;
  }

  // if we are calculating zero points for mosaics, but not tgroups,
  // rationalize based on the mosaics
  if (MOSAIC_ZEROPT) {
    for (int ic = 0; ic < Nphotcodes; ic++) {
      double zpt = get_median_zpt_mosaics (photcodes[ic][0].code);
      fprintf (stderr, "rationalize zero points by mosaic for %s (%d) : %f\n", photcodes[ic][0].name, photcodes[ic][0].code, zpt);
      if (isnan(zpt)) continue;
      set_median_zpt_tgroups (photcodes[ic][0].code, zpt);
      set_median_zpt_mosaics (photcodes[ic][0].code, zpt);
      set_median_zpt_images  (photcodes[ic][0].code, zpt);
    }
    return;
  }    

  // otherwise, rationalize based on the images
  for (int ic = 0; ic < Nphotcodes; ic++) {
    double zpt = get_median_zpt_images (photcodes[ic][0].code);
    fprintf (stderr, "rationalize zero points by image for %s (%d) : %f\n", photcodes[ic][0].name, photcodes[ic][0].code, zpt);
    if (isnan(zpt)) continue;
    set_median_zpt_tgroups (photcodes[ic][0].code, zpt);
    set_median_zpt_mosaics (photcodes[ic][0].code, zpt);
    set_median_zpt_images  (photcodes[ic][0].code, zpt);
  }
}

double get_median_zpt_images (short photcode) {

  double *mlist;

  ALLOCATE (mlist, double, Nimage);

  int N = 0;
  for (int i = 0; i < Nimage; i++) {
    int mycode = GetPhotcodeEquivCodebyCode (image[i].photcode);
    if (mycode != photcode) continue;
    if (!(image[i].flags & ID_IMAGE_IMAGE_PHOTCAL)) continue;
    mlist[N] = image[i].McalPSF;
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

  fprintf (stderr, "rationalize by image using %d pts\n", N);
  return stats.median; 
}

void set_median_zpt_images (short photcode, double zpt) {

  if (!isfinite(zpt)) return; // do not break the zero points

  for (int i = 0; i < Nimage; i++) {
    int mycode = GetPhotcodeEquivCodebyCode (image[i].photcode);
    if (mycode != photcode) continue;
    // fprintf (stderr, "IMAGE %d zpt %f -> ", i, image[i].McalPSF);

    int applyOffset = TRUE;
    TGroup *mygrp = getTGroupForImage (i);
    if (mygrp && (mygrp->flags & ID_IMAGE_TGROUP_PHOTCAL)) applyOffset = FALSE;

    Mosaic *mymos = getMosaicForImage (i);
    if (mymos && (mymos->flags & ID_IMAGE_MOSAIC_PHOTCAL)) applyOffset = FALSE;

    if (applyOffset) {
      image[i].McalPSF -= zpt;
      image[i].McalAPER -= zpt;
    }
    // fprintf (stderr, "%f (%d)\n", image[i].McalPSF, applyOffset);
  }

  return; 
}

static int setMcal_init_done = FALSE;
void plot_setMcal (double *list, int Npts) {

  off_t i;
  double *xlist;
  Graphdata graphdata;

  if (!PLOTSTUFF) return;

  if (Npts == 0) {
    // fprintf (stderr, "cannot set image Mcal values yet (nan Mave)\n");
    return;
  }

  plot_defaults (&graphdata);
  graphdata.xmin = -0.01;
  graphdata.xmax = +1.01;
  graphdata.ymin = -0.21;
  graphdata.ymax = +0.21;

  if (!setMcal_init_done) {
    int kapa = get_graph(0);
    if (kapa < 1) {
      kapa = open_graph(0);
      if (!kapa) return;
    }

    KapaClearSections (kapa);
    KapaClearPlots (kapa);
    KapaSetLimits (kapa, &graphdata);
    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, &graphdata);
    setMcal_init_done = TRUE;
  }

  ALLOCATE (xlist, double, Npts);
  for (i = 0; i < Npts; i++) {
    xlist[i] = i / (float) Npts;
  }

  plot_list_add (&graphdata, xlist, list, Npts);

  free (xlist);
}

void plot_images () {

  off_t i, bin;
  double *xlist, *Mlist, *dlist;
  Graphdata graphdata;

  // if (FREEZE_IMAGES) return;

  ALLOCATE (xlist, double, Nimage);
  ALLOCATE (dlist, double, Nimage);
  ALLOCATE (Mlist, double, Nimage);

  /**** dMcal vs airmass ****/
  float minAirmass = 1000.0;
  float maxAirmass =    0.0;
  float minMcal    = +100.0;
  float maxMcal    = -100.0;
  float mindMcal    = +100.0;
  float maxdMcal    = -100.0;

  int Nplot = 0;

  for (i = 0; i < Nimage; i++) {

    if (FREEZE_IMAGES && isMosaicChip(image[i].photcode)) continue;

    Mlist[Nplot] = image[i].McalPSF;
    dlist[Nplot] = image[i].dMcal;
    xlist[Nplot] = image[i].secz;
    minAirmass = MIN (image[i].secz, minAirmass);
    maxAirmass = MAX (image[i].secz, maxAirmass);
    minMcal = MIN (image[i].McalPSF, minMcal);
    maxMcal = MAX (image[i].McalPSF, maxMcal);
    mindMcal = MIN (image[i].dMcal, mindMcal);
    maxdMcal = MAX (image[i].dMcal, maxdMcal);
    
    Nplot ++;
  }

  float AirmassRange  = MAX(1.2*(maxAirmass - minAirmass), 0.25);
  float AirmassCenter = 0.5*(maxAirmass + minAirmass);

  float McalRange  = MAX(1.2*(maxMcal - minMcal), 0.21);
  float McalCenter = 0.5*(maxMcal + minMcal);

  float dMcalRange  = MAX(1.2*(maxdMcal - mindMcal), 0.21);
  float dMcalCenter = 0.5*(maxdMcal + mindMcal);

  plot_defaults (&graphdata);
  graphdata.xmin = AirmassCenter - 0.5*AirmassRange;
  graphdata.xmax = AirmassCenter + 0.5*AirmassRange;
  graphdata.ymin = McalCenter - 0.5*McalRange;
  graphdata.ymax = McalCenter + 0.5*McalRange;
  plot_list (&graphdata, xlist, Mlist, Nimage, "airmass vs Mcal", "%s.airmass.png", OUTROOT);

  plot_defaults (&graphdata);
  graphdata.xmin = McalCenter - 0.5*McalRange;
  graphdata.xmax = McalCenter + 0.5*McalRange;
  graphdata.ymin = dMcalCenter - 0.5*dMcalRange;
  graphdata.ymax = dMcalCenter + 0.5*dMcalRange;
  plot_list (&graphdata, Mlist, dlist, Nimage, "Mcal vs dMcal", "%s.Mcal.dMcal.png", OUTROOT);

# define NBIN 200
  REALLOCATE (xlist, double, NBIN);
  REALLOCATE (Mlist, double, NBIN);

  /**** dMcal histgram ****/
  for (i = 0; i < NBIN; i++) xlist[i] = 0.00025*i;
  bzero (Mlist, NBIN*sizeof(double));

  for (i = 0; i < Nimage; i++) {
    if (FREEZE_IMAGES && isMosaicChip(image[i].photcode)) continue;

    bin = image[i].dMcal / 0.00025;
    bin = MAX (0, MIN (NBIN - 1, bin));
    Mlist[bin] += 1.0;
  }

  plot_defaults (&graphdata);
  graphdata.style = 1;
  graphdata.xmin = dMcalCenter - 0.5*dMcalRange;
  graphdata.xmax = dMcalCenter + 0.5*dMcalRange;
  plot_list (&graphdata, xlist, Mlist, NBIN, "dMcal hist", "%s.dMcalhist.png", OUTROOT);

  free (dlist);
  free (xlist);
  free (Mlist);
}

StatType statsImageN (Catalog *catalog) {

  off_t i, j, m, c, n, N;
  double *list, *dlist;
  float Mcal, Mmos, Mgrp, Mgrid;

  StatType stats;
  bzero (&stats, sizeof (StatType));

  // we no longer blindly apply FREEZE_IMAGES to all images, only to mosaics
  // if (FREEZE_IMAGES) return (stats);

  ALLOCATE (list, double, Nimage);
  ALLOCATE (dlist, double, Nimage);

  n = 0;
  for (i = 0; i < Nimage; i++) {
    if (image[i].flags & (ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_FEW)) continue;

    if (FREEZE_IMAGES && isMosaicChip(image[i].photcode)) continue;

    N = 0;
    for (j = 0; j < N_onImage[i]; j++) {

      m = ImageToMeasure[i][j];
      c = ImageToCatalog[i][j];

      Mcal  = getMcal  (m, c, MAG_CLASS_PSF);
      if (isnan(Mcal)) continue;
      Mmos  = getMmos  (m, c);
      if (isnan(Mmos)) continue;
      Mgrp  = getMgrp  (m, c, catalog[c].measureT[m].airmass, NULL);
      if (isnan(Mgrp)) continue;
      Mgrid = getMgridTiny (&catalog[c].measureT[m]);
      if (isnan(Mgrid)) continue;
      N++;
    }
    list[n] = N;
    dlist[n] = 1;
    n++;
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

StatType statsImageX (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  off_t i, n;
  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  // if (FREEZE_IMAGES) return (stats);

  ALLOCATE (list, double, Nimage);
  ALLOCATE (dlist, double, Nimage);

  n = 0;
  for (i = 0; i < Nimage; i++) {

    if (image[i].flags & (ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_FEW)) continue;

    if (FREEZE_IMAGES && isMosaicChip(image[i].photcode)) continue;

    list[n] = image[i].McalChiSq;
    dlist[n] = 1;
    n++;
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

StatType statsImageM (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  off_t i, n;
  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  // if (FREEZE_IMAGES) return (stats);

  ALLOCATE (list, double, Nimage);
  ALLOCATE (dlist, double, Nimage);

  n = 0;
  for (i = 0; i < Nimage; i++) {

    if (image[i].flags & (ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_FEW)) continue;

    if (FREEZE_IMAGES && isMosaicChip(image[i].photcode)) continue;

    list[n] = image[i].McalPSF;
    dlist[n] = 1;
    n++;
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

StatType statsImagedM (Catalog *catalog) {
  OHANA_UNUSED_PARAM(catalog);

  off_t i, n;
  double *list, *dlist;
  StatType stats;

  bzero (&stats, sizeof (StatType));
  // if (FREEZE_IMAGES) return (stats);

  ALLOCATE (list, double, Nimage);
  ALLOCATE (dlist, double, Nimage);

  n = 0;
  for (i = 0; i < Nimage; i++) {

    if (image[i].flags & (ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_FEW)) continue;

    if (FREEZE_IMAGES && isMosaicChip(image[i].photcode)) continue;

    list[n] = image[i].dMcal;
    dlist[n] = 1;
    n++;
  }

  liststats_setmode (&stats, "MEAN");

  liststats (list, dlist, NULL, n, &stats);
  free (list);
  free (dlist);
  return (stats);
}

void clearImages (void) {
  image = NULL;
}
