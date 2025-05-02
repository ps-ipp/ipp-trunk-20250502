# include "addstar.h"
# include "loadgalphot.h"

# define GET_COLUMN(VER,OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&header_##VER, &ftable_##VER, NAME, type, &NrowNew, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type"); \
  if (firstCol) { Nrow = NrowNew; firstCol = FALSE; } \
  else { myAssert (Nrow == NrowNew, "table length mismatch"); }

static float *Xpt = NULL;
static float *Ypt = NULL;
static float *Rpt = NULL;
static float *Rsr = NULL;
static char *mask = NULL;

static float *chisqFit = NULL;
static float ZeroPt = 0;

GalPhot_Stars *loadgalphot_readstars (char *filename, int *nstars, AddstarClientOptions *options) {

  // read in the full FITS files ('cause I don't have a partial read option)
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read stellar parameter file: %s", filename);

  int i, j, Ncol, Nsample;
  off_t Nrow = 0;
  off_t NrowNew;

  char type[16];
  int firstCol;

  // load in PHU for astrometry
  Header PHU;
  if (!gfits_fread_header (f, &PHU)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return NULL;
  }

  Header header_xfit;
  FTable ftable_xfit;
  if (!gfits_find_Xheader (f, &header_xfit, "SkyChip.xfit")) {
    if (VERBOSE) fprintf (stderr, "can't read galaxy photometry header\n");
    gfits_free_header (&header_xfit);
    gfits_free_header (&PHU);
    fclose (f);
    return NULL;
  }
  ftable_xfit.header = &header_xfit;

  if (header_xfit.Naxis[1] == 0) {
    if (VERBOSE) fprintf (stderr, "no data in file %s, skipping\n", filename);
    gfits_free_header (&header_xfit);
    gfits_free_header (&PHU);
    fclose (f);
    *nstars = 0;
    return NULL;
  }

  // in xgal, we have numerical values to specify the model types
  // in xfit, we have strings.  these are listed with their numerical 
  // match in the PHU.  I need to read the list of names and set up a string hash

  int ModelTypeDev = -1;
  int ModelTypeExp = -1;

  int ModelTypeN;
  char **ModelTypeName;
  int *ModelTypeNum;
  int *ModelTypeHash;
  if (!gfits_scan (&header_xfit, "MTNUM", "%d", 1, &ModelTypeN)) { myAbort ("fail"); }
  ALLOCATE (ModelTypeName, char *, ModelTypeN);
  ALLOCATE (ModelTypeNum,  int, ModelTypeN);
  ALLOCATE (ModelTypeHash, int, ModelTypeN);

  int tmpnumber, namehash[256][3];
  for (i = 0; i < 256; i++) {
    namehash[i][0] = -1;
    namehash[i][1] = -1;
    namehash[i][2] = -1;
  }

  ZeroPt = GetZeroPoint();

  char name[256], tmpname[256];
  for (i = 0; i < ModelTypeN; i++) {
    snprintf (name, 64, "MTNAM%02d", i);
    if (!gfits_scan (&header_xfit, name, "%s", 1, tmpname)) { myAbort ("fail"); }
    ModelTypeName[i] = strcreate (tmpname);
    int found = FALSE;
    int myHash = strhash (tmpname, 0xff);
    if (VERBOSE == 2) fprintf (stderr, "model %s, hash: %d\n", tmpname, myHash);
    ModelTypeHash[i] = myHash;
    for (j = 0; (j < 3) && !found; j++) {
      if (namehash[myHash][j] != -1) continue;
      found = TRUE;
      namehash[myHash][j] = i;
    }
    myAssert (found, "hash conflict");
    snprintf (name, 64, "MTVAL%02d", i);
    if (!gfits_scan (&header_xfit, name, "%d", 1, &tmpnumber)) { myAbort ("fail"); }
    ModelTypeNum[i] = tmpnumber;
    if (!strcasecmp (ModelTypeName[i], "PS_MODEL_EXP")) {
      ModelTypeExp = ModelTypeNum[i];
    }
    if (!strcasecmp (ModelTypeName[i], "PS_MODEL_DEV")) {
      ModelTypeDev = ModelTypeNum[i];
    }
  }

  if (!gfits_fread_ftable_data (f, &ftable_xfit, FALSE)) {
    if (VERBOSE) fprintf (stderr, "can't read galaxy photometry data\n");
    gfits_free_header (&header_xfit);
    gfits_free_header (&PHU);
    fclose (f);
    return (NULL);
  }

  firstCol = TRUE;
  GET_COLUMN (xfit, ID_fit,         "IPP_IDET",      int);
  GET_COLUMN (xfit, MODEL_TYPE_str, "MODEL_TYPE",    char);  int NcharModel = Ncol;
  GET_COLUMN (xfit, EXT_WIDTH_MAJ,  "EXT_WIDTH_MAJ", float);
  GET_COLUMN (xfit, EXT_WIDTH_MIN,  "EXT_WIDTH_MIN", float);
  GET_COLUMN (xfit, EXT_THETA,      "EXT_THETA",     float);
  GET_COLUMN (xfit, EXT_THETA_ERR,  "EXT_THETA_ERR", float);
  GET_COLUMN (xfit, INDEX,          "EXT_PAR_07",    float);
  int Nfit = Nrow;

  float DEV_INDEX_VALUE = 0.5 / 4.0;
  float EXP_INDEX_VALUE = 0.5 / 1.0;

  // note that INDEX here is defined as 1.0 / (2 \nu), where \nu is the traditional sersic
  // expression.

  myAssert (NcharModel < 256, "max model type name is very long: %d", NcharModel);

  // free the memory associated with the FITS files
  gfits_free_header (&header_xfit);
  gfits_free_table (&ftable_xfit);

  char string[256];

  // convert MODEL_TYPE_str to MODEL_TYPE_fit (transforming strings to values)
  int *MODEL_TYPE_fit;
  ALLOCATE (MODEL_TYPE_fit, int, Nfit);
  for (i = 0; i < Nfit; i++) {
    memset (string, 0, 256);
    memcpy (string, &MODEL_TYPE_str[i*NcharModel], NcharModel); string[NcharModel] = 0;
    stripwhite (string);
    int myHash = strhash (string, 0xff);
    int found = FALSE;
    for (j = 0; !found && (j < 3); j++) {
      int n = namehash[myHash][j];
      if (strcmp(ModelTypeName[n], string)) continue;
      MODEL_TYPE_fit[i] = ModelTypeNum[n];
      found = TRUE;
    }
    myAssert (found, "unknown model name? %s", string);
  }

  Header header_xgal;
  FTable ftable_xgal;
  if (!gfits_find_Xheader (f, &header_xgal, "SkyChip.xgal")) {
    if (VERBOSE) fprintf (stderr, "can't read galaxy photometry header\n");
    gfits_free_header (&header_xgal);
    gfits_free_header (&PHU);
    fclose (f);
    return NULL;
  }

  ftable_xgal.header = &header_xgal;
  if (header_xgal.Naxis[1] == 0) {
    if (VERBOSE) fprintf (stderr, "no data in file %s, skipping\n", filename);
    gfits_free_header (&header_xgal);
    gfits_free_header (&PHU);
    fclose (f);
    return NULL;
  }

  if (!gfits_fread_ftable_data (f, &ftable_xgal, FALSE)) {
    if (VERBOSE) fprintf (stderr, "can't read galaxy photometry data\n");
    gfits_free_header (&header_xgal);
    gfits_free_header (&PHU);
    fclose (f);
    return (NULL);
  }

  firstCol = TRUE;
  GET_COLUMN (xgal, ID_gal,        "IPP_IDET",     int);
  GET_COLUMN (xgal, MODEL_TYPE_gal,"MODEL_TYPE",   int);
  GET_COLUMN (xgal, NPIX,          "NPIX",  	   float);
  GET_COLUMN (xgal, X_FIT,         "X_FIT", 	   float);
  GET_COLUMN (xgal, Y_FIT,         "Y_FIT", 	   float);
  GET_COLUMN (xgal, GAL_FLUX,      "GAL_FLUX",     float); Nsample = Ncol;
  GET_COLUMN (xgal, GAL_FLUX_ERR,  "GAL_FLUX_ERR", float); myAssert (Ncol == Nsample, "invalid table");
  GET_COLUMN (xgal, GAL_CHISQ,     "GAL_CHISQ",    float); myAssert (Ncol == Nsample, "invalid table");
  GET_COLUMN (xgal, FR_MAJOR_MIN,  "FR_MAJOR_MIN", float);
  GET_COLUMN (xgal, FR_MAJOR_MAX,  "FR_MAJOR_MAX", float);
  GET_COLUMN (xgal, FR_MAJOR_DEL,  "FR_MAJOR_DEL", float);
  GET_COLUMN (xgal, FR_MINOR_MIN,  "FR_MINOR_MIN", float);
  GET_COLUMN (xgal, FR_MINOR_MAX,  "FR_MINOR_MAX", float);
  GET_COLUMN (xgal, FR_MINOR_DEL,  "FR_MINOR_DEL", float);
  int Ngal = Nrow;

  // i need to match the two lists based on (ID_gal == ID_fit), (MODEL_TYPE_gal == MODEL_TYPE_fit)
  GalPhotIDset fitSet, galSet;
  fitSet.ID   = ID_fit;
  fitSet.type = MODEL_TYPE_fit;
  fitSet.N    = Nfit;
  galSet.ID   = ID_gal;
  galSet.type = MODEL_TYPE_gal;
  galSet.N    = Ngal;
  int *idx_gal = join_IDs (&galSet, &fitSet);

  // free the memory associated with the FITS files
  gfits_free_header (&header_xgal);
  gfits_free_table (&ftable_xgal);

  GalPhot_Stars *stars = NULL;
  ALLOCATE (stars, GalPhot_Stars, Nrow);

  Coords coords;
  GetCoords (&coords, &PHU);

  // we reuse these arrays : allocate just once:
  ALLOCATE (Xpt, float, Nsample);
  ALLOCATE (Ypt, float, Nsample);
  ALLOCATE (Rpt, float, Nsample);
  ALLOCATE (Rsr, float, Nsample);
  ALLOCATE (mask, char, Nsample);
  ALLOCATE (chisqFit, float, Nsample);

  Fit2D *fit = fit2d_init (2);
  fit->ClipNiter = 3;
  fit->ClipNsigma = 5.0;

  // use the list of index values from above to join entries with the same index

  int Nbad = 0;
  for (i = 0; i < Nrow; i++) {

    int ifit = idx_gal[i];
    if (ifit < 0) { 
      fprintf (stderr, "%d %f %f %d\n", i, X_FIT[i], Y_FIT[i], MODEL_TYPE_gal[i]);
      Nbad ++; 
      continue; 
    } // skip galaxy models with bad IDs

    double R, D;
    XY_to_RD (&R, &D, X_FIT[i], Y_FIT[i], &coords);
    R = ohana_normalize_angle (R);

    stars[i].R = R;
    stars[i].D = D;
    stars[i].flag  = FALSE;
    stars[i].found = FALSE;

    dvo_galphot_init (&stars[i].galphot);

    // stars[i].galphot.R = R;
    // stars[i].galphot.D = D;
    stars[i].galphot.Xfit = X_FIT[i];
    stars[i].galphot.Yfit = Y_FIT[i];

    // I need to match up with the entries in xfit to get:
    // theta, theta_err, index, 
    // note that FR_MAJOR, etc are the fractions of the stack fit value
    stars[i].galphot.theta     = EXT_THETA[ifit];
    stars[i].galphot.thetaErr  = EXT_THETA_ERR[ifit];
    stars[i].galphot.Npix      = NPIX[i];
    stars[i].galphot.modelType = MODEL_TYPE_gal[i];
    if (INDEX) {
      stars[i].galphot.index     = INDEX[ifit];
    } else {
      if (MODEL_TYPE_gal[i] == ModelTypeDev) { stars[i].galphot.index = DEV_INDEX_VALUE; }
      if (MODEL_TYPE_gal[i] == ModelTypeExp) { stars[i].galphot.index = EXP_INDEX_VALUE; }
    }
    stars[i].galphot.detID     = ID_gal[i];
    stars[i].galphot.photcode  = options->photcode;
    stars[i].galphot.imageID   = IMAGE_ID;

    // I have a grid of measurements with (Flux, dFlux, Chisq) at each point
    // I need to find the minimum position (interpolated) in this 2D space

    // I want to use this flag to set a bit in secfilt.  is that possible?  or maybe galphot.dummy becomes flags
    FitChisqMinimum (fit, &stars[i].galphot, &GAL_CHISQ[Nsample*i], &GAL_FLUX[Nsample*i], &GAL_FLUX_ERR[Nsample*i], Nsample, FR_MAJOR_MIN[i], FR_MAJOR_MAX[i], FR_MAJOR_DEL[i], FR_MINOR_MIN[i], FR_MINOR_MAX[i], FR_MINOR_DEL[i]);

    // I could either multiply FR_MAJOR_MIN, etc above or the fitted values below
    stars[i].galphot.majorAxis    *= EXT_WIDTH_MAJ[ifit];
    stars[i].galphot.majorAxisErr *= EXT_WIDTH_MAJ[ifit];
    stars[i].galphot.minorAxis    *= EXT_WIDTH_MIN[ifit];
    stars[i].galphot.minorAxisErr *= EXT_WIDTH_MIN[ifit];
  }

  free (Xpt);
  free (Ypt);
  free (Rpt);
  free (Rsr);
  free (mask);
  free (chisqFit);

  fit2d_free (fit);

  fprintf (stderr, "Nbad: %d of %d\n", Nbad, (int) Nrow);

  *nstars = Nrow;
  return (stars);
}

int loadgalphot_sortStars (GalPhot_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ GalPhot_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].R < stars[B].R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

// for some reason, gcc does not (always?) pick up a declaration of this function 
long lrint (double value);

int FitChisqMinimum (Fit2D *fit, GalPhot *galphot, 
		     float *chisq, float *flux, float *fluxErr, int Npts, 
		     float MajorMin, float MajorMax, float MajorDel, 
		     float MinorMin, float MinorMax, float MinorDel) {

  int i;

  // validate the square dimensions??

  // int Nx = nearbyint((MajorMax - MajorMin) / MajorDel) + 1;
  // int Ny = nearbyint((MinorMax - MinorMin) / MinorDel) + 1;

  int Nx = lrint((MajorMax - MajorMin) / MajorDel) + 1;
  int Ny = lrint((MinorMax - MinorMin) / MinorDel) + 1;

  myAssert (Nx*Ny == Npts, "inconsistent grid");
  
  // here are the steps:
  // find the minimum chisq grid point (set X,Y values)
  // rescale chisq values so minimum is == Npix
  // fit 2D parabola

  memset (mask, 0, Npts*sizeof(char));

  // the values passed in are reduced chisq, but I want to get the 
  // fit for the total chisq so the errors are correct
  for (i = 0; (i < Npts); i++) {
    chisq[i] *= galphot->Npix;
  }

  float chisqMin = FLT_MAX;
  int iMin = -1;
  int Nvalid = 0;
  for (i = 0; i < Npts; i++) {
    if (!isfinite(chisq[i])) {
      mask[i] = TRUE;
      continue;
    }
    int iY = i / Nx;
    int iX = i % Nx;
    Xpt[i] = MajorMin + MajorDel*iX;
    Ypt[i] = MinorMin + MinorDel*iY;
    Nvalid ++;
    if (chisq[i] < chisqMin) {
      chisqMin = chisq[i];
      iMin = i;
    }
  }
  if (!Nvalid) {
    return FALSE; // we cannot do anything if there is no finite chisq
  }
  if (Nvalid < 6) { galphot->flags |= ID_GALPHOT_TOO_FEW; goto bad_fit; }

  for (i = 0; FALSE && (i < Npts); i++) {
    chisq[i] *= galphot->Npix;
  }

# if (0)
  {
    FILE *f = fopen ("galphot.dump.dat", "w");
    for (i = 0; i < Npts; i++) {
      fprintf (f, "%d %d %f %f %f %f\n", i, mask[i], Xpt[i], Ypt[i], chisq[i], flux[i]);
    }
    fclose (f);
  }
# endif

  // fit with 3 iterations, 5 sigma clipping (set above after fit2d_init)
  // on failure (for any reason), the stars[i].flag is set to TRUE (XXX bad choice)
  if (!fit2d (fit, Xpt, Ypt, chisq, chisqFit, mask, Npts)) { galphot->flags |= ID_GALPHOT_FAIL_FIT; goto bad_fit; }

  // get X,Y for the chisq min from the fit:
  float Det = 1.0 / (4*fit->c20*fit->c02 - SQ(fit->c11));
  float Xmin = (fit->c01*fit->c11 - 2.0*fit->c02*fit->c10) * Det;
  float Ymin = (fit->c10*fit->c11 - 2.0*fit->c20*fit->c01) * Det;

  // allow the fitting minimum to be just outside the grid, but no further
  int inRange = TRUE;
  inRange = inRange && (Xmin > MajorMin - 2*MajorDel);
  inRange = inRange && (Xmin < MajorMax + 2*MajorDel);
  inRange = inRange && (Ymin > MinorMin - 2*MinorDel);
  inRange = inRange && (Ymin < MinorMax + 2*MinorDel);

  if (!inRange) { galphot->flags |= ID_GALPHOT_OUT_OF_RANGE; goto bad_fit; }

  // chisqMin @ (Xmin,Ymin)
  float A, B, C, Q;

  // Xoff @ chisqMin + 1.0, delta Y = 0.0
  A = fit->c20;
  B = fit->c10 + 2*fit->c20*Xmin + fit->c11*Ymin;
  C = -1;
  Q = B*B - 4*A*C;
  if (Q < 0.0) { galphot->flags |= ID_GALPHOT_BAD_ERROR; goto bad_err; }
  if (A <= 0.0) { galphot->flags |= ID_GALPHOT_BAD_ERROR; goto bad_err; }
  
  float dXmin = (-B + sqrt(Q)) / (2.0 * A);

  // Yoff @ chisqMin + 1.0, delta X = 0.0
  A = fit->c02;
  B = fit->c01 + 2*fit->c02*Ymin + fit->c11*Xmin;
  C = -1;
  Q = B*B - 4*A*C;
  if (Q <  0.0) { galphot->flags |= ID_GALPHOT_BAD_ERROR; goto bad_err; }
  if (A <= 0.0) { galphot->flags |= ID_GALPHOT_BAD_ERROR; goto bad_err; }

  float dYmin = (-B + sqrt(Q)) / (2.0 * A);

  // test the formulae:
# if (0)
  float Zmin = fit->c00 + fit->c10*Xmin + fit->c01*Ymin + fit->c20*Xmin*Xmin + fit->c02*Ymin*Ymin + fit->c11*Xmin*Ymin;
  float ZdX  = fit->c00 + fit->c10*(Xmin + dXmin) + fit->c01*Ymin + fit->c20*(Xmin + dXmin)*(Xmin + dXmin) + fit->c02*Ymin*Ymin + fit->c11*(Xmin + dXmin)*Ymin;
  float ZdY  = fit->c00 + fit->c10*Xmin + fit->c01*(Ymin + dYmin) + fit->c20*Xmin*Xmin + fit->c02*(Ymin + dYmin)*(Ymin + dYmin) + fit->c11*Xmin*(Ymin + dYmin);
  fprintf (stderr, "Zmin: %f : %f %f\n", Zmin, ZdX - Zmin, ZdY - Zmin);
# endif

  galphot->majorAxis = Xmin;
  galphot->minorAxis = Ymin;
  galphot->majorAxisErr = dXmin;
  galphot->minorAxisErr = dYmin;

  // convert (Xmin, Ymin) to (iXmin, iYmin)
  float iXminF = (Xmin - MajorMin) / MajorDel;
  float iYminF = (Ymin - MinorMin) / MinorDel;

  // interpolate in X and in Y
  int   iXmin = floor(iXminF);
  iXmin = MAX(0,MIN(Nx - 2, iXmin)); // force range of iXmin to be 0,Nx-1
  float fXmin = iXminF - iXmin;

  int   iYmin = floor(iYminF);
  iYmin = MAX(0,MIN(Ny - 2, iYmin)); // force range of iYmin to be 0,Ny-1
  float fYmin = iYminF - iYmin;

  float V00 = flux[iXmin+0 + (iYmin+0)*Nx];
  float V01 = flux[iXmin+0 + (iYmin+1)*Nx];
  float V10 = flux[iXmin+1 + (iYmin+0)*Nx];
  float V11 = flux[iXmin+1 + (iYmin+1)*Nx];

  float Vx0 = V10*fXmin + V00*(1.0 - fXmin);
  float Vx1 = V11*fXmin + V01*(1.0 - fXmin);

  float value = fYmin * Vx1 + (1.0 - fYmin) * Vx0;

  float dV00 = fluxErr[iXmin+0 + (iYmin+0)*Nx];
  float dV01 = fluxErr[iXmin+0 + (iYmin+1)*Nx];
  float dV10 = fluxErr[iXmin+1 + (iYmin+0)*Nx];
  float dV11 = fluxErr[iXmin+1 + (iYmin+1)*Nx];

  float dVx0 = dV10*fXmin + dV00*(1.0 - fXmin);
  float dVx1 = dV11*fXmin + dV01*(1.0 - fXmin);

  float dValue = fYmin * dVx1 + (1.0 - fYmin) * dVx0;

  // use bilinear interpolation to choose Flux @ (Xmin,Ymin)?
  // float magRaw    = -2.5*log10(flux[iMin]) + ZeroPt; // correct for exptime?
  // float magErrRaw = sqrt(fluxErr[iMin]) / flux[iMin];

  galphot->mag    = -2.5*log10(value) + ZeroPt; // correct for exptime?
  galphot->magErr = sqrt(dValue) / value;
  galphot->chisq  = chisqMin / galphot->Npix;
  
  return TRUE;

bad_fit:
  galphot->majorAxis = Xpt[iMin];
  galphot->minorAxis = Ypt[iMin];
  galphot->majorAxisErr = MajorDel;
  galphot->minorAxisErr = MinorDel;

  galphot->mag    = -2.5*log10(flux[iMin]) + ZeroPt; // correct for exptime?
  galphot->magErr = sqrt(fluxErr[iMin]) / flux[iMin];
  galphot->chisq  = chisqMin / galphot->Npix;
  return FALSE;

bad_err:
  galphot->majorAxis = Xmin;
  galphot->minorAxis = Ymin;
  galphot->majorAxisErr = MajorDel;
  galphot->minorAxisErr = MinorDel;

  galphot->mag    = -2.5*log10(flux[iMin]) + ZeroPt; // correct for exptime?
  galphot->magErr = sqrt(fluxErr[iMin]) / flux[iMin];
  galphot->chisq  = chisqMin / galphot->Npix;
  return FALSE;
}

