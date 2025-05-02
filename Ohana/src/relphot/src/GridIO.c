# include "relphot.h"

int GridCorrectionSave () {

  if (!GRID_ZEROPT) return TRUE;

  char filename[1024], uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();

  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  snprintf (filename, 1024, "%s/gridcorr.mean.%s.fits", CATDIR, uniquer);
  if (!GridCorrectionSaveFile (filename, GRID_MEAN)) return FALSE;
  GRID_MEANFILE = strcreate (filename);  // save in the global so we can pass to e.g., reload_catalogs

  snprintf (filename, 1024, "%s/gridcorr.stdev.%s.fits", CATDIR, uniquer);
  if (!GridCorrectionSaveFile (filename, GRID_STDEV)) return FALSE;

  snprintf (filename, 1024, "%s/gridcorr.npts.%s.fits", CATDIR, uniquer);
  if (!GridCorrectionSaveFile (filename, GRID_NPTS)) return FALSE;

  return TRUE;
}

int GridCorrectionSaveFile (char *filename, GridOutputMode mode) {

  /* open file for input */
  FILE *f = fopen (filename, "w");
  if (!f) {
    gprint (GP_ERR, "can't open file for write : %s\n", filename);
    return FALSE;
  }

  // generate a PHU 

  Header phu_header;
  Matrix phu_matrix;

  gfits_init_header   (&phu_header);
  gfits_create_header (&phu_header);
  gfits_modify_alt    (&phu_header, "EXTEND", "%t",  1, TRUE);
  gfits_create_matrix (&phu_header, &phu_matrix);
  gfits_fwrite_header (f, &phu_header);
  gfits_fwrite_matrix (f, &phu_matrix);
  gfits_free_header (&phu_header);
  gfits_free_matrix (&phu_matrix);

  int Nlast = -1;

  while (TRUE) {
    // getGridCorreNext() takes a photcode and returns the *next* valid grid correction pointer (start at -1)
    GridCorrectionType *GridCorr = getGridCorrNext (&Nlast);
    if (!GridCorr) break;

    int filter = GetPhotcodeEquivCodebyCode (GridCorr->photcode);
    char *filtname = GetPhotcodeNamebyCode (filter); // reference, do not free
    char *photname = GetPhotcodeNamebyCode (GridCorr->photcode); // reference, do not free

    Header header;
    Matrix matrix;

    gfits_init_header (&header);
    header.bitpix   = (mode == GRID_NPTS) ? 32 : -32;
    header.Naxes    = 2;
    header.Naxis[0] = GridCorr->Nx;
    header.Naxis[1] = GridCorr->Ny;

    /* create the appropriate header and matrix */
    gfits_create_header (&header);
    gfits_create_matrix (&header, &matrix);

    // create header & matrix
    if (!gfits_modify (&header, "PHOTCODE", "%d", 1, GridCorr->photcode)) myAbort ("failed to set PHOTCODE");
    if (!gfits_modify (&header, "FILTER",   "%s", 1, filtname))           myAbort ("failed to set FILTER");
    if (!gfits_modify (&header, "SEASON",   "%d", 1, 0))                  myAbort ("failed to set SEASON"); // XXX hard-wired for now
    if (!gfits_modify (&header, "X_CHIP",   "%d", 1, -1))                 myAbort ("failed to set X_CHIP");
    if (!gfits_modify (&header, "Y_CHIP",   "%d", 1, -1))                 myAbort ("failed to set Y_CHIP");
    if (!gfits_modify (&header, "DX_CHIP",  "%f", 1, GridCorr->dX))       myAbort ("failed to set DX_CHIP");
    if (!gfits_modify (&header, "DY_CHIP",  "%f", 1, GridCorr->dY))       myAbort ("failed to set DY_CHIP");
    if (!gfits_modify (&header, "NX_CHIP",  "%d", 1, GridCorr->Nx))       myAbort ("failed to set NX_CHIP");
    if (!gfits_modify (&header, "NY_CHIP",  "%d", 1, GridCorr->Ny))       myAbort ("failed to set NY_CHIP");

    int Nx = GridCorr->Nx;

    float *fvalue = (float *) matrix.buffer;
    int   *ivalue = (int   *) matrix.buffer;
    
    for (int iy = 0; iy < GridCorr->Ny; iy++) {
      for (int ix = 0; ix < GridCorr->Nx; ix++) {
	switch (mode) {
	  case GRID_MEAN:  fvalue[ix + iy*Nx] = GridCorr-> Mgrid[ix][iy]; break;
	  case GRID_STDEV: fvalue[ix + iy*Nx] = GridCorr->dMgrid[ix][iy]; break;
	  case GRID_NPTS:  ivalue[ix + iy*Nx] = GridCorr->nMgrid[ix][iy]; break;
	}
      }
    }
    
    char extname[85];
    switch (mode) {
      // XXX If we add seasons, add here:
      case GRID_MEAN:  snprintf (extname, 85, "%s.%s", photname, "MEAN");  break;
      case GRID_STDEV: snprintf (extname, 85, "%s.%s", photname, "STDEV"); break;
      case GRID_NPTS:  snprintf (extname, 85, "%s.%s", photname, "NPTS");  break;
    }
    gfits_modify (&header, "EXTNAME", "%s", 1, extname);

    gfits_primary_to_extended (&header, "IMAGE", "Image Extension");
    gfits_fwrite_header  (f, &header);
    gfits_fwrite_matrix  (f, &matrix);

    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  }
  fclose (f);

  return TRUE;
}

void GridCorrectionLoad (char *filename) {

  int photcode;

  Header phu;
  Header header;
  Matrix matrix;

  if (!GRID_ZEROPT) return;

  // allocate the basic containing structures:
  initGridBins ();

  /* open file for input */
  FILE *f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for read : %s\n", filename);
    return;
  }

  // read the PHU header
  if (!gfits_fread_header (f, &phu)) return;
  
  // move to the first extension
  int Nbytes = gfits_data_size (&phu);
  fseeko (f, Nbytes, SEEK_CUR);

  gfits_free_header (&phu);

  while (gfits_fread_header (f, &header)) {
    if (!gfits_fread_matrix (f, &matrix, &header))                         myAbort ("failed to read matrix");

    if (!gfits_scan (&header, "PHOTCODE", "%d", 1, &photcode))    myAbort ("failed to get PHOTCODE");
    GridCorrectionType *GridCorr = newGridCorrByCode (photcode);
    GridCorr->photcode = photcode;

    if (!gfits_scan (&header, "DX_CHIP",  "%f", 1, &GridCorr->dX)) myAbort ("failed to get DX_CHIP");
    if (!gfits_scan (&header, "DY_CHIP",  "%f", 1, &GridCorr->dY)) myAbort ("failed to get DY_CHIP");
    if (!gfits_scan (&header, "NX_CHIP",  "%d", 1, &GridCorr->Nx)) myAbort ("failed to get NX_CHIP");
    if (!gfits_scan (&header, "NY_CHIP",  "%d", 1, &GridCorr->Ny)) myAbort ("failed to get NY_CHIP");
    
    GridCorr->dMgrid = NULL;
    GridCorr->nMgrid = NULL;
    ALLOCATE (GridCorr->Mgrid, float *, GridCorr->Nx);
    for (int ix = 0; ix < GridCorr->Nx; ix++) {
      ALLOCATE (GridCorr->Mgrid[ix], float, GridCorr->Ny);
    }      

    int Nx = GridCorr->Nx;

    float *fvalue = (float *) matrix.buffer;
    
    for (int iy = 0; iy < GridCorr->Ny; iy++) {
      for (int ix = 0; ix < GridCorr->Nx; ix++) {
	GridCorr-> Mgrid[ix][iy] = fvalue[ix + iy*Nx];
      }
    }
    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  }
  gfits_free_header (&header);
}
