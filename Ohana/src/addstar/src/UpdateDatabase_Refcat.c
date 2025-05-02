# include "addstar.h"

int UpdateDatabase_Refcat (AddstarClientOptions *options, SkyRegion *patch, char *refcat) {

  int i, Nstars, Nsubset;
  Catalog catalog;
  Stars *stars, **subset;
  SkyList *skylist;

  if (options[0].mode != M_REFCAT) {
    fprintf (stderr, "error: expecting only REFCAT mode\n");
    return (FALSE);
  }

  if (!check_dir_access (CATDIR, VERBOSE)) exit (1);

  /*** update catalog: average, measure, etc ***/
  
  /* find correpsonding regions for image */
  skylist = SkyListByPatch (ServerSky, -1, patch);

  /* reduce regions to existing subset, if necessary */
  if (options[0].only_match || options[0].existing_regions) {
    SkyList *tmp;
    tmp = SkyListExistingSubset (skylist, CATDIR);
    SkyListFree (skylist);
    skylist = tmp;
  }
  if (VERBOSE) fprintf (stderr, "writing to %d regions\n", skylist[0].Nregions);

  for (i = 0; i < skylist[0].Nregions; i++) {

    // set the parameters which guide catalog open/load/create
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = skylist[0].filename[i];
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    if (options[0].update) catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
  
    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      exit (2);
    }

    // Naverage_disk == 0 implies an empty catalog file
    // for only_match, skip empty catalogs
    if ((catalog.Naverage_disk == 0) && options[0].only_match) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    // load stars from corresponding catalog 
    stars = grefcat (refcat, skylist[0].regions[i], options[0].photcode, &Nstars);

    subset = find_subset (skylist[0].regions[i], stars, Nstars, &Nsubset);
    find_matches_refstars (skylist[0].regions[i], subset, Nsubset, &catalog, options[0]);
    if (Nsubset) free (subset);

    if (!options[0].only_images) {
      SetProtect (TRUE);
      if (options[0].update) {
	catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
	dvo_catalog_update (&catalog, VERBOSE);
      } else {
	dvo_catalog_save (&catalog, VERBOSE);
      }
    }
    dvo_catalog_unlock (&catalog);
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
    free (stars);
  }
  return (TRUE);
}
