# include "data.h"
# define MIN_VAR 1e-8
int periodogram_fm (int argc, char **argv) {
  
  int N;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  int OPTIMAL = FALSE;
  if ((N = get_argument (argc, argv, "-optimal"))) {
    OPTIMAL = TRUE;
    remove_argument (N, &argc, argv);
  }

  int LINEAR = FALSE;
  if ((N = get_argument (argc, argv, "-linear"))) {
    LINEAR = TRUE;
    remove_argument (N, &argc, argv);
  }

  int Nperiods = 1024;
  if ((N = get_argument (argc, argv, "-Nperiods"))) {
    remove_argument (N, &argc, argv);
    Nperiods = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 8) {
    gprint (GP_ERR, "USAGE: periodogram_fm (time) (flux) (dflux) (minP) (maxP) (period) (power)\n");
    return (FALSE);
  }
  
  // XXX allow dflux to be dropped?

  Vector *time, *flux, *dflux, *power, *period;
  if ((time   = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((flux   = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dflux  = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((period = SelectVector (argv[6], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((power  = SelectVector (argv[7], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  double minP = atof(argv[4]);
  double maxP = atof(argv[5]);

  REQUIRE_VECTOR_FLT (time, FALSE); 
  REQUIRE_VECTOR_FLT (flux, FALSE); 
  REQUIRE_VECTOR_FLT (dflux, FALSE); 

  /* find the max baseline, sum the inverse variances */
  double minT = time[0].elements.Flt[0];
  double maxT = time[0].elements.Flt[0];
  opihi_flt Weight = 0;
  int Npt = time[0].Nelements;

  opihi_flt *tv =  time[0].elements.Flt;
  opihi_flt *df = dflux[0].elements.Flt;
  for (int i = 0; i < Npt; i++, tv++, df++) {
    minT = MIN (minT, *tv);
    maxT = MAX (maxT, *tv);
    // skip points with 0.0 error? or add minimum variance?
    Weight += 1.0 / (SQ(*df) + MIN_VAR); // MIN_VAR : added in quadrature to avoid Inf (XXX make this a user parameter?)
  }

  double WeightInv = 1.0 / Weight;
  double dTime = maxT - minT;
  if (dTime == 0) {
    gprint (GP_ERR, "ERROR: time range is zero\n");
    return (FALSE);
  }

  int Np = 0;
  int NP = Nperiods;
  ResetVector (power,  OPIHI_FLT, NP);
  ResetVector (period, OPIHI_FLT, NP);

  // for testing, we are going to write out the terms explicitly 
  // for optimization, some of the trig functions can be calculated more efficiently 
  // by storing cos,sin(w t) and using some recurrence rules

  // maxP = minP * dP^n [n = 0 -- Nperiods-1]
  // log(maxP) = log(minP) + (Nperiods - 1) * log (dP)
  // log(dP) = (log(maxP) - log(minP)) / (Nperiods - 1)
  double dP = LINEAR ? (maxP - minP)/(Nperiods - 1) : pow(10.0, ((log10(maxP) - log10(minP))/(Nperiods - 1)));

  // XXX consider choice of period or frequency step
  double P = minP;
  for (Np = 0; (Np < Nperiods) && (P <= maxP); Np++) {
    double w = 2*M_PI/P;
    
    /* find the period offset tau  */
    tv = time[0].elements.Flt;
    df = dflux[0].elements.Flt;

    double cs = 0.0, sn = 0.0, sn2 = 0.0, cs2 = 0.0;

    for (int i = 0; i < Npt; i++, tv++, df++) {
      opihi_flt wt = WeightInv / (SQ(*df) + MIN_VAR);
      cs  += wt*cos (*tv*w);
      sn  += wt*sin (*tv*w);
      cs2 += wt*cos (*tv*w*2);
      sn2 += wt*sin (*tv*w*2);
    }
    double ytan = sn2 - 2*cs*sn;
    double xtan = cs2 - (SQ(cs) - SQ(sn));
    double tau = 0.5*atan2 (ytan, xtan) / w;
      
    /* find the power at this period */
    tv = time[0].elements.Flt;
    df = dflux[0].elements.Flt;
    opihi_flt *fv = flux[0].elements.Flt;

    // YY = YY_s - Y_s*Y_s
    // CC = CC_s - C_s*C_s
    // SS = SS_s - S_s*S_s
    // YC = YC_s - Y_s*C_s
    // YS = YS_s - Y_s*S_s
    
    double YY_s = 0.0, CC_s = 0.0, SS_s = 0.0, YC_s = 0.0, YS_s = 0.0;
    double Y_s = 0.0, C_s = 0.0, S_s = 0.0;
    for (int i = 0; i < Npt; i++, tv++, fv++, df++) {
      opihi_flt wt = WeightInv / (SQ(*df) + MIN_VAR);
      cs = cos (w*(*tv-tau));
      sn = sin (w*(*tv-tau));

      double wtf = *fv*wt;
      Y_s += wtf;
      C_s += wt*cs;
      S_s += wt*sn;

      YY_s += wtf*(*fv);
      YC_s += wtf*cs;
      YS_s += wtf*sn;

      CC_s += wt*cs*cs;
      SS_s += wt*sn*sn;
    }
    double YY = YY_s - SQ(Y_s);
    double YC = YC_s - Y_s*C_s;
    double YS = YS_s - Y_s*S_s;
    double CC = CC_s - SQ(C_s);
    double SS = SS_s - SQ(S_s);

    double Pa = SQ(YC) / CC;
    double Pb = SQ(YS) / SS;
    double Po = (Pa + Pb) / YY;

    power[0].elements.Flt[Np] = Po;
    period[0].elements.Flt[Np] = P;
    if (Np >= NP) {
      NP += 100;
      REALLOCATE (power[0].elements.Flt, opihi_flt, NP);
      REALLOCATE (period[0].elements.Flt, opihi_flt, NP);
    }

    // double ratio = 1 + 0.1*P/dTime;

    if (VERBOSE) gprint (GP_ERR, "tau: %f, P: %f, ratio: %f, dTime: %f, nextP: %f\n", tau, P, dP, dTime, P*dP);

    if (OPTIMAL) {
      dP = 0.4 * SQ(P) / dTime;
      P += dP;
      continue;
    }

    if (LINEAR) {
      P += dP;
    } else {
      P *= dP;
    }
  }

  ResetVector (power,  OPIHI_FLT, Np);
  ResetVector (period, OPIHI_FLT, Np);
 
  return (TRUE);
}
