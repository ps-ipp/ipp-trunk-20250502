# include "addstar.h"
# include "bsc.h"

static short BSC_U = 0;
static short BSC_B = 0;
static short BSC_V = 0;
static time_t J2000 = 0;
static time_t J2012 = 0;

int getbsc_setup () {

  J2000 = ohana_date_to_sec ("2000/01/01,00:00:00");
  J2012 = ohana_date_to_sec ("2012/01/01,00:00:00");
  NAMED_PHOTCODE (BSC_U, "BSC_U");
  NAMED_PHOTCODE (BSC_B, "BSC_B");
  NAMED_PHOTCODE (BSC_V, "BSC_V");

  return TRUE;
}

int getbsc_star (BSC_Stars *star, char *line) {

  int i;

  star[0].flag = FALSE; // has this yet been matched in dvo?
  dvo_average_init (&star[0].average);
  for (i = 0; i < NMEAS_MAX; i++) {
    dvo_measure_init (&star[0].measure[i]);
  }

  // the reported position is at the J2000 epoch

  double Robs, Dobs;
  dparse (&Robs, 1, line);
  dparse (&Dobs, 2, line);

  double uR, uD, plx;
  dparse (&uR,  11, line);
  dparse (&uD,  12, line);
  dparse (&plx, 13, line);

  double V, UB, BV;
  dparse (&V,   15, line);
  dparse (&UB,  16, line);
  dparse (&BV,  17, line);
  
  star[0].average.dR  = 0.75; // hard-wired to 750 mas, 
  star[0].average.dD  = 0.75; // hard-wired to 750 mas, 
  
  star[0].average.uR  = uR;
  star[0].average.uD  = uD;
  
  star[0].average.duR = 0.010;
  star[0].average.duD = 0.010;
  
  star[0].average.P   = plx;
  star[0].average.dP  = 0.020;

  double Rps1 = Robs + ((2012.0 - 2000.0)/3600.0)*star[0].average.uR*cos(RAD_DEG*Dobs);
  double Dps1 = Dobs + ((2012.0 - 2000.0)/3600.0)*star[0].average.uD;
  
  static time_t MeanEpoch;

  // use the PS1 epoch for average
  if (USE_PS1_EPOCH) {
    star[0].average.R = Rps1;
    star[0].average.D = Dps1;
    star[0].average.Tmean = J2012;
    MeanEpoch = J2012;
  } else {
    star[0].average.R = Robs;
    star[0].average.D = Dobs;
    star[0].average.Tmean = J2000;
    MeanEpoch = J2000;
  }

  // V must exist, but UB and BV may NAN (if BV is NAN, skip U as well)
  star[0].measure[0].t        = MeanEpoch;
  star[0].measure[0].R        = Robs;
  star[0].measure[0].D        = Dobs;
  star[0].measure[0].M        = V;
  star[0].measure[0].dM       = 0.05;
  star[0].measure[0].photcode = BSC_V;
  star[0].Nmeasure = 1;

  if (isfinite(BV)) {
    star[0].measure[1].t        = MeanEpoch;
    star[0].measure[1].R        = Robs;
    star[0].measure[1].D        = Dobs;
    star[0].measure[1].M        = V + BV;
    star[0].measure[1].dM       = 0.05;
    star[0].measure[1].photcode = BSC_B;
    star[0].Nmeasure ++;

    if (isfinite(UB)) {
      star[0].measure[2].t        = MeanEpoch;
      star[0].measure[2].R        = Robs;
      star[0].measure[2].D        = Dobs;
      star[0].measure[2].M        = V + BV + UB;
      star[0].measure[2].dM       = 0.05;
      star[0].measure[2].photcode = BSC_U;
      star[0].Nmeasure ++;
    }
  }
  return TRUE;
}

int getbsc_sortStars (BSC_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ BSC_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].average.R < stars[B].average.R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}
