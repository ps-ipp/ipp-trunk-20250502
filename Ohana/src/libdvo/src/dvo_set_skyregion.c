# include "ohana.h"

// functions to set and get the current skyregion.
static double RAs = 0.0;
static double RAe = 0.0;
static double DECs = 0.0;
static double DECe = 0.0;

int get_skyregion (double *Rs, double *Re, double *Ds, double *De) {
  *Rs = RAs;
  *Re = RAe;
  *Ds = DECs;
  *De = DECe;

  return TRUE;
}

int set_skyregion (double Rs, double Re, double Ds, double De) {
  RAs  = Rs;
  RAe  = Re;
  DECs = Ds;
  DECe = De;

  return TRUE;
}
