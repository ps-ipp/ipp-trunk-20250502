# include "relphot.h"

int mkdata (FitDataSet *dataset, int fitSlope, double valueTru, double slopeTru, int Npoints, int Nbad, int GaussianOutliers);

int main (int argc, char **argv) {
  
  int Ntests      =  100;
  int Npoints     =  100;
  int Noutliers   =   10;
  int Nbootstrap  =  100;
  int Niterations =   10;

  int N;
  if ((N = get_argument (argc, argv, "-Ntests"))) {
    remove_argument (N, &argc, argv);
    Ntests = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Npoints"))) {
    remove_argument (N, &argc, argv);
    Npoints = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Noutliers"))) {
    remove_argument (N, &argc, argv);
    Noutliers = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Nbootstrap"))) {
    remove_argument (N, &argc, argv);
    Nbootstrap = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-Niterations"))) {
    remove_argument (N, &argc, argv);
    Niterations = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int GaussianOutliers = TRUE;
  if ((N = get_argument (argc, argv, "-flat-outliers"))) {
    remove_argument (N, &argc, argv);
    GaussianOutliers = FALSE;
  }

  int FitSlope = FALSE;
  if ((N = get_argument (argc, argv, "-fit-slope"))) {
    remove_argument (N, &argc, argv);
    FitSlope = TRUE;
  }
  
  double ValuePrior = NAN;
  double ValueSigma = NAN;
  if ((N = get_argument (argc, argv, "-value-prior"))) {
    remove_argument (N, &argc, argv);
    ValuePrior = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ValueSigma = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  double SlopePrior = NAN;
  double SlopeSigma = NAN;
  if ((N = get_argument (argc, argv, "-slope-prior"))) {
    if (!FitSlope) { fprintf (stderr, "-slope-prior only valid if -fit-slope is selected\n"); exit (2); }
    remove_argument (N, &argc, argv);
    SlopePrior = atof (argv[N]);
    remove_argument (N, &argc, argv);
    SlopeSigma = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    fprintf (stderr, "USAGE: %s\n", argv[0]);
    fprintf (stderr, " some options: -Ntests [3000], -Npoints [100], -Noutliers [10], -flat-outliers\n");
    fprintf (stderr, " more options: -fit-slope, -value-prior (value) (sigma), -slope-prior (slope) (sigma)\n");
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

  FitDataSet dataset;
  FitDataSetAlloc (&dataset, Npoints + Noutliers, FitSlope, Nbootstrap);
  dataset.MaxIterations = Niterations;

  if (isfinite(ValuePrior) || isfinite(SlopePrior)) {
    FitDataSetAddPriors (&dataset);
    if (isfinite(ValuePrior)) {
      dataset.bPriorValue[0] = ValuePrior;
      dataset.bPriorSigma[0] = ValueSigma;
    }
    if (isfinite(SlopePrior)) {
      dataset.bPriorValue[1] = SlopePrior;
      dataset.bPriorSigma[1] = SlopeSigma;
    }
  }

  for (int i = 0; i < Ntests; i++) {

    double valueTru = 25.0;
    double slopeTru = 0.05;

    // generate a single fake star with Npoints real points and Noutliers bad points
    mkdata (&dataset, FitSlope, valueTru, slopeTru, Npoints, Noutliers, GaussianOutliers);

    fit1d_irls (&dataset, Npoints + Noutliers);

    if (FitSlope) {
      fprintf (stdout, "%8.3f %8.5f : %6.3f %6.3f : %8.5f %8.3f %6.3f : %6.3f %6.3f : %6.3f : %4d %4d %4d\n", valueTru, dataset.bSaveArray[0][0], dataset.bSigma[0], slopeTru, dataset.bSaveArray[1][0], dataset.bSigma[1], dataset.sigma, dataset.min, dataset.max, dataset.chisq, Npoints, Noutliers, dataset.Nmeas);
    } else {
      fprintf (stdout, "%8.3f %8.5f : %6.3f %6.3f : %6.3f %6.3f : %6.3f : %4d %4d %4d\n", valueTru, dataset.bSaveArray[0][0], dataset.bSigma[0], dataset.sigma, dataset.min, dataset.max, dataset.chisq, Npoints, Noutliers, dataset.Nmeas);
    }
  }

  ohana_gaussdev_free();

  FitDataSetFree (&dataset);

  ohana_memcheck (TRUE);
  ohana_memdump (TRUE);

  exit (0);
}

// we are using IRLS 1D fitting to solve for zero points and airmass slope
// I will generate data samples with a nominal zero point and an airmass slope

// valueObs = valueTru + slopeTru*trend

// choose a random value for 'trend' between MIN_TREND and MAX_TREND
// set the raw value to valueTru + slopeTru*trend
// vary by Gaussian deviate using sigma of VAL_ERROR

# define VAL_ERROR 0.03
# define MIN_TREND 0.0
# define MAX_TREND 1.0
int mkdata (FitDataSet *dataset, int FitSlope, double valueTru, double slopeTru, int Npoints, int Nbad, int GaussianOutliers) {
  
  for (int i = 0; i < Npoints; i++) {
    double dF = ohana_gaussdev_rnd(0.0, VAL_ERROR);

    double trend = MIN_TREND + (MAX_TREND - MIN_TREND)*drand48();

    double valueRaw = FitSlope ? valueTru + slopeTru*trend : valueTru;
    double valueObs = valueRaw + dF;

    dataset->alldata->yVector[i] = valueObs;
    dataset->alldata->dyVector[i] = VAL_ERROR;
    if (FitSlope) { 
      dataset->alldata->xVector[i] = trend;
    }
  }

  for (int i = 0; i < Nbad; i++) {
    double dF = GaussianOutliers ? ohana_gaussdev_rnd(10.0*VAL_ERROR, 10.0*VAL_ERROR) : 10.0*VAL_ERROR*drand48();

    double trend = MIN_TREND + (MAX_TREND - MIN_TREND)*drand48();

    double valueRaw = FitSlope ? valueTru + slopeTru*trend : valueTru;
    double valueObs = valueRaw + dF;

    dataset->alldata->yVector[i + Npoints] = valueObs;
    dataset->alldata->dyVector[i + Npoints] = VAL_ERROR;
    if (FitSlope) { 
      dataset->alldata->xVector[i + Npoints] = trend;
    }
  }

  return TRUE;
}
