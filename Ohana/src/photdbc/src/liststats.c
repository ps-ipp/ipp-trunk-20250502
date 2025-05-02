# include "photdbc.h"

enum {M_MEAN, M_MEDIAN, M_WT_MEAN, M_INNER_MEAN, 
      M_INNER_WTMEAN, M_CHI_INNER_MEAN, M_CHI_INNER_WTMEAN};

static int statmode;

void initstats (char *mode) {

  statmode = -1;
  if (!strcmp (mode, "MEAN")) statmode = M_MEAN;
  if (!strcmp (mode, "MEDIAN")) statmode = M_MEDIAN;
  if (!strcmp (mode, "WT_MEAN")) statmode = M_WT_MEAN;
  if (!strcmp (mode, "INNER_MEAN")) statmode = M_INNER_MEAN;
  if (!strcmp (mode, "INNER_WTMEAN")) statmode = M_INNER_WTMEAN;
  if (!strcmp (mode, "CHI_INNER_MEAN")) statmode = M_CHI_INNER_MEAN;
  if (!strcmp (mode, "CHI_INNER_WTMEAN")) statmode = M_CHI_INNER_WTMEAN;

  if (statmode == -1) {
    fprintf (stderr, "ERROR: invalid stats mode: %s\n", mode);
    exit (1);
  }
}

int liststats (double *value, double *dvalue, int N, StatType *stats) {
  
  int i, ks, ke, Nm;
  double Mo, dMo, M, dM, X2, dS, *chi;

  stats[0].Nmeas = N;
  stats[0].mean  = 0;
  if (N < 2) return (FALSE);

  dsortpair (value, dvalue, N);
  stats[0].median = value[(int)(0.5*N)];
  stats[0].min    = value[0];
  stats[0].max    = value[N-1];

  switch (statmode) {
  case M_MEDIAN:
    ks = 0;
    ke = N;
    Mo = stats[0].median;
    Nm = N;
    goto chisq;
    break;
  case M_MEAN:
  case M_WT_MEAN:
    ks = 0;
    ke = N;
    break;
  case M_INNER_MEAN:
  case M_INNER_WTMEAN:
  case M_CHI_INNER_MEAN:
  case M_CHI_INNER_WTMEAN:
    ks = 0.25*N + 0.50;
    ke = 0.75*N + 0.25;
    if (N <= 3) {
      ks = 0;
      ke = N;
    }
    break;
  }    

  if ((statmode == M_CHI_INNER_MEAN) || (statmode == M_CHI_INNER_WTMEAN)) {
    ALLOCATE (chi, double, N);
    for (i = 0; i < N; i++) {
      chi[i] = (value[i] - stats[0].median) / dvalue[i];
    }
    dsortthree (chi, value, dvalue, N);
    free (chi);
  }

  /* calculating the per-star offset based on the weighted average */
  M = dM = Nm = 0;
  if ((statmode == M_WT_MEAN) || (statmode == M_INNER_WTMEAN) || (statmode == M_CHI_INNER_WTMEAN)) {
    for (i = ks; i < ke; i++) {
      M   += value[i] / SQ (dvalue[i]);
      dM  += 1.0 / SQ (dvalue[i]);
      Nm  ++;  
    }	
    Mo = M / dM;
    dMo = sqrt (1.0 / dM);
  } else {
    for (i = ks; i < ke; i++) {
      M   += value[i];
      dM  += SQ (dvalue[i]);
      Nm  ++;  
    }	
    Mo = M / (double) Nm;
    dMo = sqrt (dM / (double) Nm);
  }

 chisq:
  /* find sigma and chisq */
  X2 = dS = 0;
  for (i = ks; i < ke; i++) {
    M  = SQ (value[i] - Mo);
    dM = SQ (dvalue[i]);
    X2 += M / dM;
    dS += M;
  }
  X2 = X2 / Nm;
  dS = sqrt (dS / Nm);

  stats[0].mean  = Mo;
  stats[0].Nmeas = Nm;
  stats[0].chisq = X2;
  stats[0].sigma = dS;
  stats[0].error = dMo;

  return (TRUE);
}

