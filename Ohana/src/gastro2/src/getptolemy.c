# include "gastro2.h"
double catalog_area (Average *average, int Naverage, SkyRegion *region);

int getptolemy (CatStats *catstats, RefCatalog *Ref) {
  
  int i, j, k, Ns; 
  double FracArea;
  Catalog catalog;
  SkyList *skylist;
  SkyTable *sky;
  SkyRegion patch;

  Ref[0].N = 0;
  Ref[0].Area = 0;
  ALLOCATE (Ref[0].stars, StarData, 1);

  patch.Rmin = catstats[0].RA[0];
  patch.Rmax = catstats[0].RA[1];
  patch.Dmin = catstats[0].DEC[0];
  patch.Dmax = catstats[0].DEC[1];

  /* load regions from GSC table, restrict to patch */
  sky = SkyTableLoadOptimal (CATDIR, NULL, GSCFILE, FALSE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  skylist = SkyListByPatch (sky, -1, &patch);
  
  for (i = 0; i < skylist[0].Nregions; i++) {
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

    // this measurement adjusts a DVO database for partial coverage
    // XXX the correction is bogus if the sky density of the catalog 
    // is too low (<< 100 stars per field)
    FracArea = 1.0;
    if (PTOLEMY_FILL_FACTOR) {
      FracArea = catalog_area (catalog.average, catalog.Naverage, skylist[0].regions[i]);
    } 

    Ns = Ref[0].N;
    Ref[0].N += catalog.Naverage;
    Ref[0].Area += FracArea * area_of_skyregion (skylist[0].regions[i]);

    REALLOCATE (Ref[0].stars, StarData, MAX (1, Ref[0].N));
    for (k = Ns, j = 0; j < catalog.Naverage; k++, j++) {
      Ref[0].stars[k].R = catalog.average[j].R;
      Ref[0].stars[k].D = catalog.average[j].D;
      Ref[0].stars[k].M = catalog.measure[catalog.average[j].measureOffset].M;
    }      
    dvo_catalog_free (&catalog);
  }
  
  Ref[0].R0 = catstats[0].RA[0];
  Ref[0].R1 = catstats[0].RA[1];
  Ref[0].D0 = catstats[0].DEC[0];
  Ref[0].D1 = catstats[0].DEC[1];

  /* calculate luminosity function of stars */
  get_luminosity_func (Ref[0].stars, Ref[0].N, &Ref[0].lum);

  if (VERBOSE) fprintf (stderr, "%d stars from PTOLEMY\n", Ref[0].N);
  return (TRUE);
}  

double catalog_area (Average *average, int Naverage, SkyRegion *region) {

  int i, xb, yb, Nb;
  int bin[10][10];
  double frac, Rmin, Rmax, Dmin, Dmax, dR, dD;

  Rmin = region[0].Rmin;
  Rmax = region[0].Rmax;
  Dmin = region[0].Dmin;
  Dmax = region[0].Dmax;
  dR = Rmax - Rmin;
  dD = Dmax - Dmin;

  for (xb = 0; xb < 10; xb++) {
    for (yb = 0; yb < 10; yb++) {
      bin[xb][yb] = 0;
    }
  }

  for (i = 0; i < Naverage; i++) {
    xb = MAX (MIN (0, 10 * (average[i].R - Rmin) / dR), 9);
    yb = MAX (MIN (0, 10 * (average[i].D - Dmin) / dD), 9);
    bin[xb][yb] ++;
  }

  Nb = 0;
  for (xb = 0; xb < 10; xb++) {
    for (yb = 0; yb < 10; yb++) {
      if (bin[xb][yb]) Nb ++;
    }
  }

  frac = Nb / 100.0;

  return (frac);
}
