# include "fakeastro.h"

int save_fake_stars (SkyTable *sky, Image *mosaic, Stars *stars, int Nstars) {

  Catalog catalog;

  // patch is generous region around image, but limited to this image
  SkyRegion *mosaicPatch = get_mosaic_patch (mosaic);

  // list of regions which cover this image
  SkyList *skylist = SkyListByPatch (sky, -1, mosaicPatch);

  int Naverage = 0;
  int Nmeasure = 0;

  INITTIME;

  int i;
  for (i = 0; i < skylist->Nregions; i++) {

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = skylist[0].filename[i];
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_MEASURE | DVO_LOAD_STARPAR; // use this for non-update mode
    // catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_STARPAR;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

    // I want to do an update here
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      exit (2);
    }

    match_fake_stars (stars, Nstars, skylist[0].regions[i], &catalog);

    /* report total updated values */
    Naverage += catalog.Naverage;
    Nmeasure += catalog.Nmeasure;

    SetProtect (TRUE);
    dvo_catalog_update (&catalog, VERBOSE);
    SetProtect (FALSE);

    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
    if (VERBOSE) MARKTIME ("save cpt: %f sec\n", dtime); RESETTIME; 
  }

  free (mosaicPatch);
  SkyListFree (skylist);
  return TRUE;
}

// XXX what about starpar values?
