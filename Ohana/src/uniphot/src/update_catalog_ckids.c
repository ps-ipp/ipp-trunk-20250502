# include "ckids.h"
void sort_fullIDs (uint64_t *I, int *S, off_t N);

void update_catalog_ckids (Catalog *catalog, FILE *foutput) {

  // I am looking for duplicate detections based on the image ID + detection ID values
  // I assume that all duplicate detections land in the same catalog, so I don't have to do a global search

  // 1) make an array (int64) of the full ID (imageID & detID) [I could do this in a 32bit value by finding the max values, but why bother?]
  // 2) sort the array
  // 3) identify unique values (count number of duplciates)

  // what do I want in the end?
  // a) unique[i] == 0, 1, count[j] : (i = 0, Nmeasure) [so I can tell which ones to drop / flag]

  off_t i;
  uint64_t *fullID;
  int *seq, *duplicates;

  if (catalog[0].Nmeasure == 0) return;

  Measure *measure = catalog[0].measure;
  Average *average = catalog[0].average;

  ALLOCATE (fullID, uint64_t, catalog[0].Nmeasure);
  ALLOCATE (seq,         int, catalog[0].Nmeasure);
  ALLOCATE (duplicates,  int, catalog[0].Nmeasure);

  memset (duplicates, 0, sizeof(int) * catalog[0].Nmeasure);

  // make an array of the full IDs
  for (i = 0; i < catalog[0].Nmeasure; i++) {
   
    // is an OR faster or a PLUS?
    fullID[i] = ((uint64_t) (measure[i].imageID) << 32) | (measure[i].detID);
    seq[i] = i;
  }

  // sort the array and sequence number
  sort_fullIDs (fullID, seq, catalog[0].Nmeasure);

  uint64_t current = fullID[0];	      // current unique value
  int count = 0;		      // number of duplicates for the current unique value
  duplicates[0] = count;	      // duplicate sequence

  for (i = 1; i < catalog[0].Nmeasure; i++) {
    if (fullID[i] == current) {
      count ++;
    } else {
      count = 0;
      current = fullID[i];
    }
    duplicates[i] = count;
  }
    
  for (i = 0; i < catalog[0].Nmeasure; i++) {
    if (!duplicates[i]) continue;
    off_t j = seq[i];
    off_t N = measure[j].averef;
    fprintf (foutput, "0x%08x 0x%08x %8.4f %8.4f %5d\n", measure[j].imageID, measure[j].detID, average[N].R, average[N].D, measure[j].photcode);
  }
  free (fullID);
  free (seq);
  free (duplicates);

  return;
}

// sort fullId vector and an index vector
void sort_fullIDs (uint64_t *I, int *S, off_t N) {

# define SWAPFUNC(A,B){ uint64_t tmp_i; int tmp_s;	\
  tmp_i = I[A]; I[A] = I[B]; I[B] = tmp_i; \
  tmp_s = S[A]; S[A] = S[B]; S[B] = tmp_s; \
}
# define COMPARE(A,B)(I[A] < I[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

