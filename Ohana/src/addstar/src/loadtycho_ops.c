# include "addstar.h"
# include "tycho.h"

static short TYCHO_B = 0;
static short TYCHO_V = 0;
static time_t J1990 = 0;
static time_t J2000 = 0;
static time_t J2012 = 0;

int gettycho_setup () {

  J1990 = ohana_date_to_sec ("1990/01/01,00:00:00");
  J2000 = ohana_date_to_sec ("2000/01/01,00:00:00");
  J2012 = ohana_date_to_sec ("2012/01/01,00:00:00");
  NAMED_PHOTCODE (TYCHO_B, "TYCHO_B");
  NAMED_PHOTCODE (TYCHO_V, "TYCHO_V");

  return TRUE;
}

int gettycho_star (Tycho_Stars *star, char *line) {

  int i;

  star[0].flag = FALSE; // has this yet been matched in dvo?
  dvo_average_init (&star[0].average);
  for (i = 0; i < NGROUP; i++) {
    dvo_measure_init (&star[0].measure[i]);
  }

  double jyR = atof (&line[178]); // EpRAm (tycho epoch for RA - 1990.0)
  double jyD = atof (&line[183]); // EpRAm (tycho epoch for DEC - 1990.0)

  time_t TychoEpoch = 0.5*(jyR + jyD) * 365.25 * 86400.0 + J1990;

  // the observed (~1991) position is that at the TychoEpoch
  double Robs = atof (&line[152]);
  double Dobs = atof (&line[165]);
  double Rref = atof (&line[15]); // RAmdeg (mean RA,  epoch J2000)
  double Dref = atof (&line[28]); // DEmdeg (mean DEC, epoch J2000)

  double Rps1, Dps1;

  if (line[13] == 'X') {
    star[0].average.R = Robs; // no valid pm, mean epoch position
    star[0].average.D = Dobs; // 

    star[0].average.dR  = atof (&line[188]) / 1000.0; // e_RAmdeg
    star[0].average.dD  = atof (&line[194]) / 1000.0; // e_DEmdeg

    star[0].average.uR  = NAN;
    star[0].average.uD  = NAN;

    star[0].average.duR = NAN;
    star[0].average.duD = NAN;

    star[0].average.Tmean = TychoEpoch;

    Rref = Robs;
    Rps1 = Robs;
    Dref = Dobs;
    Dps1 = Dobs;
    goto got_positions;
  } 

  star[0].average.dR  = atof (&line[57]) / 1000.0; // e_RAmdeg
  star[0].average.dD  = atof (&line[61]) / 1000.0; // e_DEmdeg
  
  star[0].average.uR  = atof (&line[41]) / 1000.0; // pmRA
  star[0].average.uD  = atof (&line[49]) / 1000.0; // pmDE
  
  star[0].average.duR = atof (&line[65]) / 1000.0; // e_pmRA
  star[0].average.duD = atof (&line[70]) / 1000.0; // e_pmDE
  
  if (line[13] == 'P') {
    
    double Ttycho = 0.5*(jyR + jyD) + 1990.0;

    Rref = Robs + ((2000.0 - Ttycho)/3600.0)*star[0].average.uR*cos(RAD_DEG*Dobs);
    Dref = Dobs + ((2000.0 - Ttycho)/3600.0)*star[0].average.uD;
  }

  Rps1 = Rref + ((2012.0 - 2000.0)/3600.0)*star[0].average.uR*cos(RAD_DEG*Dref);
  Dps1 = Dref + ((2012.0 - 2000.0)/3600.0)*star[0].average.uD;
  
  // use the PS1 epoch for average
  if (USE_PS1_EPOCH) {
    star[0].average.R = Rps1;
    star[0].average.D = Dps1;
    star[0].average.Tmean = J2012;
  } else {
    star[0].average.R = Rref;
    star[0].average.D = Dref;
    star[0].average.Tmean = J2000;
  }

got_positions:
  {
    float  M_B = atof (&line[110]); // BTmag
    float dM_B = atof (&line[117]); // e_BTmag
    float  M_V = atof (&line[123]); // VTmag
    float dM_V = atof (&line[130]); // e_VTmag

    // we have NGROUP measurements (B,V) x (MeanEpoch, J2000, J2012)
    star[0].measure[0].t   = TychoEpoch;
    star[0].measure[1].t   = TychoEpoch;
    star[0].measure[2].t   = J2000;
    star[0].measure[3].t   = J2000;
    star[0].measure[4].t   = J2012;
    star[0].measure[5].t   = J2012;

    star[0].measure[0].R   = Robs;
    star[0].measure[0].D   = Dobs;
    star[0].measure[1].R   = Robs;
    star[0].measure[1].D   = Dobs;

    star[0].measure[2].R   = Rref;
    star[0].measure[2].D   = Dref;
    star[0].measure[3].R   = Rref;
    star[0].measure[3].D   = Dref;

    star[0].measure[4].R   = Rps1;
    star[0].measure[4].D   = Dps1;
    star[0].measure[5].R   = Rps1;
    star[0].measure[5].D   = Dps1;

    for (i = 0; i < NGROUP; i+=2) {
      star[0].measure[i+0].M        =  M_B;
      star[0].measure[i+0].dM       = dM_B;
      star[0].measure[i+0].photcode = TYCHO_B;
      star[0].measure[i+1].M        =  M_V;
      star[0].measure[i+1].dM       = dM_V;
      star[0].measure[i+1].photcode = TYCHO_V;
    }
  }
  return TRUE;
}

int gettycho_sortStars (Tycho_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ Tycho_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].average.R < stars[B].average.R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}
