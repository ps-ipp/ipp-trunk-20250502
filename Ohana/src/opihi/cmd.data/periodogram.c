# include "data.h"

int periodogram (int argc, char **argv) {
  
  int i, N, Npt, Np, NP, VERBOSE;
  opihi_flt *tv, *fv;
  float minP, maxP, minT, maxT, dTime;
  float mean, var, w, tau, P, Pc, Ps, Po;
  float C, S, cs, sn, cs2, sn2, ratio;
  Vector *time, *flux, *power, *period;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: periodogram (time) (flux) (minP) (maxP) (period) (power)\n");
    return (FALSE);
  }
  
  if ((time = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((flux = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  minP = atof(argv[3]);
  maxP = atof(argv[4]);
  if ((period = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((power = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (time, FALSE); 
  REQUIRE_VECTOR_FLT (flux, FALSE); 

  /* find the max baseline, mean, and variance */
  minT = maxT = time[0].elements.Flt[0];
  Npt = time[0].Nelements;
  tv = time[0].elements.Flt;
  fv = flux[0].elements.Flt;
  mean = var = 0;
  for (i = 0; i < Npt; i++, tv++, fv++) {
    minT = MIN (minT, *tv);
    maxT = MAX (maxT, *tv);
    mean += *fv;
  }
  mean = mean / Npt;
  fv = flux[0].elements.Flt;
  for (i = 0; i < Npt; i++, fv++) {
    var += SQ(*fv - mean);
  }
  var = var / (Npt - 1);

  if (VERBOSE) gprint (GP_ERR, "mean: %f, var: %f, minT: %f, maxT: %f\n", mean, var, minT, maxT);

  dTime = maxT - minT;
  if (dTime == 0) {
    gprint (GP_ERR, "ERROR: time range is zero\n");
    return (FALSE);
  }

  Np = 0;
  NP = 100;
  ResetVector (power,  OPIHI_FLT, NP);
  ResetVector (period, OPIHI_FLT, NP);

  P = minP;
  while (P < maxP) {
    w = 2*M_PI/P;
    
    /* find the period offset tau  */
    tv = time[0].elements.Flt;
    cs = sn = 0;
    for (i = 0; i < Npt; i++, tv++) {
      cs += cos (*tv*w*2);
      sn += sin (*tv*2*2);
    }
    tau = 0.5*atan2 (sn, cs) / w;
      
    /* find the power at this period */
    tv = time[0].elements.Flt;
    fv = flux[0].elements.Flt;
    cs = sn = cs2 = sn2 = 0;
    for (i = 0; i < Npt; i++, tv++, fv++) {
      C = cos (w*(*tv-tau));
      S = sin (w*(*tv-tau));
      // C = cos (w**tv);
      // S = sin (w**tv);
      cs += (*fv - mean) * C;
      sn += (*fv - mean) * S;
      cs2 += SQ(C);
      sn2 += SQ(S);
    }
    Pc = SQ(cs) / cs2;
    Ps = SQ(sn) / sn2;
    Po = (Pc + Ps) / (2*var);

    power[0].elements.Flt[Np] = Po;
    period[0].elements.Flt[Np] = P;
    Np ++;
    if (Np >= NP) {
      NP += 100;
      REALLOCATE (power[0].elements.Flt, opihi_flt, NP);
      REALLOCATE (period[0].elements.Flt, opihi_flt, NP);
    }

    ratio = 1 + 0.1*P/dTime;

    if (VERBOSE) gprint (GP_ERR, "tau: %f, P: %f, ratio: %f, dTime: %f, nextP: %f\n", tau, P, ratio, dTime, P*ratio);

    P *= ratio;
  }

  ResetVector (power,  OPIHI_FLT, Np);
  ResetVector (period, OPIHI_FLT, Np);
 
  return (TRUE);
}
