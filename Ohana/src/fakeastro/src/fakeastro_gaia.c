# include "fakeastro.h"

int fakeastro_gaia () {

  INITTIME;

  SkyTable *skyTable = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (skyTable, CATDIR, "cpt");

  SkyList *skylist  = SkyListByPatch (skyTable, -1, &UserPatch);

  Catalog catalog;

  // load stars from database in these regions
  int i;
  for (i = 0; i < skylist->Nregions; i++) {
    dvo_catalog_init (&catalog, TRUE);

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = skylist[0].filename[i];
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_MEASURE | DVO_LOAD_STARPAR;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!catalog.Naverage_disk) {
	if (VERBOSE2) { fprintf (stderr, "no data in %s, skipping\n", catalog.filename); }
	dvo_catalog_unlock (&catalog);
	dvo_catalog_free (&catalog);
	continue;
    }

    make_gaia_measures (&catalog);

    SetProtect (TRUE);
    if (!dvo_catalog_update (&catalog, VERBOSE)) {
      fprintf (stderr, "ERROR: failure to update %s\n", catalog.filename);
      exit (3);
    }
    SetProtect (FALSE);

    if (!dvo_catalog_unlock (&catalog)) {
      fprintf (stderr, "ERROR: failure to unlock %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_free (&catalog);
    if (VERBOSE) MARKTIME ("save cpt: %f sec\n", dtime); RESETTIME; 
  }

  exit (0);
}


