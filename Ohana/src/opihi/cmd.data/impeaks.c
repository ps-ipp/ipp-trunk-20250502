# include "data.h"

float *findPeaksInRow (Buffer *buff, int y, float threshold, float *peaks, int *npeaks);

int impeaks (int argc, char **argv) {
  
  int iy, n;
  Vector *vecx, *vecy, *vecf;
  Buffer *buff;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: impeaks (buffer) <xvec> <yvec> <fvec> <threshold>\n");
    return (FALSE);
  }

  if ((buff = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vecx = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecf = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  float threshold = atof (argv[5]);

  // we find the peaks in the image by looking for peaks in each row, then checking their neighboring pixels

  float *image = (float *) buff[0].matrix.buffer;
  int Nx = buff[0].matrix.Naxis[0];
  int Ny = buff[0].matrix.Naxis[1];

  // we will use this vector for each row (allocated first pass only)
  float *rowPeaks = NULL;

  int Npeak = 0;
  int NPEAK = 1000;
  opihi_flt *peakX = NULL;
  opihi_flt *peakY = NULL;
  opihi_flt *peakF = NULL;
  ALLOCATE (peakX, opihi_flt, NPEAK);
  ALLOCATE (peakY, opihi_flt, NPEAK);
  ALLOCATE (peakF, opihi_flt, NPEAK);

  // exclude peaks on the outer edge
  for (iy = 1; iy < Ny; iy++) {
    
    int iyn = iy - 1;
    int iyp = iy + 1;

    int nRowPeaks;
    rowPeaks = findPeaksInRow (buff, iy, threshold, rowPeaks, &nRowPeaks);

    // each each of the row peaks to see if it is an image peak

    for (n = 0; n < nRowPeaks; n++) {
      int ix = rowPeaks[n];
      int ixn = ix - 1;
      int ixp = ix + 1;
      
      float value = image[ix + iy*Nx];

      // I know ix,iy is a peak in the row, is it above the neighbors in the previous and next rows?
      if (value <= image[ix  + iyn*Nx]) continue;
      if (value <  image[ix  + iyp*Nx]) continue;
      if (value <  image[ixn + iyn*Nx]) continue;
      if (value <= image[ixn + iyp*Nx]) continue;
      if (value <  image[ixp + iyn*Nx]) continue;
      if (value <= image[ixp + iyp*Nx]) continue;

      peakX[Npeak] = ix;
      peakY[Npeak] = iy;
      peakF[Npeak] = value;
      Npeak ++;
      if (Npeak == NPEAK) {
	NPEAK += 100;
	REALLOCATE (peakX, opihi_flt, NPEAK);
	REALLOCATE (peakY, opihi_flt, NPEAK);
	REALLOCATE (peakF, opihi_flt, NPEAK);
      }
    }

  }

  free (vecx->elements.Flt); vecx->elements.Flt = peakX; vecx->Nelements = Npeak;
  free (vecy->elements.Flt); vecy->elements.Flt = peakY; vecy->Nelements = Npeak;
  free (vecf->elements.Flt); vecf->elements.Flt = peakF; vecf->Nelements = Npeak;

  return (TRUE);
}

// skip the first and last pixel
float *findPeaksInRow (Buffer *buff, int y, float threshold, float *peaks, int *npeaks) {

  int ix;

  int Nx = buff[0].matrix.Naxis[0];

  float *row = (float *) buff[0].matrix.buffer + Nx*y;

  if (peaks == NULL) {
    ALLOCATE (peaks, float, Nx);
  }
  int Npeaks = 0;
  for (ix = 1; ix < Nx - 1; ix++) {
    if (!isfinite(row[ix])) continue; // ignore NAN values
    if (row[ix] <  threshold) continue;   // only accept pixels above threshold
    if (row[ix] <  row[ix - 1]) continue; // peak pixel must be at least preceeding pixel
    if (row[ix] <= row[ix + 1]) continue; // we accept the last pixel of a series of equal values
    
    peaks[Npeaks] = ix;
    Npeaks ++;
  }

  *npeaks = Npeaks;
  return peaks;
}
    
