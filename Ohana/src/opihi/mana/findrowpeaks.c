# include "mana.h"

int *findrowpeaks (float *row, int Nrow, float threshold, int *npeaks) {
  
  int i;
  int Npeaks, NPEAKS;
  int *peaks;

  Npeaks = 0;
  NPEAKS = 100;
  ALLOCATE (peaks, int, NPEAKS);

  if (Nrow < 3) {
    *npeaks = Npeaks;
    return (peaks);
  }
    
  /* special case for first pixel in row */
  if ((row[0] > row[1]) && (row[0] > threshold)) {
    peaks[Npeaks] = 0;
    Npeaks ++;
  }    

  for (i = 1; i < Nrow - 1; i++) {
    if (!isfinite(row[i])) continue; // ignore NAN values
    if (row[i] < threshold) continue;
    if (row[i] < row[i-1]) continue;
    if (row[i] <= row[i+1]) continue;

    peaks[Npeaks] = i;
    Npeaks ++;
    if (Npeaks >= NPEAKS) {
      NPEAKS += 100;
      REALLOCATE (peaks, int, NPEAKS);
    }
  }      

  /* special case for last pixel in row */
  if ((row[Nrow-1] >= row[Nrow-2]) && (row[Nrow-1] > threshold)) {
    peaks[Npeaks] = Nrow-1;
    Npeaks ++;
  }    

  *npeaks = Npeaks;
  return (peaks);
}
