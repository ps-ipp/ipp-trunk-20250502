# include "addstar.h"
# include "tycho.h"

// the tycho data files do not have a fixed RA,DEC range but they have fixed width lines
// files are small (~26M each) so load entire file at once.

// sequence is:
// * load all stars from the tycho file
// * sort the stars by RA
// * loop over stars
//   * load catalogs 

// lines are fixed length (207 bytes)
# define DEBUG 1
# define NBYTE 207
# define NITEM 200000

int loadtycho_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options) {
  
  int i, j, k, Nitem;
  char *buffer;

  Tycho_Stars *tstars;
  SkyList *skylist;
  SkyRegion *region;
  Catalog catalog;

  gettycho_setup ();

  ALLOCATE (buffer, char, NBYTE*NITEM);

  // scan through the entire Tycho file
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read Tycho data file: %s", filename);

  // We are going to read the file in blocks of NBYTE*NITEM bytes.  (this may be set
  // larger than a full file).  The entire block of stars will then be ingested

  while ((Nitem = fread (buffer, NBYTE, NITEM, f)) != 0) {
    if (Nitem == -1) Shutdown ("error reading from raw file %s", filename);
    if (DEBUG) fprintf (stderr, " read %d items ", Nitem);

    if (VERBOSE) fprintf (stderr, "read .. ");

    // each row generates NGROUP measurements (B,V) x (MeanEpoch, J2000, J2012)
    int Ntstars = Nitem;
    ALLOCATE (tstars, Tycho_Stars, Ntstars);

    double Rmin = 360.0;
    double Rmax =   0.0;
    double Dmin = +90.0;
    double Dmax = -90.0;

    // convert buffer into tstars, store ra,dec range
    for (i = 0; i < Nitem; i++) {
      gettycho_star (&tstars[i], &buffer[NBYTE*i]);
      Rmin = MIN (Rmin, tstars[i].average.R);
      Rmax = MAX (Rmax, tstars[i].average.R);
      Dmin = MIN (Dmin, tstars[i].average.D);
      Dmax = MAX (Dmax, tstars[i].average.D);
    }

    if (VERBOSE) fprintf (stderr, "scan %d stars (%10.6f - %10.6f, %10.6f - %10.6f) .. \n", Nitem, Rmin, Rmax, Dmin, Dmax);

    // sort the array of stars
    gettycho_sortStars (tstars, Ntstars);

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

      // loop over stars in this WISE region that are also in this output region
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
	for (k = 0; k < NGROUP; k++) { 
	  dvo_measure_init (&newcat->measure[Nmeas + k]);
	  newcat->measure[Nmeas + k] = tstars[j].measure[k];
	}
	tstars[j].flag = TRUE;

	newcat->average[Nave].Nmeasure = NGROUP;
	newcat->average[Nave].measureOffset = Nmeas;

	Nave ++;
	Nmeas += NGROUP;

	CHECK_REALLOCATE (newcat->average, Average, NAVE,  Nave,  1000);
	CHECK_REALLOCATE (newcat->measure, Measure, NMEAS, Nmeas + NGROUP, 4000);
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
      catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT | DVO_LOAD_LENSING;
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
  }
  if (VERBOSE) fprintf (stderr, "\n");
  
  fclose (f);
  free (buffer);
  return (TRUE);
}
