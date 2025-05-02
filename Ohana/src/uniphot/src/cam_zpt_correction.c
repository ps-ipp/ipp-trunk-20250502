# include "setphot.h"

// hard-coded values
char filters[5][3] = {"g", "r", "i", "z", "y"};

// camera systematic (photoflat) correction functions:
// load the correction set from a file

// note that we are allocating pointers for XY00 - XY77, but only the octal elements are set (and not the 00,07,70,77 ones)

void CamPhotomCorrectionFree (CamPhotomCorrection *cam) {

  if (!cam) return;

  int i;
  for (i = 0; i < cam->Nvalues; i++) {
    FREE (cam->matrix[i]);
    FREE (cam->header[i]);
  }
  FREE (cam->phu);

  free (cam->tstart);
  free (cam->tstop);

  free (cam->matrix);
  free (cam->header);
  free (cam);
}

CamPhotomCorrection *CamPhotomCorrectionAlloc (int Nx, int Ny, int Nfilter, int Nseason) {

  CamPhotomCorrection *cam;

  ALLOCATE (cam, CamPhotomCorrection, 1);

  cam->phu = NULL;
  ALLOCATE (cam->header, Header *, Nseason*Nfilter*Nx*Ny);
  ALLOCATE (cam->matrix, Matrix *, Nseason*Nfilter*Nx*Ny);
  ALLOCATE (cam->tstart, e_time,   Nseason);
  ALLOCATE (cam->tstop,  e_time,   Nseason);

  cam->Nx      = Nx;  	   // for gpc1, should be 8
  cam->Ny      = Ny;  	   // for gpc1, should be 8
  cam->Nfilter = Nfilter;  // for gpc1, should be 5
  cam->Nseason = Nseason;  // for gpc1, should be 5 (1 for loading highres correction)

  cam->Nchips  = Nx * Ny;
  cam->Nflats  = Nx * Ny * Nfilter;
  cam->Nvalues = Nx * Ny * Nfilter * Nseason;

  ALLOCATE (cam->matrix, Matrix *, cam->Nvalues);

  int i;
  for (i = 0; i < cam->Nvalues; i++) {
    cam->matrix[i] = NULL;
  }

  return cam;
}

CamPhotomCorrection *CamPhotomCorrectionLoad (char *filename) {

  int i, ix, iy, season, filter;

  Header *phu = gfits_alloc_header();

  /* open file for input */
  FILE *f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for read : %s\n", filename);
    return FALSE;
  }

  // read the PHU header
  if (!gfits_load_header (f, phu)) return FALSE;
  
  int Nbytes = gfits_data_size (phu);
  fseeko (f, Nbytes, SEEK_CUR);

  int Nseason, Nfilter, Nx, Ny, dX, dY, NxCCD, NyCCD;
  if (!gfits_scan (phu, "NSEASON", "%d", 1, &Nseason)) return FALSE;
  if (!gfits_scan (phu, "NFILTER", "%d", 1, &Nfilter)) return FALSE;
  if (!gfits_scan (phu, "NX", 	   "%d", 1, &Nx))      return FALSE;
  if (!gfits_scan (phu, "NY", 	   "%d", 1, &Ny))      return FALSE;
  if (!gfits_scan (phu, "DX", 	   "%d", 1, &dX))      return FALSE;
  if (!gfits_scan (phu, "DY", 	   "%d", 1, &dY))      return FALSE;
  if (!gfits_scan (phu, "NX_CHIP", "%d", 1, &NxCCD))   return FALSE;
  if (!gfits_scan (phu, "NY_CHIP", "%d", 1, &NyCCD))   return FALSE;

  CamPhotomCorrection *cam = CamPhotomCorrectionAlloc (Nx, Ny, Nfilter, Nseason);
  cam->phu = phu;

  cam->dX      = dX;  	   // superpixel sampling, x-dir
  cam->dY      = dY;  	   // superpixel sampling, y-dir

  cam->NxCCD   = NxCCD;    // pixels per chip
  cam->NyCCD   = NyCCD;    // pixels per chip

  for (i = 0; i < Nseason; i++) {
    char field[256];
    double value;
    
    snprintf (field, 256, "TS_%03d", i);
    if (!gfits_scan (phu, field, "%lf", 1, &value)) return FALSE;
    cam->tstart[i] = ohana_mjd_to_sec(value);
    
    snprintf (field, 256, "TE_%03d", i);
    if (!gfits_scan (phu, field, "%lf", 1, &value)) return FALSE;
    cam->tstop[i] = ohana_mjd_to_sec(value);
  }

  while (TRUE) {
    // load data for this header : if not found, assume we hit the end of the file
    Header *header = gfits_alloc_header();
    if (!gfits_load_header (f, header)) { 
      free (header);
      break;
    }
    
    if (!gfits_scan (header, "FILTER", "%d", 1, &filter)) myAbort ("failed to find FILTER");
    if (!gfits_scan (header, "SEASON", "%d", 1, &season)) myAbort ("failed to find SEASON");
    if (!gfits_scan (header, "X_CHIP", "%d", 1, &ix))     myAbort ("failed to find X_CHIP");
    if (!gfits_scan (header, "Y_CHIP", "%d", 1, &iy))     myAbort ("failed to find Y_CHIP");
    // XXX NOTE: astroflat.20150209.fits had ix and iy backwards in header
    // double-check that the new flats are OK

    Matrix *matrix = gfits_alloc_matrix();
    if (!gfits_load_matrix (f, matrix, header)) myAbort ("failed to read matrix");

    int index = ix + iy*cam->Nx + filter*cam->Nchips + season*cam->Nflats;
    myAssert (index >= 0, "index too small");
    myAssert (index < cam->Nvalues, "index too big");

    myAssert (!cam->matrix[index], "entry already assigned?");

    cam->matrix[index] = matrix;
    cam->header[index] = header;
  }
  return cam;
}

int CamPhotomCorrectionSave (CamPhotomCorrection *cam, char *filename) {

  int i;

  /* open file for input */
  FILE *f = fopen (filename, "w");
  if (!f) {
    gprint (GP_ERR, "can't open file for write : %s\n", filename);
    return FALSE;
  }

  Matrix matrix;

  gfits_modify_alt (cam->phu, "EXTEND", "%t",  1, TRUE);
  gfits_fwrite_header  (f, cam->phu);
  gfits_create_matrix (cam->phu, &matrix);
  gfits_fwrite_matrix  (f, &matrix);
  // write an empty matrix?

  for (i = 0; i < cam->Nvalues; i++) {
    if (!cam->matrix[i]) continue;
    gfits_primary_to_extended (cam->header[i], "IMAGE", "Image Extension");
    gfits_fwrite_header  (f, cam->header[i]);
    gfits_fwrite_matrix  (f, cam->matrix[i]);
    fflush (f);
  }

  return TRUE;
}

float CamPhotomCorrectionValue (CamPhotomCorrection *cam, int flat_id, float Xccd, float Yccd) {

  if (!flat_id) return 0.0;

  //  myAssert (flat_id > 0, "flat_id out of range");
  // myAssert (flat_id <= cam->Nvalues, "flat_id out of range");
  if ((flat_id < 0)||(flat_id > cam->Nvalues)) { return 0.0; }
  
  // validate the flat_id (not out of range?)
  // NOTE: we are setting the ID to be the sequence + 1 (match_camcorr_to_images.c:39)
  int seq = flat_id - 1;

  Matrix *matrix = cam->matrix[seq];
  myAssert (matrix, "oops");

  int NxModel = matrix->Naxis[0];

  float *buffer = (float *) matrix->buffer;

  int jx = MAX(MIN(cam->NxCCD, Xccd / cam->dX), 0);
  int jy = MAX(MIN(cam->NyCCD, Yccd / cam->dY), 0);

  float value = buffer[jx + jy*NxModel];

  float dM = isnan(value) ? 0.0 : value;

  return dM;
}

CamPhotomCorrection *merge_flatcorr_and_camcorr (FlatCorrectionTable *flatcorr, CamPhotomCorrection *camcorr) {

  int i, j, ix, iy, Nf, Ns;

  // this function is specifically tuned for merging the ubercal flats with the high-res flats

  // we have an ubercal FlatCorrectionTable with Nseasons, Nfilter, Nx, Ny entries
  // we have a highres flat with Nfilter, Nx, Ny images

  myAssert (camcorr->Nseason == 1, "no code to merge seasons flats to seasonal flats");

  // we need to create a new CamPhotomCorrection with Nx,Ny,Nfilter from camcorr and Nseason from flatcorr:
  CamPhotomCorrection *newcorr = CamPhotomCorrectionAlloc (camcorr->Nx, camcorr->Ny, camcorr->Nfilter, flatcorr->Nseason);
  newcorr->phu = gfits_alloc_header();
  gfits_create_header(newcorr->phu);

  newcorr->dX      = camcorr->dX;    // superpixel sampling, x-dir
  newcorr->dY      = camcorr->dY;    // superpixel sampling, y-dir
  newcorr->NxCCD   = camcorr->NxCCD; // pixels per chip
  newcorr->NyCCD   = camcorr->NyCCD; // pixels per chip

  for (Ns = 0; Ns < flatcorr->Nseason; Ns++) {
    newcorr->tstart[Ns] = flatcorr->tstart[Ns];
    newcorr->tstop[Ns]  = flatcorr->tstop[Ns];
  }

  gfits_modify (newcorr->phu, "NSEASON",  "%d", 1, newcorr->Nseason);
  gfits_modify (newcorr->phu, "NFILTER",  "%d", 1, newcorr->Nfilter);
  gfits_modify (newcorr->phu, "NX",       "%d", 1, newcorr->Nx);
  gfits_modify (newcorr->phu, "NY",       "%d", 1, newcorr->Ny);
  gfits_modify (newcorr->phu, "DX",       "%d", 1, newcorr->dX);
  gfits_modify (newcorr->phu, "DY",       "%d", 1, newcorr->dY);
  gfits_modify (newcorr->phu, "NX_CHIP",  "%d", 1, newcorr->NxCCD);
  gfits_modify (newcorr->phu, "NY_CHIP",  "%d", 1, newcorr->NyCCD);

  // XXX UNITS for tstart, tstop
  for (Ns = 0; Ns < newcorr->Nseason; Ns++) {
    char field[256];
    double value;
    snprintf (field, 256, "TS_%03d", Ns);
    value = ohana_sec_to_mjd (newcorr->tstart[Ns]);
    if (!gfits_modify (newcorr->phu, field, "%lf", 1, value)) return FALSE;

    snprintf (field, 256, "TE_%03d", Ns);
    value = ohana_sec_to_mjd (newcorr->tstop[Ns]);
    if (!gfits_modify (newcorr->phu, field, "%lf", 1, value))  return FALSE;
  }

  // create the new flat images (1 per season)
  for (Ns = 0; Ns < newcorr->Nseason; Ns++) {
    for (Nf = 0; Nf < newcorr->Nfilter; Nf++) {
      for (ix = 0; ix < newcorr->Nx; ix++) {
	for (iy = 0; iy < newcorr->Ny; iy++) {
	  // camcorr only had one season
	  int idxNew = ix + iy*newcorr->Nx + Nf*newcorr->Nchips + Ns*newcorr->Nflats;
	  int idxOld = ix + iy*camcorr->Nx + Nf*camcorr->Nchips;

	  if (!camcorr->matrix[idxOld]) continue;
	  myAssert (camcorr->header[idxOld], "oops");

	  // these allocate the structures but not the data
	  newcorr->matrix[idxNew] = gfits_alloc_matrix();
	  newcorr->header[idxNew] = gfits_alloc_header();

	  // these allocate the buffers
	  gfits_copy_header (camcorr->header[idxOld], newcorr->header[idxNew]);
	  gfits_copy_matrix (camcorr->matrix[idxOld], newcorr->matrix[idxNew]);

	  // using hard-wired filter names:
	  if (!gfits_modify (newcorr->header[idxNew], "FILTNAME", "%s", 1, filters[Nf])) myAbort ("failed to set FILTNAME");
	  if (!gfits_modify (newcorr->header[idxNew], "FILTER", "%d", 1, Nf)) myAbort ("failed to set FILTER");
	  if (!gfits_modify (newcorr->header[idxNew], "SEASON", "%d", 1, Ns)) myAbort ("failed to set SEASON");
	  if (!gfits_modify (newcorr->header[idxNew], "X_CHIP", "%d", 1, ix)) myAbort ("failed to set X_CHIP");
	  if (!gfits_modify (newcorr->header[idxNew], "Y_CHIP", "%d", 1, iy)) myAbort ("failed to set Y_CHIP");

	  char extname[80];
	  snprintf (extname, 80, "td_dM_%d_%s_%d_%d", Ns, filters[Ns], ix, iy);
	  if (!gfits_modify (newcorr->header[idxNew], "EXTNAME", "%s", 1, extname)) myAbort ("failed to set EXTNAME");
	}
      }
    }
  }

  // FlatCorrectionTable has:
  // table->image[seq] with metadata about the specific image
  // table->offset[seq][ix][iy] with dM values for image[seq] 

  // Set up HSC/GPC1 distinguishing limits

  int minCodeGPC1 = 10000;
  int maxCodeGPC1 = 10576;

  int minCodeHSC  = 20000;
  int maxCodeHSC  = 26111;

  int isGPC1 = FALSE;
  int isHSC  = FALSE;
    
  // loop over the flatcorr images and match from image[seq] to newcorr:
  for (i = 0; i < flatcorr->Nimage; i++) {

    int ID = flatcorr->image[i].ID;
    myAssert (flatcorr->IDtoSeq[ID] == i, "oops");

    // find the season (these are inherited from flatcorr, so they must match
    int season = -1;
    for (j = 0; j < newcorr->Nseason; j++) {
      if (flatcorr->image[i].tstart != newcorr->tstart[j]) continue;
      season = j;
    }
    myAssert (season >= 0, "oops");

    int photcode = flatcorr->image[i].photcode;

    if ((photcode >= minCodeGPC1)&&(photcode <= maxCodeGPC1)) {
      isGPC1 = TRUE;
      isHSC  = FALSE;
    }
    else if ((photcode >= minCodeHSC)&&(photcode <= maxCodeHSC)) {
      isGPC1 = FALSE;
      isHSC  = TRUE;
    }
    if (!isGPC1 && !isHSC) { continue; }

    int iy = 0;
    int ix = 0;
    int filter = 1000;
    
    // find the chip and filter from photcode:
    if (isGPC1) {
      iy = photcode % 10;
      ix = (int)(photcode / 10) % 10;
      filter = (int)(photcode / 100) % 10;
    }
    if (isHSC) {
      iy = 0;
      ix = (int) photcode % 112;
      filter = (int)(photcode / 1000) % 10;
    }
	      
    myAssert (ix     < newcorr->Nx, "oops");
    myAssert (iy     < newcorr->Ny, "oops");
    myAssert (filter < newcorr->Nfilter, "oops");

    // seq is the flatcorr entry of interest
    int seq = ix + iy * newcorr->Nx + filter * newcorr->Nchips + season * newcorr->Nflats;

    // we now have newcorr->matrix[seq]
    // flatcorr contains entries for unpopulated corner chips, but camcorr does not
    Matrix *newDelta = newcorr->matrix[seq];
    if (!newDelta) continue;

    float *values = (float *) newDelta->buffer;

    int jx, jy;
    int NxModel = newDelta->Naxis[0];
    int NyModel = newDelta->Naxis[1];

    // now loop over the newcorr pixels to get the flatcorr value:
    for (jx = 0; jx < NxModel; jx++) {
      for (jy = 0; jy < NyModel; jy++) {
	
	int chipX = jx * newcorr->dX;
	int chipY = jy * newcorr->dY;

	// get the flatcorr offset from this chip coordinate
	float delta = FlatCorrectionOffset (flatcorr, ID, chipX, chipY);

	// note that Eddie's offset is defined so that Mrel = Mcat + offset
	// while my flatfield dM is defined so that dM = Mcat - Mrel or Mrel = Mcal - dM
	// note that in the old code, measure.Mcal = -offset and Mrel = Mcat - Mcal
	values[jx + jy*NxModel] -= delta;
      }
    }
  }    

  return newcorr;
}


