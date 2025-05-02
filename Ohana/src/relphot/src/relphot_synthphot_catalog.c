# include "relphot.h"

// we've just reloaded the data from disk; we now need to apply the Image/Mosaic/Grid
// calibrations determined by the rest of the program.  We also need to set the final
// output dbFlags values

int relphot_synthphot_catalog (Catalog *catalog, SynthZeroPoints *zpts) {

  off_t j;

  int Nsecfilt = catalog[0].Nsecfilt;

  for (j = 0; j < catalog[0].Naverage; j++) {
    off_t m = catalog[0].average[j].measureOffset;
    Average *average = &catalog[0].average[j];
    Measure *measure = &catalog[0].measure[m];
    SecFilt *secfilt = &catalog[0].secfilt[j*Nsecfilt];

    relphot_synthphot_average (average, secfilt, measure, zpts);
  }
  return TRUE;
}

// set mean of chip measurements (selected by photcode range for now):
int relphot_synthphot_average (Average *average, SecFilt *secfilt, Measure *measure, SynthZeroPoints *zpts) {

  int i;
  off_t k;

  // first, determine the state of things for each of the main filters:
  // in this bit, we are using hardwired photcode values:
  // PS1_g, PS1_r, PS1_i, PS1_z, PS1_y, PS1_w == (1,2,3,4,5,6); 0 == none
  // note that Nsec = PS_N - 1

  int havePS1[7];
  int haveSYN[7];
  int needSYN[7];
  int measSYN[7];

  // was: 13.64, 13.76, 13.74, 12.94, 12.01
  float MaxMagForceSynth[] = {0.0, 13.5, 13.5, 13.5, 13.0, 12.0, 13.5};

  for (i = 0; i < 7; i++) {
    havePS1[i] = FALSE; // do we have any PS1 measurement for this filter?
    haveSYN[i] = FALSE; // do we have any synthetic measurement for this filter?
    needSYN[i] = FALSE; // force the use of the synthetic measurement for this filter?
    measSYN[i] = -1; // which entries carries the synthetic measurement for this filter?
  }

  // need to look up the (X,Y) coords from this (R,D) location
  double X, Y;
  double R = ohana_normalize_angle_to_midpoint (average->R, 0.0);
  RD_to_XY (&X, &Y, R, average->D, &zpts->coords);
  if (X < 0) return FALSE;
  if (Y < 0) return FALSE;
  if (X >= zpts->Nx) return FALSE;
  if (Y >= zpts->Ny) return FALSE;

  int Xpix = X;
  int Ypix = Y;
  int Npix = Xpix + Ypix*zpts->Nx;

  for (k = 0; k < average[0].Nmeasure; k++) {

    // skip measurements that do not have a valid photcode
    PhotCode *code = GetPhotcodebyCode (measure[k].photcode);
    if (!code) continue;

    // is this synth_grizy?
    // is this PS1?

    // are we a PS1 exposure photcode?
    if (isGPC1chip(measure[k].photcode)) {
      int Nfilter = whichGPC1filter (measure[k].photcode);
      havePS1[Nfilter] = TRUE;
    }
    if (isGPC1synth(measure[k].photcode)) {
      int Nfilter = measure[k].photcode - 3000;
      haveSYN[Nfilter] = TRUE;
      measSYN[Nfilter] = k;

      // force the use of SYN even if we have PS1 mags?
      if (measure[k].M < MaxMagForceSynth[Nfilter]) {
	needSYN[Nfilter] = TRUE;
      } 
    }
  }

  for (i = 1; i < 6; i++) {
    if (needSYN[i] || (haveSYN[i] && !havePS1[i])) {
      int Nsec = i - 1;
      float *value = (float *) zpts->matrix[Nsec].buffer;
      float ZP = !isnan(value[Npix]) ? value[Npix] : 0.0;
      secfilt[Nsec].MpsfChp  = measure[measSYN[i]].M + ZP;
      secfilt[Nsec].dMpsfChp = 0.6;
      secfilt[Nsec].Mchisq   = NAN;
      secfilt[Nsec].flags   |= ID_SECF_USE_SYNTH;
    }
  }

  return (TRUE);
}

