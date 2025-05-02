# include "addstar.h"
# include "WISE.h"

/* WISE raw data tables are not fixed bytes per row.  split lines at the RETURN char.
 * handle fractional lines at the end of each read block
 */

/* read in chunks of ~64MB */
# define NBYTE 0x40000000
# define DEBUG 0

int loadwise_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options) {
  
  int i, j, k, verbose;
  int Ntstars, NTSTARS;
  int Nbyte, Nextra, offset;

  double Rmin, Rmax, Dmin, Dmax;

  FILE *f;
  char *buffer, *p, *q;

  WISE_Stars *tstars;
  SkyList *skylist;
  SkyRegion *region;
  Catalog catalog;

  getWISE_setup ();

  int NMeasPerStar = (MODE == MODE_CATWISE) ? 2 : 4;

  ALLOCATE (buffer, char, NBYTE);

  // scan through the entire WISE file
  f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read WISE data file: %s", filename);
  // test if this is a raw datafile or gzipped...

  int skipHeader = TRUE;

  Nextra = 0;  // number excess bytes from lsat partial row
  while ((Nbyte = fread (&buffer[Nextra], 1, NBYTE-Nextra, f)) != 0) {
    if (Nbyte == -1) Shutdown ("error reading from raw file %s", filename);
    if (DEBUG) fprintf (stderr, "read %d bytes .. n", Nbyte);

    Nbyte += Nextra;

    if (VERBOSE) fprintf (stderr, "read .. ");

    /* find bounds on first complete line */
    p = buffer;
    q = memchr (p, '\n', Nbyte);
    if (q == NULL) Shutdown ("incomplete line at end of file\n");
    offset = p - buffer; // offset within this scan

    Ntstars = 0;
    NTSTARS = 10000;
    ALLOCATE (tstars, WISE_Stars, NTSTARS);

    Rmin = 360.0;
    Rmax =   0.0;
    Dmin = +90.0;
    Dmax = -90.0;

    // XXX buffer is being reread instead of continuing.

    // scan through entire buffer for star coords
    while (1) {

      if (skipHeader && (MODE == MODE_CATWISE)) {
	if (*p == 0x5C) goto skip_header; // back-slash char
	if (*p == '|') goto skip_header;
	skipHeader = FALSE; // once we get past the header, do not skip any more
      }

      // note CATWISE uses a different mapping
      getWISE_coords (p, &tstars[Ntstars].R, &tstars[Ntstars].D, Nbyte - offset);

      tstars[Ntstars].offset = offset; // offset within scan
      tstars[Ntstars].flag = FALSE;

      if (VERBOSE) {
	Rmin = MIN (Rmin, tstars[Ntstars].R);
	Rmax = MAX (Rmax, tstars[Ntstars].R);
	Dmin = MIN (Dmin, tstars[Ntstars].D);
	Dmax = MAX (Dmax, tstars[Ntstars].D);
      }

      Ntstars ++;
      CHECK_REALLOCATE (tstars, WISE_Stars, NTSTARS, Ntstars, 10000);

    skip_header:

      /* start of the next line */
      p = q + 1; // q is always RETURN char on this line
      offset = p - buffer; // offset within this scan
      if (offset == Nbyte) {
	// last line in buffer is a complete line
	Nextra = 0;
	break;
      }
      /* end of the next line */
      q = memchr (p, '\n', Nbyte - offset);
      if (q == NULL) {
	// we are on the last, incomplete line in buffer
	Nextra = Nbyte - offset;
	break;
      } 
    }
    if (VERBOSE) fprintf (stderr, "scan %d stars (%10.6f - %10.6f, %10.6f - %10.6f) .. ", Ntstars, Rmin, Rmax, Dmin, Dmax);

    // sort the tstars by RA
    getWISE_sortStars (tstars, Ntstars);

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
	if (tstars[j].R < region[0].Rmin) continue;
	if (tstars[j].R > region[0].Rmax) break;
	if (tstars[j].D < region[0].Dmin) continue;
	if (tstars[j].D > region[0].Dmax) continue;
	  
	// check if in UserPatch
	if (tstars[j].R < UserPatch.Rmin) continue;
	if (tstars[j].R > UserPatch.Rmax) break;
	if (tstars[j].D < UserPatch.Dmin) continue;
	if (tstars[j].D > UserPatch.Dmax) continue;
	  
	// offset is bytes to start of line from start of buffer
	offset = tstars[j].offset;

	dvo_average_init (&newcat->average[Nave]);
	for (k = 0; k < NMeasPerStar; k++) {
	  dvo_measure_init (&newcat->measure[Nmeas+k]);
	}
	
	newcat->average[Nave].R = tstars[j].R;
	newcat->average[Nave].D = tstars[j].D;

	switch (MODE) {
	  case MODE_PRELIM:
	    loadwise_star_prelim (&newcat->measure[Nmeas], &buffer[offset], Nbyte - offset);
	    break;
	  case MODE_ALLSKY:
	    loadwise_star_allsky (&newcat->measure[Nmeas], &buffer[offset], Nbyte - offset);
	    break;
	  case MODE_ALLWISE:
	    loadwise_star_allwise (&newcat->measure[Nmeas], &buffer[offset], Nbyte - offset);
	    break;
	  case MODE_CATWISE:
	    loadwise_star_catwise (&newcat->average[Nave], &newcat->measure[Nmeas], &buffer[offset], Nbyte - offset, &tstars[j]);
	    break;
	  default:
	    break;
	}

	if (DEBUG && (Nave < 10)) {
	  fprintf (stderr, "pos: %12.6f %12.6f +/- %10.5f %10.5f : %12.6f %12.6f +/- %10.5f %10.5f\n",
		   newcat->average[Nave].R, newcat->average[Nave].D, 
		   newcat->average[Nave].dR, newcat->average[Nave].dD, 
		   newcat->average[Nave].uR, newcat->average[Nave].uD, 
		   newcat->average[Nave].duR, newcat->average[Nave].duD);
	}
	    

	tstars[j].flag = TRUE;

	newcat->average[Nave].Nmeasure = NMeasPerStar;
	newcat->average[Nave].measureOffset = Nmeas;

	Nave ++;
	Nmeas += NMeasPerStar;

	CHECK_REALLOCATE (newcat->average, Average, NAVE,  Nave,  1000);
	CHECK_REALLOCATE (newcat->measure, Measure, NMEAS, Nmeas, 4000);
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
  for each WISE file:
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

