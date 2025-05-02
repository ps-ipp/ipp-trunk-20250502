# include "setastrom.h"

// camera systematic (astroflat) correction functions:
// load the correction set from a file

// note that we are allocating pointers for XY00 - XY77, but only the octal elements are set (and not the 00,07,70,77 ones)

static CamAstromCorrection *cam = NULL;

int CamAstromCorrectionLoad (char *filename) {

  int i, ix, iy, dir, filter;
  Header header;

  /* open file for input */
  FILE *f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for read : %s\n", filename);
    return FALSE;
  }

  ALLOCATE (cam, CamAstromCorrection, 1);

  // read the PHU header
  if (!gfits_load_header (f, &header)) return FALSE;
  
  int Nbytes = gfits_data_size (&header);
  fseeko (f, Nbytes, SEEK_CUR);

  int Ndir, Nfilter, Nx, Ny, dX, dY, NxCCD, NyCCD;
  if (!gfits_scan (&header, "NDIR",     "%d", 1, &Ndir))    return FALSE;
  if (!gfits_scan (&header, "NFILTER",  "%d", 1, &Nfilter)) return FALSE;
  if (!gfits_scan (&header, "NX", 	"%d", 1, &Nx)) 	    return FALSE;
  if (!gfits_scan (&header, "NY", 	"%d", 1, &Ny)) 	    return FALSE;
  if (!gfits_scan (&header, "DX", 	"%d", 1, &dX)) 	    return FALSE;
  if (!gfits_scan (&header, "DY", 	"%d", 1, &dY)) 	    return FALSE;

  if (!gfits_scan (&header, "NX_CHIP", "%d", 1, &NxCCD))   return FALSE;
  if (!gfits_scan (&header, "NY_CHIP", "%d", 1, &NyCCD))   return FALSE;

  ALLOCATE (cam->matrix, Matrix *, Ndir*Nfilter*Nx*Ny);

  cam->Nx      = Nx;  	   // for gpc1, should be 8
  cam->Ny      = Ny;  	   // for gpc1, should be 8
  cam->Nfilter = Nfilter;  // for gpc1, should be 5
  cam->Ndir    = Ndir;     // for gpc1, should be 2

  cam->dX      = dX;  	   // superpixel sampling, x-dir
  cam->dY      = dY;  	   // superpixel sampling, y-dir

  cam->NxCCD   = NxCCD;    // pixels per chip
  cam->NyCCD   = NyCCD;    // pixels per chip

  cam->Nchips  = Nx * Ny;
  cam->Ngroup  = Nx * Ny * Nfilter;
  cam->Nvalues = Nx * Ny * Nfilter * Ndir;

  ALLOCATE (cam->matrix, Matrix *, cam->Nvalues);
  for (i = 0; i < cam->Nvalues; i++) {
    cam->matrix[i] = NULL;
  }

  while (TRUE) {
    Header theader;

    // load data for this header : if not found, assume we hit the end of the file
    if (!gfits_load_header (f, &theader)) break;
    
    if (!gfits_scan (&theader, "FILTER",  "%d", 1, &filter))  return FALSE;
    if (!gfits_scan (&theader, "DIR",     "%d", 1, &dir))     return FALSE;

    if (!gfits_scan (&theader, "X_CHIP",  "%d", 1, &ix))      return FALSE;
    if (!gfits_scan (&theader, "Y_CHIP",  "%d", 1, &iy))      return FALSE;

    // XXX NOTE: astroflat.20150209.fits had ix and iy backwards in header
    // if (!gfits_scan (&theader, "X_CHIP",  "%d", 1, &iy))      return FALSE;
    // if (!gfits_scan (&theader, "Y_CHIP",  "%d", 1, &ix))      return FALSE;

    Matrix *matrix = NULL;
    ALLOCATE (matrix, Matrix, 1);
    if (!gfits_load_matrix (f, matrix, &theader)) break;

    int index = ix + iy*cam->Nx + filter*cam->Nchips + dir*cam->Ngroup;
    myAssert (index >= 0, "index too small");
    myAssert (index < cam->Nvalues, "index too big");

    myAssert (!cam->matrix[index], "entry already assigned?");

    // assert that cam->matrix[index] is NULL?
    cam->matrix[index] = matrix;
  }
  return TRUE;
}

// input is:
// chipID == octal chip ID (eg 40 for XY40)
// filter == (01234 = grizy)
int CamAstromCorrectionValue (int chipID, int filter, float Xccd, float Yccd, double *dX, double *dY) {

  *dX = 0.0;
  *dY = 0.0;

  int index, jx, jy, NxModel;
  Matrix *matrix;
  float *buffer, value;

  // split out the chipID number (eg 43 for XY43) into X and Y elements
  int ix = (int) (chipID / 10);
  int iy = (int) (chipID % 10);

  // dX (dir == 0)
  index = ix + iy*cam->Nx + filter*cam->Nchips;

  matrix = cam->matrix[index];
  NxModel = cam->matrix[index]->Naxis[0];

  buffer = (float *) matrix->buffer;

  jx = MAX(MIN(cam->NxCCD, Xccd / cam->dX), 0);
  jy = MAX(MIN(cam->NyCCD, Yccd / cam->dY), 0);

  value = buffer[jx + jy*NxModel];

  *dX = isnan(value) ? 0.0 : value;

  // dY (dir == 1) uses the next block
  index += cam->Ngroup;

  matrix = cam->matrix[index];
  NxModel = cam->matrix[index]->Naxis[0];

  buffer = (float *) matrix->buffer;

  jx = MAX(MIN(cam->NxCCD, Xccd / cam->dX), 0);
  jy = MAX(MIN(cam->NyCCD, Yccd / cam->dY), 0);

  value = buffer[jx + jy*NxModel];

  *dY = isnan(value) ? 0.0 : value;

  return TRUE;
}
