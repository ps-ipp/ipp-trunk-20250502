# include "addstar.h"
# include "setobjflags.h"

int setobjflags_table (SkyList *skylistInput, HostTable *hosts) {
  
  int Nstars;
  MyStars *stars = setobjflags_loadfile (&Nstars);
  if (!stars) return FALSE;

  // sort the stars by RA
  setobjflags_sortStars (stars, Nstars);

  // scan through the stars, loading the containing catalogs
  // skip through table for unsaved stars
  for (int i = 0; i < Nstars; i++) {
    if (stars[i].flag) continue;

    // scan forward until we read the UserPatch
    if (stars[i].R < UserPatch.Rmin) continue;
    if (stars[i].R > UserPatch.Rmax) break;
    if (stars[i].D < UserPatch.Dmin) continue;
    if (stars[i].D > UserPatch.Dmax) continue;

    // identify the relevant catalog
    SkyList *skylist = SkyRegionByPoint_List (skylistInput, -1, stars[i].R, stars[i].D);
    if (skylist[0].Nregions == 0) {
      SkyListFree (skylist);
      continue;
    }
    SkyRegion *region = skylist[0].regions[0];

    // select stars matching this region
    int Nsubset;
    MyStars *subset = setobjflags_make_subset (stars, Nstars, i, region, &Nsubset);

    // In parallel mode, write out the subset to a disk file.  Block until a remote host
    // is available.  In serial mode, just match against the appropriate region and save
    setobjflags_save_remote (subset, Nsubset, hosts, region, skylist[0].filename[0]);

    free (subset);
    SkyListFree (skylist);
  }
  free (stars);

  // wait for last remote clients to finish
  harvest_all ();

  return TRUE;
}

