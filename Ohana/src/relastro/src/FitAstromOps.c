# include "relastro.h"

// I am modifying FitPM with an eye to (a) threaded operations and (b) bootstrap resampling tests.

int FitAstromPoints_Project (FitStats *fitStats, double *Tmean, double *Trange, double *parRange) {

  int k;

  int Npoints = fitStats->Npoints;
  FitAstromPoint *points = fitStats->points;

  // find Tmin & Tmax from the list of accepted measurements
  double Tmin  = points[0].T;
  double Tmax  = points[0].T;
  double pRmin = +2.0;
  double pRmax = -2.0;
  double pDmin = +2.0;
  double pDmax = -2.0;

  *Tmean = 0.0;

  double Tsum = 0.0;
  double Wsum = 0.0;
  for (k = 0; k < Npoints; k++) {
    Tmin = MIN(Tmin, points[k].T);
    Tmax = MAX(Tmax, points[k].T);

    float wx = 1.0 / SQ(points[k].dX);

    Tsum += points[k].T * wx;
    Wsum += wx;

    // at this point, T is in years since J2000
    ParFactor (&points[k].pR, &points[k].pD, points[k].R, points[k].D, points[k].T);
    pRmin = MIN (pRmin, points[k].pR);
    pRmax = MAX (pRmax, points[k].pR);
    pDmin = MIN (pDmin, points[k].pD);
    pDmax = MAX (pDmax, points[k].pD);
  }
  *Trange = Tmax - Tmin;

  // mean epoch
  *Tmean = Tsum / Wsum;

  // for HIGH_SPEED, just use the center of the range
  if (RELASTRO_OP == OP_HIGH_SPEED) {
    *Tmean = 0.5*(Tmax - Tmin);
  }

  *parRange = hypot (pRmax - pRmin, pDmax - pDmin);

  /* we need to do the fit in a locally linear space; choose a ref coordinate */
  fitStats->coords.crval1 = points[0].R;
  fitStats->coords.crval2 = points[0].D;

  // project all of the R,D coordinates to a plane centered on this coordinate. set
  // the times to be relative to Tmean
  for (k = 0; k < Npoints; k++) {
    RD_to_XY (&points[k].X, &points[k].Y, points[k].R, points[k].D, &fitStats->coords);
    points[k].T -= *Tmean;
  }	  
  return TRUE;
}

FitStats *FitStatsInit (int Nmax, int Nboot) {

  FitStats *fitStats = NULL;
  ALLOCATE (fitStats, FitStats, 1);

  // counters to record successful fits or failures
  fitStats->Nave = 0;  
  fitStats->Npm = 0;   
  fitStats->Npar = 0;  
  fitStats->Nskip = 0; 
  fitStats->Noffset = 0;

  fitStats->values = NULL; // pre-allocated array for median & robust sigma
  fitStats->fit = NULL; // container to hold the fit results (Nboot > 1 for bootstrap resampling)
  fitStats->Nfit = 0;
  fitStats->NfitAlloc = Nboot;
  if (Nboot > 0) {
    ALLOCATE (fitStats->fit, FitAstromResult, Nboot);
    ALLOCATE (fitStats->values, double, Nboot);
  }

  // containers to hold the measurements for a given star
  fitStats-> Xstack = NULL; // pre-allocated array for median
  fitStats->dXstack = NULL; // pre-allocated array for median
  fitStats-> Ystack = NULL; // pre-allocated array for median
  fitStats->dYstack = NULL; // pre-allocated array for median
  fitStats->points = NULL;
  fitStats->sample = NULL;
  fitStats->nomask = NULL;
  fitStats->Npoints = 0;
  fitStats->NpointsAlloc = Nmax;
  if (Nmax > 0) {
    ALLOCATE (fitStats->points, FitAstromPoint, Nmax);
    ALLOCATE (fitStats->sample, FitAstromPoint, Nmax);
    ALLOCATE (fitStats->nomask, FitAstromPoint, Nmax);
    ALLOCATE (fitStats-> Xstack, double, Nmax);
    ALLOCATE (fitStats->dXstack, double, Nmax);
    ALLOCATE (fitStats-> Ystack, double, Nmax);
    ALLOCATE (fitStats->dYstack, double, Nmax);
  }

  // pre-allocated fit matrices for the 3 fit options
  fitStats->fitdataPos = NULL;
  fitStats->fitdataPM  = NULL;
  fitStats->fitdataPar = NULL;

  if (Nmax > 0) {
    fitStats->fitdataPos = FitAstromDataInit (2);
    fitStats->fitdataPM  = FitAstromDataInit (4);
    fitStats->fitdataPar = FitAstromDataInit (5);
  }

  /* project coordinates to a plane centered on the object with units of arcsec */
  InitCoords (&fitStats->coords, "DEC--SIN");
  fitStats->coords.cdelt1 = fitStats->coords.cdelt2 = 1.0 / 3600.0;

  // use J2000 as a reference time
  fitStats->T2000 = ohana_date_to_sec ("2000/01/01,12:00:00");
  return fitStats;
}

void FitStatsReset (FitStats *tgt) {
  tgt->Nave    = 0;  
  tgt->Npm     = 0;   
  tgt->Npar    = 0;  
  tgt->Nskip   = 0; 
  tgt->Noffset = 0;
  return;
}

void FitStatsSum (FitStats *src, FitStats *tgt) {
  tgt->Nave    += src->Nave    ;  
  tgt->Npm     += src->Npm     ;   
  tgt->Npar    += src->Npar    ;  
  tgt->Nskip   += src->Nskip   ; 
  tgt->Noffset += src->Noffset ;
  return;
}

void FitStatsFree (FitStats *fitStats) {
  if (!fitStats) return;

  FREE (fitStats->fit);
  FREE (fitStats->values);
  FREE (fitStats->points);
  FREE (fitStats->sample);
  FREE (fitStats->nomask);

  FREE (fitStats-> Xstack);
  FREE (fitStats->dXstack);
  FREE (fitStats-> Ystack);
  FREE (fitStats->dYstack);

  FitAstromDataFree (fitStats->fitdataPos);
  FitAstromDataFree (fitStats->fitdataPM);
  FitAstromDataFree (fitStats->fitdataPar);

  free (fitStats);
}

FitAstromData *FitAstromDataInit (int Nterms) {

  FitAstromData *fit = NULL;
  ALLOCATE (fit, FitAstromData, 1);

  /* do I need to do this as 2 2x2 matrix equations? */
  fit->B   = array_init (Nterms, 1);
  fit->A   = array_init (Nterms, Nterms);
  fit->Cov = array_init (Nterms, Nterms);

  ALLOCATE (fit->Beta, double, Nterms);
  ALLOCATE (fit->Beta_prev, double, Nterms);
  fit->Nterms = Nterms;

  fit->getChisq = TRUE;
  fit->getError = TRUE;

  return fit;
}

void FitPointsClearMasks (FitAstromPoint *points, int Npoints) {

  int i;
  for (i = 0; i < Npoints; i++) {
    points[i].mask = FALSE;
  }
  return;
}

void FitAstromDataFree (FitAstromData *fit) {

  if (!fit) return;

  array_free (fit->A, fit->Nterms);
  array_free (fit->B, fit->Nterms);
  array_free (fit->Cov, fit->Nterms);

  free (fit->Beta);
  free (fit->Beta_prev);

  free (fit);
  return;
}

void FitAstromPointInit (FitAstromPoint *object) {
  object->X      = 0.0;
  object->Y      = 0.0;
  object->R      = 0.0;
  object->D      = 0.0;
  object->T      = 0.0;
  object->dX     = 0.0;
  object->dY     = 0.0;
  object->dR     = 0.0;
  object->dD     = 0.0;
  object->dT     = 0.0;
  object->pR     = 0.0;
  object->pD     = 0.0;
  object->C_blue = 0.0;
  object->C_red  = 0.0;
  object->measure= -1;

  object->Wx     = 1.0;
  object->Wy     = 1.0;

  object->Qx     = 0.0;
  object->Qy     = 0.0;
  object->qx     = 0.0;
  object->qy     = 0.0;

  object->rx     = 0.0;
  object->ry     = 0.0;
  object->u      = 0.0;

  object->mask   = 0; // keep point if mask == 0
  return;
}

void FitAstromResultInit (FitAstromResult *fit) {

  fit->Ro  = 0.0;
  fit->dRo = 0.0;
  fit->Do  = 0.0;
  fit->dDo = 0.0;
  fit->uR  = 0.0;
  fit->duR = 0.0;
  fit->uD  = 0.0;
  fit->duD = 0.0;
  fit->p   = 0.0;
  fit->dp  = 0.0;

  fit->chisq = NAN;
  fit->Nfit = 0;
  fit->converged = FALSE;
  
  // this is an input value
  // if true, use the IRLS modified weight
  fit->useWeight = FALSE;

  return;
}

double weight_cauchy (double x) {
  double r = x / 2.385;
  return (1.0 / (1.0 + SQ(r)));
}

// dpsi = (d/dx) (x * weight(x))
double dpsi_cauchy (double x) {
  double r2 = SQ(x / 2.385);
  return ((1.0 - r2) / (SQ(1 + r2)));
}


// median absolute deviation
// MAD = median(abs(x - median(x)))
double MedianAbsDeviation(FitAstromPoint *points, int Npoints) {

  double *x;
  double median = 0.0;
  int i;
  
  ALLOCATE(x, double, Npoints);
  for (i = 0; i < Npoints; i++) {
    x[i] = points[i].u;
  }
  dsort(x, Npoints);

  if (Npoints % 2) {
    median = 0.5*(x[(int)(0.5*Npoints)] + x[(int)(0.5*Npoints) - 1]);
  } else {
    median = x[(int)(0.5*Npoints)];
  }

  for (i = 0; i < Npoints; i++ ) {
    x[i] = fabs(x[i] - median);
  }
  dsort(x, Npoints);

  if (Npoints % 2) {
    median = 0.5*(x[(int)(0.5*Npoints)] + x[(int)(0.5*Npoints) - 1]);
  } else {
    median = x[(int)(0.5*Npoints)];
  }
  free (x);

  return median;
}
