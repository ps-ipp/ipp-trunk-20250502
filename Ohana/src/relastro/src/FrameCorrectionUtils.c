# include "relastro.h"

// functions related to structure management

FrameCorrectionSet *FrameCorrectionSetInit () {

  FrameCorrectionSet *set = NULL;
  ALLOCATE (set, FrameCorrectionSet, 1);

  set->frame = NULL;
  set->coords = NULL;
  return set;
}

void FrameCorrectionSetFree (FrameCorrectionSet *set) {

  if (!set) return;

  FrameCorrectionFree(set->frame);
  if (set->coords) AstromOffsetMapFree (set->coords->offsetMap);
  free (set->coords);

  free (set);
  return;
}

FrameCorrectionType *FrameCorrectionInit (double scale) {

  int i;

  FrameCorrectionType *frame = NULL;
  ALLOCATE (frame, FrameCorrectionType, 1);
  
  frame->scale = scale; // degrees / pixel
  frame->Ndec = 180.0 / scale;
  if (frame->Ndec * scale < 180.0) frame->Ndec ++;

  // binD = (DEC + 89.0) / scale
  ALLOCATE (frame->Nra,  int, frame->Ndec);
  ALLOCATE (frame->dR,   double, frame->Ndec);
  ALLOCATE (frame->Roff, double *, frame->Ndec);
  ALLOCATE (frame->Doff, double *, frame->Ndec);

  // I am going to avoid corrections at |D| = 90 

  // It may be sleazy, but since I'm using SH, I'm going to just correct the region at |D|
  // > 80 with a 2D grid of corrections to X,Y projections

  for (i = 0; i < frame->Ndec; i++) {
    double D = i * scale - 89.0;
    if (D > 90.0) {
      frame->dR[i] = 0.0;
      frame->Nra[i] = 0;
      frame->Roff[i] = NULL;
      frame->Doff[i] = NULL;
      continue;
    }
    frame->dR[i] = scale / cos(RAD_DEG*D);
    frame->Nra[i] = MAX (0, 360.0 / frame->dR[i]);
    if (frame->Nra[i] * frame->dR[i] < 360.0) frame->Nra[i] ++;

    ALLOCATE (frame->Roff[i], double, frame->Nra[i]);
    ALLOCATE (frame->Doff[i], double, frame->Nra[i]);
    // ohana_memcheck (TRUE);
    // fprintf (stderr, "alloc : %d : %lx %lx\n", i, (long unsigned int)(size_t *) frame->Roff[i], (long unsigned int) (size_t *) frame->Doff[i]);
  }
  return frame;
}

void FrameCorrectionFree (FrameCorrectionType *frame) {

  if (!frame) return;

  int i;

  for (i = 0; i < frame->Ndec; i++) {
    if (frame->Roff[i]) free (frame->Roff[i]);
    if (frame->Doff[i]) free (frame->Doff[i]);
  }

  if (frame->dR)   free (frame->dR);
  if (frame->Nra)  free (frame->Nra);
  if (frame->Roff) free (frame->Roff);
  if (frame->Doff) free (frame->Doff);

  free (frame);
  
  return;
}

int FrameCorrectionFromSH (FrameCorrectionType *frame, SHterms *dR, SHterms *dD) {

  myAssert (dR->lmax == dD->lmax, "dR and dD must match\n");

  // allocate an SHterms structure to hold the Ylm values 
  SHterms *SH = SHtermsInit (dR->lmax);

  int i, j, k;
  // Now I need to apply the correction coefficients to SH images to generate an image
  for (i = 0; i < frame->Ndec; i++) {
    double D = i * frame->scale - 89.0;
    for (j = 0; j < frame->Nra[i]; j++) {
      double R = j * frame->dR[i];
      
      SHtermsForRD (SH, R, D);
      
      frame->Roff[i][j] = 0.0;
      frame->Doff[i][j] = 0.0;

      for (k = 0; k < SH->Nterms; k++) {
	frame->Roff[i][j] += dR->Fr[k]*SH->Fr[k] + dR->Fi[k]*SH->Fi[k];
	frame->Doff[i][j] += dD->Fr[k]*SH->Fr[k] + dD->Fi[k]*SH->Fi[k];
      }
    }      
  }
  SHtermsFree (SH);
  return TRUE;
}

// write out the maps as images
FrameCorrectionType *FrameCorrectionImageToSH (Header *header, Matrix *matrix, FrameCorrectionType *frame, int raDirection) {

  int i, j;

  if (!frame) {
    double pltscale;
    if (!gfits_scan (header, "PLTSCALE", "%lf", 1, &pltscale)) {
      fprintf (stderr, "failed to find PLTSCALE in header\n");
      exit (2);
    }
    frame = FrameCorrectionInit (pltscale);
  }

  Coords coords;
  GetCoords (&coords, header);

  int Nx = header->Naxis[0];
  // int Ny = header->Naxis[1];

  float   *buffer = (float *)matrix->buffer;
  double **value  = (raDirection) ? frame->Roff : frame->Doff;

  // Now I need to apply the correction coefficients to SH images to generate an image
  for (i = 0; i < frame->Ndec; i++) {
    double D = i * frame->scale - 89.0;
    for (j = 0; j < frame->Nra[i]; j++) {
      double R = j * frame->dR[i];
      
      value[i][j] = 0.0;

      double X, Y;
      if (!RD_to_XY (&X, &Y, R, D, &coords)) continue;

      int ix = X;
      int iy = Y;
      value[i][j] = buffer[ix + Nx*iy];
    }      
  }
  return frame;
}

int FrameCorrectionSHtoImage (Header *header, Matrix *matrix, FrameCorrectionType *frame, int raDirection) {

  Coords coords;
    
  gfits_init_header (header);
  header->bitpix = -32;
  header->Naxes = 2;
  header->Naxis[0] = 370.0 / frame->scale;
  header->Naxis[1] = 190.0 / frame->scale;

  int Nx = header->Naxis[0];
  int Ny = header->Naxis[1];

  gfits_create_header (header);
  gfits_create_matrix (header, matrix);

  gfits_modify (header, "PLTSCALE", "%lf", 1, frame->scale);

  InitCoords (&coords, "DEC--AIT");
  coords.cdelt1 = coords.cdelt2 = frame->scale;
  coords.crval1 = 0.0; // SAS center
  coords.crval2 = 0.0;
  coords.crpix1 = 0.5*Nx; // middle of projection is middle of map
  coords.crpix2 = 0.5*Ny; // middle of projection is middle of map
    
  PutCoords (&coords, header);

  float *buffer = (float *)matrix->buffer;

  int ix, iy;
  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      double R, D;
      int status = XY_to_RD (&R, &D, ix, iy, &coords);
      if (!status) continue;

      R = ohana_normalize_angle (R);

      int iD = (D + 89.0) / frame->scale;
      if (iD < 0) continue;
      if (iD >= frame->Ndec) continue;
      int iR = R / frame->dR[iD];
      if (iR < 0) continue;
      if (iR >= frame->Nra[iD]) continue;


      float value = NAN;
      if (raDirection) {
	value = frame->Roff[iD][iR];
      } else {
	value = frame->Doff[iD][iR];
      }
      buffer[ix + Nx*iy] = value;
    }
  }
  return TRUE;
}

// write out the AstromOffsetMap linked by the Coords structure
int FrameCorrectionMapToImage (Header *header, Matrix *matrix, Coords *coords, int raDirection) {
  // write out the maps as images

  AstromOffsetMap *map = coords->offsetMap;
  if (!map) {
    fprintf (stderr, "ERROR: missing AstromOffsetMap link in coords\n");
    return FALSE;
  }

  gfits_init_header (header);
  header->bitpix = -32;
  header->Naxes = 2;
  header->Naxis[0] = map->Nx;
  header->Naxis[1] = map->Ny;

  gfits_create_header (header);
  gfits_create_matrix (header, matrix);
  PutCoords (coords, header);

  float *buffer = (float *)matrix->buffer;

  float *value = (raDirection) ? map->dXv : map->dYv;

  int ix, iy;
  for (ix = 0; ix < map->Nx; ix++) {
    for (iy = 0; iy < map->Ny; iy++) {
      buffer[ix + map->Nx*iy] = value[ix + map->Nx*iy];
    }
  }
  return TRUE;
}

Coords *FrameCorrectionImageToMap (Header *header, Matrix *matrix, Coords *coords, int raDirection) {
  // write out the maps as images

  if (!coords) {
    ALLOCATE (coords, Coords, 1);
    GetCoords (coords, header);
    coords->offsetMap = AstromOffsetMapInit (header->Naxis[0], header->Naxis[1]);
    coords->offsetMap->dX = 1.0; // scale from projection (in arcsec) to correction patches
    coords->offsetMap->dY = 1.0;
  }
  AstromOffsetMap *map = coords->offsetMap;

  float *buffer = (float *) matrix->buffer;

  float *value = (raDirection) ? map->dXv : map->dYv;

  int ix, iy;
  for (ix = 0; ix < map->Nx; ix++) {
    for (iy = 0; iy < map->Ny; iy++) {
      value[ix + map->Nx*iy] = buffer[ix + map->Nx*iy];
    }
  }
  return coords;
}

