# include "relphot.h"
# define DISABLE_CORNER_GPC1 0

int isMosaicChip (int photcode) {

  if (isGPC1chip(photcode)) return TRUE;
  if (isGPC2chip(photcode)) return TRUE;
  if (isHSCchip(photcode))  return TRUE;
  if (isCFHchip(photcode))  return TRUE; // this is megacam
  return FALSE;
}

// for now (20140710) I need to identify gpc1 chips explicitly.  generalize in the future
// note that the 4000, 14000, 15000 sets are SIMTEST (*not* synthetic) 
int whichGPC1filter (int photcode) {

# if (DISABLE_CORNER_GPC1)
  // disable the corner chips:
  if (((photcode >  10000) && (photcode <  10077)) || (photcode == 4100)) return PS1_g; // g-band 
  if (((photcode >  10100) && (photcode <  10177)) || (photcode == 4200)) return PS1_r; // r-band
  if (((photcode >  10200) && (photcode <  10277)) || (photcode == 4300)) return PS1_i; // i-band
  if (((photcode >  10300) && (photcode <  10377)) || (photcode == 4400)) return PS1_z; // z-band
  if (((photcode >  10400) && (photcode <  10477)) || (photcode == 4500)) return PS1_y; // y-band
  if (((photcode >  10500) && (photcode <  10577)) || (photcode == 4600)) return PS1_w; // w-band
# else
  // for testing, enable the corner chips:
  if (((photcode >= 10000) && (photcode <= 10077)) || (photcode == 4100)) return PS1_g; // g-band 
  if (((photcode >= 10100) && (photcode <= 10177)) || (photcode == 4200)) return PS1_r; // r-band
  if (((photcode >= 10200) && (photcode <= 10277)) || (photcode == 4300)) return PS1_i; // i-band
  if (((photcode >= 10300) && (photcode <= 10377)) || (photcode == 4400)) return PS1_z; // z-band
  if (((photcode >= 10400) && (photcode <= 10477)) || (photcode == 4500)) return PS1_y; // y-band
  if (((photcode >= 10500) && (photcode <= 10577)) || (photcode == 4600)) return PS1_w; // w-band
# endif
  return PS1_none;
}

// for now (20140710) I need to identify gpc1 chips explicitly.  generalize in the future
int isGPC1chip (int photcode) {

# if (DISABLE_CORNER_GPC1)
  if (((photcode >  10000) && (photcode <  10077)) || (photcode == 4100)) return TRUE; // g-band
  if (((photcode >  10100) && (photcode <  10177)) || (photcode == 4200)) return TRUE; // r-band
  if (((photcode >  10200) && (photcode <  10277)) || (photcode == 4300)) return TRUE; // i-band
  if (((photcode >  10300) && (photcode <  10377)) || (photcode == 4400)) return TRUE; // z-band
  if (((photcode >  10400) && (photcode <  10477)) || (photcode == 4500)) return TRUE; // y-band
  if (((photcode >  10500) && (photcode <  10577)) || (photcode == 4600)) return TRUE; // w-band
# else
  if (((photcode >= 10000) && (photcode <= 10077)) || (photcode == 4100)) return TRUE; // g-band
  if (((photcode >= 10100) && (photcode <= 10177)) || (photcode == 4200)) return TRUE; // r-band
  if (((photcode >= 10200) && (photcode <= 10277)) || (photcode == 4300)) return TRUE; // i-band
  if (((photcode >= 10300) && (photcode <= 10377)) || (photcode == 4400)) return TRUE; // z-band
  if (((photcode >= 10400) && (photcode <= 10477)) || (photcode == 4500)) return TRUE; // y-band
  if (((photcode >= 10500) && (photcode <= 10577)) || (photcode == 4600)) return TRUE; // w-band
# endif

  return FALSE;
}

// for now, I need to identify gpc2 chips explicitly.  generalize in the future
int isGPC2chip (int photcode) {

  if ((photcode >= 30000) && (photcode <= 30077)) return TRUE; // g-band
  if ((photcode >= 30100) && (photcode <= 30177)) return TRUE; // r-band
  if ((photcode >= 30200) && (photcode <= 30277)) return TRUE; // i-band
  if ((photcode >= 30300) && (photcode <= 30377)) return TRUE; // z-band
  if ((photcode >= 30400) && (photcode <= 30477)) return TRUE; // y-band
  if ((photcode >= 30500) && (photcode <= 30577)) return TRUE; // w-band

  return FALSE;
}

// for now (20140710) I need to identify gpc1 stacks explicitly.  generalize in the future
int isGPC1stack (int photcode) {

  if ((photcode == 11000) || (photcode == 14100)) return TRUE; // g-band
  if ((photcode == 11100) || (photcode == 14200)) return TRUE; // r-band
  if ((photcode == 11200) || (photcode == 14300)) return TRUE; // i-band
  if ((photcode == 11300) || (photcode == 14400)) return TRUE; // z-band
  if ((photcode == 11400) || (photcode == 14500)) return TRUE; // y-band
  if ((photcode == 11500) || (photcode == 14600)) return TRUE; // w-band

  return FALSE;
}

// for now (20140710) I need to identify gpc1 stacks explicitly.  generalize in the future
int isGPC1warp (int photcode) {

  // diff warps get stack-like photcodes (kind of lame)
  if (IS_DIFF_DB) {
    if ((photcode == 11000) || (photcode == 14100)) return TRUE; // g-band
    if ((photcode == 11100) || (photcode == 14200)) return TRUE; // r-band
    if ((photcode == 11200) || (photcode == 14300)) return TRUE; // i-band
    if ((photcode == 11300) || (photcode == 14400)) return TRUE; // z-band
    if ((photcode == 11400) || (photcode == 14500)) return TRUE; // y-band
    if ((photcode == 11500) || (photcode == 14600)) return TRUE; // w-band
    return FALSE;
  }

  if ((photcode == 12000) || (photcode == 15100)) return TRUE; // g-band
  if ((photcode == 12100) || (photcode == 15200)) return TRUE; // r-band
  if ((photcode == 12200) || (photcode == 15300)) return TRUE; // i-band
  if ((photcode == 12300) || (photcode == 15400)) return TRUE; // z-band
  if ((photcode == 12400) || (photcode == 15500)) return TRUE; // y-band
  if ((photcode == 12500) || (photcode == 15600)) return TRUE; // w-band

  return FALSE;
}

int isGPC1synth (int photcode) {

  if ((photcode >= 3001) && (photcode <= 3006)) return TRUE; // g-band

  return FALSE;
}

int is2MASS (int photcode) {

  if ((photcode >= 2011) && (photcode <= 2013)) return TRUE;
  return FALSE;
}

int isTYCHO (int photcode) {

  if ((photcode == 2020) || (photcode == 2021)) return TRUE;
  return FALSE;
}

// for now (20160925) I need to identify HSC chips explicitly.  generalize in the future
int isHSCchip (int photcode) {

  if ((photcode >= 20000) && (photcode <= 20111)) return TRUE; // g-band
  if ((photcode >= 21000) && (photcode <= 21111)) return TRUE; // r-band
  if ((photcode >= 22000) && (photcode <= 22111)) return TRUE; // i-band
  if ((photcode >= 23000) && (photcode <= 23111)) return TRUE; // z-band
  if ((photcode >= 24000) && (photcode <= 24111)) return TRUE; // y-band

  return FALSE;
}

// for now (20160925) I need to identify CFH chips explicitly.  generalize in the future
int isCFHchip (int photcode) {

  if ((photcode >= 100) && (photcode <= 152)) return TRUE; // g-band
  if ((photcode >= 200) && (photcode <= 252)) return TRUE; // r-band
  if ((photcode >= 300) && (photcode <= 352)) return TRUE; // i-band
  if ((photcode >= 400) && (photcode <= 452)) return TRUE; // z-band
  if ((photcode >= 500) && (photcode <= 552)) return TRUE; // y-band

  return FALSE;
}

double weight_cauchy (double x) {
  double r = x / 2.385;
  return (1.0 / (1.0 + SQ(r)));
}

double VectorFractionInterpolate (double *values, float fraction, int Npts) {

  float F = fraction * Npts;
  int   N = fraction * Npts;

  if (N < 0        ) return NAN;
  if (N >= Npts - 2) return NAN;

  // interpolate between N,N+1
    
  double S = (F - N) * (values[N+1] - values[N]) + values[N];
  return S;
}
