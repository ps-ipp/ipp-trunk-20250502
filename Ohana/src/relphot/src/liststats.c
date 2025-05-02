# include "relphot.h"

void liststats_setmode (StatType *stats, char *strmode) {

  stats->statmode = STATS_NONE;
  if (!strcmp (strmode, "MEAN"))             { stats->statmode = STATS_MEAN; return; }
  if (!strcmp (strmode, "MEDIAN"))           { stats->statmode = STATS_MEDIAN; return; }
  if (!strcmp (strmode, "WT_MEAN"))          { stats->statmode = STATS_WT_MEAN; return; }
  if (!strcmp (strmode, "INNER_MEAN"))       { stats->statmode = STATS_INNER_MEAN; return; }
  if (!strcmp (strmode, "INNER_WTMEAN"))     { stats->statmode = STATS_INNER_WTMEAN; return; }
  if (!strcmp (strmode, "CHI_INNER_MEAN"))   { stats->statmode = STATS_CHI_INNER_MEAN; return; }
  if (!strcmp (strmode, "CHI_INNER_WTMEAN")) { stats->statmode = STATS_CHI_INNER_WTMEAN; return; }

  fprintf (stderr, "ERROR: invalid stats mode: %s\n", strmode);
  exit (1);
}

int liststats_init (StatType *stats) {
  stats->median      = NAN;
  stats->mean        = NAN;
  stats->sigma       = NAN;
  stats->error       = NAN;
  stats->chisq       = NAN;
  stats->min         = NAN;
  stats->max         = NAN;
  stats->Upper80     = NAN;
  stats->Lower20     = NAN;
  stats->Upper90     = NAN;
  stats->Lower10     = NAN;
  stats->Upper80Nsig = NAN;
  stats->Lower20Nsig = NAN;
  stats->Upper90Nsig = NAN;
  stats->Lower10Nsig = NAN;
  stats->total       = NAN;
  stats->Nmeas       = 0;
  return TRUE;
}

int liststats (double *value, double *dvalue, double *weight, int N, StatType *stats) {
  
  int i, ks, ke;
  double Mo, dMo, M, dM, Nm, X2, dS, R, W, *chi;

  myAssert (stats->statmode != STATS_NONE, "programming error, liststats mode not set");
  ListStatsMode myMode = stats->statmode & STATS_PRIMARY; // exclude the option bits

  liststats_init (stats);

  ke = ks = dMo = 0;

  stats[0].Nmeas = N;
  stats[0].mean  = 0;
  stats[0].sigma = 0;
  stats[0].error = 0;
  stats[0].chisq = 0;
  if (N < 1) return (FALSE);

  int N90 = MIN (N-1, 0.9*N);
  int N80 = MIN (N-1, 0.8*N);
  int N20 = MAX (0, 0.2*N);
  int N10 = MAX (0, 0.1*N);

  if (weight) {
    dsortthree (value, dvalue, weight, N);
  } else {
    if (dvalue) {
      dsortpair (value, dvalue, N);
    } else {
      dsort (value, N);
    }
  }

  // these values do not depend on the errors or weighting scheme
  stats[0].median  = value[(int)(0.5*N)];
  stats[0].min     = value[0];
  stats[0].max     = value[N-1];
  stats[0].Upper80 = value[N80];
  stats[0].Lower20 = value[N20];
  stats[0].Upper90 = value[N90];
  stats[0].Lower10 = value[N10];

  switch (myMode) {
    case STATS_MEDIAN:
      ks = 0;
      ke = N;
      Mo = stats[0].median;
      Nm = N;
      goto chisq;
      break;
    case STATS_MEAN:
    case STATS_WT_MEAN:
      ks = 0;
      ke = N;
      break;
    case STATS_INNER_MEAN:
    case STATS_INNER_WTMEAN:
    case STATS_CHI_INNER_MEAN:
    case STATS_CHI_INNER_WTMEAN:
      ks = 0.25*N + 0.50;
      ke = 0.75*N + 0.25;
      if (N <= 3) {
	ks = 0;
	ke = N;
      }
      break;
    case STATS_NONE:
    default:
      myAbort ("undefined stats");
  }    

  // for these two modes, I need a vector of the chi-square contribution
  // I'm actually just using chisq to get the correct sorting order
  if ((myMode == STATS_CHI_INNER_MEAN) || (myMode == STATS_CHI_INNER_WTMEAN)) {
    ALLOCATE (chi, double, N);
    for (i = 0; i < N; i++) {
      chi[i] = (value[i] - stats[0].median) / dvalue[i];
    }
    if (weight) {
      dsortfour (chi, value, dvalue, weight, N);
    } else {
      dsortthree (chi, value, dvalue, N);
    }
    free (chi);
  }

  int WeightedMean = FALSE;
  WeightedMean |= (myMode == STATS_WT_MEAN);
  WeightedMean |= (myMode == STATS_INNER_WTMEAN);
  WeightedMean |= (myMode == STATS_CHI_INNER_WTMEAN);
  if (!dvalue) WeightedMean = FALSE;  // warn the user?

  /* calculating the per-star offset based on the desired weighting scheme */
  M = dM = Nm = W = R = 0;
  if (weight) {
    // the weight value is multiplied by whichever nominal weighting scheme is provided 
    // thus the user should set weight to 1.0 for nominal weight, and 100 for heavy weight (or so)
    // and 0.01 for under-weight
    if (WeightedMean) {
      for (i = ks; i < ke; i++) {
	M  += value[i] * weight[i] / SQ(dvalue[i]);
	W  +=            weight[i] / SQ(dvalue[i]);
	dM += SQ (weight[i] / dvalue[i]);
	R  += weight[i] / SQ(dvalue[i]);
	Nm += 1.0;  
      }	
      Mo  = M / W;
      dMo = sqrt (dM) / R;
    } else {
      for (i = ks; i < ke; i++) {
	M  += value[i] * weight[i];
	W  +=            weight[i];
	dM += dvalue ? SQ (weight[i] * dvalue[i]) : 0.0;
	R  += weight[i];
	Nm += 1.0;  
      }	
      Mo  = M / W;
      dMo = dvalue ? sqrt (dM) / R : NAN;
    }
  } else {
    // NULL weight vector is supplied, revert to standard form (above reverts if weight[i] == 1)
    if (WeightedMean) {
      // weighted by inverse-variance
      for (i = ks; i < ke; i++) {
	M   += value[i] / SQ (dvalue[i]);
	dM  += 1.0 / SQ (dvalue[i]);
	Nm  += 1.0;  
      }	
      Mo = M / dM;
      dMo = sqrt (1.0 / dM);
    } else {
      // pure un-weighted
      for (i = ks; i < ke; i++) {
	M   += value[i];
	dM  += dvalue ? SQ (dvalue[i]) : 0.0;
	Nm  += 1.0;  
      }	
      Mo = M / Nm;
      dMo = dvalue ? sqrt (dM) / Nm : NAN;
    }
  }

 chisq:
  /* find sigma and chisq */
  X2 = dS = 0;
  for (i = ks; i < ke; i++) {
    M  = SQ (value[i] - Mo);
    if (dvalue) {
      dM = SQ (dvalue[i]);
      X2 += M / dM;
    }
    dS += M;
  }
  X2 = dvalue ? X2 / (Nm - 1) : NAN;
  dS = sqrt (dS / (Nm - 1));

  stats[0].mean  = Mo;
  stats[0].Nmeas = Nm;
  stats[0].chisq = X2;
  stats[0].sigma = dS;
  stats[0].error = dMo;

  return (TRUE);
}

int liststats_fit1d (double *value, double *err, double *x, int Npts, StatType *stats, double *dk) {

  double M = 0.0, MZ = 0.0, Z1 = 0.0, Z2 = 0.0, R = 0.0;

  for (int i = 0; i < Npts; i++) {
    double wt = 1.0 / (err[i]*err[i]);
    double V = value[i];
    double Z = x[i];
    M  += V   * wt;
    MZ += V*Z * wt;
    Z1 += Z   * wt;
    Z2 += Z*Z * wt;
    R  += 1.0 * wt;
  }

  double DetInv = (R*Z2 - Z1*Z1);
  double Det = (fabs(DetInv) < -1e6) ? 0.0 : 1.0 / DetInv; // do not allow unstable solutions

  double zp = (M*Z2 - Z1*MZ)*Det;
  double dK = (MZ*R - M*Z1)*Det;

  /* find sigma and chisq */
  double Mo = 0.0, dM = 0.0;
  double X2 = 0.0, dS = 0.0;
  for (int i = 0; i < Npts; i++) {
    Mo = zp + dK*x[i];
    M  = SQ (value[i] - Mo);
    dM = SQ (err[i]);
    X2 += M / dM;
    dS += M;
  }
  X2 = X2 / (Npts - 1);
  dS = sqrt (dS / (Npts - 1));

  *dk = dK;
  stats->mean = zp;
  stats->chisq = X2;
  stats->error = dS;
  return TRUE;
}

// These should probably be tunable:
# define MAX_ITERATIONS 10
# define FIT_TOLERANCE 1e-4
# define FLT_TOLERANCE 1e-6
# define WEIGHT_THRESHOLD 0.3

int fit_least_squares (double *fit, double *err, double *y, double *dy, double *wgt, double *wt, int Npts);

// this is a zero-order fit (constant value only)
int liststats_irls (StatDataSet *dataset, int Npoints, StatType *stats) {

  liststats_init (stats);

  if (Npoints == 0) {
    double value = NAN;
    stats->mean = value;
    stats->min  = value;
    stats->max  = value;
    stats->Nmeas = Npoints;
    stats->chisq = NAN;
    stats->sigma = NAN;
    stats->error = NAN;
    return TRUE;
  }

  if (Npoints == 1) {
    double value = dataset->flxlist[0];
    stats->mean = value;
    stats->min  = value;
    stats->max  = value;
    stats->Nmeas = Npoints;
    stats->chisq = NAN;
    stats->sigma = NAN;
    stats->error = dataset->errlist[0];
    return TRUE;
  }

  int midpt = 0.5 * Npoints;

  // make the initial guess based on the median (not weighted mean)
  for (int i = 0; i < Npoints; i++) {
    dataset->values[i] = dataset->flxlist[i];
  }
  dsort (dataset->values, Npoints);
  double value = (Npoints % 2) ? dataset->values[midpt] : 0.5*(dataset->values[midpt] + dataset->values[midpt-1]);

  // OLS (replace by median above)
  // if (!fit_least_squares (&value, dataset->flxlist, dataset->errlist, dataset->wgtlist, NULL, Npoints)) return FALSE;
  
  int converged = FALSE;
  for (int iterations = 0; !converged && (iterations < MAX_ITERATIONS); iterations++) {
    
    for (int i = 0; i < Npoints; i++) {
      // we are only including the formal error, not the weight in the definition of wt[]
      dataset->wtvals[i] = weight_cauchy ((dataset->flxlist[i] - value) / dataset->errlist[i]);
    }
    
    double oldValue = value;
    if (!fit_least_squares (&value, NULL, dataset->flxlist, dataset->errlist, dataset->wgtlist, dataset->wtvals, Npoints)) {
      value = oldValue;
      break;
    }
    
    converged = TRUE;
    if ((fabs(value - oldValue) > FIT_TOLERANCE * fabs(value)) && 
	(fabs(value - oldValue) > FLT_TOLERANCE)) {
      converged = FALSE;
    }
  }
  stats->mean = value;
      
  // calculate the weight thresholds to mask the bad points:
  for (int i = 0; i < Npoints; i++) {
    dataset->wtvals[i] = weight_cauchy ((dataset->flxlist[i] - value) / dataset->errlist[i]);
    dataset->wtlist[i] = dataset->wtvals[i];
  }
  dsort (dataset->wtlist, Npoints);
  double WtMedian = (Npoints % 2) ? dataset->wtlist[midpt] : 0.5*(dataset->wtlist[midpt] + dataset->wtlist[midpt-1]);
  double WtThreshold = WEIGHT_THRESHOLD * WtMedian;

  // save unmasked points
  int Nkeep = 0;
  stats->min = NAN;
  stats->max = NAN;
  double dChi = 0.0;
  double dSig = 0.0;
  for (int i = 0; i < Npoints; i++) {
    if ((dataset->wtvals[i] < WtThreshold) || !isfinite(dataset->flxlist[i])) {
      dataset->msklist[i] = TRUE; // mark the masked points
      continue;
    }
    dataset-> ykeep[Nkeep] = dataset->flxlist[i];
    dataset->dykeep[Nkeep] = dataset->errlist[i];
    dataset->wtkeep[Nkeep] = dataset->wgtlist[i]; // externally-supplied weight
    Nkeep ++;
    
    // record mean, error, chisq, min, max, sigma, Nmeas (unmasked points)
    stats->min = isfinite(stats->min) ? MIN(dataset->flxlist[i], stats->min) : dataset->flxlist[i];
    stats->max = isfinite(stats->max) ? MAX(dataset->flxlist[i], stats->max) : dataset->flxlist[i];
    double dValue2 = SQ(dataset->flxlist[i] - value);
    dChi += dValue2 / SQ (dataset->errlist[i]);
    dSig += dValue2;
  }
  stats->Nmeas = Nkeep;
  stats->chisq = dChi / (Nkeep - 1);
  stats->sigma = sqrt (dSig / (Nkeep - 1));

  // if percentile ranges are desired, calculate them
  if (stats->statmode & STATS_VARSTATS) {
    // save ykeep values in a vector to be sorted
    for (int i = 0; i < Nkeep; i++) {
      dataset->wtvals[i] = dataset->ykeep[i];
    }
    dsort (dataset->wtvals, Nkeep);

    int N90 = MIN (Nkeep-1, 0.9*Nkeep);
    int N80 = MIN (Nkeep-1, 0.8*Nkeep);
    int N20 = MAX (0, 0.2*Nkeep);
    int N10 = MAX (0, 0.1*Nkeep);

    stats->Upper90 = dataset->wtvals[N90];
    stats->Upper80 = dataset->wtvals[N80];
    stats->Lower20 = dataset->wtvals[N20];
    stats->Lower10 = dataset->wtvals[N10];
    
    int NkeepMid = 0.5 * Nkeep;
    double ClipMedian = (Nkeep % 2) ? dataset->wtlist[NkeepMid] : 0.5*(dataset->wtlist[NkeepMid] + dataset->wtlist[NkeepMid-1]);

    // save ykeep values in a vector to be sorted
    for (int i = 0; i < Nkeep; i++) {
      dataset->wtvals[i] = (dataset->ykeep[i] - ClipMedian) / dataset->dykeep[i];
    }
    dsort (dataset->wtvals, Nkeep);

    stats->Upper90Nsig = dataset->wtvals[N90];
    stats->Upper80Nsig = dataset->wtvals[N80];
    stats->Lower20Nsig = dataset->wtvals[N20];
    stats->Lower10Nsig = dataset->wtvals[N10];
  }

  int Nboot = 0;
  for (int iboot = 0; iboot < NBOOTSTRAP; iboot++) {
    
    // resample
    for (int i = 0; i < Nkeep; i++) {
      // I need to draw Npoints random entries from 'points' with replacement:
      int N = Nkeep * drand48();
      dataset->ysample[i]  = dataset->ykeep[N];
      dataset->dysample[i] = dataset->dykeep[N];
      dataset->wtsample[i] = dataset->wtkeep[N];
    }

    if (!fit_least_squares (&value, NULL, dataset->ysample, dataset->dysample, dataset->wtsample, NULL, Nkeep)) continue;

    dataset->bvalue[Nboot] = value;
    Nboot ++;
  }

  dsort (dataset->bvalue, Nboot);
  
  double Slo = VectorFractionInterpolate (dataset->bvalue, 0.158655, Nboot);
  double Shi = VectorFractionInterpolate (dataset->bvalue, 0.841345, Nboot);
  stats->error = (Shi - Slo) / 2.0;

  // bootstrap can sometimes yield an excessively-optimistic result for the error.  Do not let
  // the reported error be smaller than the formal error 
  double errvalue;
  if (fit_least_squares (&value, &errvalue, dataset->ykeep, dataset->dykeep, dataset->wtkeep, NULL, Nkeep)) {
    stats->error = MAX (stats->error, errvalue);
  }

  return TRUE;
}

// wgt is externally-supplied weight, wt is optional
int fit_least_squares (double *fit, double *err, double *y, double *dy, double *wgt, double *wt, int Npts) { 

  int i;

  double S0 = 0;
  double S1 = 0;

  /* perform linear fit */
  for (i = 0; i < Npts; i++, y++, dy++, wgt++) {
    if (!finite(*y)) continue;

    // wt is optional
    double dY = wt ? wt[i] * (*wgt) / SQ(*dy) : (*wgt) / SQ(*dy);

    S0 +=    dY;
    S1 += *y*dY;
  }
  if (S0 == 0.0) return FALSE;
  *fit = S1  / S0;
  if (err) { *err = 1.0 / S0; }
  return TRUE;
}

// we could define the weight to be the only scale factor:
// \mu      = \sum (value_i * weight_i) / \sum (weight_i)
// \sigma^2 = (1/R) \sum (weight_i^2 \sigma_i^2) 
// R = \sum (weight_i^2)

// or, we could define the weight to be a scale factor times the inverse error:
// \mu      = \sum (value_i * weight_i / sigma_i) / \sum (weight_i)
// \sigma^2 = (1/R) \sum (weight_i^2 \sigma_i^2) 
// R = \sum (weight_i^2)

