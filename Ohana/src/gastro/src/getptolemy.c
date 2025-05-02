# include "gastro.h"

SStars *getptolemy (CatStats *catstats, int *NSTARS) {
  
  int i, j, k, Ns, Nstars; 
  Catalog catalog;
  SStars *stars;
  SkyList *skylist;
  SkyTable *sky;
  SkyRegion patch;

  patch.Rmin = catstats[0].RA[0];
  patch.Rmax = catstats[0].RA[1];
  patch.Dmin = catstats[0].DEC[0];
  patch.Dmax = catstats[0].DEC[1];

  Nstars = 0;
  ALLOCATE (stars, SStars, 1);

  /* load regions from GSC table, restrict to patch */
  sky = SkyTableLoadOptimal (CATDIR, NULL, GSCFILE, FALSE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  skylist = SkyListByPatch (sky, -1, &patch);
  
  for (i = 0; i <skylist[0].Nregions; i++) {
    // set the parameters which guide catalog open/load/create

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = skylist[0].filename[i];
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE;
    catalog.Nsecfilt  = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    // Naverage_disk == 0 implies an empty catalog file
    // for only_match, skip empty catalogs
    if (!catalog.Naverage_disk) {
      dvo_catalog_free (&catalog);
      continue;
    }

    Ns = Nstars;
    Nstars += catalog.Naverage;

    REALLOCATE (stars, SStars, MAX (1, Nstars));
    for (k = Ns, j = 0; j < catalog.Naverage; k++, j++) {
      stars[k].X = catalog.average[j].R;
      stars[k].Y = catalog.average[j].D;
      stars[k].mag = catalog.measure[catalog.average[j].measureOffset].M;
    }      
    dvo_catalog_free (&catalog);
  }

  if (VERBOSE) fprintf (stderr, "%d stars from PTOLEMY\n", Nstars);
  *NSTARS = Nstars;
  return (stars);
}  
