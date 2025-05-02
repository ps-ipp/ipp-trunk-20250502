# include "relphot.h"

int mkstar (StatDataSet *dataset, double flux, int Npoints, int Nbad, int GaussianOutliers);

static int Nrand = 0;
double drand48_cnt () {
  double value = drand48();
  Nrand ++;
  return value;
}

typedef enum {
  FIT_FLUX_NONE,
  FIT_FLUX_MEAN,
  FIT_FLUX_IRLS,
} TestModeType;

int main (int argc, char **argv) {
  
  int N_STARS     = 3000;
  int N_POINTS    =  100;
  int N_OUTLIERS  =   10;
  // int N_BOOTSTRAP =  100;

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
  int GaussianOutliers = TRUE;
  if ((N = get_argument (argc, argv, "-flat-outliers"))) {
    remove_argument (N, &argc, argv);
    GaussianOutliers = FALSE;
  }

  if (argc != 2) {
    fprintf (stderr, "USAGE: %s (mode)\n", argv[0]);
    fprintf (stderr, " options: -Nstars [3000], -Npoints [100], -Noutliers [10], -flat-outliers\n");
    exit (2);
  }

  TestModeType mode = FIT_FLUX_NONE;
  if (!strcasecmp (argv[1], "irls")) mode = FIT_FLUX_IRLS;
  if (!strcasecmp (argv[1], "mean")) mode = FIT_FLUX_MEAN;
  if (!mode) {
    fprintf (stderr, " mode options: mean, irls\n");
    exit (2);
  }

  // plan_tests (14);

  // diag ("relastro fitpm tests");
  
  // init the random seed
  { 
    struct timeval now;
    gettimeofday (&now, NULL);
    long A = now.tv_sec + now.tv_usec * 1000000;
    srand48(A);
    // srand48(1);
  }

  ohana_gaussdev_init ();

  // allow for 5 filters and 1-to-1 outliers
  StatDataSet *dataset = StatDataSetAlloc (1, N_POINTS + N_OUTLIERS);

  StatType stats;
  liststats_init (&stats);

  int Nstars = N_STARS;
  for (int i = 0; i < Nstars; i++) {

    double lflux = 2.0 + 3.0*drand48_cnt(); // uniform in log-flux
    double flux = pow(10.0, lflux);

    int Npoints   = N_POINTS;
    int Noutliers = N_OUTLIERS;

    // generate a single fake star with N_POINTS real points and N_OUTLIERS bad points
    mkstar (dataset, flux, Npoints, Noutliers, GaussianOutliers);

    switch (mode) {
      case FIT_FLUX_MEAN:
	liststats_setmode (&stats, "MEAN");
	liststats (dataset->flxlist, dataset->errlist, dataset->wgtlist, Npoints + Noutliers, &stats);
	break;

      case FIT_FLUX_IRLS:
	liststats_irls (dataset, Npoints + Noutliers, &stats);
	break;

      default: myAbort ("programming error");
    }

    fprintf (stdout, "%8.2f %8.2f : %6.2f %6.2f : %6.3f : %4d %4d %4d\n",
	     flux, stats.mean, stats.sigma, stats.error, stats.chisq, Npoints, Noutliers, stats.Nmeas);
  }
  // return exit_status();
  ohana_gaussdev_free();

  StatDataSetFree (dataset, 1);

  ohana_memcheck (TRUE);
  ohana_memdump (TRUE);

  exit (0);
}

# define READ_NOISE_SQR 25.0
int mkstar (StatDataSet *dataset, double flux, int Npoints, int Nbad, int GaussianOutliers) {
  
  double FluxError = sqrt(flux);

  for (int i = 0; i < Npoints; i++) {
    double dF = ohana_gaussdev_rnd(0.0, FluxError);

    double fluxObs = flux + dF;
    double dFluxObs = sqrt(fluxObs + READ_NOISE_SQR); 

    dataset->flxlist[i] = fluxObs;
    dataset->errlist[i] = dFluxObs;
    dataset->wgtlist[i] = 1.0;
    dataset->ranking[i] = 1;
    dataset->measSeq[i] = i;
  }

  for (int i = 0; i < Nbad; i++) {

    // double dF = GaussianOutliers ? ohana_gaussdev_rnd(0.0, 10.0*FluxError) : 10.0*FluxError*(drand48_cnt() - 0.5);
    double dF = GaussianOutliers ? ohana_gaussdev_rnd(10.0*FluxError, 10.0*FluxError) : 10.0*FluxError*drand48_cnt();

    double fluxObs = flux + dF;
    double dFluxObs = sqrt(fluxObs + READ_NOISE_SQR); 

    dataset->flxlist[Npoints + i] = fluxObs;
    dataset->errlist[Npoints + i] = dFluxObs;
    dataset->wgtlist[Npoints + i] = 1.0;
    dataset->ranking[Npoints + i] = 1;
    dataset->measSeq[Npoints + i] = i;
  }

  return TRUE;
}
