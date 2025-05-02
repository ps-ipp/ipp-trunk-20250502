# include "relastro.h"

static int Nrand = 0;
double drand48_cnt () {
  double value = drand48();
  Nrand ++;
  return value;
}

int mkstar (FitStats *fitStats, double Ro, double Do, double uR, double uD, double plx, int Npoints, int Nbad);

# define MJD_MIN_MJD 55197   /* 2010/01/01,00:00:00 */
# define MJD_MAX_MJD 57023   /* 2015/01/01,00:00:00 */
# define MJD_MIN_YRS 10.00   /* 2010/01/01,00:00:00 */
# define MJD_MAX_YRS 14.9993 /* 2015/01/01,00:00:00 */
# define MJD_REF_YRS 12.4148 /* 2012/06/01,00:00:00 */

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

float ERROR_BIAS = 1.0;
float RA_BIAS = 0.0;
float DEC_BIAS = 0.0;

int main (int argc, char **argv) {
  
  int N_STARS     = 3000;
  int N_POINTS    =  100;
  // int N_OUTLIERS  =   10;
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
  // if ((N = get_argument (argc, argv, "-Noutliers"))) {
  //   remove_argument (N, &argc, argv);
  //   N_OUTLIERS = atoi (argv[N]);
  //   remove_argument (N, &argc, argv);
  // }
  if ((N = get_argument (argc, argv, "-Nbootstrap"))) {
    remove_argument (N, &argc, argv);
    N_BOOTSTRAP = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-error-bias"))) {
    remove_argument (N, &argc, argv);
    ERROR_BIAS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-ra-bias"))) {
    remove_argument (N, &argc, argv);
    RA_BIAS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dec-bias"))) {
    remove_argument (N, &argc, argv);
    DEC_BIAS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    fprintf (stderr, "USAGE: %s (mode)\n", argv[0]);
    fprintf (stderr, "  mode: pos, pos-irls, pos-boot, pm, pm-irls, pm-boot, plx, plx-irls, plx-boot\n");
    fprintf (stderr, " options: -Nstars [3000], -Npoints [100], -Noutliers [10], -Nbootstrap [100]\n");
    exit (2);
  }

  int mode = FIT_POS_NONE;
  if (!strcasecmp(argv[1], "pos")) {
    mode = FIT_POS_NOCLIP;
  }
  if (!strcasecmp(argv[1], "pos-irls")) {
    mode = FIT_POS_IRLS;
  }
  if (!strcasecmp(argv[1], "pos-boot")) {
    mode = FIT_POS_BOOT;
  }
  if (!strcasecmp(argv[1], "pm")) {
    mode = FIT_PM_NOCLIP;
  }
  if (!strcasecmp(argv[1], "pm-irls")) {
    mode = FIT_PM_IRLS;
  }
  if (!strcasecmp(argv[1], "pm-boot")) {
    mode = FIT_PM_BOOT;
  }
  if (!strcasecmp(argv[1], "plx")) {
    mode = FIT_PLX_NOCLIP;
  }
  if (!strcasecmp(argv[1], "plx-irls")) {
    mode = FIT_PLX_IRLS;
  }
  if (!strcasecmp(argv[1], "plx-boot")) {
    mode = FIT_PLX_BOOT;
  }

  // plan_tests (14);

  // diag ("relastro fitpm tests");
  
  // init the random seed
  { 
    struct timeval now;
    gettimeofday (&now, NULL);
    long A = now.tv_sec + now.tv_usec * 1000000;
    srand48(A);
    srand48(1);
  }

  ohana_gaussdev_init ();

  FitStats *fitStats = FitStatsInit (N_POINTS * (1 + 2*F_OUTLIERS), N_BOOTSTRAP);
  // FitStats *fitStats = FitStatsInit (N_POINTS + N_OUTLIERS, N_BOOTSTRAP);
  fitStats->fitdataPM->getChisq  = TRUE;
  fitStats->fitdataPM->getError  = TRUE;
  fitStats->fitdataPar->getChisq = TRUE;
  fitStats->fitdataPar->getError = TRUE;
  fitStats->fitdataPos->getChisq = TRUE;
  fitStats->fitdataPos->getError = TRUE;
  // NOTE: surprisingly, the chisq and error calculations only add ~2-3% to the exec time

  int i, k;

  int Nstars = N_STARS;
  for (i = 0; i < Nstars; i++) {

    double Ro  = 360.0*drand48_cnt();
    double Phi = 2.0*(drand48_cnt() - 0.5);
    double Do  = DEG_RAD * asin(Phi);

    double uR  = 0.5*(drand48_cnt() - 0.5);
    double uD  = 0.5*(drand48_cnt() - 0.5);

    double plx = 0.5*(drand48_cnt() + 0.0);

    switch (mode) {
      case FIT_POS_IRLS:
      case FIT_POS_BOOT:
      case FIT_POS_NOCLIP:
      case FIT_PM_IRLS:
      case FIT_PM_BOOT:
      case FIT_PM_NOCLIP:
	plx = 0.0;
	break;
      case FIT_PLX_IRLS:
      case FIT_PLX_BOOT:
      case FIT_PLX_NOCLIP:
	break;
      default:
	myAbort("oops");
    }

    // int Npoints = N_POINTS*drand48_cnt(); // minimum of 0 points
    // int Noutliers_max = Npoints * F_OUTLIERS;
    // int Noutliers = Noutliers_max*drand48_cnt(); // minimum of 0 points

    int Npoints = N_POINTS; // minimum of 0 points
    int Noutliers = Npoints * F_OUTLIERS;

    // generate a single fake star with N_POINTS real points and N_OUTLIERS bad points
    mkstar (fitStats, Ro, Do, uR, uD, plx, Npoints, Noutliers);

    double Tmean, Trange, parRange;
    FitAstromPoints_Project (fitStats, &Tmean, &Trange, &parRange);

    FitAstromResult fitResult;    
    FitAstromResultInit (&fitResult);

    int Nnomask;

    switch (mode) {
      case FIT_POS_NOCLIP:
	fitResult.uR = uR;
	fitResult.uD = uD;
	if (!FitPosPMfixed_Basic (&fitResult, fitStats->fitdataPos, fitStats->points, fitStats->Npoints)) goto escape;
	break;
      case FIT_POS_IRLS:
	fitResult.uR = uR;
	fitResult.uD = uD;
	if (!FitPosPMfixed_IRLS (&fitResult, fitStats->fitdataPos, fitStats->points, fitStats->Npoints)) goto escape;
	break;
      case FIT_POS_BOOT:
	fitResult.uR = uR;
	fitResult.uD = uD;
	if (!FitPosPMfixed_IRLS (&fitResult, fitStats->fitdataPos, fitStats->points, fitStats->Npoints)) goto escape;
	fitStats->Nfit = 0;
	Nnomask = BootstrapSaveUnmasked (fitStats->nomask, fitStats->points, fitStats->Npoints);
	for (k = 0; k < fitStats->NfitAlloc; k++) {
	  BootstrapResample (fitStats->sample, fitStats->nomask, Nnomask);
	  fitStats->fit[k].uR = uR;
	  fitStats->fit[k].uD = uD;
	  if (!FitPosPMfixed_Basic (&fitStats->fit[fitStats->Nfit], fitStats->fitdataPos, fitStats->sample, Nnomask)) continue;
	  fitStats->Nfit ++;
	}
	// these calls set the ERRORS on the fit parameters, not the fit values (set in IRLS above)
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_RA);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_DEC);
	break;

      case FIT_PM_NOCLIP:
	if (!FitPM_Basic (&fitResult, fitStats->fitdataPM, fitStats->points, fitStats->Npoints)) goto escape;
	break;
      case FIT_PM_IRLS:
	if (!FitPM_IRLS (&fitResult, fitStats->fitdataPM, fitStats->points, fitStats->Npoints)) goto escape;
	break;
      case FIT_PM_BOOT:
	if (!FitPM_IRLS (&fitResult, fitStats->fitdataPM, fitStats->points, fitStats->Npoints)) goto escape;
	fitStats->Nfit = 0;
	Nnomask = BootstrapSaveUnmasked (fitStats->nomask, fitStats->points, fitStats->Npoints);
	for (k = 0; k < fitStats->NfitAlloc; k++) {
	  BootstrapResample (fitStats->sample, fitStats->nomask, Nnomask);
	  if (!FitPM_Basic (&fitStats->fit[fitStats->Nfit], fitStats->fitdataPM, fitStats->sample, Nnomask)) continue;
	  fitStats->Nfit ++;
	}
	// these calls set the ERRORS on the fit parameters, not the fit values (set in IRLS above)
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_RA);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_DEC);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_uR);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_uD);
	break;

      case FIT_PLX_NOCLIP:
	if (!FitPMandPar_Basic (&fitResult, fitStats->fitdataPar, fitStats->points, fitStats->Npoints)) goto escape;
	break;
      case FIT_PLX_IRLS:
	if (!FitPMandPar_IRLS (&fitResult, fitStats->fitdataPar, fitStats->points, fitStats->Npoints)) goto escape;
	break;
      case FIT_PLX_BOOT:
	if (!FitPMandPar_IRLS (&fitResult, fitStats->fitdataPar, fitStats->points, fitStats->Npoints)) goto escape;
	fitStats->Nfit = 0;
	Nnomask = BootstrapSaveUnmasked (fitStats->nomask, fitStats->points, fitStats->Npoints);
	for (k = 0; k < fitStats->NfitAlloc; k++) {
	  BootstrapResample (fitStats->sample, fitStats->nomask, Nnomask);
	  if (!FitPMandPar_Basic (&fitStats->fit[fitStats->Nfit], fitStats->fitdataPar, fitStats->sample, Nnomask)) continue;
	  fitStats->Nfit ++;
	}
	// these calls set the ERRORS on the fit parameters, not the fit values (set in IRLS above)
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_RA);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_DEC);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_uR);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_uD);
	BootstrapRobustStats (&fitResult, fitStats->fit, fitStats->Nfit, FIT_RESULT_PLX);
	break;

      default:
	myAbort("oops");
    }

    XY_to_RD (&fitResult.Ro, &fitResult.Do, fitResult.Ro, fitResult.Do, &fitStats->coords);

    fprintf (stdout, "%12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f |  %8.6f %8.6f %8.6f %8.6f %8.6f  %d | %f\n",
	     Ro, Do, uR, uD, plx, 
	     fitResult.Ro, fitResult.Do, fitResult.uR, fitResult.uD, fitResult.p, Tmean, fitResult.dRo, fitResult.dDo, fitResult.duR, fitResult.duD, fitResult.dp, fitResult.Nfit, fitResult.chisq); 
    if (i > 0) {
      // fprintf  (stderr, ".\n");
    }

    continue;

  escape:
    fprintf (stdout, "%12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f %12.8f %8.6f %8.6f %8.6f | %12.8f |  %8.6f %8.6f %8.6f %8.6f %8.6f  %d | %f\n",
	     Ro, Do, uR, uD, plx, 
	     NAN, NAN, NAN, NAN, NAN, 0.0, NAN, NAN, NAN, NAN, NAN, 0, 0.0);
  }
  // return exit_status();
  ohana_gaussdev_free();

  exit (0);
}

int mkstar (FitStats *fitStats, double Ro, double Do, double uR, double uD, double plx, int Npoints, int Nbad) {
  
  int i;

  int Ntotal = Npoints + Nbad;

  FitAstromPoint *points = fitStats->points;

  for (i = 0; i < Npoints; i++) {
    FitAstromPointInit (&points[i]);

    // ParFactor expects a time which is in years since J2000 (MJD_..._YRS is so defined)
    points[i].T = MJD_MIN_YRS + drand48_cnt()*(MJD_MAX_YRS - MJD_MIN_YRS);
    
    double pR, pD;
    ParFactor (&pR, &pD, Ro, Do, points[i].T);

    double dR = ohana_gaussdev_rnd(0.0, POS_ERROR);
    double dD = ohana_gaussdev_rnd(0.0, POS_ERROR);

    points[i].R = Ro + (dR + plx*pR + uR*(points[i].T - MJD_REF_YRS))/3600/dcos(Do);
    points[i].D = Do + (dD + plx*pD + uD*(points[i].T - MJD_REF_YRS))/3600;

    points[i].dX = POS_ERROR*ERROR_BIAS;
    points[i].dY = POS_ERROR*ERROR_BIAS;
  }

  for (i = Npoints; i < Ntotal; i++) {
    FitAstromPointInit (&points[i]);

    // ParFactor expects a time which is in years since J2000 (MJD_..._YRS is so defined)
    points[i].T = MJD_MIN_YRS + drand48_cnt()*(MJD_MAX_YRS - MJD_MIN_YRS);
    
    double pR, pD;
    ParFactor (&pR, &pD, Ro, Do, points[i].T);

    double dR = OUT_ERROR*(drand48_cnt() - 0.5) +  RA_BIAS;
    double dD = OUT_ERROR*(drand48_cnt() - 0.5) + DEC_BIAS;

    points[i].R = Ro + (dR + plx*pR + uR*(points[i].T - MJD_REF_YRS))/3600/dcos(Do);
    points[i].D = Do + (dD + plx*pD + uD*(points[i].T - MJD_REF_YRS))/3600;

    points[i].dX = POS_ERROR*ERROR_BIAS;
    points[i].dY = POS_ERROR*ERROR_BIAS;
  }
  fitStats->Npoints = Ntotal;

  return TRUE;
}
