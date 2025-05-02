# include "addstar.h"
# include "WISE.h"

/* unlike the DR2 data, the AS data is NOT fixed bytes/row 
 * we need to handle fractional lines at the end of each read block
 */

/* read in chunks of ~64MB */
# define NBYTE 0x4000000
# define DEBUG 0

int loadwise_prelim_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options) {
  
  int i, j, verbose;
  int Nstars, NSTARS, Ntstars, NTSTARS;
  int Nbyte, Nextra, Ntotal, offset;

  double Rmin, Rmax, Dmin, Dmax;

  FILE *f;
  char *buffer, *p, *q;

  Stars **stars; // this is an array of pointers to be consistent with input to find_match_refstars
  WISE_Stars *tstars;
  SkyList *skylist;
  SkyRegion *region;
  Catalog catalog;

  getWISE_setup ();

  ALLOCATE (buffer, char, NBYTE);

  // scan through the entire WISE file
  f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read WISE data file: %s", filename);
  // test if this is a raw datafile or gzipped...

  Nextra = 0;  // number excess bytes from lsat partial row
  Ntotal = 0;  // track the total number of bytes read 
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
    ALLOCATE (tstars, WISE_Stars, NTSTARS);

    Rmin = 360.0;
    Rmax =   0.0;
    Dmin = +90.0;
    Dmax = -90.0;

    // scan through entire buffer for star coords
    while (1) {
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

      // collect array of (Stars *) stars in a new output catalog
      Nstars = 0;
      NSTARS = 3000;
      ALLOCATE (stars, Stars *, NSTARS);

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
	  
	offset = tstars[j].offset;

	ALLOCATE (stars[Nstars+0], Stars, 1);
	ALLOCATE (stars[Nstars+1], Stars, 1);
	ALLOCATE (stars[Nstars+2], Stars, 1);
	ALLOCATE (stars[Nstars+3], Stars, 1);

	InitStar (stars[Nstars+0]);
	InitStar (stars[Nstars+1]);
	InitStar (stars[Nstars+2]);
	InitStar (stars[Nstars+3]);

	stars[Nstars+0][0].average.R = tstars[j].R;
	stars[Nstars+0][0].average.D = tstars[j].D;
	stars[Nstars+1][0].average.R = tstars[j].R;
	stars[Nstars+1][0].average.D = tstars[j].D;
	stars[Nstars+2][0].average.R = tstars[j].R;
	stars[Nstars+2][0].average.D = tstars[j].D;
	stars[Nstars+3][0].average.R = tstars[j].R;
	stars[Nstars+3][0].average.D = tstars[j].D;
	loadwise_star_full (&stars[Nstars], &buffer[offset], Nbyte - offset);

	tstars[j].flag = TRUE;

	Nstars += 4;
	if (Nstars >= NSTARS - 4) {
	  NSTARS += 4000;
	  REALLOCATE (stars, Stars *, NSTARS);
	}
      }

      if (!Nstars) {
	free (stars);
	continue;
      }

      if (DEBUG) fprintf (stderr, "selected %d stars (%10.6f - %10.6f, %10.6f - %10.6f)\n", Nstars, 
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
	// loadWISE_catalog (&catalog, stars, Nstars);
	find_matches_refstars (skylist[0].regions[0], stars, Nstars, &catalog, options);
	// loadWISE_catalog (&catalog, stars, Nstars);

	SetProtect (TRUE);
	if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
	if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
	SetProtect (FALSE);

	dvo_catalog_free (&catalog);
	// free (catalog.filename);
	// XXX don't free this! it points to an element of the skytable
      }

      SkyListFree (skylist);
      for (j = 0; j < Nstars; j++) free (stars[j]);
      free (stars);
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

