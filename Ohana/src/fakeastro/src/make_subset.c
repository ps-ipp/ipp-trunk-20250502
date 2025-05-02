# include "fakeastro.h"

// assign stars in the given region to the subset (NOTE: stars are sorted by RA, start is
// first entry in stars array in this region)

FakeAstro_Stars *make_subset (FakeAstro_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset) {

  int i;

  FakeAstro_Stars *subset = NULL;

  // collect array of (Stars *) stars in a new output catalog
  int Nsubset = 0;
  int NSUBSET = 3000;
  ALLOCATE (subset, FakeAstro_Stars, NSUBSET);

  // find the rest of the stars in this output region
  for (i = start; i < Nstars; i++) {
    if (stars[i].flag) continue;

    // check if in skyregion
    if (stars[i].R < region[0].Rmin) continue;
    if (stars[i].R > region[0].Rmax) break;
    if (stars[i].D < region[0].Dmin) continue;
    if (stars[i].D > region[0].Dmax) continue;
	  
    // check if in UserPatch (a GLOBAL)
    if (stars[i].R < UserPatch.Rmin) continue;
    if (stars[i].R > UserPatch.Rmax) break;
    if (stars[i].D < UserPatch.Dmin) continue;
    if (stars[i].D > UserPatch.Dmax) continue;
	  
    subset[Nsubset] = stars[i];
    Nsubset ++;

    stars[i].flag = TRUE;

    CHECK_REALLOCATE (subset, FakeAstro_Stars, NSUBSET, Nsubset, 10000);
  }

  *nsubset = Nsubset;
  return subset;
}
