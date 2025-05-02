# include "dvoshell.h"
# define MIN_VAR 1e-8
# define NPERIODS 50000

static float minP = 0.2;
static float maxP = 200.0;

void PeriodogramSetOptions (float minimumPeriod, float maximumPeriod) {
  if (!isnan(minimumPeriod)) minP = minimumPeriod;
  if (!isnan(maximumPeriod)) maxP = maximumPeriod;
}

double percentileValue (double *values, int Nvalues, double percentile) {

  if (Nvalues <= 0) return NAN;

  if (percentile > 1.0) return NAN;
  if (percentile < 0.0) return NAN;

  int Nm = Nvalues * percentile - 0.5;
  int Np = Nm + 1;

  if (Nm < 0) return NAN;
  if (Np < 0) return NAN;
  if (Np >= Nvalues) return values[0];

  double Pm = (Nm + 0.5) / Nvalues;

  double Pvalue = (percentile - Pm) * (values[Np] - values[Nm]) * Nvalues + values[Nm];
  return Pvalue;
}

// low-level periodogram-fm function
PeriodogramResult *PeriodogramRawFloatingMean (opihi_flt *time, opihi_flt *flux, opihi_flt *dflux, int Npts) {

  /* find the max baseline, sum the inverse variances */
  double minT = time[0];
  double maxT = time[0];
  opihi_flt Weight = 0;

  // generate a sorted list of flux and calculate some percentile ranges
  ALLOCATE_PTR (fluxSort, double, Npts);
  for (int i = 0; i < Npts; i++) {
    fluxSort[i] = flux[i];
  }
  dsort (fluxSort, Npts);
  
  // I want two percentile ranges, e.g.,: 25 - 75, 5 - 95
  double P05 = percentileValue (fluxSort, Npts, 0.05);
  double P25 = percentileValue (fluxSort, Npts, 0.25);
  double P75 = percentileValue (fluxSort, Npts, 0.75);
  double P95 = percentileValue (fluxSort, Npts, 0.95);
  FREE (fluxSort);

  opihi_flt *tv =  time;
  opihi_flt *df = dflux;
  opihi_flt *fv =  flux;

  for (int i = 0; i < Npts; i++, tv++, df++) {
    minT = MIN (minT, *tv);
    maxT = MAX (maxT, *tv);
    // add minimum variance to avoid dividing by 0.0
    Weight += 1.0 / (SQ(*df) + MIN_VAR); // MIN_VAR : added in quadrature to avoid Inf (XXX make this a user parameter?)
  }

  double WeightInv = 1.0 / Weight;
  double dTime = maxT - minT;
  if (dTime == 0) {
    gprint (GP_ERR, "ERROR: time range is zero\n");
    return NULL;
  }

  int Np = 0;
  int NP = NPERIODS;
  ALLOCATE_PTR (period, double, NP);
  ALLOCATE_PTR (power, double, NP);

  // for testing, we are going to write out the terms explicitly 
  // for optimization, some of the trig functions can be calculated more efficiently 
  // by storing cos,sin(w t) and using some recurrence rules

  // maxP = minP * dP^n [n = 0 -- Nperiods-1]
  // log(maxP) = log(minP) + (Nperiods - 1) * log (dP)
  // log(dP) = (log(maxP) - log(minP)) / (Nperiods - 1)
  // double dP = LINEAR ? (maxP - minP)/(NPERIODS - 1) : pow(10.0, ((log10(maxP) - log10(minP))/(NPERIODS - 1)));

  double P = minP;
  for (Np = 0; (Np < NPERIODS) && (P <= maxP); Np++) {
    double w = 2*M_PI/P;
    
    /* find the period offset tau  */
    tv = time;
    df = dflux;

    double cs = 0.0, sn = 0.0, sn2 = 0.0, cs2 = 0.0;

    for (int i = 0; i < Npts; i++, tv++, df++) {
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
    tv = time;
    df = dflux;
    fv =  flux;

    // YY = YY_s - Y_s*Y_s
    // CC = CC_s - C_s*C_s
    // SS = SS_s - S_s*S_s
    // YC = YC_s - Y_s*C_s
    // YS = YS_s - Y_s*S_s
    
    double YY_s = 0.0, CC_s = 0.0, SS_s = 0.0, YC_s = 0.0, YS_s = 0.0;
    double Y_s = 0.0, C_s = 0.0, S_s = 0.0;
    for (int i = 0; i < Npts; i++, tv++, fv++, df++) {
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

    power[Np] = Po;
    period[Np] = P;
    if (Np >= NP) {
      NP += 100;
      REALLOCATE (power, opihi_flt, NP);
      REALLOCATE (period, opihi_flt, NP);
    }

    float dP = 0.4 * SQ(P) / dTime;
    P += dP;
  }

  ALLOCATE_PTR (result, PeriodogramResult, 1);
  result->Nmeasure = Npts;
  result->Nperiods = Np;
  result->period = period;
  result->power = power;

  result->P50 = 0.5*(P75 + P25);
  result->R50 = P75 - P25;
  result->R90 = P95 - P05;
  
  return result;
}

void PeriodogramResultFree (PeriodogramResult *result) {
  if (!result) return;

  FREE (result->period);
  FREE (result->power);
  free (result);
  return;
}

// write the period and power vectors to an extension in this output file
void PeriodogramResultSave (PeriodogramResult *result, char *extname, FILE *foutput, Average *average) {

  // XXX add some metadata to the header:
  // Nmeasurements (add into the results structure), g, r, i, z, y?

  // generate a binary table 
  Header outheader;
  FTable outtable;
  gfits_init_header (&outheader);
  gfits_init_table  (&outtable);
  outtable.header = &outheader;

  gfits_create_table_header (&outheader, "BINTABLE", extname);

  // add some metadata derived from average:
  gfits_modify (&outheader, "RA_OBJ",  "%lf", 1, average->R);
  gfits_modify (&outheader, "DEC_OBJ", "%lf", 1, average->D);
  gfits_modify (&outheader, "OBJ_ID",  "%d", 1, average->objID);
  gfits_modify (&outheader, "CAT_ID",  "%d", 1, average->catID);
  gfits_modify (&outheader, "NMEASURE", "%d", 1, result->Nmeasure);
  gfits_modify (&outheader, "P50", "%lf", 1, result->P50);
  gfits_modify (&outheader, "R50", "%lf", 1, result->R50);
  gfits_modify (&outheader, "R90", "%lf", 1, result->R90);
  gfits_define_bintable_column (&outheader, "D", "PERIOD", NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&outheader, "D", "POWER", NULL, NULL, 1.0, 0.0);

  gfits_create_table (&outheader, &outtable);

  gfits_set_bintable_column (&outheader, &outtable, "PERIOD", result->period, result->Nperiods);
  gfits_set_bintable_column (&outheader, &outtable, "POWER",  result->power,  result->Nperiods);

  // write the actual table data
  gfits_fwrite_Theader (foutput, &outheader);
  gfits_fwrite_table   (foutput, &outtable);
    
  gfits_free_header (&outheader);
  gfits_free_table  (&outtable);
}
