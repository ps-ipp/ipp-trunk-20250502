# include "addstar.h"
# include "loadICRF.h"

// ICRF table is ASCII text, with the following interesting columns (fixed bytes)
//   0       : type (C - calibrator, N - non-calibrator, U - unreliable)
//   3 -  21 : names (make into an extID?)
//  24 -  39 : RA (ICRF)
//  40 -  55 : DEC (ICRF)
//  57 -  62 : dR (mas) [use dX,dY in milliarcsec 'pixels']
//  64 -  69 : dD (mas)
//  79 -  84 : Number of observations used (put in dt?)
//  87 -  92 : S-band total flux density integrated over entire map,  Jy  (note 87 may be <) : 
//  94 -  99 : S-band unresolved flux density at long VLBA baselines, Jy
// 102 - 107 : C-band total flux density integrated over entire map,  Jy
// 109 - 114 : C-band unresolved flux density at long VLBA baselines, Jy
// 117 - 122 : X-band total flux density integrated over entire map,  Jy
// 124 - 129 : X-band unresolved flux density at long VLBA baselines, Jy
// 132 - 137 : U-band total flux density integrated over entire map,  Jy
// 139 - 144 : U-band unresolved flux density at long VLBA baselines, Jy
// 147 - 152 : K-band total flux density integrated over entire map,  Jy
// 154 - 159 : K-band unresolved flux density at long VLBA baselines, Jy

// I'm going to save these are FluxPSF (total), FluxAp (unresolved), and mags for 
// the signficant ones (AB_m from Jy).  Use new photcodes to represent C,X,U,K

/* 
#  IVS name J2000 name  Right ascension Declination        D_alp  D_Del   Corr    #Obs  S-band flux    C-band Flux    X-band Flux   U-band flux    K-band Flux    Type Cat
#                                                                                       Total  Unres   Total  Unres   Total  Unres  Total  Unres   Total  Unres
#                       hr mn seconds   deg mn seconds      mas    mas                   Jy     Jy      Jy     Jy      Jy     Jy     Jy     Jy      Jy     Jy
C  2357+080 J0000+0816  00 00 07.031141 +08 16 45.05175    0.46   0.85   0.758     41  -1.00  -1.00   -1.00  -1.00    0.020 <0.014  -1.00  -1.00   -1.00  -1.00   X    rfc_2014c
01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
0         1         2         3         4         5         6         7         8         9         0         1         2         3         4         5         6         
*/


int loadICRF_addstar(ICRF_Stars *star, float fluxPSF, float fluxAp, double R, double D, float dX, float dY, int photcode);

ICRF_Stars *loadICRF_readstars (char *filename, int *nstars) {

  // read in the full FITS files ('cause I don't have a partial read option)
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read stellar parameter file: %s", filename);

  int Nstars = 0;
  int NSTARS = 1000;
  ICRF_Stars *stars = NULL;
  ALLOCATE (stars, ICRF_Stars, NSTARS);

  double R, D;

  // ICRF QSO photcodes (we store flux in Jy and AB mags) 
  PhotCode *codeS = GetPhotcodebyName ("ICRF_S"); if (!codeS) { fprintf (stderr, "missing ICRF_S, exiting\n"); exit (1); }
  PhotCode *codeC = GetPhotcodebyName ("ICRF_C"); if (!codeC) { fprintf (stderr, "missing ICRF_C, exiting\n"); exit (1); }
  PhotCode *codeX = GetPhotcodebyName ("ICRF_X"); if (!codeX) { fprintf (stderr, "missing ICRF_X, exiting\n"); exit (1); }
  PhotCode *codeU = GetPhotcodebyName ("ICRF_U"); if (!codeU) { fprintf (stderr, "missing ICRF_U, exiting\n"); exit (1); }
  PhotCode *codeK = GetPhotcodebyName ("ICRF_K"); if (!codeK) { fprintf (stderr, "missing ICRF_K, exiting\n"); exit (1); }

  /* read in stars line-by-line */
  char line[1024];
  while (scan_line (f, line) != EOF) {
    stripwhite (line);
    if (line[0] == 0) continue;
    if (line[0] == '#') continue;

    // objects with values other than "C" in the first column are not good for
    // calibration, so just skip.
    if (line[0] != 'C') continue;

    // terminate the partial strings containing RA and DEC:
    char cr = line[39]; line[39] = 0;
    char cd = line[55]; line[55] = 0;

    if (!ohana_str_to_radec (&R, &D, &line[24], &line[40])) {
      line[39] = cr;
      line[55] = cd;
      fprintf (stderr, "problem with coords: %s\n", line);
      exit (1);
    }
    R = ohana_normalize_angle (R);
    
    line[63] = 0;
    float dR = atof(&line[57]);
    line[70] = 0;
    float dD = atof(&line[64]);

    int dX = 1000.0*100*dR;
    int dY = 1000.0*100*dD;

    // XXX set this somewhere?
    // line[85] = 0;
    // int Nmeas = atoi(&line[79]);

    line[ 93] = 0; float fluxSt = (line[ 87] == '<') ? NAN : atof(&line[ 87]); if (fluxSt == -1.0) fluxSt = NAN;
    line[100] = 0; float fluxSu = (line[ 94] == '<') ? NAN : atof(&line[ 94]); if (fluxSu == -1.0) fluxSu = NAN;
    line[108] = 0; float fluxCt = (line[102] == '<') ? NAN : atof(&line[102]); if (fluxCt == -1.0) fluxCt = NAN;
    line[115] = 0; float fluxCu = (line[109] == '<') ? NAN : atof(&line[109]); if (fluxCu == -1.0) fluxCu = NAN;
    line[123] = 0; float fluxXt = (line[117] == '<') ? NAN : atof(&line[117]); if (fluxXt == -1.0) fluxXt = NAN;
    line[130] = 0; float fluxXu = (line[124] == '<') ? NAN : atof(&line[124]); if (fluxXu == -1.0) fluxXu = NAN;
    line[138] = 0; float fluxUt = (line[132] == '<') ? NAN : atof(&line[132]); if (fluxUt == -1.0) fluxUt = NAN;
    line[145] = 0; float fluxUu = (line[139] == '<') ? NAN : atof(&line[139]); if (fluxUu == -1.0) fluxUu = NAN;
    line[153] = 0; float fluxKt = (line[147] == '<') ? NAN : atof(&line[147]); if (fluxKt == -1.0) fluxKt = NAN;
    line[160] = 0; float fluxKu = (line[154] == '<') ? NAN : atof(&line[154]); if (fluxKu == -1.0) fluxKu = NAN;

    // we treat each valid flux as a new star
    if (isfinite(fluxSt)) {
      loadICRF_addstar (&stars[Nstars], fluxSt, fluxSu, R, D, dX, dY, codeS->code);
      Nstars++;
      CHECK_REALLOCATE (stars, ICRF_Stars, NSTARS, Nstars, 1000);
    }
    if (isfinite(fluxCt)) {
      loadICRF_addstar (&stars[Nstars], fluxCt, fluxCu, R, D, dX, dY, codeC->code);
      Nstars++;
      CHECK_REALLOCATE (stars, ICRF_Stars, NSTARS, Nstars, 1000);
    }
    if (isfinite(fluxXt)) {
      loadICRF_addstar (&stars[Nstars], fluxXt, fluxXu, R, D, dX, dY, codeX->code);
      Nstars++;
      CHECK_REALLOCATE (stars, ICRF_Stars, NSTARS, Nstars, 1000);
    }
    if (isfinite(fluxUt)) {
      loadICRF_addstar (&stars[Nstars], fluxUt, fluxUu, R, D, dX, dY, codeU->code);
      Nstars++;
      CHECK_REALLOCATE (stars, ICRF_Stars, NSTARS, Nstars, 1000);
    }
    if (isfinite(fluxKt)) {
      loadICRF_addstar (&stars[Nstars], fluxKt, fluxKu, R, D, dX, dY, codeK->code);
      Nstars++;
      CHECK_REALLOCATE (stars, ICRF_Stars, NSTARS, Nstars, 1000);
    }

  }
  *nstars = Nstars;
  return (stars);
}

int loadICRF_addstar(ICRF_Stars *star, float fluxPSF, float fluxAp, double R, double D, float dX, float dY, int photcode) {

  InitICRF_Star (star);

  star->R = R;
  star->D = D;
  star->measure.R = R;
  star->measure.D = D;
  star->measure.dXccd = dX;   // in centi-pixels, where 1 pixel = 1 mas
  star->measure.dYccd = dY;   // in centi-pixels, where 1 pixel = 1 mas

  star->measure.FluxPSF = fluxPSF; // flux in Jy
  star->measure.dFluxPSF = 0.01;  // this is just a guess
  star->measure.FluxAp = fluxAp;  // flux in Jy
  star->measure.dFluxAp = 0.01;   // this is just a guess

  star->measure.M = (fluxPSF > 0.0) ? 8.9 - 2.5*log10(fluxPSF) : NAN;
  star->measure.dM = (fluxPSF > 0.0) ? 0.01 / fluxPSF : NAN;
  star->measure.Map = (fluxAp > 0.0) ? 8.9 - 2.5*log10(fluxAp) : NAN;
  star->measure.dMap = (fluxAp > 0.0) ? 0.01 / fluxAp: NAN;
  star->measure.photcode = photcode;
  star->measure.dbFlags |= ID_MEAS_ICRF_QSO;
  return TRUE;
}

int loadICRF_sortStars (ICRF_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ ICRF_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].R < stars[B].R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

int InitICRF_Star (ICRF_Stars *star) {

    dvo_measure_init (&star[0].measure);
    star[0].found = FALSE; // FALSE = not yet assigned to an object
    star[0].flag = FALSE; // FALSE = not yet assigned to a subset
    return TRUE;
}
