# include "addstar.h"
# include "loadstarpar.h"

int loadstarpar_catalog (StarPar_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options) {

  Catalog catalog;

  // now we have all of the loaded stars in this catalog
  dvo_catalog_init (&catalog, TRUE);
  catalog.filename = filename;
  catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
  catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_STARPAR;
  catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    
  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, region, VERBOSE, "w")) {
    fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
    exit (2);
  }

  find_matches_starpar (region, stars, Nstars, &catalog, options);
    
  SetProtect (TRUE);
  if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
  if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
  SetProtect (FALSE);
  
  dvo_catalog_free (&catalog);

  return (TRUE);
}
