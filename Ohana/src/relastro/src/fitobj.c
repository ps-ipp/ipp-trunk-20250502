# include "relastro.h"

Catalog *mkstar (FitStats *fitStats, double Ro, double Do, double uR, double uD, double plx, int Npoints, int Nbad);

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

enum {
  FIT_POS_NONE,
  FIT_POS_IRLS,
  FIT_POS_BOOT,
  FIT_POS_NOCLIP,
  FIT_PM_IRLS,
  FIT_PM_BOOT,
  FIT_PM_NOCLIP,
  FIT_PLX_IRLS,
  FIT_PLX_BOOT,
  FIT_PLX_NOCLIP,
};

static int Nsecfilt = 0;

int main (int argc, char **argv) {
  
  int N_STARS     = 3000;
  int N_POINTS    =  100;
  int N_OUTLIERS  =   10;
  int N_BOOTSTRAP =  100;

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
  if ((N = get_argument (argc, argv, "-Noutliers"))) {
    remove_argument (N, &argc, argv);
    N_OUTLIERS = atoi (argv[N]);
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
    fprintf (stderr, "  test astrometry fitting using UpdateObjects_Chip code\n");
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
    fprintf (stderr, "failed to load photcode file dvo.photcodes (copy from ipp/ippconfig)\n");
    exit (2);
  }
  Nsecfilt = GetPhotcodeNsecfilt();

  // init the random seed
  { 
    struct timeval now;
    gettimeofday (&now, NULL);
    long A = now.tv_sec + now.tv_usec * 1000000;
    srand48(A);
    // srand48(1);
  }

  ohana_gaussdev_init ();

  int i;

  FitStats *fitStats = FitStatsInit (N_POINTS + N_OUTLIERS, N_BOOTSTRAP);

  int Nstars = N_STARS;
  for (i = 0; i < Nstars; i++) {

    // generate random stars uniformly distributed on the sphere (ra = 0,36)
    double Ro  = 360.0*drand48();
    double Phi = 2.0*(drand48() - 0.5);
    double Do  = DEG_RAD * asin(Phi);

    double uR  = 0.5*(drand48() - 0.5);
    double uD  = 0.5*(drand48() - 0.5);

    double plx = (FIT_MODE == FIT_PM_AND_PAR) ? 0.5*(drand48() + 0.0) : 0.0;

    // generate a single fake star with N_POINTS real points and N_OUTLIERS bad points
    Catalog *catalog = mkstar (fitStats, Ro, Do, uR, uD, plx, N_POINTS, N_OUTLIERS);

    if (!UpdateObjects_Chips (catalog->average, catalog->secfilt, catalog->measureT, NULL, Nsecfilt, fitStats, 0, 0)) goto escape;

    double Tmean = ohana_sec_to_mjd(catalog->average->Tmean);
    double Tyear = (Tmean - MJD_J2000) / 365.25;
    fprintf (stdout, "%12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f |  %8.6f %8.6f %8.6f %8.6f %8.6f  %d | %f %f %f\n",
	     Ro, Do, uR, uD, plx, 
	     catalog->average->R,  catalog->average->D,  catalog->average->uR,  catalog->average->uD,  catalog->average->P, Tyear, 
	     catalog->average->dR, catalog->average->dD, catalog->average->duR, catalog->average->duD, catalog->average->dP, catalog->average->Npos,
	     catalog->average->ChiSqAve, catalog->average->ChiSqPM, catalog->average->ChiSqPar);

    continue;

  escape:
    fprintf (stdout, "%12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f |  %8.6f %8.6f %8.6f %8.6f %8.6f  %d | %f\n",
	     NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, 0.0, NAN, NAN, NAN, NAN, NAN, 0, 0.0);
  }
  // return exit_status();
  ohana_gaussdev_free();

  exit (0);
}

Catalog *mkstar (FitStats *fitStats, double Ro, double Do, double uR, double uD, double plx, int Npoints, int Nbad) {
  OHANA_UNUSED_PARAM(fitStats);
  
  int i;

  int Ntotal = Npoints + Nbad;

  ALLOCATE_PTR (catalog, Catalog, 1);
  ALLOCATE (catalog->average, Average, 1);
  ALLOCATE (catalog->secfilt, SecFilt, Nsecfilt);
  ALLOCATE (catalog->measureT, MeasureTiny, Npoints + Nbad);
  
  dvo_average_init (catalog->average);
  for (i = 0; i < Ntotal; i++) {
    dvo_measureT_init (&catalog->measureT[i]);
  }

  catalog->average->objID = 1;
  catalog->average->catID = 1;

  catalog->average->R = Ro;
  catalog->average->D = Do;
  catalog->average->uR = uR;
  catalog->average->uD = uD;
  catalog->average->uRgal = uR;
  catalog->average->uDgal = uD;

  // int T2000 = ohana_date_to_sec ("2000/01/01,12:00:00");
  for (i = 0; i < Npoints; i++) {
    // ParFactor expects a time which is in years since J2000 (MJD_..._YRS is so defined)
    double Tyears = MJD_MIN_YRS + drand48()*(MJD_MAX_YRS - MJD_MIN_YRS);
    double Tmjd   = 365.25*Tyears + MJD_J2000;

    catalog->measureT[i].t = ohana_mjd_to_sec (Tmjd);
    
    double pR, pD;
    ParFactor (&pR, &pD, Ro, Do, Tyears);

    double dR = ohana_gaussdev_rnd(0.0, POS_ERROR);
    double dD = ohana_gaussdev_rnd(0.0, POS_ERROR);

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

  for (i = Npoints; i < Ntotal; i++) {
    // ParFactor expects a time which is in years since J2000 (MJD_..._YRS is so defined)
    double Tyears = MJD_MIN_YRS + drand48()*(MJD_MAX_YRS - MJD_MIN_YRS);
    double Tmjd   = 365.25*Tyears + MJD_J2000;

    catalog->measureT[i].t = ohana_mjd_to_sec (Tmjd);
    
    double pR, pD;
    ParFactor (&pR, &pD, Ro, Do, Tyears);

    double dR = OUT_ERROR*(drand48() - 0.5);
    double dD = OUT_ERROR*(drand48() - 0.5);

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
