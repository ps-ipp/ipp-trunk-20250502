# include "addstar.h"
# include "ukirt_uhs.h"

// assign stars in the given region to the subset (NOTE: stars are sorted by RA, start is
// first entry in stars array in this region)

UKIRT_Stars *loadukirt_uhs_make_subset (UKIRT_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset) {

  int i;

  // collect array of (Stars *) stars in a new output catalog
  int Nsubset = 0;
  int NSUBSET = 3000;

  ALLOCATE_PTR (subset, UKIRT_Stars, NSUBSET);
  // for (int i = 0; i < NSUBSET; i++) {
  //   ALLOCATE (subset[i].measure, Measure, UKIRT_NFILTER);
  // }

  // find the rest of the stars in this output region
  for (i = start; i < Nstars; i++) {
    if (stars[i].flag) continue;

    // check if in skyregion
    if (stars[i].average.R <  region[0].Rmin) continue;
    if (stars[i].average.R >= region[0].Rmax) break;
    if (stars[i].average.D <  region[0].Dmin) continue;
    if (stars[i].average.D >= region[0].Dmax) continue;
	  
    // check if in UserPatch (a GLOBAL)
    if (stars[i].average.R < UserPatch.Rmin) continue;
    if (stars[i].average.R > UserPatch.Rmax) break;
    if (stars[i].average.D < UserPatch.Dmin) continue;
    if (stars[i].average.D > UserPatch.Dmax) continue;
	  
    // the *measure value is a copy of the version allocated to stars[i]
    // do NOT free subset[i].measure
    subset[Nsubset] = stars[i];
    Nsubset ++;

    stars[i].flag = TRUE;
    
    CHECK_REALLOCATE (subset, UKIRT_Stars, NSUBSET, Nsubset, 10000);

    /*
    if (NSTARS >= Nstars) {
      NSTARS += 10000;
      REALLOCATE (stars, UKIRT_Stars, NSTARS);
      for (int j = Nstars; j < NSTARS; j++) {
	ALLOCATE (stars[j].measure, Measure, NFILTER);
      }
    }
    */
  }

  *nsubset = Nsubset;
  return subset;
}
