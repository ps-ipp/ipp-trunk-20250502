# include "addstar.h"

// pass in options and skylist?
int resort_unthreaded (SkyList *skylist, int ForceSort) {

  off_t i;
  off_t Naverage, Nmeasure;
  Catalog catalog;

  /* match stars to existing catalog data (or otherwise manipulate catalog data) */
  Naverage = Nmeasure = 0;
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    // chose the catalog file (local or remote?)
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? hostfile : skylist[0].filename[i];

    // set the parameters which guide catalog open/load/create
    catalog.catformat   = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode     = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog.catcompress = dvo_catalog_catcompress (CATCOMPRESS); // set the default catcompress from config data
    catalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_LENSING | DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT;
    catalog.Nsecfilt    = GetPhotcodeNsecfilt ();

    // an error exit status here is a significant error (disk I/O or file access)
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      exit (2);
    }

    // Naverage_disk == 0 implies an empty catalog file, skip empty catalogs
    if (catalog.Naverage_disk == 0) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    if (!ForceSort && catalog.sorted) {
      if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
      dvo_catalog_free (&catalog);
      continue;
    }

    // to force the sort, we need to set sorted 'false'
    if (ForceSort) catalog.sorted = FALSE;
    resort_catalog (&catalog);

    // report total updated values 
    Naverage += catalog.Naverage;
    Nmeasure += catalog.Nmeasure;

    // write out catalog, if appropriate
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
  }
  return TRUE;
}


