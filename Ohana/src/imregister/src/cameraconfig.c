# include "imregister.h"
static char *version = "cameraconfig $Revision: 1.3 $";

void usage ();

int main (int argc, char **argv) {

  int i, Nccd, Nx, Ny, mosaic_x, mosaic_y, N, use_biassec;
  int dx, dy;
  char *config, *file;
  char CameraConfig[256];
  char field[64], line[256], keyword[64];
  char ID[64], *IDsel;
  double x, y, Xo, Yo, theta;
  int Choice, GetID, SEQ, GetN, Nsel;
  int NCCD, AXES, CCDS, CCDN, XOFF, YOFF, XFLIP, YFLIP, XO, YO, THETA;
  int AXIS0, AXIS1, MOSAIC_X, MOSAIC_Y, DATASEC, BIASSEC, USE_BIASSEC;
  char datasec[64], biassec[64];

  bzero (ID, 64);
  get_version (argc, argv, version);

  /*** load configuration info ***/
  file = SelectConfigFile (&argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }
  if (!ScanConfig (config, "CAMERA_CONFIG", "%s", 0, CameraConfig)) {
    fprintf (stderr, "ERROR: can't find CAMERA_CONFIG in configuration file\n");
    exit (1);
  }
  free (config);
  free (file);

  /* load camera config file */
  config = LoadConfigFile (CameraConfig);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find camera config file %s\n", CameraConfig);
    exit (1);
  }

  if (argc == 1) { 
    fprintf (stderr, "USAGE: cameraconfig [-options]\n");
    fprintf (stderr, "using %s for config information\n", CameraConfig);
    exit (1);
  }

  /* command line options */
  if ((N = get_argument (argc, argv, "-h"))) usage ();

  /* command line options */
  NCCD = FALSE;
  if ((N = get_argument (argc, argv, "-Nccd"))) {
    remove_argument (N, &argc, argv);
    NCCD = TRUE;
  }

  AXES = FALSE;
  if ((N = get_argument (argc, argv, "-axes"))) {
    remove_argument (N, &argc, argv);
    AXES = TRUE;
  }

  AXIS0 = FALSE;
  if ((N = get_argument (argc, argv, "-axis0"))) {
    remove_argument (N, &argc, argv);
    AXIS0 = TRUE;
  }

  AXIS1 = FALSE;
  if ((N = get_argument (argc, argv, "-axis1"))) {
    remove_argument (N, &argc, argv);
    AXIS1 = TRUE;
  }

  MOSAIC_X = FALSE;
  if ((N = get_argument (argc, argv, "-mosaicx"))) {
    remove_argument (N, &argc, argv);
    MOSAIC_X = TRUE;
  }
  MOSAIC_Y = FALSE;
  if ((N = get_argument (argc, argv, "-mosaicy"))) {
    remove_argument (N, &argc, argv);
    MOSAIC_Y = TRUE;
  }

  CCDS = FALSE;
  if ((N = get_argument (argc, argv, "-ccds"))) {
    remove_argument (N, &argc, argv);
    CCDS = TRUE;
  }

  CCDN = FALSE;
  if ((N = get_argument (argc, argv, "-ccdn"))) {
    remove_argument (N, &argc, argv);
    CCDN = TRUE;
  }

  SEQ = FALSE;
  if ((N = get_argument (argc, argv, "-seq"))) {
    remove_argument (N, &argc, argv);
    SEQ = TRUE;
  }

  XOFF = FALSE;
  if ((N = get_argument (argc, argv, "-xoff"))) {
    remove_argument (N, &argc, argv);
    XOFF = TRUE;
  }

  YOFF = FALSE;
  if ((N = get_argument (argc, argv, "-yoff"))) {
    remove_argument (N, &argc, argv);
    YOFF = TRUE;
  }

  XO = FALSE;
  if ((N = get_argument (argc, argv, "-Xo"))) {
    remove_argument (N, &argc, argv);
    XO = TRUE;
  }

  YO = FALSE;
  if ((N = get_argument (argc, argv, "-Yo"))) {
    remove_argument (N, &argc, argv);
    YO = TRUE;
  }

  THETA = FALSE;
  if ((N = get_argument (argc, argv, "-theta"))) {
    remove_argument (N, &argc, argv);
    THETA = TRUE;
  }

  XFLIP = FALSE;
  if ((N = get_argument (argc, argv, "-xflip"))) {
    remove_argument (N, &argc, argv);
    XFLIP = TRUE;
  }

  YFLIP = FALSE;
  if ((N = get_argument (argc, argv, "-yflip"))) {
    remove_argument (N, &argc, argv);
    YFLIP = TRUE;
  }

  DATASEC = FALSE;
  if ((N = get_argument (argc, argv, "-datasec"))) {
    remove_argument (N, &argc, argv);
    DATASEC = TRUE;
  }
  BIASSEC = FALSE;
  if ((N = get_argument (argc, argv, "-biassec"))) {
    remove_argument (N, &argc, argv);
    BIASSEC = TRUE;
  }
  USE_BIASSEC = FALSE;
  if ((N = get_argument (argc, argv, "-usebiassec"))) {
    remove_argument (N, &argc, argv);
    USE_BIASSEC = TRUE;
  }

  GetID = FALSE;
  Nsel = 0;
  if ((N = get_argument (argc, argv, "-ID"))) {
    GetID = TRUE;
    remove_argument (N, &argc, argv);
    Nsel = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  GetN = FALSE;
  IDsel = NULL;
  if ((N = get_argument (argc, argv, "-N"))) {
    GetN = TRUE;
    remove_argument (N, &argc, argv);
    IDsel = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) { 
    fprintf (stderr, "USAGE: cameraconfig [-options]\n");
    exit (1);
  }

  /* load data from config file */
  ScanConfig (config, "NCCD", "%d", 1, &Nccd);
  ScanConfig (config, "NAXIS1", "%d", 1, &Nx);
  ScanConfig (config, "NAXIS2", "%d", 1, &Ny);
  ScanConfig (config, "MOSAIC_X", "%d", 1, &mosaic_x);
  ScanConfig (config, "MOSAIC_Y", "%d", 1, &mosaic_y);
  ScanConfig (config, "USE_BIASSEC", "%d", 1, &use_biassec);
  
  if (NCCD) fprintf (stdout, "%d\n", Nccd);
  if (AXES) fprintf (stdout, "%d %d\n", Nx, Ny);
  if (AXIS0) fprintf (stdout, "%d\n", Nx);
  if (AXIS1) fprintf (stdout, "%d\n", Ny);
  if (MOSAIC_X) fprintf (stdout, "%d\n", mosaic_x);
  if (MOSAIC_Y) fprintf (stdout, "%d\n", mosaic_y);
  if (USE_BIASSEC) fprintf (stdout, "%d\n", use_biassec);
  
  ScanConfig (config, "CHIPID_KEYWORD", "%s", 1, keyword);
  
  Choice = SEQ || CCDS || CCDN || XOFF || YOFF || XFLIP || YFLIP || DATASEC || BIASSEC || XO || YO || THETA;

  for (i = 0; i < Nccd; i++) {
    sprintf (field, "CCD.%d", i);
    ScanConfig (config, field, "%s", 1, line);
    sscanf (line, "%s %lf %lf %d %d %s %s %lf %lf %lf", 
	    ID, &x, &y, &dx, &dy, datasec, biassec, &Xo, &Yo, &theta);

    if (GetN  && strnumcmp (IDsel, ID)) fprintf (stdout, "%d\n", i);
    if (GetID && (Nsel == i)) fprintf (stdout, "%s\n", ID);
    if (SEQ)     fprintf (stdout, "%d ", i);
    if (CCDS)    fprintf (stdout, "%s ", ID);
    if (CCDN)    fprintf (stdout, "%02d", i);
    if (XOFF)    fprintf (stdout, "%f ", x);
    if (YOFF)    fprintf (stdout, "%f ", y);
    if (XFLIP)   fprintf (stdout, "%d ", dx);
    if (YFLIP)   fprintf (stdout, "%d ", dy);
    if (DATASEC) fprintf (stdout, "%s ", datasec);
    if (BIASSEC) fprintf (stdout, "%s ", biassec);

    if (XO)      fprintf (stdout, "%7.1f ", Xo);
    if (YO)      fprintf (stdout, "%7.1f ", Yo);
    if (THETA)   fprintf (stdout, "%7.3f ", theta);

    if (Choice) fprintf (stdout, "\n");

  }

  exit (0);
}

void usage () {

  fprintf (stderr, "cameraconfig [option] : lookup camera parameters\n");
  fprintf (stderr, "   -Nccd         : number of CCDs\n");
  fprintf (stderr, "   -axes         : x & y dimensions\n");
  fprintf (stderr, "   -axis0        : x dimension (CCD)\n");
  fprintf (stderr, "   -axis1        : y dimension (CCD)\n");
  fprintf (stderr, "   -mosaicx      : x dimension (mosaic)\n");
  fprintf (stderr, "   -mosaicy      : y dimension (mosaic)\n");
  fprintf (stderr, "   -usebiassec   : use header BIASSEC\n");

  fprintf (stderr, "   -seq          : chip sequence number\n");
  fprintf (stderr, "   -ccds         : chip extension ID\n");
  fprintf (stderr, "   -xoff         : x offset in mosaic\n");
  fprintf (stderr, "   -yoff         : y offset in mosaic\n");
  fprintf (stderr, "   -xflip        : x flip in mosaic\n");
  fprintf (stderr, "   -yflip        : y flip in mosaic\n");
  fprintf (stderr, "   -datasec      : DATASEC value\n");
  fprintf (stderr, "   -biassec      : BIASSEC value\n");

  fprintf (stderr, "   -ID [N]       : return ID for seq\n");
  fprintf (stderr, "   -N [ID]       : return seq for ID\n");
  exit (2);

}
