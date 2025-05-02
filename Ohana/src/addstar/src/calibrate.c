# include "addstar.h"

static int InternalCal;
static int Ncal, NCAL;
static off_t *Nstar;
static float *Mobs, *dMobs, *Mref, *dMref, *Minst;

void InitCalibration (int mode) {

  InternalCal = mode;

  fprintf (stderr, "calibrating the image...\n");
  Ncal = 0;
  NCAL = 1000;
  ALLOCATE (Mobs,  float,  NCAL);
  ALLOCATE (dMobs, float,  NCAL);
  ALLOCATE (Mref,  float,  NCAL);
  ALLOCATE (dMref, float,  NCAL);
  ALLOCATE (Minst, float,  NCAL);
  ALLOCATE (Nstar, off_t,  NCAL);
}
  
void SaveCalibration (float Mo, float dMo, float Mr, float dMr, float Mi, off_t N) {

  Mobs[Ncal]  = Mo;
  dMobs[Ncal] = dMo;
  Mref[Ncal]  = Mr;
  dMref[Ncal] = dMr;
  Minst[Ncal] = Mi;
  Nstar[Ncal] = N;
  Ncal ++;

  if (Ncal == NCAL) {
    NCAL += 1000;
    REALLOCATE (Mobs,  float, NCAL);
    REALLOCATE (dMobs, float, NCAL);
    REALLOCATE (Mref,  float, NCAL);
    REALLOCATE (dMref, float, NCAL);
    REALLOCATE (Minst, float, NCAL);
    REALLOCATE (Nstar, off_t, NCAL);
  }
}

/* use the linked list to navigate the measures; safe for unsorted measures */
void AddToCalibration (Average *average, SecFilt *secfilt, Measure *measure, Measure *new, off_t *next, off_t Nstar) {

  int i, j, m, Nsec, found0, found1, found2;
  float CalM0, CalM1, CalM2, dCalM;
  float Mcal, color, factor, Minst;
  short CalC0, CalC1, CalC2;

  PhotCode *mycode;  // photcode of this measurement
  PhotCode *incode;  // mycode.equiv (internal reference)
  PhotCode *excode;  // incode.equiv (external reference)

  found0 = found1 = found2 = FALSE;
  CalM0 = CalM1 = CalM2 = dCalM = NAN;

  // we have two options here: 
  //  - calibrate to internal system (Mcal)
  //  - calibrate to external system (Mref)

  mycode = GetPhotcodebyCode (new[0].photcode);
  incode = GetPhotcodebyCode (mycode[0].equiv);
  excode = GetPhotcodebyCode (incode[0].equiv);

  if (InternalCal) {
    CalC0 = incode[0].code;
    Nsec  = GetPhotcodeNsec (CalC0);
  } else {
    CalC0 = excode[0].code;
    Nsec  = GetPhotcodeNsec (CalC0);
  }
  /* check if this reference code is an average magnitude */
  if (Nsec != -1) {
    CalM0 = secfilt[Nsec].MpsfChp;
    dCalM = secfilt[Nsec].dMpsfChp;
    found0 = TRUE;
  }

  CalC1 = mycode[0].c1;
  Nsec  = GetPhotcodeNsec (CalC1);
  if (Nsec != -1) {
    CalM1 = secfilt[Nsec].MpsfChp;
    found1 = TRUE;
  }

  CalC2 = mycode[0].c2;
  Nsec  = GetPhotcodeNsec (CalC2);
  if (Nsec != -1) {
    CalM2 = secfilt[Nsec].MpsfChp;
    found2 = TRUE;
  }

  if (!CalC1 && !CalC2) {
    found1 = found2 = TRUE;
    CalM1 = CalM2 = 0.0;
  }

  m = average[0].measureOffset;
  for (i = 0; i < average[0].Nmeasure; i++) {
    if (measure[m].photcode == CalC0) { 
      found0 = TRUE; 
      CalM0  = measure[m].M; 
      dCalM  = measure[m].dM; 
    }
    if (measure[m].photcode == CalC1) { 
      found1 = TRUE; 
      CalM1  = measure[m].M; 
    }
    if (measure[m].photcode == CalC2) { 
      found2 = TRUE; 
      CalM2  = measure[m].M; 
    }
    if (found0 && found1 && found2) {
      Mcal   = new[0].M + 0.001*mycode[0].C + mycode[0].K*(new[0].airmass - 1.0) - GetZeroPoint();
      color  = CalM1 - CalM2 - 0.001*mycode[0].dX;
      factor = color;
      for (j = 0; j < mycode[0].Nc; j++) {
	Mcal += mycode[0].X[j]*factor;
	factor *= color;
      }
      if (!InternalCal) {
	Mcal += 0.001*incode[0].C;
      }
      // if we want to apply a Mcal -> Mref color correction, we need the additional color term
      Minst = new[0].M - GetZeroPoint() - new[0].dt;
      SaveCalibration (Mcal, new[0].dM, CalM0, dCalM, Minst, Nstar);
      if ((DUMP != NULL) && !strcmp (DUMP, "cal")) {
	fprintf (stdout, "cal-match : %10.6f %10.6f : %7.4f %6.4f  %7.4f %6.4f   %7.4f : %7.4f %7.4f\n", average[0].R, average[0].D, Mcal, new[0].dM, CalM0, dCalM, Minst, new[0].airmass, color);
      }
      return;
    }
    m = next[m];
  }
  return;
}

void FindCalibration (Image *image) {

  int i, MaxN, *Nlist, Nkeep;
  float N, M1, M2, *Dmag, *dDmag;
  float dMo, dMr, Mw, Dmed, W1, W2, NSigma;

  /* reject multiple matched-stars */
  /* find maximum value of Nstar[] */
  MaxN = -1;
  for (i = 0; i < Ncal; i++) {
    MaxN = MAX (Nstar[i], MaxN);
  }
  if (MaxN == -1) {
    fprintf (stderr, "no clean stars\n");
    image[0].McalPSF  = 10.000;
    image[0].McalAPER = 10.000;
    image[0].dMcal    = 10.000;
    return;
  }
  /* create a hash array from Nstar[] entries */
  ALLOCATE (Nlist, int, MaxN + 1);
  if (MaxN >= 0) {
    memset (Nlist, 0, sizeof(int));
  }
  for (i = 0; i < Ncal; i++) {
    Nlist[Nstar[i]] ++;
  }
  
  /* accumulate delta mags */
  ALLOCATE (Dmag, float, Ncal);
  ALLOCATE (dDmag, float, Ncal);
  Nkeep = 0;
  for (i = 0; i < Ncal; i++) {
    /* if this entry has too many (or two few?) matches, skip it */
    if (Nlist[Nstar[i]] != 1) continue;

    /* clip by instrumental magnitude */
    if (Minst[i] > CAL_INSTMAG_MAX) continue;
    if (Minst[i] < CAL_INSTMAG_MIN) continue;
    
    /* XXX EAM: note the artificial 0.005 dmag here */
    dMr = MAX (0.005, dMref[i]);
    dMo = MAX (0.005, dMobs[i]);

    Dmag[Nkeep] = (Mobs[i] - Mref[i]);
    dDmag[Nkeep] = (dMr*dMr + dMo*dMo);
    Nkeep ++;
  }

  if (Nkeep < 5) {
    fprintf (stderr, "too few stars\n");
    image[0].McalPSF  = 10.000;
    image[0].McalAPER = 10.000;
    image[0].dMcal    = 10.000;
    return;
  }
  fsortpair (Dmag, dDmag, Nkeep);

  /* take sort list of Dmag, find median */
  Dmed = Dmag[(int)(0.5*Nkeep)];

  /* exclude points with abs(Dmag - Dmed) / dDmag > 2.5 */

  /* accumulate delta mags (25% - 75% of clipped range) */
  W1 = 0.0;
  W2 = 0.0;
  M1 = 0.0;
  M2 = 0.0;
  N  = 0.0;
  for (i = 0; i < Nkeep; i++) {
    NSigma = fabs (Dmag[i] - Dmed) / sqrt (dDmag[i]);
    if (NSigma > 2.5) continue;
    W1 += Dmag[i] / dDmag[i];
    W2 += 1 / dDmag[i];
    M1 += Dmag[i];
    M2 += SQ (Dmag[i]);
    N  += 1.0; 
  }

  if (N > 1) {
    M1 = M1 / N;
    M2 = sqrt (fabs(M2/N - M1*M1));
    Mw = W1 / W2;
    fprintf (stdout, "STATUS: SUCCESS\n");
    fprintf (stdout, "ZERO_POINT_MEAN      = %7.4f\n", M1);
    fprintf (stdout, "ZERO_POINT_WTMEAN    = %7.4f\n", Mw);
    fprintf (stdout, "ZERO_POINT_STDEV     = %7.4f\n", M2);
    fprintf (stdout, "ZERO_POINT_PRECISION = %7.4f\n", M2 / sqrt (N));
    fprintf (stdout, "ZERO_POINT_NSTARS    =    %4.0f\n", N);
    // fprintf (stderr, "N: %.0f, mean: %f, wt mean: %f, stdev: %f, precision: %f\n", N, M1, Mw, M2, M2 / sqrt (N));
    image[0].McalPSF    = M1;
    image[0].McalAPER   = M1;
    image[0].dMcal      = M2 / sqrt (N);
    image[0].nFitPhotom = N;
  } else {
    fprintf (stderr, "too few stars\n");
    image[0].McalPSF    = 10.000;
    image[0].McalAPER   = 10.000;
    image[0].dMcal      = 10.000;
    image[0].nFitPhotom = 0;
  }
}
