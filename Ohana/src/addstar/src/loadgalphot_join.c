# include "addstar.h"
# include "loadgalphot.h"

// find the entries in 'fit' which match entries in 'gal'
// return an index from gal -> fit (for gal[i], fit[index[i]] is a match)
int *join_IDs (GalPhotIDset *gal, GalPhotIDset *fit) {
  
  int Nmatch = 0;
  int NMATCH = gal->N;
  int *idx_gal = NULL;
  ALLOCATE (idx_gal, int, gal->N);

  int *seq_fit, *seq_gal;
  ALLOCATE (seq_gal, int, gal->N);
  ALLOCATE (seq_fit, int, fit->N);
  { 
    int i;
    for (i = 0; i < gal->N; i++) { seq_gal[i] = i; idx_gal[i] = -1; }
    for (i = 0; i < fit->N; i++) { seq_fit[i] = i; }
  }

  // sort the sequences 
  sort_IDs_seqonly (gal->ID, seq_gal, gal->N);
  sort_IDs_seqonly (fit->ID, seq_fit, fit->N);

  int ifit, igal;
  for (ifit = igal = 0; (igal < gal->N) && (ifit < fit->N);) {
    int Igal = seq_gal[igal];
    int Ifit = seq_fit[ifit];

    if (gal->ID[Igal] < 0) { igal++; continue; }
    if (fit->ID[Ifit] < 0) { ifit++; continue; }

    int dID = gal->ID[Igal] - fit->ID[Ifit];

    if (dID < 0) { igal++; continue; }
    if (dID > 0) { ifit++; continue; }

    // look for all matches of list2() to list1(i)
    // this allows for multiple values of ID1 or ID2
    int ifit_first = ifit;
    int found = FALSE;
    for (ifit = ifit_first; (dID == 0) && (ifit < fit->N); ifit++) {
      Ifit = seq_fit[ifit];
      dID = gal->ID[Igal] - fit->ID[Ifit];
      if ((dID == 0) && (gal->type[Igal] == fit->type[Ifit])){
	idx_gal[Igal] = Ifit;
	found = TRUE;
	Nmatch ++;
	if (Nmatch >= NMATCH) {
	  NMATCH += 1000;
	  REALLOCATE (idx_gal, int, NMATCH);
	}
	break;
      }
    }
    myAssert (found, "gal entry not found?");
    ifit = ifit_first;
    igal ++;
  }
  free (seq_gal);
  free (seq_fit);

  // myAssert (Nmatch == gal->N, "did not find matches for all galaxies");
  
  for (igal = 0; igal < gal->N; igal++) {
    ifit = idx_gal[igal];
    if (ifit < 0) continue;
    myAssert (fit->ID[ifit]   == gal->ID[igal],   "ID mis-match");
    myAssert (fit->type[ifit] == gal->type[igal], "type mis-match");
  }

  return (idx_gal);
}
