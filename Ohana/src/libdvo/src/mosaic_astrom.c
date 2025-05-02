# include <dvo.h>

//  the block of code below was used to define the chip->mosaic relationship before this
//  was assigned to the coords value

off_t getDISentry (e_time start, e_time stop, e_time *startMos, off_t *indexMos, off_t Nmosaic);
void SortDISindex (e_time *S, off_t *I, off_t N);

// Generate the links images[i].coords.mosaic & images[i].parent which point at the parent
// coords structure and the parent image structure, respectively.  These links are built
// based on the date/time of the exposure (image->tzero) and will not necessarily work if
// & when we use 2 cameras (e.g., gpc1 & gpc2).

// for 22M images, this function takes about 5 seconds.

// Note that the links (images[i].coords.mosaic, images[i].parent) will break if we
// reallocate the Image array in the middle of program
int BuildChipMatch (Image *images, off_t Nimages) {

  off_t i, j, NDIS;

  off_t  Ndis = 0;
  off_t  *DISentry = NULL;
  e_time *DIStzero = NULL;

  if (DISentry != NULL) free (DISentry);
  if (DIStzero != NULL) free (DIStzero);

  // allocate containers for DIS indexing
  Ndis = 0;
  NDIS = 100;
  ALLOCATE (DISentry, off_t, NDIS);
  ALLOCATE (DIStzero, e_time, NDIS);

  // find all DIS images, save tzero (& photcode?) 
  for (i = 0; i < Nimages; i++) {
    if (strcmp(&images[i].coords.ctype[4], "-DIS")) continue;
    DISentry[Ndis] = i;
    DIStzero[Ndis] = images[i].tzero;
    Ndis ++;
    if (Ndis >= NDIS) {
      NDIS += 100;
      REALLOCATE (DISentry, off_t, NDIS);
      REALLOCATE (DIStzero, e_time, NDIS);
    }
  }

  // sort the index, start, and stop by the start times:
  SortDISindex (DIStzero, DISentry, Ndis);

  int NfailMatch = 0;
  int NgoodMatch = 0;

  /* find all matched WRP images */
  for (i = 0; i < Nimages; i++) {
    images[i].parent        = NULL; // reset to NULL
    images[i].coords.mosaic = NULL; // reset to NULL
    if (strcmp(&images[i].coords.ctype[4], "-WRP")) continue; // only define link for WRP coords

    j = getDISentry (images[i].tzero, images[i].tzero + (int) images[i].exptime, DIStzero, DISentry, Ndis);
    if (j == -1) {
      char *mydate = ohana_sec_to_date (images[i].tzero);
      if (NfailMatch < 100) fprintf (stderr, "WARNING: can't find matching mosaic: ID %d, name: %s, Dateobs %s,  \n", images[i].imageID, images[i].name, mydate);
      free (mydate);
      NfailMatch ++;
      continue;
    }
    if (j >= Nimages) myAbort("invalid DIS entry");
    images[i].parent = &images[j];
    images[i].coords.mosaic = &images[j].coords;
    NgoodMatch ++;
  }

  fprintf (stderr, "matched chips for %d images, %d failed (%d total)\n", NgoodMatch, NfailMatch, (int) Nimages); 

  free (DISentry);
  free (DIStzero);

  return (TRUE);
}

// use bisection to find the overlapping mosaic
off_t getDISentry (e_time start, e_time stop, e_time *startMos, off_t *indexMos, off_t Nmosaic) {

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
    if (startMos[N] < start) continue;
    if (startMos[N] > stop) return (-1);
    return (indexMos[N]);
  }

  return (-1);
}

// sort two times vectors and an index by first time vector
void SortDISindex (e_time *S, off_t *I, off_t N) {

# define SWAPFUNC(A,B){ e_time tmp_t; off_t tmp_i; \
  tmp_t = S[A]; S[A] = S[B]; S[B] = tmp_t; \
  tmp_i = I[A]; I[A] = I[B]; I[B] = tmp_i; \
}
# define COMPARE(A,B)(S[A] < S[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

