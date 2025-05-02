# include "imregister.h"
# include "imreg.h"

void sortstr (char **S, off_t *X, off_t N) {

# define SWAPFUNC(A,B){ \
  char  *tmp = S[A]; S[A] = S[B]; S[B] = tmp; \
  off_t itmp = X[A]; X[A] = X[B]; X[B] = itmp; \
}
# define COMPARE(A,B)(strcmp(S[A], S[B]) < 0)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* input is a subset index of image list, output is a new subset */
off_t *unique_entries (RegImage *image, off_t Nimage, off_t *subset, off_t *Nmatch) {

  off_t i, j, k, m, Nsubset;
  off_t N, NMATCH;
  off_t *match, *entry;
  char idxline[128];
  char **index;

  if (!output.unique) return (subset);

  /* create output index */
  N = 0;
  NMATCH = 1000;
  ALLOCATE (match, off_t, NMATCH);

  Nsubset = *Nmatch;

  /* index = filename.ccd */
  ALLOCATE (index, char *, Nsubset);
  ALLOCATE (entry, off_t, Nsubset);
  for (i = 0; i < Nsubset; i++) {
    sprintf (idxline, "%s.%02d", image[subset[i]].filename, image[subset[i]].ccd);
    index[i] = strcreate (idxline);
    entry[i] = subset[i];
  }
  sortstr (index, entry, Nsubset);

  /* find unique sequences */
  for (i = 0; i < Nsubset; ) {
    for (j = i + 1; (j < Nsubset) && (!strcmp (index[i], index[j])); j++);

    /* find first entry with bias != 0, else entry i */
    m = i;
    for (k = i; k < j; k++) {
      if (image[entry[k]].bias != 0) {
	m = k;
	break;
      }
    }
    
    /* add unique entry to output list */
    match[N] = entry[m];
    N ++;
    if (N == NMATCH) {
      NMATCH += 1000;
      REALLOCATE (match, off_t, NMATCH);
    }

    /* j always points to the next entry */
    i = j;
  }

  for (i = 0; i < Nsubset; i++) free (index[i]);
  free (index);
  free (entry);
  free (subset);

  *Nmatch = N;
  return (match);
}
