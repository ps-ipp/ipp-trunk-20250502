# include "addstar.h"
# include "bsc.h"

// the bsc data file is small enough to be read simply using scan_line

// sequence is:
// * load all stars from the bsc file
// * sort the stars by RA
// * loop over stars
//   * load catalogs 

# define DEBUG 1

int loadbsc_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options) {
  
  int i, j, k;
  char line[128];

  BSC_Stars *tstars;
  SkyList *skylist;
  SkyRegion *region;
  Catalog catalog;

  getbsc_setup ();

  // scan through the entire Bsc file
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read BSC data file: %s", filename);

  // each row generates NGROUP measurements (B,V) x (MeanEpoch, J2000, J2012)
  int Ntstars = 0;
  int NTSTARS = 1000;
  ALLOCATE (tstars, BSC_Stars, NTSTARS);

  double Rmin = 360.0;
  double Rmax =   0.0;
  double Dmin = +90.0;
  double Dmax = -90.0;

  while ((scan_line_maxlen(f, line, 128)) != EOF) {
    stripwhite(line);
    if (! *line) continue;

    if (line[0] == '#') continue;

    getbsc_star (&tstars[Ntstars], line);
    Rmin = MIN (Rmin, tstars[Ntstars].average.R);
    Rmax = MAX (Rmax, tstars[Ntstars].average.R);
    Dmin = MIN (Dmin, tstars[Ntstars].average.D);
    Dmax = MAX (Dmax, tstars[Ntstars].average.D);

    Ntstars ++;
    CHECK_REALLOCATE (tstars, BSC_Stars, NTSTARS, Ntstars, 1000);
  }
  if (VERBOSE) fprintf (stderr, "scan %d stars (%10.6f - %10.6f, %10.6f - %10.6f) .. \n", Ntstars, Rmin, Rmax, Dmin, Dmax);

  // sort the array of stars
  getbsc_sortStars (tstars, Ntstars);
  
  // scan through the stars, find the next unused containing catalog
  // skip through table for unsaved stars
  for (i = 0; i < Ntstars; i++) {
    if (tstars[i].flag) continue;

    // scan forward until we reach the UserPatch
    if (tstars[i].average.R < UserPatch.Rmin) continue;
    if (tstars[i].average.R > UserPatch.Rmax) break;
    if (tstars[i].average.D < UserPatch.Dmin) continue;
    if (tstars[i].average.D > UserPatch.Dmax) continue;

    // identify the relevant catalog
    skylist = SkyRegionByPoint_List (skytable, -1, tstars[i].average.R, tstars[i].average.D);
    if (skylist[0].Nregions == 0) {
      SkyListFree (skylist);
      continue;
    }
    region = skylist[0].regions[0];
    if (DEBUG) fprintf (stderr, "writing to %s\n", skylist[0].filename[0]);

    int Nave = 0;
    int Nmeas = 0;
    int NAVE = 1000;
    int NMEAS = 4000;

    Catalog *newcat = NULL;
    ALLOCATE (newcat, Catalog, 1);
    dvo_catalog_init (newcat, TRUE);
    ALLOCATE (newcat->average, Average, NAVE);
    ALLOCATE (newcat->measure, Measure, NMEAS);

    // select stars in this region
    for (j = i; j < Ntstars; j++) {
      if (tstars[j].flag) continue;

      // check if in skyregion
      if (tstars[j].average.R < region[0].Rmin) continue;
      if (tstars[j].average.R > region[0].Rmax) break;
      if (tstars[j].average.D < region[0].Dmin) continue;
      if (tstars[j].average.D > region[0].Dmax) continue;
      
      // check if in UserPatch
      if (tstars[j].average.R < UserPatch.Rmin) continue;
      if (tstars[j].average.R > UserPatch.Rmax) break;
      if (tstars[j].average.D < UserPatch.Dmin) continue;
      if (tstars[j].average.D > UserPatch.Dmax) continue;
      
      dvo_average_init (&newcat->average[Nave]);
      newcat->average[Nave] = tstars[j].average;
      for (k = 0; k < tstars[j].Nmeasure; k++) { 
	dvo_measure_init (&newcat->measure[Nmeas + k]);
	newcat->measure[Nmeas + k] = tstars[j].measure[k];
      }
      tstars[j].flag = TRUE;
      
      newcat->average[Nave].Nmeasure = tstars[j].Nmeasure;
      newcat->average[Nave].measureOffset = Nmeas;
      
      Nave ++;
      Nmeas += tstars[j].Nmeasure;
      
      CHECK_REALLOCATE (newcat->average, Average, NAVE,  Nave,  1000);
      CHECK_REALLOCATE (newcat->measure, Measure, NMEAS, Nmeas + tstars[j].Nmeasure, 4000);
    }
    newcat->Naverage = Nave;
    newcat->Nmeasure = Nmeas;

    if (!newcat->Naverage) {
      dvo_catalog_free (newcat);
      free (newcat);
      continue;
    }

    // now we have all of the loaded stars in this catalog
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[0];
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    
    int verbose = FALSE;
    
    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, skylist[0].regions[0], verbose, "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      exit (2);
    }
    
    // assume no input star matches an existing star 
    // simply add to the existing table
    find_matches_closest_refstars(region, newcat, &catalog, options);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, verbose)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);

    SkyListFree (skylist);
    dvo_catalog_free (newcat);
    free (newcat);
  }
  free (tstars);
  if (VERBOSE) fprintf (stderr, "done\n");
  
  fclose (f);
  return (TRUE);
}
