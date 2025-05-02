# include "addstar.h"

SkyList *SkyListExistingSubset (SkyList *input, char *path) {
  OHANA_UNUSED_PARAM(path);
  
  int i, status, Nsubset, NSUBSET;
  SkyList *subset;
  struct stat filestats;
  
  Nsubset = 0;
  NSUBSET = 100;
  ALLOCATE (subset, SkyList, 1);
  ALLOCATE (subset[0].regions, SkyRegion *, NSUBSET);
  ALLOCATE (subset[0].filename, char *, NSUBSET);
  subset[0].ownElements = FALSE; // free these elements when freeing the list

  /* match the basename against the GSCRegion file names */
  for (i = 0; i < input[0].Nregions; i++) {
    status = stat (input[0].filename[i], &filestats);
    if ((status == -1) && (errno == ENOENT)) continue;
    /* give an error for other conditions? */

    subset[0].regions[Nsubset] = input[0].regions[i];
    subset[0].filename[Nsubset] = input[0].filename[i];
    Nsubset ++;
    if (Nsubset >= NSUBSET) {
	NSUBSET += 100;
	REALLOCATE (subset[0].regions, SkyRegion *, NSUBSET);
	REALLOCATE (subset[0].filename, char *, NSUBSET);
    }
    subset[0].Nregions = Nsubset;
  }
  return (subset);
}
