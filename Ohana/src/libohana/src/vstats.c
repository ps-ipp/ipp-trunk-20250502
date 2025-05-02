# include <ohana.h>

int vstats_setmode (VStatsType *stats, char *mode) {

  stats->statmode = VSTATS_NONE;
  if (!strcmp (mode, "MEAN"))             { stats->statmode = VSTATS_MEAN; return TRUE; }
  if (!strcmp (mode, "MEDIAN"))           { stats->statmode = VSTATS_MEDIAN; return TRUE; }
  if (!strcmp (mode, "WT_MEAN"))          { stats->statmode = VSTATS_WT_MEAN; return TRUE; }
  if (!strcmp (mode, "INNER_MEAN"))       { stats->statmode = VSTATS_INNER_MEAN; return TRUE; }
  if (!strcmp (mode, "INNER_WTMEAN"))     { stats->statmode = VSTATS_INNER_WTMEAN; return TRUE; }
  if (!strcmp (mode, "CHI_INNER_MEAN"))   { stats->statmode = VSTATS_CHI_INNER_MEAN; return TRUE; }
  if (!strcmp (mode, "CHI_INNER_WTMEAN")) { stats->statmode = VSTATS_CHI_INNER_WTMEAN; return TRUE; }

  push_error ("ERROR: invalid stats mode");
  return FALSE;
}

// NOTE: this function resorts the input vectors.  if this is not what you want, you need
// to either pass a vector copy or save the sequence
int vstats_getstats (double *value, double *dvalue, double *weight, int N, VStatsType *stats) {
  
  int i, ks, ke;
  double Mo, dMo, M, dM, Nm, X2, dS, R, W, *chi;

  myAssert (stats->statmode != VSTATS_NONE, "programming error, liststats mode not set");

  ke = ks = dMo = 0;

  stats[0].Nmeas   = N;
  stats[0].mean    = NAN;
  stats[0].median  = NAN;
  stats[0].sigma   = NAN;
  stats[0].error   = NAN;
  stats[0].chisq   = NAN;
  stats[0].min     = NAN;
  stats[0].max     = NAN;
  stats[0].Upper80 = NAN;
  stats[0].Lower20 = NAN;
  stats[0].total   = NAN;

  // take care of some ridiculous special cases
  if (N < 1) return FALSE;

  if (N == 1) {
    stats[0].mean    = value[0];
    stats[0].median  = value[0];
    stats[0].sigma   = NAN;
    stats[0].error   = dvalue ? dvalue[0] : NAN;
    stats[0].chisq   = NAN;
    stats[0].min     = value[0];
    stats[0].max     = value[0];
    stats[0].Upper80 = value[0];
    stats[0].Lower20 = value[0];
    stats[0].total   = value[0];
    return TRUE;
  }

  // for less than 5 entries, this is equivalent to max,min
  int N80 = MIN (N-1, 0.8*N);
  int N20 = MAX (  0, 0.2*N);

  if (weight && dvalue) {
    dsortthree (value, dvalue, weight, N);
  } else {
    if (dvalue) {
      dsortpair (value, dvalue, N);
    } else if (weight) {
      dsortpair (value, weight, N);
    } else {
      dsort (value, N);
    }
  }

  // these values do not depend on the errors or weighting scheme
  if (N % 2) {
    stats[0].median  = value[(int)(N/2)];
  } else {
    stats[0].median  = 0.5*(value[(int)(N/2)] + value[(int)(N/2) - 1]);
  }
  stats[0].min     = value[0];
  stats[0].max     = value[N-1];
  stats[0].Upper80 = value[N80];
  stats[0].Lower20 = value[N20];

  switch (stats->statmode) {
    case VSTATS_MEDIAN:
      ks = 0;
      ke = N;
      Mo = stats[0].median;
      Nm = N;
      goto chisq;
      break;
    case VSTATS_MEAN:
    case VSTATS_WT_MEAN:
      ks = 0;
      ke = N;
      break;
    case VSTATS_INNER_MEAN:
    case VSTATS_INNER_WTMEAN:
    case VSTATS_CHI_INNER_MEAN:
    case VSTATS_CHI_INNER_WTMEAN:
      ks = 0.25*N + 0.50;
      ke = 0.75*N + 0.25;
      if (N <= 3) {
	ks = 0;
	ke = N;
      }
      break;
    case VSTATS_NONE:
      myAbort ("undefined stats");
  }    

  // for these two modes, I need a vector of the chi-square contribution
  // I'm actually just using chisq to get the correct sorting order
  if ((stats->statmode == VSTATS_CHI_INNER_MEAN) || (stats->statmode == VSTATS_CHI_INNER_WTMEAN)) {
    if (!dvalue) myAbort ("invalid combination: Chisq-based mean, no dvalue provided");
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
  WeightedMean |= (stats->statmode == VSTATS_WT_MEAN);
  WeightedMean |= (stats->statmode == VSTATS_INNER_WTMEAN);
  WeightedMean |= (stats->statmode == VSTATS_CHI_INNER_WTMEAN);

  if (WeightedMean && !dvalue) myAbort ("invalid combination: weighted mean, no dvalue provided");

  /* calculate stats based on the desired weighting scheme */
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
    dM = dvalue ? SQ (dvalue[i]) : 1.0;
    X2 += M / dM;
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

// NOTE: this function resorts the input vectors.  if this is not what you want, you need
// to either pass a vector copy or save the sequence
int vstats_getstats_f (float *value, float *dvalue, float *weight, int N, VStatsType *stats) {
  
  int i, ks, ke;
  float Mo, dMo, M, dM, Nm, X2, dS, R, W, *chi;

  myAssert (stats->statmode != VSTATS_NONE, "programming error, liststats mode not set");

  ke = ks = dMo = 0;

  stats[0].Nmeas   = N;
  stats[0].mean    = NAN;
  stats[0].median  = NAN;
  stats[0].sigma   = NAN;
  stats[0].error   = NAN;
  stats[0].chisq   = NAN;
  stats[0].min     = NAN;
  stats[0].max     = NAN;
  stats[0].Upper80 = NAN;
  stats[0].Lower20 = NAN;
  stats[0].total   = NAN;

  // take care of some ridiculous special cases
  if (N < 1) return FALSE;

  if (N == 1) {
    stats[0].mean    = value[0];
    stats[0].median  = value[0];
    stats[0].sigma   = NAN;
    stats[0].error   = dvalue ? dvalue[0] : NAN;
    stats[0].chisq   = NAN;
    stats[0].min     = value[0];
    stats[0].max     = value[0];
    stats[0].Upper80 = value[0];
    stats[0].Lower20 = value[0];
    stats[0].total   = value[0];
    return TRUE;
  }

  // for less than 5 entries, this is equivalent to max,min
  int N80 = MIN (N-1, 0.8*N);
  int N20 = MAX (  0, 0.2*N);

  if (weight && dvalue) {
    fsortthree (value, dvalue, weight, N);
  } else {
    if (dvalue) {
      fsortpair (value, dvalue, N);
    } else if (weight) {
      fsortpair (value, weight, N);
    } else {
      fsort (value, N);
    }
  }

  // these values do not depend on the errors or weighting scheme
  if (N % 2) {
    stats[0].median  = value[(int)(N/2)];
  } else {
    stats[0].median  = 0.5*(value[(int)(N/2)] + value[(int)(N/2) - 1]);
  }
  stats[0].min     = value[0];
  stats[0].max     = value[N-1];
  stats[0].Upper80 = value[N80];
  stats[0].Lower20 = value[N20];

  switch (stats->statmode) {
    case VSTATS_MEDIAN:
      ks = 0;
      ke = N;
      Mo = stats[0].median;
      Nm = N;
      goto chisq;
      break;
    case VSTATS_MEAN:
    case VSTATS_WT_MEAN:
      ks = 0;
      ke = N;
      break;
    case VSTATS_INNER_MEAN:
    case VSTATS_INNER_WTMEAN:
    case VSTATS_CHI_INNER_MEAN:
    case VSTATS_CHI_INNER_WTMEAN:
      ks = 0.25*N + 0.50;
      ke = 0.75*N + 0.25;
      if (N <= 3) {
	ks = 0;
	ke = N;
      }
      break;
    case VSTATS_NONE:
      myAbort ("undefined stats");
  }    

  // for these two modes, I need a vector of the chi-square contribution
  // I'm actually just using chisq to get the correct sorting order
  if ((stats->statmode == VSTATS_CHI_INNER_MEAN) || (stats->statmode == VSTATS_CHI_INNER_WTMEAN)) {
    if (!dvalue) myAbort ("invalid combination: Chisq-based mean, no dvalue provided");
    ALLOCATE (chi, float, N);
    for (i = 0; i < N; i++) {
      chi[i] = (value[i] - stats[0].median) / dvalue[i];
    }
    if (weight) {
      fsortfour (chi, value, dvalue, weight, N);
    } else {
      fsortthree (chi, value, dvalue, N);
    }
    free (chi);
  }

  int WeightedMean = FALSE;
  WeightedMean |= (stats->statmode == VSTATS_WT_MEAN);
  WeightedMean |= (stats->statmode == VSTATS_INNER_WTMEAN);
  WeightedMean |= (stats->statmode == VSTATS_CHI_INNER_WTMEAN);

  if (WeightedMean && !dvalue) myAbort ("invalid combination: weighted mean, no dvalue provided");

  /* calculate stats based on the desired weighting scheme */
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
    dM = dvalue ? SQ (dvalue[i]) : 1.0;
    X2 += M / dM;
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

// we could define the weight to be the only scale factor:
// \mu      = \sum (value_i * weight_i) / \sum (weight_i)
// \sigma^2 = (1/R) \sum (weight_i^2 \sigma_i^2) 
// R = \sum (weight_i^2)

// or, we could define the weight to be a scale factor times the inverse error:
// \mu      = \sum (value_i * weight_i / sigma_i) / \sum (weight_i)
// \sigma^2 = (1/R) \sum (weight_i^2 \sigma_i^2) 
// R = \sum (weight_i^2)

// 
