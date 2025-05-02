# include "addstar.h"
# define BYTES_STAR 23
# define BLOCK 1000
# define DNSTARS 1000

static SkyTable *sky = NULL;

static short int GSC_M;

Catalog *getgsc (SkyRegion *patch) {
  
  unsigned int i; 
  SkyList *skylist;

  NAMED_PHOTCODE (GSC_M, "GSC");

  /* load regions from GSC table, restrict to patch */
  if (!sky) {
    sky = SkyTableFromGSC (GSCFILE, SKY_DEPTH_HST, VERBOSE);
    SkyTableSetFilenames (sky, GSCDIR, "cpt");
  }
  skylist = SkyListByPatch (sky, -1, patch);
  
  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, 1);
  ALLOCATE (catalog->measure, Measure, 1);
  ALLOCATE (catalog->lensing, Lensing, 1);

  SkyRegion region;
  region.Rmin =   0.0;
  region.Rmax = 360.0;
  region.Dmin = -90.0;
  region.Dmax = +90.0;

  for (i = 0; i < skylist[0].Nregions; i++) {
    Catalog *newcat = rd_gsc (skylist[0].filename[i]);

    AddstarClientOptions matchOptions;
    matchOptions.radius = 0.4; // tight radius at this stage
    matchOptions.calibrate = FALSE;
    matchOptions.only_match = FALSE;
    matchOptions.nosort = FALSE;
    matchOptions.photcode = 0; // use an invalid photcode to avoid touching secfilt
    
    find_matches_closest (&region, newcat, catalog, matchOptions);
    
    if (newcat) {
      dvo_catalog_free (newcat);
      free (newcat);
    }
  }
  
  SkyListFree (skylist);

  if (VERBOSE) fprintf (stderr, "%d stars from HST GSC\n", (int) catalog->Naverage);
  return (catalog);
}  

Catalog *rd_gsc (char *filename) {
  
  int i, Nbytes, nbytes, Nline, Nbyte;
  char *buffer;
  FILE *f;

  int Nave = 0;
  int Nmeas = 0;
  int NAVE = 1000;
  int NMEAS = 1000;

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, NAVE);
  ALLOCATE (catalog->measure, Measure, NMEAS);

  f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't find catalog file %s\n", filename);
    exit (2);
  }
  
  Nbytes = BLOCK*BYTES_STAR;
  ALLOCATE (buffer, char, Nbytes);
  while ((nbytes = fread (buffer, 1, Nbytes, f)) > 0) {
    Nline = nbytes / BYTES_STAR;
    for (i = 0; i < Nline; i++) {

      dvo_average_init (&catalog->average[Nave]);
      dvo_measure_init (&catalog->measure[Nmeas]);

      Nbyte = i*BYTES_STAR;
      dparse (&catalog->average[Nave].R, 1, &buffer[Nbyte]);
      dparse (&catalog->average[Nave].D, 2, &buffer[Nbyte]);

      if (catalog->average[Nave].R < UserPatch.Rmin) continue;
      if (catalog->average[Nave].R > UserPatch.Rmax) continue;
      if (catalog->average[Nave].D < UserPatch.Dmin) continue;
      if (catalog->average[Nave].D > UserPatch.Dmax) continue;

      catalog->measure[Nmeas].R = catalog->average[Nave].R;
      catalog->measure[Nmeas].D = catalog->average[Nave].D;

      fparse (&catalog->measure[Nmeas].M, 3, &buffer[Nbyte]);
      catalog->measure[Nmeas].dM = NAN;
      catalog->measure[Nmeas].photcode = GSC_M;
      catalog->measure[Nmeas].t = 0;

      catalog->average[Nave].Nmeasure = 1;
      catalog->average[Nave].measureOffset = Nmeas;

      Nave ++;
      Nmeas ++;

      CHECK_REALLOCATE (catalog->average, Average, NAVE,  Nave,  1000);
      CHECK_REALLOCATE (catalog->measure, Measure, NMEAS, Nmeas, 1000);
    }
  }
  fclose (f);
  free (buffer);
  return (catalog);
}
