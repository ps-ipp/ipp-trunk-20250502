# include "addstar.h"
# include "gaia.h"
# define NSTARS_MAX 1000000

int loadgaia_table (int Nstart, int Nend, SkyList *skylistInput, HostTable *hosts, char **filename, AddstarClientOptions *options) {
  OHANA_UNUSED_PARAM(hosts);
  
  int i;

  int Nstars = 0;
  Gaia_Stars *stars = NULL;
  for (i = Nstart; (Nstars < NSTARS_MAX) && (i < Nend); i++) {
    // read the next file and append to the current set of stars
    fprintf (stderr, "loading %s\n", filename[i]);
    stars = loadgaia_readstars (filename[i], stars, &Nstars, options);
  }
  Nstart = i; // we pass back the entry for the next file to be read
  if (!stars) return Nstart;

  fprintf (stderr, "writing %d stars to dvo\n", Nstars);

  // sort the stars by RA
  loadgaia_sortStars (stars, Nstars);

  // scan through the stars, loading the containing catalogs
  // skip through table for unsaved stars
  for (i = 0; i < Nstars; i++) {
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
    Gaia_Stars *subset = loadgaia_make_subset (stars, Nstars, i, region, &Nsubset);

    // In parallel mode, write out the subset to a disk file.  Block until a remote host
    // is available.  In serial mode, just match against the appropriate region and save
    // NOTE: disable parallel mode for now: 
    // loadgaia_save_remote (subset, Nsubset, hosts, region, skylist[0].filename[0], options);
    loadgaia_catalog (subset, Nsubset, region, skylist[0].filename[0], options);
    free (subset);
    SkyListFree (skylist);
  }

  // wait for last remote clients to finish
  // NOTE: disable parallel mode for now: 
  // harvest_all ();

  free (stars);
  return Nstart;
}

