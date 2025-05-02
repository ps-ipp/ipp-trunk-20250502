# include "relastro.h"

static int Nrand = 0;
double drand48_cnt () {
  double value = drand48();
  Nrand ++;
  return value;
}

Catalog *mkstar (FitStats *fitStats, double Ro, double Do, double uR, double uD, double plx, int Npoints, int Nbad, int Nstack);

# define MJD_MIN_MJD 55197   /* 2010/01/01,00:00:00 */
# define MJD_MAX_MJD 57023   /* 2015/01/01,00:00:00 */
# define MJD_MIN_YRS 10.00   /* 2010/01/01,00:00:00 */
# define MJD_MAX_YRS 14.9993 /* 2015/01/01,00:00:00 */
# define MJD_REF_YRS 12.4148 /* 2012/06/01,00:00:00 */

# define MJD_J2000   51544.5 /* 2000/01/01,12:00:00 */

# define POS_ERROR 0.010 /* arcsec */
# define OUT_ERROR 0.500 /* arcsec */

// # define N_STARS 3000
// # define N_POINTS 100
// # define N_OUTLIERS 5
// # define N_BOOTSTRAP 100

# define dcos(THETA) cos(RAD_DEG*THETA)
# define dsin(THETA) sin(RAD_DEG*THETA)

static int Nsecfilt = 0;

int main (int argc, char **argv) {
  
  int N_STARS     = 3000;
  int N_POINTS    =  100;
  int N_STACK     =    6;
  int N_BOOTSTRAP =  100;
  float F_OUTLIERS  =  0.1;

  int N;
  if ((N = get_argument (argc, argv, "-Nstars"))) {
    remove_argument (N, &argc, argv);
    N_STARS = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Npoints"))) {
    remove_argument (N, &argc, argv);
    N_POINTS = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Nstack"))) {
    remove_argument (N, &argc, argv);
    N_STACK = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-foutliers"))) {
    remove_argument (N, &argc, argv);
    F_OUTLIERS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Nbootstrap"))) {
    remove_argument (N, &argc, argv);
    N_BOOTSTRAP = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    fprintf (stderr, "USAGE: %s (mode)\n", argv[0]);
    fprintf (stderr, "  mode: pos, pm, plx\n");
    fprintf (stderr, " options: -Nstars [3000], -Npoints [100], -Noutliers [10], -Nbootstrap [100]\n");
    exit (2);
  }

  FIT_MODE = FIT_NONE;
  if (!strcasecmp(argv[1], "pos")) {
    FIT_MODE = FIT_AVERAGE;
  }
  if (!strcasecmp(argv[1], "pm")) {
    FIT_MODE = FIT_PM_ONLY;
  }
  if (!strcasecmp(argv[1], "plx")) {
    FIT_MODE = FIT_PM_AND_PAR;
  }
  if (FIT_MODE == FIT_NONE) {
    fprintf (stderr, "USAGE: %s (mode)\n", argv[0]);
    fprintf (stderr, "  mode: pos, pm, plx\n");
    fprintf (stderr, " options: -Nstars [3000], -Npoints [100], -Noutliers [10], -Nbootstrap [100]\n");
    exit (2);
  }

  // some relastro globals need to be set
  N_BOOTSTRAP_SAMPLES = N_BOOTSTRAP;
  USE_GALAXY_MODEL = TRUE;
  MaxMeanOffset = 10.0;

  // load reference table here
  if (!LoadPhotcodesText ("dvo.photcodes")) {
    fprintf (stderr, "failed to load photcodes\n");
    exit (2);
  }
  Nsecfilt = GetPhotcodeNsecfilt();

  // init the random seed
  { 
    struct timeval now;
    gettimeofday (&now, NULL);
    long A = now.tv_sec + now.tv_usec * 1000000;
    srand48(A);
    srand48(1);
  }

  ohana_gaussdev_init ();

  int i;

  FitStats *fitStats = FitStatsInit (N_POINTS * (1 + 2*F_OUTLIERS), N_BOOTSTRAP);

  int Nstars = N_STARS;
  for (i = 0; i < Nstars; i++) {

    // double Ro  = 360.0*drand48_cnt();
    double Ro  = 0.1*drand48_cnt();

    double Phi = 2.0*(drand48_cnt() - 0.5);
    double Do  = DEG_RAD * asin(Phi);

    double uR  = 0.5*(drand48_cnt() - 0.5);
    double uD  = 0.5*(drand48_cnt() - 0.5);

    double plx = 0.5*(drand48_cnt() + 0.0);
    if (FIT_MODE != FIT_PM_AND_PAR) plx = 0.0;

    int Npoints = N_POINTS*drand48_cnt();        // minimum of 0 points
    int Noutliers_max = Npoints * F_OUTLIERS;
    int Noutliers = Noutliers_max*drand48_cnt(); // minimum of 0 points
    int Nstack = N_STACK*drand48_cnt();          // minimum of 0 points

    if (Npoints + Nstack + Noutliers == 0) continue;

    // generate a single fake star with N_POINTS real points and N_OUTLIERS bad points
    Catalog *catalog = mkstar (fitStats, Ro, Do, uR, uD, plx, Npoints, Noutliers, Nstack);

    UpdateObjects_Stack (catalog->average, catalog->secfilt, catalog->measureT, NULL, Nsecfilt, fitStats);
    UpdateObjects_Chips (catalog->average, catalog->secfilt, catalog->measureT, NULL, Nsecfilt, fitStats, 0, 0);

    double Tmean = ohana_sec_to_mjd(catalog->average->Tmean);
    double Tyear = (Tmean - MJD_J2000) / 365.25;
    fprintf (stdout, "%3d %3d %3d %12.7f %12.7f %8.5f %8.5f %8.5f | %12.7f %12.7f %8.5f %8.5f %8.5f | %12.7f |  %8.5f %8.5f %8.5f %8.5f %8.5f  %3d |  %10.5f %10.5f %8.5f %8.5f | %6.2f %6.2f %6.2f  0x%08x\n",
	     Npoints, Noutliers, Nstack, Ro, Do, uR, uD, plx, 
	     catalog->average->R,  catalog->average->D,  catalog->average->uR,  catalog->average->uD,  catalog->average->P, Tyear, 
	     catalog->average->dR, catalog->average->dD, catalog->average->duR, catalog->average->duD, catalog->average->dP, catalog->average->Npos,
	     catalog->average->Rstk,  catalog->average->Dstk,  catalog->average->dRstk,  catalog->average->dDstk,  
	     catalog->average->ChiSqAve, catalog->average->ChiSqPM, catalog->average->ChiSqPar, catalog->average->flags);
  }
  ohana_gaussdev_free();

  exit (0);
}

Catalog *mkstar (FitStats *fitStats, double Ro, double Do, double uR, double uD, double plx, int Npoints, int Nbad, int Nstack) {
  
  int i;

  int Ntotal = Npoints + Nbad + Nstack;

  ALLOCATE_PTR (catalog, Catalog, 1);
  ALLOCATE (catalog->average, Average, 1);
  ALLOCATE (catalog->secfilt, SecFilt, Nsecfilt);
  ALLOCATE (catalog->measureT, MeasureTiny, Ntotal);
  
  dvo_average_init (catalog->average);
  for (i = 0; i < Ntotal; i++) {
    dvo_measureT_init (&catalog->measureT[i]);
  }

  catalog->average->objID = 1;
  catalog->average->catID = 1;

  // int T2000 = ohana_date_to_sec ("2000/01/01,12:00:00");
  for (i = 0; i < Npoints; i++) {
    // ParFactor expects a time which is in years since J2000 (MJD_..._YRS is so defined)
    double Tyears = MJD_MIN_YRS + drand48_cnt()*(MJD_MAX_YRS - MJD_MIN_YRS);
    double Tmjd   = 365.25*Tyears + MJD_J2000;

    catalog->measureT[i].t = ohana_mjd_to_sec (Tmjd);
    
    double pR, pD;
    ParFactor (&pR, &pD, Ro, Do, Tyears);

    double dR = ohana_gaussdev_rnd(0.0, POS_ERROR);
    double dD = ohana_gaussdev_rnd(0.0, POS_ERROR);

    catalog->measureT[i].R = Ro + (dR + plx*pR + uR*(Tyears - MJD_REF_YRS))/3600/dcos(Do);
    catalog->measureT[i].D = Do + (dD + plx*pD + uD*(Tyears - MJD_REF_YRS))/3600;

    // force some points to loop over the boundary:
    if (drand48() > 0.5) {
      catalog->measureT[i].R += 360.0;
    }
    if (drand48() > 0.8) {
      catalog->measureT[i].R += 360.0;
    }

    catalog->measureT[i].dXccd = ToShortPixels(POS_ERROR);
    catalog->measureT[i].dYccd = ToShortPixels(POS_ERROR);

    catalog->measureT[i].photcode = 10233;

    // nominal values to avoid dropping in MeasFilterTest
    catalog->measureT[i].M = 20.0;
    catalog->measureT[i].dM = 0.02;
    catalog->measureT[i].dt = 2.5*log10(30.0);
  }

  for (i = Npoints; i < Npoints + Nbad; i++) {
    // ParFactor expects a time which is in years since J2000 (MJD_..._YRS is so defined)
    double Tyears = MJD_MIN_YRS + drand48_cnt()*(MJD_MAX_YRS - MJD_MIN_YRS);
    double Tmjd   = 365.25*Tyears + MJD_J2000;

    catalog->measureT[i].t = ohana_mjd_to_sec (Tmjd);
    
    double pR, pD;
    ParFactor (&pR, &pD, Ro, Do, Tyears);

    double dR = OUT_ERROR*(drand48_cnt() - 0.5);
    double dD = OUT_ERROR*(drand48_cnt() - 0.5);

    catalog->measureT[i].R = Ro + (dR + plx*pR + uR*(Tyears - MJD_REF_YRS))/3600/dcos(Do);
    catalog->measureT[i].D = Do + (dD + plx*pD + uD*(Tyears - MJD_REF_YRS))/3600;

    catalog->measureT[i].dXccd = ToShortPixels(POS_ERROR);
    catalog->measureT[i].dYccd = ToShortPixels(POS_ERROR);

    catalog->measureT[i].photcode = 10233;

    // nominal values to avoid dropping in MeasFilterTest
    catalog->measureT[i].M = 20.0;
    catalog->measureT[i].dM = 0.02;
    catalog->measureT[i].dt = 2.5*log10(30.0);
  }

  // double oRstk = POS_ERROR*(drand48_cnt() - 0.5);
  // double oDstk = POS_ERROR*(drand48_cnt() - 0.5);

  double oRstk = 0.0*(drand48_cnt() - 0.5);
  double oDstk = 0.0*(drand48_cnt() - 0.5);

  for (i = Npoints + Nbad; i < Ntotal; i++) {
    // ParFactor expects a time which is in years since J2000 (MJD_..._YRS is so defined)
    double Tyears = MJD_MIN_YRS + drand48_cnt()*(MJD_MAX_YRS - MJD_MIN_YRS);
    double Tmjd   = 365.25*Tyears + MJD_J2000;

    catalog->measureT[i].t = ohana_mjd_to_sec (Tmjd);
    
    catalog->measureT[i].R = Ro + oRstk/3600.0/dcos(Do);
    catalog->measureT[i].D = Do + oDstk/3600.0;

    catalog->measureT[i].dXccd = ToShortPixels(POS_ERROR);
    catalog->measureT[i].dYccd = ToShortPixels(POS_ERROR);

    catalog->measureT[i].photcode = 11200;

    // nominal values to avoid dropping in MeasFilterTest
    catalog->measureT[i].M = 20.0;
    catalog->measureT[i].dM = 0.02;
    catalog->measureT[i].dt = 2.5*log10(30.0);
  }

  if (Ntotal > 0) {
    catalog->average->R = catalog->measureT[0].R;
    catalog->average->D = catalog->measureT[0].D;
    catalog->average->uRgal = uR;
    catalog->average->uDgal = uD;
  }

  catalog->average->Nmeasure = Ntotal;

  return catalog;
}

// these dummy functions are used to avoid including ImageOps, etc
int areImagesMatched () {
  return FALSE;
}
float getColorBlue (off_t meas, int cat) {
  OHANA_UNUSED_PARAM(meas);
  OHANA_UNUSED_PARAM(cat);
  return (NAN);
}
float getColorRed (off_t meas, int cat) {
  OHANA_UNUSED_PARAM(meas);
  OHANA_UNUSED_PARAM(cat);
  return (NAN);
}
