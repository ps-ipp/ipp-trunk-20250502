# include "fakeastro.h"

Catalog *load_fake_stars (SkyList *skylist, int *ncatalog) {

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, skylist->Nregions);

  // load stars from database in these regions
  int i;
  for (i = 0; i < skylist->Nregions; i++) {
    dvo_catalog_init (&catalog[i], TRUE);

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&catalog[i], TRUE);
    catalog[i].filename  = skylist[0].filename[i];
    catalog[i].catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog[i].catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog[i].catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_STARPAR;
    catalog[i].Nsecfilt  = GetPhotcodeNsecfilt ();
    if (!dvo_catalog_open (&catalog[i], skylist[0].regions[i], VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog[i].filename);
      exit (1);
    }
    dvo_catalog_unlock (&catalog[i]);

    if (!catalog[i].Naverage_disk) {
	if (VERBOSE2) { fprintf (stderr, "no data in %s, skipping\n", catalog[i].filename); }
	dvo_catalog_free (&catalog[i]);
	continue;
    }
  }

  *ncatalog = skylist->Nregions;
  return catalog;
}
