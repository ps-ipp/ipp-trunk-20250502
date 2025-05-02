# include "addstar.h"
# include "setobjflags.h"

int setobjflags_catalog (MyStars *stars, int Nstars, SkyRegion *region, char *filename) {

  Catalog catalog;

  // now we have all of the loaded stars in this catalog
  dvo_catalog_init (&catalog, TRUE);
  catalog.filename = filename;
  catalog.catflags = DVO_LOAD_AVERAGE;
  catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    
  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, region, VERBOSE, "w")) {
    fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
    exit (2);
  }

  find_matches_setobjflags (region, stars, Nstars, &catalog);
    
  SetProtect (TRUE);
  if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
  if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
  SetProtect (FALSE);
  
  dvo_catalog_free (&catalog);

  return (TRUE);
}
