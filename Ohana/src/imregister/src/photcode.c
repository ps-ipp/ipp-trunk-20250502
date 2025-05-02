# include "imregister.h"
static char *version = "photcode $Revision: 3.4 $";

int main (int argc, char **argv) {

  Header header;
  char detector[80], filter[80], *ID;
  int i, ccd, VERBOSE, N, Nfilter;

  get_version (argc, argv, version);
  ConfigInit (&argc, argv);
  ConfigCamera ();
  ConfigFilter ();

  VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-quiet"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = FALSE;
  }

  if (argc != 4) { 
    fprintf (stderr, "USAGE: photcode (file.fits) (ccd) (mode)\n");
    exit (1);
  }

  /* read in image header */
  if (!gfits_read_header (argv[1], &header)) {
    if (VERBOSE) fprintf (stderr, "ERROR: can't find image file %s (1)\n", argv[1]);
    exit (1);
  }

  gfits_scan (&header, CameraKeyword, "%s", 1, detector);
  for (i = 0; i < strlen(detector); i++) { detector[i] = toupper (detector[i]); }
  for (i = 0; i < strlen(detector); i++) { if (isspace (detector[i])) detector[i] = '.'; }

  gfits_scan (&header, FilterKeyword,   "%s", 1, filter);
  for (i = 0; i < strlen (filter); i++) { if (isspace (filter[i])) filter[i] = '.'; }
  Nfilter = FILTER_NONE;
  for (i = 0; (i < NFILTER) && (Nfilter == FILTER_NONE); i++) {
    if (!strcasecmp (filter, filtername[i])) {
      Nfilter = filternum[i];
    }
  }      
  if (Nfilter == FILTER_NONE) {
    fprintf (stderr, "ERROR: invalid filter %s\n", filter);
    exit (1);
  }
  strcpy (filter, filterhash[Nfilter]);

  if (!strcasecmp (argv[3], "mef")) {
    ID = strcreate (argv[2]);
  } else {
    ALLOCATE (ID, char, 80);
    gfits_scan (&header, CCDnumKeyword,  "%s", 1, ID);
  }
  ccd = -1;
  for (i = 0; (i < Nccd) && (ccd == -1); i++) {
    if (strnumcmp (ccds[i], ID)) {
      ccd = i;
    }
  }
  if (ccd == -1) {
    fprintf (stderr, "warning: ccd %d not found in camera config file\n", ccd);
    ccd = 0;
  }

  fprintf (stdout, "%s.%s.%02d\n", detector, filter, ccd);

  exit (0);

}
