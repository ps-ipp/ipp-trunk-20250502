# include "mkcmf.h"

# define ZERO_POINT 25.0
# define SKY 100.0
# define DSKY 2.0
# define PSFCHI 1.3
# define CRN 3.0
# define EXTN 4.0
# define FX 1.5
# define FY 1.0
# define DF 10.0
# define PSFQUAL 0.98
# define FLAGS 0x1101

void writeStars_PS1_V5_Lensing (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
void writeStars_PS1_V5 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
void writeStars_PS1_V4 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
void writeStars_PS1_V3 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
void writeStars_PS1_V2 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
void writeStars_PS1_V1 (FTable *ftable, double *X, double *Y, double *M, int Nstars);
void writeStars_PS1_SV3 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
void writeStars_PS1_DV5 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
void writeStars_PS1_DEV_1 (FTable *ftable, double *X, double *Y, double *M, int Nstars);
void writeStars_PS1_DEV_0 (FTable *ftable, double *X, double *Y, double *M, int Nstars);

int WriteXSRCtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars, float zeroPt, float exptime);
int WriteXFITtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
int WriteXRADtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars, int Nrad);
int WriteXGALtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);
int WriteDETFtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars);

int ADDNOISE = TRUE;
float BAD_PSFQF_FRAC = 0.0;

static Coords coords;
static float exptime = 1.0;
static float aper_offset = 0.0;
static float aper_scale = 1.0; // this is set to 10^-0.4*aper_offset below is aper_offset is non-zero
static float kron_scale = 1.0; // extra offset between psf and kron mags

static char reserved[] =  "Reserved space.  This line can be used to add a new FITS card.";

int main (int argc, char **argv) {

  // generate a simple, fake cmf file
  // load a text table with X,Y,Mag (instrumental?)

  int N, Nstars, NSTARS, found;
  unsigned int *Flag;
  double *X, *Y, *M, Xmax, Ymax;

  FILE *f, *fits;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  int APPEND = FALSE;
  if ((N = get_argument (argc, argv, "-append"))) {
    remove_argument (N, &argc, argv);
    APPEND = TRUE;
  }

  int isStack = FALSE;
  int isForcedWarp = FALSE;
  int isForcedGalaxy = FALSE;
  int isDiff  = FALSE;
  int WriteXSRC = FALSE;
  int WriteXFIT = FALSE;
  int WriteXRAD = FALSE;
  int WriteXGAL = FALSE;
  int WriteDETF = FALSE;
  if ((N = get_argument (argc, argv, "-stack"))) {
    remove_argument (N, &argc, argv);
    WriteXSRC = WriteXFIT = WriteXRAD = WriteDETF = TRUE;
    isStack = TRUE;
  }
  if ((N = get_argument (argc, argv, "-forcedwarp"))) {
    remove_argument (N, &argc, argv);
    WriteXSRC = WriteXFIT = WriteXRAD = TRUE;
    isForcedWarp = TRUE;
  }
  if ((N = get_argument (argc, argv, "-forcedgalaxy"))) {
    remove_argument (N, &argc, argv);
    WriteXSRC = WriteXFIT = WriteXRAD = TRUE;
    WriteXGAL = TRUE;
    isForcedGalaxy = TRUE;
  }
  if ((N = get_argument (argc, argv, "-diff"))) {
    remove_argument (N, &argc, argv);
    WriteXSRC = FALSE;
    WriteXFIT = TRUE;
    WriteXRAD = FALSE;
    WriteDETF = TRUE;
    isDiff = TRUE;
  }
  
  if (!isStack && !isForcedWarp && !isDiff) {
    WriteDETF = TRUE;
  }
  
  static char *photcode = "SIMTEST.r.Chip";
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    photcode = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  static double mjd;
  mjd = NAN;
  if ((N = get_argument (argc, argv, "-mjd"))) {
    remove_argument (N, &argc, argv);
    mjd = strtod (argv[N], NULL);
    remove_argument (N, &argc, argv);
  }

  static char *date = "2001-01-01";
  if ((N = get_argument (argc, argv, "-date"))) {
    remove_argument (N, &argc, argv);
    date = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  static char *tess_id = "RINGS.V3";
  if ((N = get_argument (argc, argv, "-tess_id"))) {
    remove_argument (N, &argc, argv);
    tess_id = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  static char *skycell = "skycell.1133.081";
  if ((N = get_argument (argc, argv, "-skycell"))) {
    remove_argument (N, &argc, argv);
    skycell = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  static char *time = "00:00:00";
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    time = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  char extname[80], exthead[80], extroot[80];
  strcpy (extroot, "Chip");
  strcpy (extname, "Chip.psf");
  strcpy (exthead, "Chip.hdr");
  if ((N = get_argument (argc, argv, "-extroot"))) {
    remove_argument (N, &argc, argv);
    snprintf (extroot, 80, "%s", argv[N]);
    snprintf (extname, 80, "%s.psf", argv[N]);
    snprintf (exthead, 80, "%s.hdr", argv[N]);
    remove_argument (N, &argc, argv);
  }

  double RA = 10.0;
  double DEC = 20.0;
  if ((N = get_argument (argc, argv, "-radec"))) {
    remove_argument (N, &argc, argv);
    RA = atof (argv[N]);
    remove_argument (N, &argc, argv);
    DEC = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  double CRPIX1 = 0.0;
  double CRPIX2 = 0.0;
  if ((N = get_argument (argc, argv, "-crpix"))) {
    remove_argument (N, &argc, argv);
    CRPIX1 = atof (argv[N]);
    remove_argument (N, &argc, argv);
    CRPIX2 = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-exptime"))) {
    remove_argument (N, &argc, argv);
    exptime = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }  

  // aperture-like and psf-like mags may have different effective zero points.  
  // by setting this, we can check that we can apply different zero points
  if ((N = get_argument (argc, argv, "-aper-offset"))) {
    remove_argument (N, &argc, argv);
    aper_offset = atof (argv[N]);
    aper_scale = pow(10.0, -0.4*aper_offset);
    remove_argument (N, &argc, argv);
  }  
  if ((N = get_argument (argc, argv, "-kron-scale"))) {
    remove_argument (N, &argc, argv);
    kron_scale = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }  

  // XXX note that the airmass and ra,dec,mjd can be inconsistent (for a given observatory location)
  float airmass = 1.0;
  if ((N = get_argument (argc, argv, "-airmass"))) {
    remove_argument (N, &argc, argv);
    airmass = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }  

  int imageID = -1;
  if ((N = get_argument (argc, argv, "-imageID"))) {
    remove_argument (N, &argc, argv);
    imageID = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }  
  int sourceID = -1;
  if ((N = get_argument (argc, argv, "-sourceID"))) {
    remove_argument (N, &argc, argv);
    sourceID = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }  

  int NX = 0;
  int NY = 0;
  if ((N = get_argument (argc, argv, "-size"))) {
    remove_argument (N, &argc, argv);
    NX = atof (argv[N]);
    remove_argument (N, &argc, argv);
    NY = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // add support for all cmf types
  static char *type = "PS1_V2";
  if ((N = get_argument (argc, argv, "-type"))) {
    remove_argument (N, &argc, argv);
    type = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // add support for all cmf types
  int FROM_COORDS = FALSE;
  if ((N = get_argument (argc, argv, "-coords"))) {
    remove_argument (N, &argc, argv);
    FROM_COORDS = TRUE;
  }

  // expect a field with the photom flags for each detection
  int READ_FLAGS = FALSE;
  if ((N = get_argument (argc, argv, "-flags"))) {
    remove_argument (N, &argc, argv);
    READ_FLAGS = TRUE;
  }

  // add support for all cmf types
  ADDNOISE = TRUE;
  if ((N = get_argument (argc, argv, "-no-noise"))) {
    remove_argument (N, &argc, argv);
    ADDNOISE = FALSE;
  }

  // random bad PSF_QF values
  if ((N = get_argument (argc, argv, "-bad-psfqf-frac"))) {
    remove_argument (N, &argc, argv);
    BAD_PSFQF_FRAC = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    fprintf (stderr, "USAGE mkcmf (input) (output) [-date date] [-time time] [-radec ra dec] [-cmftype type]\n");
    exit (2);
  }

  // init the random seed
  { 
    struct timeval now;
    gettimeofday (&now, NULL);
    int  B = getpid();
    long A = now.tv_sec - now.tv_usec * 1000000 + B*100000;
    srand48(A);
  }
    
  /* bore site center guess */
  InitCoords (&coords, "DEC--TAN");
  coords.crval1 = RA;
  coords.crval2 = DEC;
  coords.crpix1 = CRPIX1;
  coords.crpix2 = CRPIX2;
  
  if (isStack || isForcedWarp || isForcedGalaxy || isDiff) {
    coords.cdelt1 = 0.25/3600.0;
    coords.cdelt2 = 0.25/3600.0;
  } else {
    coords.cdelt1 = 0.257/3600.0;
    coords.cdelt2 = 0.257/3600.0;
  }

  // load stars and generate complete output fields
  f = fopen (argv[1], "r");
  if (f == NULL) {
    fprintf (stderr, "unable to open input file %s\n", argv[1]);
    exit (1);
  }
    
  ohana_gaussdev_init ();

  // load test stars from a file:
  Nstars = 0;
  NSTARS = 100;
  ALLOCATE (X, double, NSTARS);
  ALLOCATE (Y, double, NSTARS);
  ALLOCATE (M, double, NSTARS);
  ALLOCATE (Flag, unsigned int, NSTARS);

  Xmax = Ymax = 0;
  while (1) {
    int status;
    double ra, dec, mag, xobs, yobs, xraw, yraw, mraw;
    unsigned int flags;
    if (FROM_COORDS) {
      status = fscanf (f, "%lf %lf %lf %lf %lf %lf", &ra, &dec, &mag, &xraw, &yraw, &mraw);
      RD_to_XY (&xobs, &yobs, ra, dec, &coords);
    } else {
      status = fscanf (f, "%lf %lf %lf", &xobs, &yobs, &mraw);
    }
    if (status == EOF) break;
    if (READ_FLAGS) {
      status = fscanf (f, "%x", &flags);
      if (status == EOF) {
	  fprintf (stderr, "error: missing flag for last star?\n");
	  exit (1);
      }
      Flag[Nstars] = flags;
    } else {
      Flag[Nstars] = 0;
    }

    X[Nstars] = xobs;
    Y[Nstars] = yobs;
    M[Nstars] = mraw;

    Xmax  = MAX(Xmax, X[Nstars]);
    Ymax  = MAX(Ymax, Y[Nstars]);

    if (Nstars == NSTARS - 1) {
      NSTARS += 100;
      REALLOCATE (X, double, NSTARS);
      REALLOCATE (Y, double, NSTARS);
      REALLOCATE (M, double, NSTARS);
      REALLOCATE (Flag, unsigned int, NSTARS);
    }
    Nstars ++;
  }
  if (NX && NY) {
      Xmax = NX;
      Ymax = NY;
  }

  // create primary header
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);

  // XXX add minimum needed header fields 
  gfits_print (&header, "IMNAXIS1", "%d", 1, (int)(Xmax));
  gfits_print (&header, "IMNAXIS2", "%d", 1, (int)(Ymax));
  gfits_print (&header, "NAXIS1",   "%d", 1, (int)(Xmax));
  gfits_print (&header, "NAXIS2",   "%d", 1, (int)(Ymax));

  gfits_modify (&header, "NSTARS",   "%d", 1, Nstars);
  if (!isForcedGalaxy) {
    gfits_modify (&header, "PHOTCODE", "%s", 1, photcode);
  }
  if (isfinite(mjd)) {
    gfits_modify (&header, "MJD-OBS", "%lf", 1, mjd);
  } else {
    gfits_modify (&header, "DATE-OBS", "%s", 1, date);
    gfits_modify (&header, "UTC-OBS",  "%s", 1, time);
  }
  gfits_modify (&header, "ZERO_PT", "%lf", 1, ZERO_POINT); // this is now not used
  gfits_modify (&header, "EXPTIME", "%lf", 1, exptime);
  gfits_modify (&header, "AIRMASS", "%lf", 1, airmass);
  gfits_modify (&header, "NASTRO",   "%d", 1, 100); 

  if (imageID == -1) imageID = 100000.0*drand48();
  if (sourceID == -1) sourceID = 1000.0*drand48();
  gfits_modify (&header, "IMAGEID",  "%d", 1, imageID);
  gfits_modify (&header, "SOURCEID", "%d", 1, sourceID);

  PutCoords (&coords, &header);
  gfits_modify (&header, "EXTNAME",   "%s", 1, exthead);

  // various other fields needed to ipptopsps
  gfits_modify (&header, "FWHM_MAJ",          "%f", 1, FX*2.8);
  gfits_modify (&header, "FWHM_MIN",          "%f", 1, FY*2.8);
  gfits_modify (&header, "MSKY_MN",           "%f", 1, SKY);
  gfits_modify (&header, "MSKY_SIG",          "%f", 1, DSKY);
  gfits_modify (&header, "ZPT_ERR",           "%f", 1, 0.03);
  gfits_modify (&header, "ANGLE",             "%f", 1, 0.1);
  gfits_modify (&header, "IQ_FW1",            "%f", 1, FX*2.8);
  gfits_modify (&header, "IQ_FW2",            "%f", 1, FX*2.8);
  gfits_modify (&header, "IQ_M2C",            "%f", 1, FX*2.8);
  gfits_modify (&header, "IQ_M2S",            "%f", 1, FX*2.8);
  gfits_modify (&header, "IQ_M3",             "%f", 1, FX*2.8);
  gfits_modify (&header, "IQ_M4",             "%f", 1, FX*2.8);
  gfits_modify (&header, "APMIFIT",           "%f", 1, 0.1);
  gfits_modify (&header, "DAPMIFIT",          "%f", 1, 0.03);
  gfits_modify (&header, "DETECTOR",          "%s", 1, "CCID58-1-02a2");

  gfits_modify (&header, "HIERARCH DETEFF.MAGREF", "%f", 1, -5.0);

  gfits_modify (&header, "PSFMODEL",          "%s", 1, "PS_MODEL_PS1_V1");
  gfits_modify (&header, "AST_CDX",           "%f", 1, 0.05);
  gfits_modify (&header, "AST_CDY",           "%f", 1, 0.04);

  float zeroPt = 25.0 + 0.3*(drand48() - 0.5);

  if (!isStack && !isForcedWarp) {
    gfits_modify (&header, "HIERARCH DETREND.MASK",      "%s", 1, "detref615.XY33.fits");
    gfits_modify (&header, "HIERARCH DETREND.DARK",      "%s", 1, "GPC1.DARKTEST.norm.856.0.XY33.fits");
    gfits_modify (&header, "HIERARCH DETREND.FLAT",      "%s", 1, "GPC1.FLATTEST.300.XY33.co.fits");
    gfits_modify (&header, "HIERARCH DETREND.NOISEMAP",  "%s", 1, "GPC1.noisemap.norm.965.0.XY33.fits");
    gfits_modify (&header, "HIERARCH DETREND.NONLIN",    "%s", 1, "linearity_data.XY33.fits");
    gfits_modify (&header, "HIERARCH DETREND.VIDEODARK", "%s", 1, "GPC1.VIDEODARK.979.0.XY33.fits");
    gfits_modify (&header, "ZPT_OBS",                    "%f", 1, zeroPt);
  }

  if (isStack) {
    gfits_modify (&header, "HIERARCH FPA.ZP", "%f", 1, zeroPt);
    gfits_modify (&header, "TESS_ID",         "%s", 1, tess_id);
    gfits_modify (&header, "SKYCELL",         "%s", 1, skycell);
    gfits_modify (&header, "ZPT_OBS",         "%f", 1, zeroPt);
    gfits_modify (&header, "NINPUTS",         "%d", 1, 5);

    // if we have multiple stacks using the same input image IDs, we run into trouble
    int nFrame = 800 * drand48();

    int i;
    for (i = 0; i < 5; i++) {
      char field[64], expname[64];
      sprintf (field, "INP_%04d", i);
      sprintf (expname, "o5745g01%02do.%03d%03d.wrp.1199763.skycell.1315.090.fits", i, nFrame, i);
      gfits_modify (&header, field, "%s", 1, expname);

      sprintf (field, "SCL_%04d", i);
      gfits_modify (&header, field, "%f", 1, 12 + 0.2*i);

      sprintf (field, "ZPT_%04d", i);
      gfits_modify (&header, field, "%f", 1, 25 + 0.1*i);

      sprintf (field, "EXP_%04d", i);
      gfits_modify (&header, field, "%f", 1, 45 + 0.05*i);

      sprintf (field, "AIR_%04d", i);
      gfits_modify (&header, field, "%f", 1, 1 + 0.05*i);
    }
  }

  if (isForcedWarp) {
    gfits_modify (&header, "HIERARCH FPA.FILTERID", "%s", 1, "r.00000"); // does this affect anything? I don't actually think so...
    gfits_modify (&header, "HIERARCH FPA.ZP", "%f", 1, zeroPt);
    gfits_modify (&header, "TESS_ID",         "%s", 1, tess_id);
    gfits_modify (&header, "SKYCELL",         "%s", 1, skycell);
    gfits_modify (&header, "PHOT_V",                "%s", 1, "38100");
  }
  if (isForcedGalaxy) {
    gfits_modify (&header, "HIERARCH FPA.FILTERID", "%s", 1, ""); // does this affect anything? I don't actually think so...
    gfits_modify (&header, "HIERARCH FPA.ZP", "%f", 1, 25.0);
  }

  if (isDiff) {
    gfits_modify (&header, "HIERARCH FPA.ZP",  "%f", 1, zeroPt);
    gfits_modify (&header, "HIERARCH PPSUB.INPUT", "%s", 1, "o5076g0214o.93155.wrp.1221054.skycell.2289.079.fits");
    gfits_modify (&header, "HIERARCH PPSUB.REFERENCE", "%s", 1, "RINGS.V3.skycell.2289.079.stk.4063469.unconv.fits");
    gfits_modify (&header, "PHOT_V",              "%s", 1, "38100");

    // keywords related to convolution & diff image process:
    gfits_modify (&header, "HIERARCH PPSUB.KERNEL",  "%s", 1, "ISIS(20,(1.1,6)(2.3,4)(4.5,2),0,1.00e+00)");
    gfits_modify (&header, "HIERARCH SUBTRACTION.MODE",  "%d", 1, 2);
    gfits_modify (&header, "HIERARCH SUBTRACTION.STAMPS",  "%d", 1, 210);
    gfits_modify (&header, "HIERARCH SUBTRACTION.DEV.MEAN",  "%f", 1, 0.01);
    gfits_modify (&header, "HIERARCH SUBTRACTION.DEV.RMS",  "%f", 1, 0.02);
    gfits_modify (&header, "HIERARCH SUBTRACTION.NORM",  "%f", 1, 0.05);
    gfits_modify (&header, "HIERARCH SUBTRACTION.CONVOL.MAX",  "%f", 1, 1.5);
    gfits_modify (&header, "HIERARCH SUBTRACTION.DECONV.MAX",  "%f", 1, 0.1);

    // CZW There are other fields, but they don't appear to be used by ipp2psps.
  }

  int i;
  for (i = 1; i < 32; i++) {
    gfits_modify_alt (&header, "COMMENT", "%C", i, reserved);
  }

  ftable.header = &theader;

  // set up desired CMF type:
  found = FALSE;
  if (!strcmp(type, "PS1_DEV_0")) {
    writeStars_PS1_DEV_0 (&ftable, X, Y, M, Nstars);
    found = TRUE;
  }
  if (!strcmp(type, "PS1_DEV_1")) {
    writeStars_PS1_DEV_1 (&ftable, X, Y, M, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_V1")) {
    writeStars_PS1_V1 (&ftable, X, Y, M, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_V2")) {
    writeStars_PS1_V2 (&ftable, X, Y, M, Flag, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_V3")) {
    writeStars_PS1_V3 (&ftable, X, Y, M, Flag, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_V4")) {
    writeStars_PS1_V4 (&ftable, X, Y, M, Flag, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_V5")) {
    writeStars_PS1_V5 (&ftable, X, Y, M, Flag, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_V5_Lensing")) {
    writeStars_PS1_V5_Lensing (&ftable, X, Y, M, Flag, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_SV3")) {
    writeStars_PS1_SV3 (&ftable, X, Y, M, Flag, Nstars); 
    found = TRUE;
  }
  if (!strcmp(type, "PS1_DV5")) {
    writeStars_PS1_DV5 (&ftable, X, Y, M, Flag, Nstars);
    found = TRUE;
  }
  if (!found) {
    fprintf (stderr, "ERROR: unknown CMF type %s\n", type);
    exit (1);
  }

  gfits_modify (ftable.header, "EXTHEAD",   "%s", 1, exthead);
  gfits_modify (ftable.header, "EXTNAME",   "%s", 1, extname);

  // open for append (if you want a new file, need to blow the old object)
  if (APPEND) {
    fits = fopen (argv[2], "a+");
  } else {
    fits = fopen (argv[2], "w");
  }
  if (fits == NULL) {
    fprintf (stderr, "ERROR: can't open output file %s\n", argv[2]);
    exit (1);
  }

  /* if we are appending, fix up header */
  if (APPEND) {
    static char simple[] = "XTENSION= 'IMAGE  '            / Image extension";
    int Ns, No;
    Ns = strlen (simple);
    No = 80 - Ns;
    strncpy_nowarn (header.buffer, simple, Ns);
    memset (&header.buffer[Ns], ' ', No);
  }

  gfits_fwrite_header  (fits, &header);
  gfits_fwrite_matrix  (fits, &matrix);
  gfits_fwrite_Theader (fits, ftable.header);
  gfits_fwrite_table   (fits, &ftable);

  if (WriteXSRC) WriteXSRCtable (fits, extroot, X, Y, M, Flag, Nstars, zeroPt, exptime);
  if (WriteXFIT) WriteXFITtable (fits, extroot, X, Y, M, Flag, Nstars);

  if (WriteXRAD && isStack) WriteXRADtable (fits, extroot, X, Y, M, Flag, Nstars, 3);
  if (WriteXRAD && isForcedWarp) WriteXRADtable (fits, extroot, X, Y, M, Flag, Nstars, 1);
  if (WriteXGAL) WriteXGALtable (fits, extroot, X, Y, M, Flag, Nstars);

  if (WriteDETF) WriteDETFtable (fits, extroot, X, Y, M, Flag, Nstars);

  fclose (fits);

  exit (0);
}


void writeStars_PS1_DEV_0 (FTable *ftable, double *X, double *Y, double *M, int Nstars) {

  int i;
  PS1_DEV_0 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, PS1_DEV_0, Nstars);
  for (i = 0; i < Nstars; i++) {
    stars[i].detID = i;

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].M = M[i];

    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;
    stars[i].dM = fSN;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;
    stars[i].psfQF   = PSFQUAL;
    stars[i].nFrames   = 1;
  }

  gfits_table_set_PS1_DEV_0 (ftable, stars, Nstars, TRUE);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_DEV_0");
}

void writeStars_PS1_DEV_1 (FTable *ftable, double *X, double *Y, double *M, int Nstars) {

  int i;
  PS1_DEV_1 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, PS1_DEV_1, Nstars);
  for (i = 0; i < Nstars; i++) {
    stars[i].detID = i;

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].M = M[i];

    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;
    stars[i].dM = fSN;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;
    stars[i].psfQF   = PSFQUAL;
    stars[i].nFrames   = 1;
    stars[i].flags     = FLAGS;
  }

  gfits_table_set_PS1_DEV_1 (ftable, stars, Nstars, TRUE);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_DEV_1");
}

void writeStars_PS1_V1 (FTable *ftable, double *X, double *Y, double *M, int Nstars) {

  int i;
  CMF_PS1_V1 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, CMF_PS1_V1, Nstars);
  for (i = 0; i < Nstars; i++) {
    stars[i].detID = i;

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].M = M[i];

    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;
    stars[i].dM = fSN;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].psfNdof   = 1;
    stars[i].psfNpix   = 2;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;
    stars[i].psfQF   = PSFQUAL;
    stars[i].nFrames   = 1;
    stars[i].flags     = FLAGS;
  }

  gfits_table_set_CMF_PS1_V1 (ftable, stars, Nstars, TRUE);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_V1");
}

void writeStars_PS1_V2 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {

  int i;
  CMF_PS1_V2 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, CMF_PS1_V2, Nstars);
  for (i = 0; i < Nstars; i++) {
    stars[i].detID = i;

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].M = M[i];

    // randomly give poor PSFQF values
    if ((BAD_PSFQF_FRAC > 0.0) && (drand48() < BAD_PSFQF_FRAC)) {
      stars[i].psfQF   = 0.25;
    } else {
      stars[i].psfQF   = PSFQUAL;
    }
    
    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;
    stars[i].dM = fSN;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].psfNdof   = 1;
    stars[i].psfNpix   = 2;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;
    stars[i].nFrames   = 1;
    stars[i].flags     = Flag[i];
  }

  gfits_table_set_CMF_PS1_V2 (ftable, stars, Nstars, TRUE);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_V2");
}

void writeStars_PS1_V3 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {

  int i;
  CMF_PS1_V3 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, CMF_PS1_V3, Nstars);
  for (i = 0; i < Nstars; i++) {
    stars[i].detID = i;

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].M = M[i];
    stars[i].Map = M[i] + aper_offset - 0.05;

    // randomly give poor PSFQF values
    if ((BAD_PSFQF_FRAC > 0.0) && (drand48() < BAD_PSFQF_FRAC)) {
      stars[i].psfQF   = 0.25;
    } else {
      stars[i].psfQF   = PSFQUAL;
    }
    
    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;
    stars[i].dM = fSN;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].psfNdof   = 1;
    stars[i].psfNpix   = 2;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;
    stars[i].nFrames   = 1;
    stars[i].flags     = Flag[i];

    stars[i].kronFlux  = flux * kron_scale * aper_scale;
    stars[i].kronFluxErr = fSN * flux * kron_scale * aper_scale;
  }

  gfits_table_set_CMF_PS1_V3 (ftable, stars, Nstars, TRUE);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_V3");
}

void writeStars_PS1_V4 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {

  int i;
  CMF_PS1_V4 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, CMF_PS1_V4, Nstars);
  for (i = 0; i < Nstars; i++) {
    stars[i].detID = i;

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].M = M[i];
    stars[i].Map = M[i] + aper_offset - 0.05;
    stars[i].MapRaw = M[i] + aper_offset - 0.10;

    stars[i].Flux = flux;
    stars[i].dFlux = flux * fSN;

    stars[i].apFlux = pow(10.0, -0.4*stars[i].Map);
    stars[i].apFluxErr = stars[i].apFlux * fSN;

    // randomly give poor PSFQF values
    if ((BAD_PSFQF_FRAC > 0.0) && (drand48() < BAD_PSFQF_FRAC)) {
      stars[i].psfQF     = 0.25;
      stars[i].psfQFperf = 0.24;
    } else {
      stars[i].psfQF     = PSFQUAL;
      stars[i].psfQFperf = MAX(PSFQUAL - 0.01, 0.0);
    }
    
    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;
    stars[i].dM = fSN;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].psfNdof   = 1;
    stars[i].psfNpix   = 2;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;
    stars[i].nFrames   = 1;
    stars[i].flags     = Flag[i];

    stars[i].kronFlux  = flux * kron_scale * aper_scale;
    stars[i].kronFluxErr = fSN * flux * kron_scale * aper_scale;
  }

  gfits_table_set_CMF_PS1_V4 (ftable, stars, Nstars, TRUE);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_V4");
}

void writeStars_PS1_V5 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {

  int i;
  CMF_PS1_V5 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, CMF_PS1_V5, Nstars);
  for (i = 0; i < Nstars; i++) {

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].detID = i;
    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;

    stars[i].posangle = 10.0;
    stars[i].pltscale = coords.cdelt1*3600.;

    stars[i].M = M[i];
    stars[i].dM = fSN;

    stars[i].Flux = flux;
    stars[i].dFlux = flux * fSN;

    stars[i].Map = M[i] + aper_offset - 0.05;
    stars[i].MapRaw = M[i] + aper_offset - 0.10;

    stars[i].apRadius = 8.0;

    stars[i].apFlux = pow(10.0, -0.4*stars[i].Map);
    stars[i].apFluxErr = stars[i].apFlux * fSN;

    stars[i].Mcalib = M[i] + ZERO_POINT + 2.5*log10(exptime);
    stars[i].dMcal = 0.05;

    XY_to_RD (&stars[i].RA, &stars[i].DEC, X[i], Y[i], &coords);
    stars[i].apNpix = 3.14*8.0*8.0;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;

    stars[i].k         = 1.0;
    stars[i].fwhmMaj   = FX*2.8;
    stars[i].fwhmMin   = FY*2.8;

    // randomly give poor PSFQF values
    if ((BAD_PSFQF_FRAC > 0.0) && (drand48() < BAD_PSFQF_FRAC)) {
      stars[i].psfQF     = 0.25;
      stars[i].psfQFperf = 0.24;
    } else {
      stars[i].psfQF     = PSFQUAL;
      stars[i].psfQFperf = MAX(PSFQUAL - 0.01, 0.0);
    }

    stars[i].psfNdof   = 1;
    stars[i].psfNpix   = 2;

    stars[i].Mxx       = FX;
    stars[i].Mxy       = 0.01;
    stars[i].Myy       = FX;
    stars[i].M3c       = FX;
    stars[i].M3s       = FX;
    stars[i].M4c       = FX;
    stars[i].M4s       = FX;
    stars[i].Mr1       = FX;
    stars[i].Mrh       = FX;

    stars[i].kronFlux  = flux * kron_scale * aper_scale;
    stars[i].kronFluxErr = fSN * flux * kron_scale * aper_scale;

    stars[i].kronInner = fSN * flux * 0.9 * aper_scale;
    stars[i].kronOuter = fSN * flux * 1.5 * aper_scale;

    stars[i].skyLimitRad = 1;
    stars[i].skyLimitFlux = 2;
    stars[i].skyLimitSlope = 0.2;

    stars[i].flags     = Flag[i];
    stars[i].flags2    = 0x80;

    stars[i].nFrames   = 1;
  }

  gfits_table_set_CMF_PS1_V5 (ftable, stars, Nstars);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_V5");
}

void writeStars_PS1_SV3 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {

  int i;
  CMF_PS1_SV3 *stars;
  float flux, fSN;

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, CMF_PS1_SV3, Nstars);
  for (i = 0; i < Nstars; i++) {

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].detID = i;
    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;

    stars[i].posangle = 10.0;
    stars[i].pltscale = 0.25;

    stars[i].M = M[i];
    stars[i].dM = fSN;

    stars[i].Flux = flux;
    stars[i].dFlux = flux * fSN;

    stars[i].Map = M[i] + aper_offset - 0.05;
    stars[i].MapRaw = M[i] + aper_offset - 0.10;

    stars[i].apRadius = 8.0;

    stars[i].apFlux = pow(10.0, -0.4*stars[i].Map);
    stars[i].apFluxErr = stars[i].apFlux * fSN;

    stars[i].Mcalib = M[i] + ZERO_POINT + 2.5*log10(exptime);
    stars[i].dMcal = 0.05;

    XY_to_RD (&stars[i].RA, &stars[i].DEC, X[i], Y[i], &coords);
    stars[i].apNpix = 3.14*8.0*8.0;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;

    stars[i].k         = 1.0;
    stars[i].fwhmMaj   = FX*2.8;
    stars[i].fwhmMin   = FY*2.8;

    // randomly give poor PSFQF values
    if ((BAD_PSFQF_FRAC > 0.0) && (drand48() < BAD_PSFQF_FRAC)) {
      stars[i].psfQF     = 0.25;
      stars[i].psfQFperf = 0.24;
    } else {
      stars[i].psfQF     = PSFQUAL;
      stars[i].psfQFperf = MAX(PSFQUAL - 0.01, 0.0);
    }

    stars[i].psfNdof   = 1;
    stars[i].psfNpix   = 2;

    stars[i].Mxx       = FX;
    stars[i].Mxy       = 0.01;
    stars[i].Myy       = FX;
    stars[i].M3c       = FX;
    stars[i].M3s       = FX;
    stars[i].M4c       = FX;
    stars[i].M4s       = FX;
    stars[i].Mr1       = FX;
    stars[i].Mrh       = FX;

    stars[i].kronFlux  = flux * kron_scale * aper_scale;
    stars[i].kronFluxErr = fSN * flux * kron_scale * aper_scale;

    stars[i].kronInner = fSN * flux * 0.9 * aper_scale;
    stars[i].kronOuter = fSN * flux * 1.5 * aper_scale;

    // stars[i].skyLimitRad = 1;
    // stars[i].skyLimitFlux = 2;
    // stars[i].skyLimitSlope = 0.2;

    stars[i].flags     = Flag[i];
    stars[i].flags2    = 0x80;

    stars[i].nFrames   = 1;
  }

  gfits_table_set_CMF_PS1_SV3 (ftable, stars, Nstars);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_SV3");
}

void writeStars_PS1_DV5 (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {

  int i;
  CMF_PS1_DV5 *stars;
  float flux, fSN;

  ALLOCATE(stars, CMF_PS1_DV5, Nstars);
  for (i = 0; i < Nstars; i++) {
    flux = pow(10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN * ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow(10.0, -0.4 * M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].detID = i;
    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;

    stars[i].posangle = 10.0;
    stars[i].pltscale = 0.25;

    stars[i].M = M[i];
    stars[i].dM = fSN;

    stars[i].Flux = flux;
    stars[i].dFlux = flux * fSN;

    stars[i].Map = M[i] + aper_offset - 0.05;
    stars[i].MapRaw = M[i] + aper_offset - 0.10;

    stars[i].apRadius = 8.0;

    stars[i].apFlux = pow(10.0, -0.4 * stars[i].Map);
    stars[i].apFluxErr = stars[i].apFlux * fSN;

    stars[i].Mpeak = M[i] + 1.0;
    
    stars[i].Mcalib = M[i] + ZERO_POINT + 2.5 * log10(exptime);
    stars[i].dMcal  = 0.05;

    XY_to_RD(&stars[i].RA, &stars[i].DEC, X[i], Y[i], &coords);

    stars[i].sky = SKY;
    stars[i].dSky = DSKY;
    stars[i].psfChisq = PSFCHI;
    stars[i].crNsigma = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx = FX;
    stars[i].fy = FY;
    stars[i].df  = DF;
    
    if ((BAD_PSFQF_FRAC > 0.0) && (drand48() < BAD_PSFQF_FRAC)) {
      stars[i].psfQF = 0.25;
      stars[i].psfQFperf = 0.24;
    } else {
      stars[i].psfQF = PSFQUAL;
      stars[i].psfQFperf = MAX(PSFQUAL - 0.01, 0.0);
    }

    stars[i].psfNdof = 1;
    stars[i].psfNpix = 2;

    stars[i].Mxx = FX;
    stars[i].Mxy = 0.01;
    stars[i].Myy = FX;
    stars[i].Mr1 = FX;
    stars[i].Mrh = FX;

    stars[i].kronFlux = flux * kron_scale * aper_scale;
    stars[i].kronFluxErr = fSN * flux * kron_scale * aper_scale;

    stars[i].kronInner = fSN * flux * 0.9 * aper_scale;
    stars[i].kronOuter = fSN * flux * 1.5 * aper_scale;

    stars[i].chipNum = 1;
    stars[i].chipX   = (short) (floor( stars[i].X));
    stars[i].chipY   = (short) (floor( stars[i].Y));
    
    if ((BAD_PSFQF_FRAC > 0.0) && (drand48() < BAD_PSFQF_FRAC)) {
      stars[i].D_Npos = 1;
      stars[i].D_Fratio = 0.5;
      stars[i].D_Nratio_bad = 0.5;
      stars[i].D_Nratio_mask = 0.0;
      stars[i].D_Nratio_all = 0.5;
    }
    else {
      stars[i].D_Npos = 2;
      stars[i].D_Fratio = 1.0;
      stars[i].D_Nratio_bad = 1.0;
      stars[i].D_Nratio_mask = 0.0;
      stars[i].D_Nratio_all = 1.0;
    }      
    stars[i].D_Rp = 10.2;
    stars[i].D_SNp = 844.6;
    stars[i].D_Rm = 12.24;
    stars[i].D_SNm = 983.145;

    stars[i].flags = Flag[i];
    stars[i].flags2 = 0x80;

    stars[i].nFrames = 1;
  }

  gfits_table_set_CMF_PS1_DV5(ftable, stars, Nstars, TRUE);
  gfits_modify (ftable->header, "EXTTYPE", "%s", 1, "PS1_DV5");
}
    
    
void writeStars_PS1_V5_Lensing (FTable *ftable, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {

  int i;
  CMF_PS1_V5_R2_Lensing *stars;
  float flux, fSN;

  // We want some fraction of the stars to have zero psfqfperfect so that we can test the 
  // ForcedWarpMasked table construction, but we don't want things to be random so we
  // change every Nstars / modulo to zero
  int modulo = 0;
  if (BAD_PSFQF_FRAC > 0) {
    modulo = (int) (1.0 / BAD_PSFQF_FRAC);
  }

  // XXX add gaussian-distributed noise based on counts
  // this needs to make different output 'stars' entries depending on the desired type
  ALLOCATE (stars, CMF_PS1_V5_R2_Lensing, Nstars);
  for (i = 0; i < Nstars; i++) {

    flux = pow (10.0, -0.4*M[i]);
    fSN = 1.0 / sqrt(flux);

    if (ADDNOISE) {
      X[i] += FX * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      Y[i] += FY * fSN * ohana_gaussdev_rnd(0.0, 1.0);
      M[i] += fSN*ohana_gaussdev_rnd(0.0, 1.0);
      flux = pow (10.0, -0.4*M[i]);
      fSN = 1.0 / sqrt(flux);
    }

    stars[i].detID = i;
    stars[i].X = X[i];
    stars[i].Y = Y[i];
    stars[i].dX = FX * fSN;
    stars[i].dY = FY * fSN;

    stars[i].posangle = 10.0;
    stars[i].pltscale = 0.25;

    stars[i].M = M[i];
    stars[i].dM = fSN;

    stars[i].Flux = flux;
    stars[i].dFlux = flux * fSN;

    stars[i].Map = M[i] + aper_offset - 0.05;
    stars[i].MapRaw = M[i] + aper_offset - 0.10;

    stars[i].apRadius = 8.0;

    stars[i].apFlux = pow(10.0, -0.4*stars[i].Map);
    stars[i].apFluxErr = stars[i].apFlux * fSN;

    stars[i].Mcalib = M[i] + ZERO_POINT + 2.5*log10(exptime);
    stars[i].dMcal = 0.05;

    XY_to_RD (&stars[i].RA, &stars[i].DEC, X[i], Y[i], &coords);
    stars[i].apNpix = 3.14*8.0*8.0;

    stars[i].Mpeak     = M[i] + 1.0;
    stars[i].sky       = SKY;
    stars[i].dSky      = DSKY;
    stars[i].psfChisq  = PSFCHI;
    stars[i].crNsigma  = CRN;
    stars[i].extNsigma = EXTN;
    stars[i].fx        = FX;
    stars[i].fy        = FY;
    stars[i].df        = DF;

    stars[i].k         = 1.0;
    stars[i].fwhmMaj   = FX*2.8;
    stars[i].fwhmMin   = FY*2.8;

    if (modulo && ((i+1) % modulo == 0)) {
      stars[i].psfQF     = 0.0;
      stars[i].psfQFperf = 0.0;
    } else {
      stars[i].psfQF     = PSFQUAL;
      stars[i].psfQFperf = MAX(PSFQUAL - 0.01, 0.0);
    }

    stars[i].psfNdof   = 1;
    stars[i].psfNpix   = 2;

    stars[i].Mxx       = FX;
    stars[i].Mxy       = 0.01;
    stars[i].Myy       = FX;
    stars[i].M3c       = FX;
    stars[i].M3s       = FX;
    stars[i].M4c       = FX;
    stars[i].M4s       = FX;
    stars[i].Mr1       = FX;
    stars[i].Mrh       = FX;

    stars[i].X11_sm_obj = FX;
    stars[i].X12_sm_obj = FX;
    stars[i].X22_sm_obj = FY;
    stars[i].E1_sm_obj  = FX;
    stars[i].E2_sm_obj  = FX;

    stars[i].X11_sh_obj = FX;
    stars[i].X12_sh_obj = 0.2;
    stars[i].X22_sh_obj = FY;
    stars[i].E1_sh_obj  = FX*0.9;
    stars[i].E2_sh_obj  = FX*0.8;

    stars[i].X11_sm_psf = FX;
    stars[i].X12_sm_psf = FX;
    stars[i].X22_sm_psf = FX;
    stars[i].E1_sm_psf  = FX;
    stars[i].E2_sm_psf  = FX;

    stars[i].X11_sh_psf = FX;
    stars[i].X12_sh_psf = FX;
    stars[i].X22_sh_psf = FX;
    stars[i].E1_sh_psf  = FX;
    stars[i].E2_sh_psf  = FX;

    stars[i].srcChipNum = 23;
    stars[i].srcChipX   = X[i] + 10.0;
    stars[i].srcChipY   = Y[i] + 10.0;

    stars[i].kronFlux  = flux * kron_scale * aper_scale;
    stars[i].kronFluxErr = fSN * flux * kron_scale * aper_scale;

    stars[i].kronInner = fSN * flux * 0.9 * aper_scale;
    stars[i].kronOuter = fSN * flux * 1.5 * aper_scale;

    stars[i].skyLimitRad = 1;
    stars[i].skyLimitFlux = 2;
    stars[i].skyLimitSlope = 0.2;

    stars[i].flags     = Flag[i];
    stars[i].flags2    = 0x80;

    stars[i].nFrames   = 1;
  }

  gfits_table_set_CMF_PS1_V5_R2_Lensing (ftable, stars, Nstars);
  gfits_modify (ftable->header, "EXTTYPE",   "%s", 1, "PS1_V5");
}

int WriteXSRCtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars, float zeroPt, float exptime) {
  OHANA_UNUSED_PARAM(Flag);

  int i, j;
  Header header;
  FTable ftable;

  char extdata[80];
  snprintf (extdata, 80, "%s.xsrc", extroot);

  /* Example Code to create a bintable */
  {
    gfits_create_table_header (&header, "BINTABLE", extdata);

    gfits_define_bintable_column (&header, "1J", "IPP_IDET"           , "no comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "1E", "X_EXT"              , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "Y_EXT"              , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "X_EXT_SIG"          , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "Y_EXT_SIG"          , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "F25_ARATIO"         , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "F25_THETA"          , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_MAG"          , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_MAG_ERR"      , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_RADIUS"       , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_RADIUS_ERR"   , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_RADIUS_50"    , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_RADIUS_50_ERR", "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_RADIUS_90"    , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_RADIUS_90_ERR", "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "PETRO_FILL"         , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "HALF_LIGHT_RADIUS"  , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "G_RT"               , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "G_RA"               , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "G_S2"               , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "G_A"                , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "1E", "G_BUMPY"            , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "9E", "PROF_SB"            , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "9E", "PROF_FLUX"          , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "9E", "PROF_FILL"          , "no comment", NULL, 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&header, &ftable);

    // create intermediate storage arrays
    int    *IPP_IDET            ; ALLOCATE (IPP_IDET            ,  int   , Nstars);
    float  *X_EXT               ; ALLOCATE (X_EXT               ,  float , Nstars);
    float  *Y_EXT               ; ALLOCATE (Y_EXT               ,  float , Nstars);
    float  *X_EXT_SIG           ; ALLOCATE (X_EXT_SIG           ,  float , Nstars);
    float  *Y_EXT_SIG           ; ALLOCATE (Y_EXT_SIG           ,  float , Nstars);
    float  *F25_ARATIO          ; ALLOCATE (F25_ARATIO          ,  float , Nstars);
    float  *F25_THETA           ; ALLOCATE (F25_THETA           ,  float , Nstars);
    float  *PETRO_MAG           ; ALLOCATE (PETRO_MAG           ,  float , Nstars);
    float  *PETRO_MAG_ERR       ; ALLOCATE (PETRO_MAG_ERR       ,  float , Nstars);
    float  *PETRO_RADIUS        ; ALLOCATE (PETRO_RADIUS        ,  float , Nstars);
    float  *PETRO_RADIUS_ERR    ; ALLOCATE (PETRO_RADIUS_ERR    ,  float , Nstars);
    float  *PETRO_RADIUS_50     ; ALLOCATE (PETRO_RADIUS_50     ,  float , Nstars);
    float  *PETRO_RADIUS_50_ERR ; ALLOCATE (PETRO_RADIUS_50_ERR ,  float , Nstars);
    float  *PETRO_RADIUS_90     ; ALLOCATE (PETRO_RADIUS_90     ,  float , Nstars);
    float  *PETRO_RADIUS_90_ERR ; ALLOCATE (PETRO_RADIUS_90_ERR ,  float , Nstars);
    float  *PETRO_FILL          ; ALLOCATE (PETRO_FILL          ,  float , Nstars);
    float  *HALF_LIGHT_RADIUS   ; ALLOCATE (HALF_LIGHT_RADIUS   ,  float , Nstars);
    float  *G_RT                ; ALLOCATE (G_RT                ,  float , Nstars);
    float  *G_RA                ; ALLOCATE (G_RA                ,  float , Nstars);
    float  *G_S2                ; ALLOCATE (G_S2                ,  float , Nstars);
    float  *G_A                 ; ALLOCATE (G_A                 ,  float , Nstars);
    float  *G_BUMPY             ; ALLOCATE (G_BUMPY             ,  float , Nstars);
    float  *PROF_SB             ; ALLOCATE (PROF_SB             ,  float , 9*Nstars);
    float  *PROF_FLUX           ; ALLOCATE (PROF_FLUX           ,  float , 9*Nstars);
    float  *PROF_FILL           ; ALLOCATE (PROF_FILL           ,  float , 9*Nstars);

    float magtime = 2.5*log10(exptime);

    // assign the storage arrays
    for (i = 0; i < Nstars; i++) {
      float flux = pow (10.0, -0.4*M[i]);
      float fSN = 1.0 / sqrt(flux);

      IPP_IDET            [i] = i;
      X_EXT               [i] = X[i];
      Y_EXT               [i] = Y[i];
      X_EXT_SIG           [i] = FX * fSN;
      Y_EXT_SIG           [i] = FY * fSN;
      F25_ARATIO          [i] = 0.9;
      F25_THETA           [i] = 10.0; // seems to be in degrees in the cmfs
      PETRO_MAG           [i] = M[i] + zeroPt + magtime;
      PETRO_MAG_ERR       [i] = 1.0 / fSN; // note that the real cmfs have PETRO_MAG_ERR inverted
      PETRO_RADIUS        [i] = 8.0;
      if (i == 2) {
	// This case is here to exercise a case for ipp2psps.
	PETRO_RADIUS_ERR    [i] = NAN;
      }
      else {
	PETRO_RADIUS_ERR    [i] = 0.1;
      }
      PETRO_RADIUS_50     [i] = 4.0;
      PETRO_RADIUS_50_ERR [i] = 0.1;
      PETRO_RADIUS_90     [i] = 12.0;
      PETRO_RADIUS_90_ERR [i] = 0.1;
      PETRO_FILL          [i] = 0.95;
      HALF_LIGHT_RADIUS   [i] = 6.0;
      G_RT                [i] = 1.0;
      G_RA                [i] = 1.0;
      G_S2                [i] = 1.0;
      G_A                 [i] = 1.0;
      G_BUMPY             [i] = 1.0;
      for (j = 0; j < 9; j++) {
	PROF_SB             [j + 9*i] = flux / 10.0;
	PROF_FLUX           [j + 9*i] = flux;
	PROF_FILL           [j + 9*i] = 0.9;
      }
    }

    // set the table data
    gfits_set_bintable_column (&header, &ftable, "IPP_IDET"           , IPP_IDET            , Nstars);
    gfits_set_bintable_column (&header, &ftable, "X_EXT"              , X_EXT               , Nstars);
    gfits_set_bintable_column (&header, &ftable, "Y_EXT"              , Y_EXT               , Nstars);
    gfits_set_bintable_column (&header, &ftable, "X_EXT_SIG"          , X_EXT_SIG           , Nstars);
    gfits_set_bintable_column (&header, &ftable, "Y_EXT_SIG"          , Y_EXT_SIG           , Nstars);
    gfits_set_bintable_column (&header, &ftable, "F25_ARATIO"         , F25_ARATIO          , Nstars);
    gfits_set_bintable_column (&header, &ftable, "F25_THETA"          , F25_THETA           , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_MAG"          , PETRO_MAG           , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_MAG_ERR"      , PETRO_MAG_ERR       , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_RADIUS"       , PETRO_RADIUS        , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_RADIUS_ERR"   , PETRO_RADIUS_ERR    , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_RADIUS_50"    , PETRO_RADIUS_50     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_RADIUS_50_ERR", PETRO_RADIUS_50_ERR , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_RADIUS_90"    , PETRO_RADIUS_90     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_RADIUS_90_ERR", PETRO_RADIUS_90_ERR , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PETRO_FILL"         , PETRO_FILL          , Nstars);
    gfits_set_bintable_column (&header, &ftable, "HALF_LIGHT_RADIUS"  , HALF_LIGHT_RADIUS   , Nstars);
    gfits_set_bintable_column (&header, &ftable, "G_RT"               , G_RT                , Nstars);
    gfits_set_bintable_column (&header, &ftable, "G_RA"               , G_RA                , Nstars);
    gfits_set_bintable_column (&header, &ftable, "G_S2"               , G_S2                , Nstars);
    gfits_set_bintable_column (&header, &ftable, "G_A"                , G_A                 , Nstars);
    gfits_set_bintable_column (&header, &ftable, "G_BUMPY"            , G_BUMPY             , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PROF_SB"            , PROF_SB             , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PROF_FLUX"          , PROF_FLUX           , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PROF_FILL"          , PROF_FILL           , Nstars);
  }

  gfits_fwrite_Theader (fits, &header);
  gfits_fwrite_table  (fits, &ftable);
  gfits_free_header (&header);
  gfits_free_table (&ftable);

  return TRUE;
}

# define NMODEL 9
static char ModelNames[NMODEL][16] = {
  "PS_MODEL_GAUSS", 
  "PS_MODEL_PGAUSS", 
  "PS_MODEL_QGAUSS", 
  "PS_MODEL_PS1V1", 
  "PS_MODEL_RGAUSS", 
  "PS_MODEL_SERSIC", 
  "PS_MODEL_EXP", 
  "PS_MODEL_DEV", 
  "PS_MODEL_TRAIL"};

int WriteXFITtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {
  OHANA_UNUSED_PARAM(Flag);

  int i;
  Header header;
  FTable ftable;

  char extdata[80];
  snprintf (extdata, 80, "%s.xfit", extroot);

  /* Example Code to create a bintable */
  {
    gfits_create_table_header (&header, "BINTABLE", extdata);

    // define the table layout
    gfits_define_bintable_column (&header, "J",   "IPP_IDET"         , "no comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "E",   "X_EXT"            , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "Y_EXT"            , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "X_EXT_SIG"        , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "Y_EXT_SIG"        , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "SKY_EXT"          , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "RA_EXT"           , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "DEC_EXT"          , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_INST_MAG"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_INST_MAG_SIG" , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_CHISQ"        , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "J",   "EXT_NDOF"         , "no comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "J",   "EXT_MODEL_TYPE"   , "no comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "I",   "EXT_FLAGS"        , "no comment", NULL, 1.0, FT_BZERO_INT16); // unsigned
    gfits_define_bintable_column (&header, "E",   "PSF_INST_MAG"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "AP_MAG"           , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "KRON_MAG"         , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "J",   "NPARAMS"          , "no comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "15A", "MODEL_TYPE"       , "no comment", NULL, 1.0, 0.0); 
    gfits_define_bintable_column (&header, "E",   "EXT_WIDTH_MAJ"    , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_WIDTH_MIN"    , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_THETA"        , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_WIDTH_MAJ_ERR", "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_WIDTH_MIN_ERR", "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_THETA_ERR"    , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "EXT_PAR_07"       , "no comment", NULL, 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&header, &ftable);

    // create intermediate storage arrays
    int    *IPP_IDET           ; ALLOCATE (IPP_IDET         ,  int   , Nstars);
    float  *X_EXT              ; ALLOCATE (X_EXT            ,  float , Nstars);
    float  *Y_EXT              ; ALLOCATE (Y_EXT            ,  float , Nstars);
    float  *X_EXT_SIG          ; ALLOCATE (X_EXT_SIG        ,  float , Nstars);
    float  *Y_EXT_SIG          ; ALLOCATE (Y_EXT_SIG        ,  float , Nstars);
    float  *SKY_EXT            ; ALLOCATE (SKY_EXT          ,  float , Nstars);
    float  *RA_EXT             ; ALLOCATE (RA_EXT           ,  float , Nstars);
    float  *DEC_EXT            ; ALLOCATE (DEC_EXT          ,  float , Nstars);
    float  *EXT_INST_MAG       ; ALLOCATE (EXT_INST_MAG     ,  float , Nstars);
    float  *EXT_INST_MAG_SIG   ; ALLOCATE (EXT_INST_MAG_SIG ,  float , Nstars);
    float  *EXT_CHISQ          ; ALLOCATE (EXT_CHISQ        ,  float , Nstars);
    int    *EXT_NDOF           ; ALLOCATE (EXT_NDOF         ,  int   , Nstars);
    int    *EXT_MODEL_TYPE     ; ALLOCATE (EXT_MODEL_TYPE   ,  int   , Nstars);
    short  *EXT_FLAGS          ; ALLOCATE (EXT_FLAGS        ,  short , Nstars);
    float  *PSF_INST_MAG       ; ALLOCATE (PSF_INST_MAG     ,  float , Nstars);
    float  *AP_MAG             ; ALLOCATE (AP_MAG           ,  float , Nstars);
    float  *KRON_MAG           ; ALLOCATE (KRON_MAG         ,  float , Nstars);
    int    *NPARAMS            ; ALLOCATE (NPARAMS          ,  int   , Nstars);
    char   *MODEL_TYPE         ; ALLOCATE (MODEL_TYPE       ,  char  , Nstars*15);
    float  *EXT_WIDTH_MAJ      ; ALLOCATE (EXT_WIDTH_MAJ    ,  float , Nstars);
    float  *EXT_WIDTH_MIN      ; ALLOCATE (EXT_WIDTH_MIN    ,  float , Nstars);
    float  *EXT_THETA          ; ALLOCATE (EXT_THETA        ,  float , Nstars);
    float  *EXT_WIDTH_MAJ_ERR  ; ALLOCATE (EXT_WIDTH_MAJ_ERR,  float , Nstars);
    float  *EXT_WIDTH_MIN_ERR  ; ALLOCATE (EXT_WIDTH_MIN_ERR,  float , Nstars);
    float  *EXT_THETA_ERR      ; ALLOCATE (EXT_THETA_ERR    ,  float , Nstars);
    float  *EXT_PAR_07         ; ALLOCATE (EXT_PAR_07       ,  float , Nstars);

    // assign the storage arrays
    for (i = 0; i < Nstars; i++) {
      float flux = pow (10.0, -0.4*M[i]);
      float fSN = 1.0 / sqrt(flux);

      double ra, dec;
      XY_to_RD (&ra, &dec, X[i], Y[i], &coords);

      IPP_IDET         [i] = i;
      X_EXT            [i] = X[i];
      Y_EXT            [i] = Y[i];
      X_EXT_SIG        [i] = FX * fSN;
      Y_EXT_SIG        [i] = FY * fSN;
      SKY_EXT          [i] = 0.1;
      RA_EXT           [i] = ra;
      DEC_EXT          [i] = dec;
      EXT_INST_MAG     [i] = M[i];
      EXT_INST_MAG_SIG [i] = fSN;
      EXT_CHISQ        [i] = 1.0;
      EXT_NDOF         [i] = 10;
      EXT_MODEL_TYPE   [i] = -1;
      EXT_FLAGS        [i] = 0x0010;
      PSF_INST_MAG     [i] = M[i];
      AP_MAG           [i] = M[i];
      KRON_MAG         [i] = M[i];
      NPARAMS          [i] = 7;
      EXT_WIDTH_MAJ    [i] = FX*2.8;
      EXT_WIDTH_MIN    [i] = FY*2.8;
      EXT_THETA        [i] = 0.57;
      EXT_WIDTH_MAJ_ERR[i] = FX*0.28;
      EXT_WIDTH_MIN_ERR[i] = FY*0.28;
      EXT_THETA_ERR    [i] = 0.0057;
      EXT_PAR_07       [i] = NAN;

      strcpy (&MODEL_TYPE[15*i], "PS_MODEL_DEV");
    }

    // set the table data
    gfits_set_bintable_column (&header, &ftable, "IPP_IDET"         , IPP_IDET         , Nstars);
    gfits_set_bintable_column (&header, &ftable, "X_EXT"            , X_EXT            , Nstars);
    gfits_set_bintable_column (&header, &ftable, "Y_EXT"            , Y_EXT            , Nstars);
    gfits_set_bintable_column (&header, &ftable, "X_EXT_SIG"        , X_EXT_SIG        , Nstars);
    gfits_set_bintable_column (&header, &ftable, "Y_EXT_SIG"        , Y_EXT_SIG        , Nstars);
    gfits_set_bintable_column (&header, &ftable, "SKY_EXT"          , SKY_EXT          , Nstars);
    gfits_set_bintable_column (&header, &ftable, "RA_EXT"           , RA_EXT           , Nstars);
    gfits_set_bintable_column (&header, &ftable, "DEC_EXT"          , DEC_EXT          , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_INST_MAG"     , EXT_INST_MAG     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_INST_MAG_SIG" , EXT_INST_MAG_SIG , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_CHISQ"        , EXT_CHISQ        , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_NDOF"         , EXT_NDOF         , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_MODEL_TYPE"   , EXT_MODEL_TYPE   , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_FLAGS"        , EXT_FLAGS        , Nstars);
    gfits_set_bintable_column (&header, &ftable, "PSF_INST_MAG"     , PSF_INST_MAG     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "AP_MAG"           , AP_MAG           , Nstars);
    gfits_set_bintable_column (&header, &ftable, "KRON_MAG"         , KRON_MAG         , Nstars);
    gfits_set_bintable_column (&header, &ftable, "NPARAMS"          , NPARAMS          , Nstars);
    gfits_set_bintable_column (&header, &ftable, "MODEL_TYPE"       , MODEL_TYPE       , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_WIDTH_MAJ"    , EXT_WIDTH_MAJ    , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_WIDTH_MIN"    , EXT_WIDTH_MIN    , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_THETA"        , EXT_THETA        , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_WIDTH_MAJ_ERR", EXT_WIDTH_MAJ_ERR, Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_WIDTH_MIN_ERR", EXT_WIDTH_MIN_ERR, Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_THETA_ERR"    , EXT_THETA_ERR    , Nstars);
    gfits_set_bintable_column (&header, &ftable, "EXT_PAR_07"       , EXT_PAR_07       , Nstars);

    // add model type info in header:
    gfits_modify (&header, "MTNUM", "%d", 1, NMODEL);
    for (i = 0; i < NMODEL; i++) {
      char field[64];
      snprintf (field, 64, "MTNAM%02d", i);
      gfits_modify (&header, field,   "%s", 1, ModelNames[i]);
      snprintf (field, 64, "MTVAL%02d", i);
      gfits_modify (&header, field,   "%d", 1, i);
    }
  }
  /**/

  gfits_fwrite_Theader (fits, &header);
  gfits_fwrite_table  (fits, &ftable);
  gfits_free_header (&header);
  gfits_free_table (&ftable);

  return TRUE;
}

# define NXGRID 5
# define NYGRID 5
# define BINSTEP 0.1

int WriteXGALtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {
  OHANA_UNUSED_PARAM(Flag);

  int i;
  Header header;
  FTable ftable;

  char extdata[80];
  snprintf (extdata, 80, "%s.xgal", extroot);

  /* Example Code to create a bintable */
  {
    gfits_create_table_header (&header, "BINTABLE", extdata);

    int Nbin = NXGRID * NYGRID;
    char fmt[16];
    snprintf (fmt, 16, "%dE", Nbin);

    // define the table layout
    gfits_define_bintable_column (&header, "J",   "IPP_IDET"         , "no comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "J",   "MODEL_TYPE"       , "no comment", NULL, 1.0, 0.0); 
    gfits_define_bintable_column (&header, "E",   "X_FIT"            , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "Y_FIT"            , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "NPIX"             , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, fmt,   "GAL_FLUX"         , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, fmt,   "GAL_FLUX_ERR"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, fmt,   "GAL_CHISQ"        , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "FR_MAJOR_MIN"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "FR_MAJOR_MAX"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "FR_MAJOR_DEL"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "FR_MINOR_MIN"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "FR_MINOR_MAX"     , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "FR_MINOR_DEL"     , "no comment", NULL, 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&header, &ftable);

    // create intermediate storage arrays
    int    *IPP_IDET           ; ALLOCATE (IPP_IDET         ,  int   , Nstars);
    int    *MODEL_TYPE         ; ALLOCATE (MODEL_TYPE       ,  int   , Nstars);
    float  *X_FIT              ; ALLOCATE (X_FIT            ,  float , Nstars);
    float  *Y_FIT              ; ALLOCATE (Y_FIT            ,  float , Nstars);
    float  *NPIX               ; ALLOCATE (NPIX             ,  float , Nstars);
    float  *GAL_FLUX           ; ALLOCATE (GAL_FLUX         ,  float , Nstars*Nbin);
    float  *GAL_FLUX_ERR       ; ALLOCATE (GAL_FLUX_ERR     ,  float , Nstars*Nbin);
    float  *GAL_CHISQ          ; ALLOCATE (GAL_CHISQ        ,  float , Nstars*Nbin);
    float  *FR_MAJOR_MIN       ; ALLOCATE (FR_MAJOR_MIN     ,  float , Nstars);
    float  *FR_MAJOR_MAX       ; ALLOCATE (FR_MAJOR_MAX     ,  float , Nstars);
    float  *FR_MAJOR_DEL       ; ALLOCATE (FR_MAJOR_DEL     ,  float , Nstars);
    float  *FR_MINOR_MIN       ; ALLOCATE (FR_MINOR_MIN     ,  float , Nstars);
    float  *FR_MINOR_MAX       ; ALLOCATE (FR_MINOR_MAX     ,  float , Nstars);
    float  *FR_MINOR_DEL       ; ALLOCATE (FR_MINOR_DEL     ,  float , Nstars);

    // assign the storage arrays
    for (i = 0; i < Nstars; i++) {
      float flux = pow (10.0, -0.4*M[i]);

      double ra, dec;
      XY_to_RD (&ra, &dec, X[i], Y[i], &coords);

      IPP_IDET         [i] = i;
      MODEL_TYPE       [i] = 7;
      X_FIT            [i] = X[i];
      Y_FIT            [i] = Y[i];
      NPIX             [i] = 10.0;
      FR_MAJOR_MIN     [i] = FX*2.7;
      FR_MAJOR_DEL     [i] = FX*BINSTEP;
      FR_MAJOR_MAX     [i] = FR_MAJOR_MIN[i] + FR_MAJOR_DEL[i]*(NXGRID - 1); // this is probabaly an error in the real file, careful
      FR_MINOR_MIN     [i] = FY*2.7; 
      FR_MINOR_DEL     [i] = FY*BINSTEP; 
      FR_MINOR_MAX     [i] = FR_MINOR_MIN[i] + FR_MINOR_DEL[i]*(NYGRID - 1); 

      int xs = NXGRID / 2;
      int ys = NYGRID / 2;

      // is it major (fast) * minor (slow) or vice versa?
      int Npt = 0;
      int ix, iy;
      for (ix = -xs; ix <= xs; ix++) {
	for (iy = -ys; iy <= ys; iy++) {
	  float scale = (1.0 + ix*ix + 1.2*iy*iy); // a parabola with min at ix,iy = 0,0
	  GAL_FLUX     [Nbin*i + (ix + xs) + NXGRID*(iy + ys)] = flux / scale;
	  GAL_FLUX_ERR [Nbin*i + (ix + xs) + NXGRID*(iy + ys)] = sqrt(flux / scale);
	  GAL_CHISQ    [Nbin*i + (ix + xs) + NXGRID*(iy + ys)] = 10.0 * scale;
	  Npt ++;
	}
      }
    }

    // set the table data
    gfits_set_bintable_column (&header, &ftable, "IPP_IDET"         , IPP_IDET         , Nstars);
    gfits_set_bintable_column (&header, &ftable, "MODEL_TYPE"       , MODEL_TYPE       , Nstars);
    gfits_set_bintable_column (&header, &ftable, "X_FIT"            , X_FIT            , Nstars);
    gfits_set_bintable_column (&header, &ftable, "Y_FIT"            , Y_FIT            , Nstars);
    gfits_set_bintable_column (&header, &ftable, "NPIX"             , NPIX             , Nstars);
    gfits_set_bintable_column (&header, &ftable, "GAL_FLUX"         , GAL_FLUX         , Nstars);
    gfits_set_bintable_column (&header, &ftable, "GAL_FLUX_ERR"     , GAL_FLUX_ERR     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "GAL_CHISQ"        , GAL_CHISQ         , Nstars);
    gfits_set_bintable_column (&header, &ftable, "FR_MAJOR_MIN"     , FR_MAJOR_MIN     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "FR_MAJOR_MAX"     , FR_MAJOR_MAX     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "FR_MAJOR_DEL"     , FR_MAJOR_DEL     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "FR_MINOR_MIN"     , FR_MINOR_MIN     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "FR_MINOR_MAX"     , FR_MINOR_MAX     , Nstars);
    gfits_set_bintable_column (&header, &ftable, "FR_MINOR_DEL"     , FR_MINOR_DEL     , Nstars);
  }
  /**/

  gfits_fwrite_Theader (fits, &header);
  gfits_fwrite_table  (fits, &ftable);
  gfits_free_header (&header);
  gfits_free_table (&ftable);

  return TRUE;
}

static float psfFWHM[3] = {4.8, 6.0, 8.0};

int WriteXRADtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars, int Nrad) {
  OHANA_UNUSED_PARAM(Flag);

  int i, j, k;
  Header header;
  FTable ftable;

  char extdata[80];
  snprintf (extdata, 80, "%s.xrad", extroot);

  /* Example Code to create a bintable */
  {
    gfits_create_table_header (&header, "BINTABLE", extdata);

    // define the table layout
    gfits_define_bintable_column (&header, "J",   "IPP_IDET"         , "no comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "E",   "X_APER"           , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "Y_APER"           , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E",   "PSF_FWHM"         , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "9E",  "APER_FLUX"        , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "9E",  "APER_FLUX_ERR"    , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "9E",  "APER_FLUX_STDEV"  , "no comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "9E",  "APER_FILL"        , "no comment", NULL, 1.0, 0.0);

    // generate the output array that carries the data
    gfits_create_table (&header, &ftable);

    // create intermediate storage arrays
    int    *IPP_IDET           ; ALLOCATE (IPP_IDET         ,  int   , Nrad*Nstars);
    float  *X_APER             ; ALLOCATE (X_APER           ,  float , Nrad*Nstars);
    float  *Y_APER             ; ALLOCATE (Y_APER           ,  float , Nrad*Nstars);
    float  *PSF_FWHM           ; ALLOCATE (PSF_FWHM         ,  float , Nrad*Nstars);
    float  *APER_FLUX          ; ALLOCATE (APER_FLUX        ,  float , Nrad*Nstars*9);
    float  *APER_FLUX_ERR      ; ALLOCATE (APER_FLUX_ERR    ,  float , Nrad*Nstars*9);
    float  *APER_FLUX_STDEV    ; ALLOCATE (APER_FLUX_STDEV  ,  float , Nrad*Nstars*9);
    float  *APER_FILL          ; ALLOCATE (APER_FILL        ,  float , Nrad*Nstars*9);

    // assign the storage arrays
    for (i = 0; i < Nstars; i++) {
      float flux = pow (10.0, -0.4*M[i]);
      // float fSN = 1.0 / sqrt(flux);

      for (k = 0; k < Nrad; k++) {
	int N = Nrad*i + k;
	IPP_IDET         [N] = i;
	X_APER           [N] = X[i];
	Y_APER           [N] = Y[i];
	PSF_FWHM         [N] = psfFWHM[k];
	for (j = 0; j < 9; j ++) {
	  float Flux = flux*(1.0 + 0.02*j - 0.2);
	  APER_FLUX      [j + 9*N] = Flux;
	  APER_FLUX_ERR  [j + 9*N] = sqrt(Flux);
	  APER_FLUX_STDEV[j + 9*N] = sqrt(Flux*1.1);
	  APER_FILL      [j + 9*N] = 0.9;
	}
      }
    }

    // set the table data
    gfits_set_bintable_column (&header, &ftable, "IPP_IDET"         , IPP_IDET         , Nrad*Nstars);
    gfits_set_bintable_column (&header, &ftable, "X_APER"           , X_APER           , Nrad*Nstars);
    gfits_set_bintable_column (&header, &ftable, "Y_APER"           , Y_APER           , Nrad*Nstars);
    gfits_set_bintable_column (&header, &ftable, "PSF_FWHM"         , PSF_FWHM         , Nrad*Nstars);
    gfits_set_bintable_column (&header, &ftable, "APER_FLUX"        , APER_FLUX        , Nrad*Nstars);
    gfits_set_bintable_column (&header, &ftable, "APER_FLUX_ERR"    , APER_FLUX_ERR    , Nrad*Nstars);
    gfits_set_bintable_column (&header, &ftable, "APER_FLUX_STDEV"  , APER_FLUX_STDEV  , Nrad*Nstars);
    gfits_set_bintable_column (&header, &ftable, "APER_FILL"        , APER_FILL        , Nrad*Nstars);
  }
  /**/

  gfits_fwrite_Theader (fits, &header);
  gfits_fwrite_table  (fits, &ftable);
  gfits_free_header (&header);
  gfits_free_table (&ftable);

  return TRUE;
}

int WriteDETFtable (FILE *fits, char *extroot, double *X, double *Y, double *M, unsigned int *Flag, int Nstars) {
  OHANA_UNUSED_PARAM(X);
  OHANA_UNUSED_PARAM(Y);
  OHANA_UNUSED_PARAM(M);
  OHANA_UNUSED_PARAM(Flag);
  OHANA_UNUSED_PARAM(Nstars);

  int i;
  
  Header header;
  FTable ftable;

  char extdata[80];
  snprintf(extdata, 80, "%s.deteff", extroot);

  int Nbins = 13;
  float OFFSET[13] = {-2.0, -1.0, -0.5, -0.25, -0.1, -0.05, 0.0, 0.05, 0.1, 0.25, 0.5, 1.0, 2.0};

  {
    gfits_create_table_header(&header, "BINTABLE", extdata);

    gfits_define_bintable_column(&header, "1E", "OFFSET"      , "label for field   1", NULL, 1.0, 0.0);
    gfits_define_bintable_column(&header, "1J", "COUNTS"      , "label for field   2", NULL, 1.0, FT_BZERO_INT32);
    gfits_define_bintable_column(&header, "1E", "DIFF.MEAN"   , "label for field   3", NULL, 1.0, 0.0);
    gfits_define_bintable_column(&header, "1E", "DIFF.STDEV"  , "label for field   4", NULL, 1.0, 0.0);
    gfits_define_bintable_column(&header, "1E", "ERR.MEAN"    , "label for field   5", NULL, 1.0, 0.0);

    gfits_create_table(&header, &ftable);

    int   *COUNTS;     ALLOCATE(COUNTS, int, Nbins);
    float *DIFF_MEAN;  ALLOCATE(DIFF_MEAN, float, Nbins);
    float *DIFF_STDEV; ALLOCATE(DIFF_STDEV, float, Nbins);
    float *ERR_MEAN;   ALLOCATE(ERR_MEAN, float, Nbins);

    float rate = 2.0 + 0.2 * (drand48() - 0.5);
    
    for (i = 0; i < Nbins; i++) {
      COUNTS[i] = (int) floor(460.0 / (1.0 + exp(rate * OFFSET[i])));
      DIFF_MEAN[i] = (drand48() - 0.5) / 100.0;
      if (OFFSET[i] > 0) {
	DIFF_MEAN[i] += -0.5 * OFFSET[i];
      }
      DIFF_STDEV[i] = sqrt(2) * fabs(DIFF_MEAN[i]);
      ERR_MEAN[i] = ((rate - 2.0) + (OFFSET[i] + 2.0)/16.0);
    }

    gfits_set_bintable_column(&header, &ftable, "OFFSET", OFFSET, Nbins);
    gfits_set_bintable_column(&header, &ftable, "COUNTS", COUNTS, Nbins);
    gfits_set_bintable_column(&header, &ftable, "DIFF.MEAN", DIFF_MEAN, Nbins);
    gfits_set_bintable_column(&header, &ftable, "DIFF.STDEV", DIFF_STDEV, Nbins);
    gfits_set_bintable_column(&header, &ftable, "ERR.MEAN", ERR_MEAN, Nbins);
    
    gfits_modify(&header, "HIERARCH DETEFF.MAGREF", "%f", 1, -6.806806);
    gfits_modify(&header, "HIERARCH DETEFF.NUM", "%d", 1, 500);
  }

  gfits_fwrite_Theader(fits, &header);
  gfits_fwrite_table(fits, &ftable);
  gfits_free_header(&header);
  gfits_free_table(&ftable);
  
  return TRUE;
}

int WriteDummyTable (FILE *fits) {

  Header header;
  FTable ftable;

  /* Example Code to create a bintable 
  {
    gfits_create_table_header (&header, "BINTABLE", extroot);

    // define the table layout
    gfits_define_bintable_column (&header, "D", "NAME", "comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "E", "NAME", "comment", NULL, 1.0, 0.0);
    gfits_define_bintable_column (&header, "J", "NAME", "comment", NULL, 1.0, FT_BZERO_INT32); // unsigned
    gfits_define_bintable_column (&header, "I", "NAME", "comment", NULL, 1.0, FT_BZERO_INT16); // unsigned

    // generate the output array that carries the data
    gfits_create_table (&header, &ftable);

    // create intermediate storage arrays
    double *R; ALLOCATE (R,  double, Nvalue);
    float  *M; ALLOCATE (M,  float , Nvalue);
    int    *I; ALLOCATE (I,  int   , Nvalue);
    short  *P; ALLOCATE (P,  short , Nvalue);

    // assign the storage arrays
    for (i = 0; Nvalue; i++) {
      R[i] = value;
      M[i] = value;
      I[i] = value;
      P[i] = value;
    }

    // set the table data
    gfits_set_bintable_column (&header, &ftable, "NAME", R, catalog->Nmeasure);
    gfits_set_bintable_column (&header, &ftable, "NAME", M, catalog->Nmeasure);
    gfits_set_bintable_column (&header, &ftable, "NAME", I, catalog->Nmeasure);
    gfits_set_bintable_column (&header, &ftable, "NAME", P, catalog->Nmeasure);
  }
  */

  gfits_fwrite_Theader (fits, &header);
  gfits_fwrite_table  (fits, &ftable);
  gfits_free_header (&header);
  gfits_free_table (&ftable);
  
  return TRUE;
}

