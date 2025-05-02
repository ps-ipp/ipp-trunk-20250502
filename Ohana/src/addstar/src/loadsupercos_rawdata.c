# include "addstar.h"
# include "supercos.h"

# define NBYTE 228
# define NRECORDS 1000000
# define DEBUG 0

int loadsupercos_rawdata (Image *image, int *imlist, int Nimage, SkyTable *skytable, char *filename, AddstarClientOptions options) {
  OHANA_UNUSED_PARAM(Nimage);
  
  int i;
  int Nrecords;
  
  FILE *f;
  char *buffer;

  Detection *sstars;
  Catalog catalog;

  // the data files are written as an integer number of 228 byte records
  ALLOCATE (buffer, char, NRECORDS*NBYTE);

  f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read the Supercosmos data file: %s", filename);

  ALLOCATE (sstars, Detection, NRECORDS); 

  double Rmin = 360.0;
  double Rmax =   0.0;
  double Dmin = +90.0;
  double Dmax = -90.0;

  Catalog *newcat = NULL;
  ALLOCATE (newcat, Catalog, 1);
  dvo_catalog_init (newcat, TRUE);
  ALLOCATE (newcat->average, Average, NRECORDS);
  ALLOCATE (newcat->measure, Measure, NRECORDS);

  while ((Nrecords = fread (buffer, NBYTE, NRECORDS, f)) != 0) {
    if (Nrecords == -1) Shutdown ("error reading from raw file %s", filename);
    if (DEBUG) fprintf (stderr, "read %d bytes (%d bytes x %d rows)", NBYTE * Nrecords, NBYTE, Nrecords);

    if (VERBOSE) fprintf (stderr, "read .. ");

    // extract the basic values from the buffer
    for (i = 0; i < Nrecords; i++) {
	sstars[i].objID    = *(int64_t *) &buffer[i*NBYTE + 0];
	sstars[i].surveyID = *(int8_t *)  &buffer[i*NBYTE + 8];
	sstars[i].plateID  = *(int32_t *) &buffer[i*NBYTE + 9];
	sstars[i].ra       = *(double *)  &buffer[i*NBYTE + 33];
	sstars[i].dec      = *(double *)  &buffer[i*NBYTE + 41];
	sstars[i].xCen     = *(double *)  &buffer[i*NBYTE + 129];
	sstars[i].yCen     = *(double *)  &buffer[i*NBYTE + 137];
	sstars[i].aU       = *(float *)   &buffer[i*NBYTE + 145];
	sstars[i].bU       = *(float *)   &buffer[i*NBYTE + 149];
	sstars[i].thetaU   = *(int16_t *) &buffer[i*NBYTE + 154];
	sstars[i].class    = *(int8_t *)  &buffer[i*NBYTE + 165];
	sstars[i].quality  = *(int32_t *) &buffer[i*NBYTE + 204];
	sstars[i].prfStat  = *(float *)   &buffer[i*NBYTE + 208];
	sstars[i].prfMag   = *(float *)   &buffer[i*NBYTE + 212];
	sstars[i].gMag     = *(float *)   &buffer[i*NBYTE + 216];
	sstars[i].sMag     = *(float *)   &buffer[i*NBYTE + 220];
    }

    // convert to the Stars format
    for (i = 0; i < Nrecords; i++) {

	dvo_average_init (&newcat->average[i]);
	dvo_measure_init (&newcat->measure[i]);

	newcat->measure[i].Xccd = sstars[i].xCen;
	newcat->measure[i].Yccd = sstars[i].yCen;

	newcat->measure[i].M = sstars[i].sMag;
	newcat->measure[i].Map = sstars[i].gMag;
	newcat->measure[i].photFlags = sstars[i].class;

	newcat->average[i].R = sstars[i].ra;
	newcat->average[i].D = sstars[i].dec;

	newcat->measure[i].R = sstars[i].ra;
	newcat->measure[i].D = sstars[i].dec;

	int Ni = imlist[sstars[i].plateID];
	if (Ni == -1) abort();

	// XXX fix these
	newcat->measure[i].McalPSF  = 0.0;
	newcat->measure[i].McalAPER = 0.0;
	newcat->measure[i].dt = image[Ni].exptime;

	double sidtime = image[Ni].sidtime;
	double latitude = image[Ni].latitude;
	
	double alt, az;
	altaz (&alt, &az, 15.0*sidtime - newcat->average[i].R, newcat->average[i].D, latitude);

	newcat->measure[i].airmass = 1.0 / cos(RAD_DEG*alt);
	newcat->measure[i].az = az;
	newcat->measure[i].t = image[Ni].tzero;
	newcat->measure[i].imageID = image[Ni].imageID;
	newcat->measure[i].photcode = image[Ni].photcode;

	newcat->measure[i].FWx = ToShortPixels(sstars[i].aU);
	newcat->measure[i].FWy = ToShortPixels(sstars[i].bU);
	newcat->measure[i].theta = ToShortDegrees(sstars[i].thetaU);

	Rmin = MIN (Rmin, newcat->average[i].R);
	Rmax = MAX (Rmax, newcat->average[i].R);
	Dmin = MIN (Dmin, newcat->average[i].D);
	Dmax = MAX (Dmax, newcat->average[i].D);
    }

    if (VERBOSE) fprintf (stderr, "scan %d stars (%10.6f - %10.6f, %10.6f - %10.6f) .. ", Nrecords, Rmin, Rmax, Dmin, Dmax);

    SkyList *skylist = SkyListForStars (skytable, -1, newcat);

    for (i = 0; i < skylist[0].Nregions; i++) {
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
    
    if (VERBOSE) fprintf (stderr, "done\n");
  }

  if (VERBOSE) fprintf (stderr, "\n");
  
  free (sstars);

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














