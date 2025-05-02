# include "relphot.h"

int LimitDensityCatalog_ByNmeasure (Catalog *subcatalog, Catalog *oldcatalog);

int bcatalog (Catalog *subcatalog, Catalog *catalog, int Ncat) {
  
  off_t i, j, offset;
  int found, Ns, *Nvalid;
  off_t NAVERAGE, NMEASURE, Naverage, Nmeasure, Nm;
  float mag;
  int Ncode, Ntime, Ndophot, Nmag, Nsigma, Nimag, Nfew, Ngalaxy, Npsfqf, Nnan, Nbad, Npoor;

  int Nsecfilt = GetPhotcodeNsecfilt ();
  assert (Nsecfilt == catalog[0].Nsecfilt);

  /* we are moving only the subset of measurements from catalog[0] to subcatalog[0] */
  memset(subcatalog, 0, sizeof(*subcatalog));
  NAVERAGE = 50;
  NMEASURE = 1000;
  ALLOCATE (subcatalog[0].measureT, MeasureTiny, NMEASURE);
  ALLOCATE (subcatalog[0].averageT, AverageTiny, NAVERAGE);
  ALLOCATE (subcatalog[0].secfilt,  SecFilt,     NAVERAGE*Nsecfilt);
  Nmeasure = Naverage = 0;

  int NmMax = 0;
  int NmMin = catalog[0].Nmeasure;

  Ncode = Ntime = Ndophot = Nmag = Nsigma = Nimag = Nfew = Npsfqf = Ngalaxy = Nnan = Nbad = Npoor = 0;

  // copy the following fields to the subcatalog:
  subcatalog[0].catID = catalog[0].catID;
  subcatalog[0].filename = catalog[0].filename;

  // array to count measures with photcodes->equiv to each of the secfilts
  ALLOCATE (Nvalid, int, Nsecfilt);

  /* exclude stars not in range or with too few measurements */
  for (i = 0; i < catalog[0].Naverage; i++) {
    // if (catalog[0].average[i].Nmeasure < 2) continue; 

    /* start with all stars good */
    CopyAverageToTiny (&subcatalog[0].averageT[Naverage], &catalog[0].average[i]);
    subcatalog[0].averageT[Naverage].measureOffset = Nmeasure;

    for (j = 0; j < Nsecfilt; j++) {
      subcatalog[0].secfilt[Nsecfilt*Naverage+j] = catalog[0].secfilt[Nsecfilt*i+j];
      Nvalid[j] = 0; // reset the Nvalid array for this star
    }

    // reset the calculated average magnitudes for active photcode only
    ResetAverageActivePhotcodes (&subcatalog[0].secfilt[Nsecfilt*Naverage]);

    Nm = 0;
    int nEXT = 0;
    int nPSF = 0;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {

      offset = catalog[0].average[i].measureOffset + j;

      /* select measurements by photcode */
      PhotCode *code = GetPhotcodebyCode (catalog[0].measure[offset].photcode);
      if (!code) continue;
      found = FALSE;
      int Nsec = -1; // Nsec equivalent for the measurement
      for (Ns = 0; !found && (Ns < Nphotcodes); Ns++) {
	if (code->equiv != photcodes[Ns][0].code) continue;
	found = TRUE;
	Nsec = GetPhotcodeNsec(code->equiv);
      }
      if (!found) {
	Ncode ++; 
	continue; 
      }

      /* select measurements by time */
      if (TimeSelect) {
	if (catalog[0].measure[offset].t < TSTART) { Ntime ++; continue; }
	if (catalog[0].measure[offset].t > TSTOP)  { Ntime ++; continue; }
      }

      /* select measurements by quality */
      if (DophotSelect && ((catalog[0].measure[offset].photFlags >> 16) != DophotValue)) { Ndophot ++; continue; }

      float Maper = USE_APER_FOR_STARGAL ? catalog[0].measure[offset].Map : catalog[0].measure[offset].Mkron;

      // skip garbage measurements
      if (isnan(catalog[0].measure[offset].psfQF)     || (catalog[0].measure[offset].psfQF < 0.95))     { Npsfqf ++; continue; }
      if (isnan(catalog[0].measure[offset].psfQFperf) || (catalog[0].measure[offset].psfQFperf < 0.95)) { Npsfqf ++; continue; }
      if (isnan(catalog[0].measure[offset].M)) { Nnan ++; continue; }
      if (isnan(Maper)) { Nnan ++; continue; }

      // require 0x01 in photFlags (fitted with a PSF) -- add to the test data
      if (REQUIRE_PSFFIT && (catalog[0].measure[offset].photFlags & 0x01) == 0) { Nbad ++; continue; }

      // very loose cut on PSF - APER mag (Map or Mkron) -- this lightly rejects galaxies & other oddities
      float Mkp = catalog[0].measure[offset].M - Maper;
      if (fabs(Mkp) > 1.0) { Nbad ++; continue; }

      if (catalog[0].measure[offset].photFlags & code->photomBadMask) { 
	Nbad++; 
	continue;
      }
      if (catalog[0].measure[offset].photFlags & code->photomPoorMask) { Npoor++; continue;}

      // weak test for galaxies
      if (Mkp > 0.5) {
	nEXT ++;
      } else {
	nPSF ++;
      }

      /* select measurements by mag limit */
      mag = PhotCat (&catalog[0].measure[offset], MAG_CLASS_PSF);
      if (mag > MAG_LIM) { Nmag ++; continue; } 

      /* select measurements by measurement error */
      if ((SIGMA_LIM > 0) && (catalog[0].measure[offset].dM > SIGMA_LIM)) { Nsigma ++; continue; }

      /* select measurements by mag limit */
      if (ImagSelect) {
	mag = PhotInst (&catalog[0].measure[offset], MAG_CLASS_PSF);
	if (mag < ImagMin) { Nimag ++; continue; }
	if (mag > ImagMax) { Nimag ++; continue; }
      }

      // count this measurement as valid for this secfilt entry
      if (Nsec > -1) {
	assert (Nsec < Nsecfilt);
	Nvalid[Nsec] ++;
      }

      CopyMeasureToTiny (&subcatalog[0].measureT[Nmeasure], &catalog[0].measure[offset]);
      subcatalog[0].measureT[Nmeasure].dbFlags &= ~ID_MEAS_SKIP_PHOTOM;
      subcatalog[0].measureT[Nmeasure].averef = Naverage;
      ResetMeasureZeroPoints (&subcatalog[0].measureT[Nmeasure], Nmeasure, Ncat);

      Nmeasure ++;
      Nm ++;
      if (Nmeasure == NMEASURE) {
	NMEASURE += 1000;
	REALLOCATE (subcatalog[0].measureT, MeasureTiny, NMEASURE);
      }
      myAssert (Nmeasure < NMEASURE, "realloc failure");
    } // end of catalog.average.Nmeasure loop

    // skip object if it is likely to be a galaxy
    if (nEXT >= nPSF) {
      Nmeasure -= Nm;
      Ngalaxy ++;
      continue; 
    }

    NmMin = MIN (Nm, NmMin);
    NmMax = MAX (Nm, NmMax);

    // XXXX test : what checks do I need to make elsewhere to avoid problems here?
    if (Nm <= STAR_TOOFEW) { /* enough measurements total? */
      Nmeasure -= Nm;
      Nfew ++;
      continue; 
    }
    int anySecfiltGood = FALSE;
    for (Ns = 0; Ns < Nsecfilt; Ns++) { /* enough measurements in at least one band? */
      if (Nvalid[Ns] <= STAR_TOOFEW) continue;
      anySecfiltGood = TRUE;
    }
    if (!anySecfiltGood) {
      Nmeasure -= Nm;
      Nfew ++;
      continue; 
    }

    // for w-band photometry (& other cases?) convert gri photometry
    // to a synthetic w-band magnitude
    add_synthetic_mags (&subcatalog[0].averageT[Naverage], 
			&subcatalog[0].secfilt[Nsecfilt*Naverage], 
			&subcatalog[0].measureT[Nmeasure],
			&Nmeasure,
			&Nm);
    if (Nmeasure == NMEASURE) {
      NMEASURE += 1000;
      REALLOCATE (subcatalog[0].measureT, MeasureTiny, NMEASURE);
    }
    myAssert (Nmeasure < NMEASURE, "realloc failure");

    subcatalog[0].averageT[Naverage].Nmeasure = Nm;
    Naverage ++;
    if (Naverage == NAVERAGE) {
      NAVERAGE += 50;
      REALLOCATE (subcatalog[0].averageT, AverageTiny, NAVERAGE);
      REALLOCATE (subcatalog[0].secfilt,  SecFilt,     NAVERAGE*Nsecfilt);
    }
  }
  REALLOCATE (subcatalog[0].averageT, AverageTiny, MAX (Naverage, 1));
  REALLOCATE (subcatalog[0].measureT, MeasureTiny, MAX (Nmeasure, 1));
  REALLOCATE (subcatalog[0].secfilt, SecFilt, Nsecfilt*MAX (Naverage, 1));
  subcatalog[0].Naverage = Naverage;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = catalog[0].Nsecfilt;
  subcatalog[0].Nsecfilt_mem = Naverage * catalog[0].Nsecfilt;

  if (VERBOSE) {
    fprintf (stderr, "using "OFF_T_FMT" stars ("OFF_T_FMT" measures) of "OFF_T_FMT" ("OFF_T_FMT" measures) for catalog %s, %d < Nm < %d\n", 
	     subcatalog[0].Naverage,  subcatalog[0].Nmeasure,  catalog[0].Naverage, catalog[0].Nmeasure, catalog[0].filename, NmMin, NmMax);
    fprintf (stderr, "rejections: %d stars have too few measures:\n   %d code, %d time, %d dophot, %d mag, %d sigma, %d imag, %d psfqf, %d Nnan, %d galaxies, %d bad, %d poor\n", 
	     Nfew, Ncode, Ntime, Ndophot, Nmag, Nsigma, Nimag, Npsfqf, Nnan, Ngalaxy, Nbad, Npoor);
  }

  // limit the total number of stars in the catalog
  if (MaxDensityUse) {
    LimitDensityCatalog_ByNmeasure (subcatalog, catalog);
  }

  free (Nvalid)

  return (TRUE);
}

// sort by decreasing Nmeasure (X)
void sort_by_Nmeasure (int *X, off_t *Y, off_t N) {

# define SWAPFUNC(A,B){ int tmpI; off_t tmpT;	\
  tmpI = X[A]; X[A] = X[B]; X[B] = tmpI; \
  tmpT = Y[A]; Y[A] = Y[B]; Y[B] = tmpT; \
}
# define COMPARE(A,B)(X[A] > X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

int LimitDensityCatalog_ByNmeasure (Catalog *subcatalog, Catalog *oldcatalog) {

  Catalog tmpcatalog;

  double Rmin, Rmax, Dmin, Dmax;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  gfits_scan (&oldcatalog[0].header, "RA0",  "%lf", 1, &Rmin);
  gfits_scan (&oldcatalog[0].header, "DEC0", "%lf", 1, &Dmin);
  gfits_scan (&oldcatalog[0].header, "RA1",  "%lf", 1, &Rmax);
  gfits_scan (&oldcatalog[0].header, "DEC1", "%lf", 1, &Dmax);

  if (VERBOSE2) fprintf (stderr, "extracting from catalog covering region %f,%f to %f,%f\n", Rmin, Dmin, Rmax, Dmax);

  float AREA = fabs(Dmax - Dmin) * fabs(Rmax - Rmin) * cos (0.5*RAD_DEG*(Dmax + Dmin));
  assert (AREA > 0);

  off_t Nmax = MaxDensityValue * AREA;
  if (subcatalog[0].Naverage <= Nmax) {
    if (VERBOSE) {
      fprintf (stderr, "subcatalog has less than the max density\n");
    }
    return (TRUE);
  }

  off_t Naverage = subcatalog[0].Naverage;

  // select a random subset of Nmax stars from subcatalog using Fisher-Yates

  // we are going to select Nmax entries by generating a random-sorted index list
  int *value;
  off_t *index, i, j, ave;
  ALLOCATE (index, off_t, Naverage);
  ALLOCATE (value, int, Naverage);
  for (i = 0; i < Naverage; i++) {
    index[i] = i;
    value[i] = subcatalog[0].averageT[i].Nmeasure;
  }
  sort_by_Nmeasure (value, index, Naverage);

  // count the number of measurements this selection will yield
  off_t NMEASURE = 0;
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    NMEASURE += subcatalog[0].averageT[ave].Nmeasure;
  }

  // allocate the output data 
  ALLOCATE (tmpcatalog.averageT, AverageTiny, Nmax);
  ALLOCATE (tmpcatalog.measureT, MeasureTiny, NMEASURE);
  ALLOCATE (tmpcatalog.secfilt,  SecFilt, Nmax * Nsecfilt);

  off_t Nmeasure = 0;

  // copy the Nmax selected entries from subcatalog to tmpcatalog (adjusting links)
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    tmpcatalog.averageT[i] = subcatalog[0].averageT[ave];
    tmpcatalog.averageT[i].measureOffset = Nmeasure;
    for (j = 0; j < tmpcatalog.averageT[i].Nmeasure; j++) {
      off_t offset = subcatalog[0].averageT[ave].measureOffset + j;
      tmpcatalog.measureT[Nmeasure] = subcatalog[0].measureT[offset];
      tmpcatalog.measureT[Nmeasure].averef = i;
      Nmeasure ++;
    }
    for (j = 0; j < Nsecfilt; j++) {
      tmpcatalog.secfilt[i*Nsecfilt + j] = subcatalog[0].secfilt[ave*Nsecfilt + j];
    }
  }

  if (VERBOSE) {
    char *basename = filebasename (oldcatalog[0].filename);
    fprintf (stderr, "limited to "OFF_T_FMT" ("OFF_T_FMT" subset, "OFF_T_FMT" total) stars, "OFF_T_FMT" ("OFF_T_FMT" subset, "OFF_T_FMT" total) measures for catalog %s\n", 
	     Nmax, subcatalog[0].Naverage, oldcatalog[0].Naverage, Nmeasure, subcatalog[0].Nmeasure,  oldcatalog[0].Nmeasure, basename);
    free (basename);
  }

  free (index);
  free (value);
  free (subcatalog[0].averageT);
  free (subcatalog[0].measureT);
  free (subcatalog[0].secfilt);

  subcatalog[0].averageT = tmpcatalog.averageT;
  subcatalog[0].measureT = tmpcatalog.measureT;
  subcatalog[0].secfilt = tmpcatalog.secfilt;
  subcatalog[0].Naverage = Nmax;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = oldcatalog[0].Nsecfilt;
  subcatalog[0].Nsecfilt_mem = Naverage * oldcatalog[0].Nsecfilt;

  return (TRUE);
}

int LimitDensityCatalog (Catalog *subcatalog, Catalog *catalog) {

  Catalog tmpcatalog;

  double Rmin, Rmax, Dmin, Dmax;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  gfits_scan (&catalog[0].header, "RA0",  "%lf", 1, &Rmin);
  gfits_scan (&catalog[0].header, "DEC0", "%lf", 1, &Dmin);
  gfits_scan (&catalog[0].header, "RA1",  "%lf", 1, &Rmax);
  gfits_scan (&catalog[0].header, "DEC1", "%lf", 1, &Dmax);

  if (VERBOSE2) fprintf (stderr, "extracting from catalog covering region %f,%f to %f,%f\n", Rmin, Dmin, Rmax, Dmax);

  float AREA = fabs(Dmax - Dmin) * fabs(Rmax - Rmin) * cos (0.5*RAD_DEG*(Dmax + Dmin));
  assert (AREA > 0);

  off_t Nmax = MaxDensityValue * AREA;
  if (subcatalog[0].Naverage <= Nmax) {
    if (VERBOSE) {
      fprintf (stderr, "subcatalog has less than the max density\n");
    }
    return (TRUE);
  }

  off_t Naverage = subcatalog[0].Naverage;

  // select a random subset of Nmax stars from subcatalog using Fisher-Yates

  // we are going to select Nmax entries by generating a random-sorted index list
  off_t *index, tmp, i, j, ave;
  ALLOCATE (index, off_t, Naverage);
  for (i = 0; i < Naverage; i++) {
    index[i] = i;
  }
  for (i = 0; i < Naverage; i++) {
    j = (Naverage - i) * drand48() + i; // a number between i and Naverage
    tmp = index[j];
    index[j] = index[i];
    index[i] = tmp;
  }

  // count the number of measurements this selection will yield
  off_t NMEASURE = 0;
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    NMEASURE += subcatalog[0].averageT[ave].Nmeasure;
  }

  // allocate the output data 
  ALLOCATE (tmpcatalog.averageT, AverageTiny, Nmax);
  ALLOCATE (tmpcatalog.measureT, MeasureTiny, NMEASURE);
  ALLOCATE (tmpcatalog.secfilt,  SecFilt, Nmax * Nsecfilt);

  off_t Nmeasure = 0;

  // copy the Nmax selected entries from subcatalog to tmpcatalog (adjusting links)
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    tmpcatalog.averageT[i] = subcatalog[0].averageT[ave];
    tmpcatalog.averageT[i].measureOffset = Nmeasure;
    for (j = 0; j < tmpcatalog.averageT[i].Nmeasure; j++) {
      off_t offset = subcatalog[0].averageT[ave].measureOffset + j;
      tmpcatalog.measureT[Nmeasure] = subcatalog[0].measureT[offset];
      tmpcatalog.measureT[Nmeasure].averef = i;
      Nmeasure ++;
    }
    for (j = 0; j < Nsecfilt; j++) {
      tmpcatalog.secfilt[i*Nsecfilt + j] = subcatalog[0].secfilt[ave*Nsecfilt + j];
    }
  }

  if (VERBOSE) {
    fprintf (stderr, "limited to "OFF_T_FMT" of "OFF_T_FMT" stars ("OFF_T_FMT" of "OFF_T_FMT" measures) for catalog %s\n", 
	     Nmax, subcatalog[0].Naverage, Nmeasure, subcatalog[0].Nmeasure,  catalog[0].filename);
  }

  free (subcatalog[0].averageT);
  free (subcatalog[0].measureT);
  free (subcatalog[0].secfilt);

  subcatalog[0].averageT = tmpcatalog.averageT;
  subcatalog[0].measureT = tmpcatalog.measureT;
  subcatalog[0].secfilt = tmpcatalog.secfilt;
  subcatalog[0].Naverage = Nmax;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = catalog[0].Nsecfilt;
  subcatalog[0].Nsecfilt_mem = Naverage * catalog[0].Nsecfilt;

  return (TRUE);
}
