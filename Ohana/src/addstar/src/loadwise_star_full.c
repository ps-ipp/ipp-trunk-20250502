# include "addstar.h"
# include "WISE.h"

int setWISE_var_flag_allsky (Measure *measure, char qual);
int setWISE_ext_flag_allsky (Measure *measure, char ptr);
int setWISE_var_flag_prelim (Measure *measure, char qual);
int setWISE_ext_flag_prelim (Measure *measure, char ptr);
int setWISE_blend_flag (Measure *measure, char *ptr);

int setCatWISE_ab_flag (Measure *measure, char qual);

# define CATWISE_EPOCH 1432771200 /* MJD 57170 */

// fill in the data for a CatWISE double star.  takes a pointer to the start of the line the
// RA and DEC have already been set
int loadwise_star_catwise (Average *average, Measure *measure, char *line, int Nmax, WISE_Stars *tstars) {

  if (line == NULL) Shutdown ("format error in WISE");

  /*** CATWISE has fixed width data, not pipe-separated, etc ***/

  // https://irsa.ipac.caltech.edu/data/WISE/CatWISE/gator_docs/catwise_colDescriptions.html

  // We are supplied a pointer (line) to the start of a full line of data.  The CatWISE
  // format uses fixed-width fields with white-space separators.  The ra & dec values have
  // already been extracted.

  // reference epoch for CATWise:
  average[0].Tmean = CATWISE_EPOCH;

  // correct the position information for the systematic corrections
  double RA_SYS_OFFSET = RA_SYS_OFFRAW / cos(RAD_DEG*tstars->D);

  average[0].R  = tstars->R + RA_SYS_OFFSET;
  average[0].D  = tstars->D + DE_SYS_OFFSET;
  average[0].dR = getDoubleNAN (line, 71, 79); // sig_ra -- add 5/1000 to avoid truncation
  average[0].dD = getDoubleNAN (line, 80, 88); // sig_dec

  // P.M. values go on average:
  average[0].uR  = getDoubleNAN (line, 1194, 1203) + uRA_SYS_OFFSET; // PMRA
  average[0].uD  = getDoubleNAN (line, 1204, 1213) + uDE_SYS_OFFSET; // PMDec
  average[0].duR = getDoubleNAN (line, 1214, 1222); // sigPMRA 
  average[0].duD = getDoubleNAN (line, 1223, 1231); // sigPMDec

  measure[0].R     = average[0].R;
  measure[0].D     = average[0].D;
  measure[1].R     = average[0].R;
  measure[1].D     = average[0].D;

  // I can assign dRA and dDEC to dX and dY if I can consistently set posangle and pltscale
  measure[0].dXccd = ToShortPixels(average[0].dR + 0.005); // sig_ra -- add 5/1000 to avoid truncation
  measure[0].dYccd = ToShortPixels(average[0].dD + 0.005); // sig_dec
  measure[0].posangle = 0.0;
  measure[0].pltscale = 1.0;

  // we only know a single set of values for both bands 
  measure[1].dXccd    = measure[0].dXccd;
  measure[1].dYccd    = measure[0].dYccd;
  measure[1].posangle = measure[0].posangle;
  measure[1].pltscale = measure[0].pltscale;

  measure[0].Xccd = getDoubleNAN (line,  98, 106); // wx
  measure[0].Yccd = getDoubleNAN (line, 107, 115); // wy
  measure[1].Xccd = measure[0].Xccd;
  measure[1].Yccd = measure[0].Yccd;

  // W1
  measure[0].M         = getDoubleNAN (line, 248, 254); // w1mpro
  measure[0].dM        = getDoubleNAN (line, 255, 264); // w1sigmpro
  measure[0].psfChisq  = getDoubleNAN (line, 265, 275); // w1rchi2
  measure[0].photFlags = 0;

  measure[0].Mkron     = getDoubleNAN (line, 563, 572); // w1mag_4
  measure[0].dMkron    = getDoubleNAN (line, 573, 582); // w1sigm_4
  measure[0].Map       = getDoubleNAN (line, 339, 345); // w1mag
  measure[0].dMap      = getDoubleNAN (line, 346, 352); // w1sigm

  measure[0].Xfix      = getDoubleNAN (line, 880, 891); // w1k
  measure[0].Yfix      = getDoubleNAN (line, 989, 903); // w1mLQ

  // W2
  measure[1].M         = getDoubleNAN (line, 276, 282); // w2mpro
  measure[1].dM        = getDoubleNAN (line, 283, 292); // w2sigmpro
  measure[1].psfChisq  = getDoubleNAN (line, 293, 303); // w2rchi2
  measure[1].photFlags = 0;

  measure[1].Mkron     = getDoubleNAN (line, 591, 600); // w2mag_4 
  measure[1].dMkron    = getDoubleNAN (line, 601, 610); // w2sigm_4
  measure[1].Map       = getDoubleNAN (line, 367, 373); // w2mag
  measure[1].dMap      = getDoubleNAN (line, 374, 380); // w2sigm

  measure[1].Xfix      = getDoubleNAN (line, 995,  1006); // w2k
  measure[1].Yfix      = getDoubleNAN (line, 1013, 1018); // w2mLQ

  measure[0].psfQF     = getDoubleNAN (line, 304, 314); // rchi2
  measure[1].psfQF     = measure[0].psfQF;

  setCatWISE_blend_flag (measure, line); // na & nb

  setCatWISE_sat_flag (&measure[0], line, 323, 330); // w1sat
  setCatWISE_sat_flag (&measure[1], line, 331, 338); // w2sat

  // cc_flags is a field which takes 15 chars, but only 4 are used
  // bytes 1653 - 1656 are the allWise cc_flag entries for W1, W2, W3, W4
  // skip lines for which the 4 chars have the value 'null'
  if (strncmp(&line[1653], "null", 4)) {
      setWISE_cc_flag (&measure[0], line[1653]); // W1 cc_flag
      setWISE_cc_flag (&measure[1], line[1654]); // W2 cc_flag
  }

  // cc_flags is a field which takes 8 chars, but only 2 are used
  // bytes 1735 & 1736 are the ab_flag entries for W1 & W2
  // they use the same values as cc_flags
  setCatWISE_ab_flag (&measure[0], line[1735]); // W1 ab_flag
  setCatWISE_ab_flag (&measure[1], line[1736]); // W2 ab_flag

  double mjdmean = getDoubleNAN (line, 1126, 1138); // MeanObsMJD
  
  // the release is based on data taken in the period 14 January 2010 to 29 April 2010
  if (mjdmean == 0.0) {
    measure[0].t = 0;
  } else {
    measure[0].t = ohana_mjd_to_sec (mjdmean);
  }
  measure[1].t = measure[0].t;

  measure[0].photcode  = WISE_W1;
  measure[0].detID   = 0;
  measure[0].imageID = 0;

  measure[1].photcode  = WISE_W2;
  measure[1].detID   = 0;
  measure[1].imageID = 0;

  return TRUE;
}

// fill in the data for a WISE quad star.  takes a pointer to the start of the line the
// RA and DEC have already been set
int loadwise_star_allwise (Measure *measure, char *line, int Nmax) {

  int i;
  char *ptr;

  if (line == NULL) Shutdown ("format error in WISE");

  ptr = line;

  // I can assign dRA and dDEC to dX and dY if I can consistently set posangle and pltscale
  measure[0].posangle = 0.0;
  measure[0].pltscale = 1.0;

  ptr = skipNbounds (ptr, '|', 3, Nmax); // skip: desig, ra, dec,
  measure[0].dXccd = ToShortPixels((strtod (ptr, NULL) + 0.005)); // sig_ra -- add 5/1000 to avoid truncation
  ptr = nextWISEfield (ptr);
  measure[0].dYccd = ToShortPixels((strtod (ptr, NULL) + 0.005)); // sig_dec
  ptr = nextWISEfield (ptr);

  // we only know a single set of values for all 4 bands 
  for (i = 1; i < 4; i++) {
      measure[i].dXccd    = measure[0].dXccd;
      measure[i].dYccd    = measure[0].dYccd;
      measure[i].posangle = measure[0].posangle;
      measure[i].pltscale = measure[0].pltscale;
  }
  ptr = skipNbounds (ptr, '|', 5, Nmax); // skip: sig_radec, glon, glat, elon, elat

  measure[0].Xccd = strtod (ptr, NULL); // wx
  ptr = nextWISEfield (ptr); // skip wx
  measure[0].Yccd = strtod (ptr, NULL); // wy
  ptr = nextWISEfield (ptr); // skip wy

  for (i = 1; i < 4; i++) {
      measure[i].Xccd    = measure[0].Xccd;
      measure[i].Yccd    = measure[0].Yccd;
  }
  ptr = skipNbounds (ptr, '|', 4, Nmax); // skip: cntr, source_id, coadd_id, src

  // W1
  for (i = 0; i < 4; i++) {
    char *endpoint;
      measure[i].M  = strtod (ptr, &endpoint); // w?mpro
      if (endpoint == ptr) {
	measure[i].M  = NAN;
      }
      ptr = nextWISEfield (ptr);
      measure[i].dM = strtod (ptr, &endpoint); // w?sigmpro
      if (endpoint == ptr) {
	measure[i].dM  = NAN;
      }
      ptr = skipNbounds (ptr, '|', 2, Nmax); // skip: w?sigmpro, w?snr
      measure[i].psfChisq = strtod (ptr, NULL); // w?rchi2
      measure[i].psfQF = strtod (ptr, NULL); // w?rchi2
      ptr = nextWISEfield (ptr); // skip : w1rchi2

      // init the photFlags field
      measure[i].photFlags = 0;
  }

  ptr = nextWISEfield (ptr); // skip: rchi2

  // set blend flags for all 4 measures
  setWISE_blend_flag (measure, ptr); // nb & na both used here
  ptr = skipNbounds (ptr, '|', 2, Nmax); // skip: nb, na

  for (i = 0; i < 4; i++) {
      setWISE_sat_flag (&measure[i], ptr); // w1sat
      ptr = nextWISEfield (ptr); 
  }
  ptr = nextWISEfield (ptr); // skip satnum

  ptr = skipNbounds (ptr, '|', 15, Nmax); // skip pm info

  for (i = 0; i < 4; i++) {
      setWISE_cc_flag (&measure[i], ptr[i]); // cc_flags
  }
  ptr = nextWISEfield (ptr); // skip cc_flags
  ptr = nextWISEfield (ptr); // skip rel

  // set ext flags for all 4 measures
  setWISE_ext_flag_allsky (measure, ptr[0]); // ext_flg
  ptr = nextWISEfield (ptr); // skip ext_flags

  for (i = 0; i < 4; i++) {
      setWISE_var_flag_allsky (&measure[i], ptr[i]); // var_flg
  }
  ptr = nextWISEfield (ptr); // skip var_flags

  for (i = 0; i < 4; i++) {
      setWISE_ph_qual (&measure[i], ptr[i]); // ph_qual
  }

  ptr = skipNbounds (ptr, '|', 157, Nmax); // skip: det_bit, moon_lev, w?nm, w?m, w?cov, etc, etc.
  // w?frtr dropped, (-4), use_src, best_use_cntr, ngrp added (+3), w?dmag dropped, w?k added, 

  for (i = 0; i < 4; i++) {
    ptr = skipNbounds (ptr, '|', 8, Nmax); // skip: w?magp, w?sigp1,2, w?dmag, w?ndf, w?m1q, w?mjdmin, w?mjdmax
    // w?magp is at 217, ph_qual is at 60
    // double mjdmin = strtod (ptr, NULL); // mjd min
    // ptr = nextWISEfield (ptr); // skip
    // double mjdmax = strtod (ptr, NULL); // mjd max
    // ptr = nextWISEfield (ptr); // skip mjd max
    double mjdmean = strtod (ptr, NULL); // mjd mean
    ptr = nextWISEfield (ptr); // skip mjd mean

    // fprintf (stderr, "w%d mjd: %f\n", i, mjdmean);

    // the release is based on data taken in the period 14 January 2010 to 29 April 2010
    if (mjdmean == 0.0) {
      measure[i].t = 0;
    } else {
      measure[i].t = ohana_mjd_to_sec (mjdmean);
    }
  }

  measure[0].photcode  = WISE_W1;
  measure[0].detID   = 0;
  measure[0].imageID = 0;

  measure[1].photcode  = WISE_W2;
  measure[1].detID   = 0;
  measure[1].imageID = 0;

  measure[2].photcode  = WISE_W3;
  measure[2].detID   = 0;
  measure[2].imageID = 0;

  measure[3].photcode  = WISE_W4;
  measure[3].detID   = 0;
  measure[3].imageID = 0;

  return TRUE;
}

// there are slight format differences between the prelim data dump and the allsky data dump:
// * after ph_qual & det_bit : new field moon_lev
// * for each filter, after w?sigp2 : new fields w?dmag, w?ndf, w?mlq, w?mjdmin, w?mjdmax, w?mjdmean
// * after w4mdjmean : new fields rho12, rho23, rho34, q12, q23, q34
// * after k_msig_2mass : new fields best_use_cntr, ngrp

// fill in the data for a WISE quad star.  takes a pointer to the start of the line the
// RA and DEC have already been set
int loadwise_star_allsky (Measure *measure, char *line, int Nmax) {

  int i;
  char *ptr;

  if (line == NULL) Shutdown ("format error in WISE");

  ptr = line;

  // I can assign dRA and dDEC to dX and dY if I can consistently set posangle and pltscale
  measure[0].posangle = 0.0;
  measure[0].pltscale = 1.0;

  ptr = skipNbounds (ptr, '|', 3, Nmax); // skip: desig, ra, dec,
  measure[0].dXccd = ToShortPixels(strtod (ptr, NULL)); // sig_ra
  ptr = nextWISEfield (ptr);
  measure[0].dYccd = ToShortPixels(strtod (ptr, NULL)); // sig_dec
  ptr = nextWISEfield (ptr);

  // we only know a single set of values for all 4 bands 
  for (i = 1; i < 4; i++) {
      measure[i].dXccd    = measure[0].dXccd;
      measure[i].dYccd    = measure[0].dYccd;
      measure[i].posangle = measure[0].posangle;
      measure[i].pltscale = measure[0].pltscale;
  }
  ptr = skipNbounds (ptr, '|', 5, Nmax); // skip: sig_radec, glon, glat, elon, elat

  measure[0].Xccd = strtod (ptr, NULL); // wx
  ptr = nextWISEfield (ptr); // skip wx
  measure[0].Yccd = strtod (ptr, NULL); // wy
  ptr = nextWISEfield (ptr); // skip wy

  for (i = 1; i < 4; i++) {
      measure[i].Xccd    = measure[0].Xccd;
      measure[i].Yccd    = measure[0].Yccd;
  }
  ptr = skipNbounds (ptr, '|', 4, Nmax); // skip: cntr, source_id, coadd_id, src

  // W1
  for (i = 0; i < 4; i++) {
    char *endpoint;
      measure[i].M  = strtod (ptr, &endpoint); // w?mpro
      if (endpoint == ptr) {
	measure[i].M  = NAN;
      }
      ptr = nextWISEfield (ptr);
      measure[i].dM = strtod (ptr, &endpoint); // w?sigmpro
      if (endpoint == ptr) {
	measure[i].dM  = NAN;
      }
      ptr = skipNbounds (ptr, '|', 2, Nmax); // skip: w?sigmpro, w?snr
      measure[i].psfChisq = strtod (ptr, NULL); // w?rchi2
      ptr = nextWISEfield (ptr); // skip : w1rchi2

      // init the photFlags field
      measure[i].photFlags = 0;
  }

  ptr = nextWISEfield (ptr); // skip: rchi2

  // set blend flags for all 4 measures
  setWISE_blend_flag (measure, ptr); // nb & na both used here
  ptr = skipNbounds (ptr, '|', 2, Nmax); // skip: nb, na

  for (i = 0; i < 4; i++) {
      setWISE_sat_flag (&measure[i], ptr); // w1sat
      ptr = nextWISEfield (ptr); 
  }
  ptr = nextWISEfield (ptr); // skip satnum

  for (i = 0; i < 4; i++) {
      setWISE_cc_flag (&measure[i], ptr[i]); // cc_flg
  }
  ptr = nextWISEfield (ptr); // skip cc_flags

  // set ext flags for all 4 measures
  setWISE_ext_flag_allsky (measure, ptr[0]); // ext_flg
  ptr = nextWISEfield (ptr); // skip ext_flags

  for (i = 0; i < 4; i++) {
      setWISE_var_flag_allsky (&measure[i], ptr[i]); // var_flg
  }
  ptr = nextWISEfield (ptr); // skip var_flags

  for (i = 0; i < 4; i++) {
      setWISE_ph_qual (&measure[i], ptr[i]); // ph_qual
  }

  ptr = skipNbounds (ptr, '|', 159, Nmax); // skip: det_bit, moon_lev, w?nm, w?m, w?cov, etc, etc.

  for (i = 0; i < 4; i++) {
    ptr = skipNbounds (ptr, '|', 8, Nmax); // skip: det_bit, moon_lev, w?nm, w?m, w?cov, etc, etc.

    // double mjdmin = strtod (ptr, NULL); // mjd min
    // ptr = nextWISEfield (ptr); // skip
    // double mjdmax = strtod (ptr, NULL); // mjd max
    // ptr = nextWISEfield (ptr); // skip mjd max
    double mjdmean = strtod (ptr, NULL); // mjd mean
    ptr = nextWISEfield (ptr); // skip mjd mean

    // fprintf (stderr, "w%d mjd: %f\n", i, mjdmean);

    // the release is based on data taken in the period 14 January 2010 to 29 April 2010
    if (mjdmean == 0.0) {
      measure[i].t = 0;
    } else {
      measure[i].t = ohana_mjd_to_sec (mjdmean);
    }
  }

  measure[0].photcode  = WISE_W1;
  measure[0].detID   = 0;
  measure[0].imageID = 0;

  measure[1].photcode  = WISE_W2;
  measure[1].detID   = 0;
  measure[1].imageID = 0;

  measure[2].photcode  = WISE_W3;
  measure[2].detID   = 0;
  measure[2].imageID = 0;

  measure[3].photcode  = WISE_W4;
  measure[3].detID   = 0;
  measure[3].imageID = 0;

  return TRUE;
}

// fill in the data for a WISE quad star.  takes a pointer to the start of the line the
// RA and DEC have already been set
int loadwise_star_prelim (Measure *measure, char *line, int Nmax) {

  int i;
  char *ptr;

  if (line == NULL) Shutdown ("format error in WISE");

  ptr = line;

  // I can assign dRA and dDEC to dX and dY if I can consistently set posangle and pltscale
  measure[0].posangle = 0.0;
  measure[0].pltscale = 1.0;

  ptr = skipNbounds (ptr, '|', 3, Nmax); // skip: desig, ra, dec,
  measure[0].dXccd = ToShortPixels(strtod (ptr, NULL)); // sig_ra
  ptr = nextWISEfield (ptr);
  measure[0].dYccd = ToShortPixels(strtod (ptr, NULL)); // sig_dec
  ptr = nextWISEfield (ptr);

  // we only know a single set of values for all 4 bands 
  for (i = 1; i < 4; i++) {
      measure[i].dXccd    = measure[0].dXccd;
      measure[i].dYccd    = measure[0].dYccd;
      measure[i].posangle = measure[0].posangle;
      measure[i].pltscale = measure[0].pltscale;
  }
  ptr = skipNbounds (ptr, '|', 5, Nmax); // skip: sig_radec, glon, glat, elon, elat

  measure[0].Xccd = strtod (ptr, NULL); // wx
  ptr = nextWISEfield (ptr); // skip wx
  measure[0].Yccd = strtod (ptr, NULL); // wy
  ptr = nextWISEfield (ptr); // skip wy

  for (i = 1; i < 4; i++) {
      measure[i].Xccd    = measure[0].Xccd;
      measure[i].Yccd    = measure[0].Yccd;
  }
  ptr = skipNbounds (ptr, '|', 4, Nmax); // skip: cntr, source_id, coadd_id, src

  // W1
  for (i = 0; i < 4; i++) {
      measure[i].M  = strtod (ptr, NULL); // w?mpro
      ptr = nextWISEfield (ptr);
      measure[i].dM = strtod (ptr, NULL); // w?sigmpro
      ptr = skipNbounds (ptr, '|', 2, Nmax); // skip: w?sigmpro, w?snr
      measure[i].psfChisq = strtod (ptr, NULL); // w?rchi2
      ptr = nextWISEfield (ptr); // skip : w1rchi2

      // init the photFlags field
      measure[i].photFlags = 0;
  }

  ptr = nextWISEfield (ptr); // skip: rchi2

  // set blend flags for all 4 measures
  setWISE_blend_flag (measure, ptr); // nb & na both used here
  ptr = skipNbounds (ptr, '|', 2, Nmax); // skip: nb, na

  for (i = 0; i < 4; i++) {
      setWISE_sat_flag (&measure[i], ptr); // w1sat
      ptr = nextWISEfield (ptr); 
  }
  ptr = nextWISEfield (ptr); // skip satnum

  for (i = 0; i < 4; i++) {
      setWISE_cc_flag (&measure[i], ptr[i]); // cc_flg
  }
  ptr = nextWISEfield (ptr); // skip cc_flags

  // set ext flags for all 4 measures
  setWISE_ext_flag_prelim (measure, *ptr); // ext_flg
  ptr = nextWISEfield (ptr); // skip ext_flags

  for (i = 0; i < 4; i++) {
      setWISE_var_flag_prelim (&measure[i], ptr[i]); // var_flg
  }
  ptr = nextWISEfield (ptr); // skip var_flags

  for (i = 0; i < 4; i++) {
      setWISE_ph_qual (&measure[i], ptr[i]); // ph_qual
  }

  double jd = 2455263.0; // NOTE : WISE prelim release does not contain per-detection time info. 
  // the release is based on data taken in the period 14 January 2010 to 29 April 2010
  measure[0].t = ohana_jd_to_sec (jd);
  measure[1].t = measure[0].t;
  measure[2].t = measure[0].t;
  measure[3].t = measure[0].t;

  measure[0].photcode  = WISE_W1;
  measure[0].detID   = 0;
  measure[0].imageID = 0;

  measure[1].photcode  = WISE_W2;
  measure[1].detID   = 0;
  measure[1].imageID = 0;

  measure[2].photcode  = WISE_W3;
  measure[2].detID   = 0;
  measure[2].imageID = 0;

  measure[3].photcode  = WISE_W4;
  measure[3].detID   = 0;
  measure[3].imageID = 0;

  return TRUE;
}

/* return a pointer to the first char after the next field separator (|) */
char *nextWISEfield (char *line) {

  char *p, *q;

  p = line;
  q = strchr (p, '|');
  if (q == NULL) return (NULL);
  p = q + 1;
  if (*p == 0) return (NULL);
  return (p);
}

# define FLAG_PH_A 	      0x00000001 // quality flag 'A'
# define FLAG_PH_B 	      0x00000002 // quality flag 'B'
# define FLAG_PH_C 	      0x00000004 // quality flag 'C'
# define FLAG_PH_U 	      0x00000008 // quality flag 'U'
# define FLAG_PH_X 	      0x00000010 // quality flag 'X'
# define FLAG_PH_Z 	      0x00000020 // quality flag 'Z'

# define FLAG_SATURATED_PIX   0x00000100 // sat > 0.0

# define FLAG_CC_PERSIST      0x00010000 // 'p' or 'P'
# define FLAG_CC_HALO         0x00020000 // 'h' or 'H'
# define FLAG_CC_GHOST        0x00020000 // 'o' or 'O' (note overloading)
# define FLAG_CC_SPIKE        0x00040000 // 'd' or 'D'
# define FLAG_CC_SPURIOUS     0x00080000 // Upper Case letters

# define FLAG_BLEND_ACTIVE    0x00100000 // nb > 1, na == 0
# define FLAG_BLEND_PASSIVE   0x00200000 // nb > 1, na == 1

# define FLAG_EXTENDED        0x01000000 // ext == 1
# define FLAG_EXT_IN_XSC      0x02000000 // ext == 1
# define FLAG_EXT_BY_XSC      0x04000000 // ext == 1

// note prelim version had 1 -> 1-4, 2 -> 5-6
# define FLAG_VARIABLE_LEVEL1 0x10000000 // var_flg == 0 to 5
# define FLAG_VARIABLE_LEVEL2 0x20000000 // var_flg == 6 or 7
# define FLAG_VARIABLE_LEVEL3 0x40000000 // var_flg == 8 or 9

int setWISE_blend_flag (Measure *measure, char *ptr) {

    int nb = atoi (ptr);
    if (nb == 1) return TRUE;

    ptr = nextWISEfield (ptr); // skip to na
    int na = atoi (ptr);

    if (na == 0) {
	measure[0].photFlags |= FLAG_BLEND_ACTIVE;
	measure[1].photFlags |= FLAG_BLEND_ACTIVE;
	measure[2].photFlags |= FLAG_BLEND_ACTIVE;
	measure[3].photFlags |= FLAG_BLEND_ACTIVE;
    } else {
	measure[0].photFlags |= FLAG_BLEND_PASSIVE;
	measure[1].photFlags |= FLAG_BLEND_PASSIVE;
	measure[2].photFlags |= FLAG_BLEND_PASSIVE;
	measure[3].photFlags |= FLAG_BLEND_PASSIVE;
    }
    return TRUE;
}

int setCatWISE_blend_flag (Measure *measure, char *line) {

  // na
  int nb = atoi(getLineSegment(line, 315, 318)); // nb
  int na = atoi(getLineSegment(line, 319, 322)); // na

  if (nb != 1) {
    if (na == 0) {
      measure[0].photFlags |= FLAG_BLEND_ACTIVE;
      measure[1].photFlags |= FLAG_BLEND_ACTIVE;
    } else {
      measure[0].photFlags |= FLAG_BLEND_PASSIVE;
      measure[1].photFlags |= FLAG_BLEND_PASSIVE;
    }
  }
  return TRUE;
}

int setWISE_sat_flag (Measure *measure, char *ptr) {

    float sat = strtod (ptr, NULL);
    if (sat > 0.0) {
	measure[0].photFlags |= FLAG_SATURATED_PIX;
    }
    return TRUE;
}

int setCatWISE_sat_flag (Measure *measure, char *ptr, int start, int end) {

  float sat = getDoubleRAW (ptr, start, end);
  if (sat > 0.0) {
    measure[0].photFlags |= FLAG_SATURATED_PIX;
  }
  return TRUE;
}

int setCatWISE_ab_flag (Measure *measure, char qual) {

  switch (qual) {
    case 'P': measure[0].photFlags2 |= FLAG_CC_PERSIST | FLAG_CC_SPURIOUS; break;
    case 'H': measure[0].photFlags2 |= FLAG_CC_HALO    | FLAG_CC_SPURIOUS; break;
    case 'D': measure[0].photFlags2 |= FLAG_CC_SPIKE   | FLAG_CC_SPURIOUS; break;
    case 'O': measure[0].photFlags2 |= FLAG_CC_GHOST   | FLAG_CC_SPURIOUS; break;

    case '0': break;
    default: 
      fprintf (stderr, "error in ab_flag: %c\n", qual);
  }      
  return (TRUE);
}

int setWISE_cc_flag (Measure *measure, char qual) {

  switch (qual) {
    case 'p': measure[0].photFlags |= FLAG_CC_PERSIST; break;
    case 'h': measure[0].photFlags |= FLAG_CC_HALO;    break;
    case 'd': measure[0].photFlags |= FLAG_CC_SPIKE;   break;
    case 'o': measure[0].photFlags |= FLAG_CC_GHOST;   break;

    case 'P': measure[0].photFlags |= FLAG_CC_PERSIST | FLAG_CC_SPURIOUS; break;
    case 'H': measure[0].photFlags |= FLAG_CC_HALO    | FLAG_CC_SPURIOUS; break;
    case 'D': measure[0].photFlags |= FLAG_CC_SPIKE   | FLAG_CC_SPURIOUS; break;
    case 'O': measure[0].photFlags |= FLAG_CC_GHOST   | FLAG_CC_SPURIOUS; break;

    case '0': break;
    default: 
      fprintf (stderr, "error in cc_flag: %c\n", qual);
  }      
  return (TRUE);
}

int setWISE_ext_flag_allsky (Measure *measure, char value) {

  switch (value) {
    case '0':
      return TRUE;
    case '1':
      measure[0].photFlags |= FLAG_EXTENDED;
      measure[1].photFlags |= FLAG_EXTENDED;
      measure[2].photFlags |= FLAG_EXTENDED;
      measure[3].photFlags |= FLAG_EXTENDED;
      return TRUE;
    case '2':
      measure[0].photFlags |= FLAG_EXT_IN_XSC;
      measure[1].photFlags |= FLAG_EXT_IN_XSC;
      measure[2].photFlags |= FLAG_EXT_IN_XSC;
      measure[3].photFlags |= FLAG_EXT_IN_XSC;
      return TRUE;
    case '3':
      measure[0].photFlags |= FLAG_EXTENDED | FLAG_EXT_IN_XSC;
      measure[1].photFlags |= FLAG_EXTENDED | FLAG_EXT_IN_XSC;
      measure[2].photFlags |= FLAG_EXTENDED | FLAG_EXT_IN_XSC;
      measure[3].photFlags |= FLAG_EXTENDED | FLAG_EXT_IN_XSC;
      return TRUE;
    case '4':
      measure[0].photFlags |= FLAG_EXT_BY_XSC;
      measure[1].photFlags |= FLAG_EXT_BY_XSC;
      measure[2].photFlags |= FLAG_EXT_BY_XSC;
      measure[3].photFlags |= FLAG_EXT_BY_XSC;
      return TRUE;
    case '5':
      measure[0].photFlags |= FLAG_EXTENDED | FLAG_EXT_BY_XSC;
      measure[1].photFlags |= FLAG_EXTENDED | FLAG_EXT_BY_XSC;
      measure[2].photFlags |= FLAG_EXTENDED | FLAG_EXT_BY_XSC;
      measure[3].photFlags |= FLAG_EXTENDED | FLAG_EXT_BY_XSC;
      return TRUE;
    default: 
      fprintf (stderr, "programming error\n");
      abort();
  }
  return TRUE;
}

int setWISE_ext_flag_prelim (Measure *measure, char value) {

  switch (value) {
    case '0':
      return TRUE;
    case '1':
      measure[0].photFlags |= FLAG_EXTENDED;
      measure[1].photFlags |= FLAG_EXTENDED;
      measure[2].photFlags |= FLAG_EXTENDED;
      measure[3].photFlags |= FLAG_EXTENDED;
      return TRUE;
    default: 
      fprintf (stderr, "programming error\n");
      abort();
  }
  return TRUE;
}

// NOTE: var flag definition changed slightly between prelim & allsky
int setWISE_var_flag_allsky (Measure *measure, char qual) {

  switch (qual) {
    case 'n':
	return TRUE;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
	measure[0].photFlags |= FLAG_VARIABLE_LEVEL1;
	return TRUE;
    case '6':
    case '7':
	measure[0].photFlags |= FLAG_VARIABLE_LEVEL2;
	return TRUE;
    case '8':
    case '9':
      measure[0].photFlags |= FLAG_VARIABLE_LEVEL3;
      return TRUE;
    default: 
      fprintf (stderr, "programming error\n");
      abort();
  }
}

int setWISE_var_flag_prelim (Measure *measure, char qual) {

  switch (qual) {
    case '0':
      return TRUE;
    case '1':
    case '2':
    case '3':
    case '4':
	measure[0].photFlags |= FLAG_VARIABLE_LEVEL1;
	return TRUE;
    case '5':
    case '6':
    case '7':
	measure[0].photFlags |= FLAG_VARIABLE_LEVEL2;
	return TRUE;
    case '8':
    case '9':
      measure[0].photFlags |= FLAG_VARIABLE_LEVEL3;
      return TRUE;
    default: 
      fprintf (stderr, "programming error\n");
      abort();
  }
}

int setWISE_ph_qual (Measure *measure, char qual) {

  switch (qual) {
    case 'A': measure[0].photFlags |= FLAG_PH_A; break;
    case 'B': measure[0].photFlags |= FLAG_PH_B; break;
    case 'C': measure[0].photFlags |= FLAG_PH_C; break;
    case 'U': measure[0].photFlags |= FLAG_PH_U; break;
    case 'X': measure[0].photFlags |= FLAG_PH_X; break;
    case 'Z': measure[0].photFlags |= FLAG_PH_Z; break;
    default: 
      fprintf (stderr, "error in ph_flag: %c\n", qual);
  }      
  return (TRUE);
}

/* unused photFlag bits:

0x0000.0020
0x0000.0040
0x0000.0080

0x0000.0200
0x0000.0400
0x0000.0800

0x0000.1000
0x0000.2000
0x0000.4000
0x0000.8000

0x0040.0000
0x0080.0000

0x0200.0000
0x0400.0000
0x0800.0000

0x8000.0000

*/
