# include "addstar.h"
# include "2mass.h"

/* unlike the DR2 data, the AS data is NOT fixed bytes/row 
 * we need to handle fractional lines at the end of each read block
 */

/* read in chunks of ~64MB */
# define NBYTE 0x4000000
# define DEBUG 0

int load2mass_as_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options) {
  
  int i, j, k, verbose;
  int Ntstars, NTSTARS;
  int Nbyte, Nextra, offset;

  double Rmin, Rmax, Dmin, Dmax;

  FILE *f;
  char *buffer, *p, *q;

  TMStars *tstars;
  SkyList *skylist;
  SkyRegion *region;
  Catalog catalog;

  get2mass_setup (-1);

  ALLOCATE (buffer, char, NBYTE);

  // scan through the entire 2MASS file
  f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read 2mass data file: %s", filename);
  // test if this is a raw datafile or gzipped...

  Nextra = 0;  // number excess bytes from lsat partial row
  while ((Nbyte = fread (&buffer[Nextra], 1, NBYTE-Nextra, f)) != 0) {
    if (Nbyte == -1) Shutdown ("error reading from raw file %s", filename);
    if (DEBUG) fprintf (stderr, "read %d bytes", Nbyte);

    Nbyte += Nextra;

    if (VERBOSE) fprintf (stderr, "read .. ");

    /* find bounds on first complete line */
    p = buffer;
    q = memchr (p, '\n', Nbyte);
    if (q == NULL) Shutdown ("incomplete line at end of file\n");
    offset = p - buffer; // offset within this scan

    Ntstars = 0;
    NTSTARS = 10000;
    ALLOCATE (tstars, TMStars, NTSTARS);

    Rmin = 360.0;
    Rmax =   0.0;
    Dmin = +90.0;
    Dmax = -90.0;

    // scan through entire buffer for star coords
    while (1) {
      get2mass_coords (p, &tstars[Ntstars].R, &tstars[Ntstars].D, Nbyte - offset);
      tstars[Ntstars].offset = offset; // offset within scan
      tstars[Ntstars].flag = FALSE;

      if (VERBOSE) {
	Rmin = MIN (Rmin, tstars[Ntstars].R);
	Rmax = MAX (Rmax, tstars[Ntstars].R);
	Dmin = MIN (Dmin, tstars[Ntstars].D);
	Dmax = MAX (Dmax, tstars[Ntstars].D);
      }

      Ntstars ++;
      CHECK_REALLOCATE (tstars, TMStars, NTSTARS, Ntstars, 10000);

      /* start of the next line */
      p = q + 1;
      offset = p - buffer; // offset within this scan
      if (offset == Nbyte) {
	// last line in buffer is a complete line
	Nextra = 0;
	break;
      }
      /* end of the next line */
      q = memchr (p, '\n', Nbyte - offset);
      if (q == NULL) {
	// last, incomplete line in buffer
	Nextra = Nbyte - offset;
	break;
      } 
    }
    if (VERBOSE) fprintf (stderr, "scan %d stars (%10.6f - %10.6f, %10.6f - %10.6f) .. ", Ntstars, Rmin, Rmax, Dmin, Dmax);

    // sort the tstars by RA
    get2mass_sortStars (tstars, Ntstars);

    // scan through the stars, loading the containing catalogs
    // skip through table for unsaved stars
    for (i = 0; i < Ntstars; i++) {
      if (tstars[i].flag) continue;

      // scan forward until we read the UserPatch
      if (tstars[i].R < UserPatch.Rmin) continue;
      if (tstars[i].R > UserPatch.Rmax) break;
      if (tstars[i].D < UserPatch.Dmin) continue;
      if (tstars[i].D > UserPatch.Dmax) continue;

      // identify the relevant catalog
      skylist = SkyRegionByPoint_List (skytable, -1, tstars[i].R, tstars[i].D);
      if (skylist[0].Nregions == 0) {
	  SkyListFree (skylist);
	  continue;
      }
      region = skylist[0].regions[0];
      if (DEBUG) fprintf (stderr, "writing to %s\n", skylist[0].filename[0]);

      int Nave = 0;
      int Nmeas = 0;
      int NAVE = 1000;
      int NMEAS = 3000;

      Catalog *newcat = NULL;
      ALLOCATE (newcat, Catalog, 1);
      dvo_catalog_init (newcat, TRUE);
      ALLOCATE (newcat->average, Average, NAVE);
      ALLOCATE (newcat->measure, Measure, NMEAS);

      // loop over stars in this 2mass region that are also in this output region
      for (j = i; j < Ntstars; j++) {
	if (tstars[j].flag) continue;

	// check if in skyregion
	if (tstars[j].R < region[0].Rmin) continue;
	if (tstars[j].R > region[0].Rmax) break;
	if (tstars[j].D < region[0].Dmin) continue;
	if (tstars[j].D > region[0].Dmax) continue;
	  
	// check if in UserPatch
	if (tstars[j].R < UserPatch.Rmin) continue;
	if (tstars[j].R > UserPatch.Rmax) break;
	if (tstars[j].D < UserPatch.Dmin) continue;
	if (tstars[j].D > UserPatch.Dmax) continue;
	  
	offset = tstars[j].offset;

	dvo_average_init (&newcat->average[Nave]);
	for (k = 0; k < 4; k++) {
	  dvo_measure_init (&newcat->measure[Nmeas+k]);
	}

	// make sure we have enough space here for up to 5 more measurements:
	CHECK_REALLOCATE (newcat->measure, Measure, NMEAS, Nmeas + 5, 3000);

	int Nmeasure = 0; // we are adding 3 (JHK) or 5 (+ B_T, V_T) measurements

	newcat->average[Nave].R = tstars[j].R;
	newcat->average[Nave].D = tstars[j].D;
	get2mass_3star_full (&newcat->measure[Nmeas], &buffer[offset], &Nmeasure);

	tstars[j].flag = TRUE;

	newcat->average[Nave].Nmeasure = Nmeasure;
	newcat->average[Nave].measureOffset = Nmeas;

	Nave ++;
	Nmeas += Nmeasure;

	CHECK_REALLOCATE (newcat->average, Average, NAVE,  Nave,  1000);
      }
      newcat->Naverage = Nave;
      newcat->Nmeasure = Nmeas;

      if (!newcat->Naverage) {
	dvo_catalog_free (newcat);
	free (newcat);
	continue;
      }

      if (DEBUG) fprintf (stderr, "selected %d stars (%10.6f - %10.6f, %10.6f - %10.6f)\n", 
			  (int) newcat->Naverage, 
			  region[0].Rmin, region[0].Rmax, region[0].Dmin, region[0].Dmax);

      if (1) {
	verbose = VERBOSE;
	VERBOSE = FALSE;

	// now we have all of the loaded stars in this catalog
	dvo_catalog_init (&catalog, TRUE);
	catalog.filename = skylist[0].filename[0];
	catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
	catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
	catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
	catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

	// an error exit status here is a significant error
	if (!dvo_catalog_open (&catalog, skylist[0].regions[0], VERBOSE, "w")) {
	  fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
	  exit (2);
	}

	// assume no input star matches an existing star 
	// simply add to the existing table
	find_matches_refstars (skylist[0].regions[0], newcat, &catalog, options);

	SetProtect (TRUE);
	if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
	if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
	SetProtect (FALSE);
	dvo_catalog_free (&catalog);
      }

      SkyListFree (skylist);
      dvo_catalog_free (newcat);
      free (newcat);
      VERBOSE = verbose;
    }
    free (tstars);
    if (VERBOSE) fprintf (stderr, "done\n");

    // at end, p points at the start of last, partial line
    if (Nextra) memmove (buffer, p, Nextra);
  }

  if (VERBOSE) fprintf (stderr, "\n");
  
  fclose (f);
  free (buffer);
  return (TRUE);
}

/*
  for each 2mass file:
  for each data block
  generate a table of: R, D, byte, flag
  for each unsaved star
  find containing catalog
  load catalog
  find all contained stars
  add to catalog
  save catalog 
  mark all contained stars
*/

