# include "getstar.h"

/* add selected catalog objects to the output catalog */
int select_by_region (Catalog *output, Catalog *catalog, SkyRegion *region, int start, int end) {

  int i, j, offset, m, Nsecfilt, code, Nsec;
  int Nave, NAVE, Nmeas, NMEAS, needMeas;
  double R, D, AREA;
  float mag;

  Nsecfilt = output[0].Nsecfilt;

  if (output == NULL) Shutdown ("output not defined");

  /* identify selection criteria */
  if (VERBOSE) fprintf (stderr, "extracting for time range %d to %d\n", start, end);
  if (VERBOSE) fprintf (stderr, "extracting for region %f,%f to %f,%f\n",
                        region[0].Rmin, region[0].Dmin, region[0].Rmax, region[0].Dmax);

  if (output[0].header.buffer != NULL) {
    gfits_modify (&output[0].header, "RA0",  "%lf", 1, region[0].Rmin);
    gfits_modify (&output[0].header, "DEC0", "%lf", 1, region[0].Dmin);
    gfits_modify (&output[0].header, "RA1",  "%lf", 1, region[0].Rmax);
    gfits_modify (&output[0].header, "DEC1", "%lf", 1, region[0].Dmax);
  }

  code = photcode[0].code;
  Nsec = GetPhotcodeNsec (code);
  needMeas = (Nsec == -1);

  /* select the average objects in this region */
  Nave = output[0].Naverage;
  NAVE = output[0].Naverage + 1000;
  REALLOCATE (output[0].average, Average, NAVE);
  REALLOCATE (output[0].secfilt, SecFilt, NAVE*Nsecfilt);

  Nmeas = output[0].Nmeasure;
  NMEAS = output[0].Nmeasure + 1000;
  REALLOCATE (output[0].measure, Measure, NMEAS);

  // if MaxDensityUse is set, we need to determine the MagLimitValue for this catalog by
  // examining the magnitude distribution.  generate a magnitude histogram
  if (MaxDensityUse) {
    int bin, Nmax, Nbins, Nsum, *Nmag;
    float MagMin, MagMax, dMag;
    double Rmin, Rmax, Dmin, Dmax;

    MagLimitUse = TRUE; // we will use the MagLimit implied by the mag distribution below

    gfits_scan (&catalog[0].header, "RA0",  "%lf", 1, &Rmin);
    gfits_scan (&catalog[0].header, "DEC0", "%lf", 1, &Dmin);
    gfits_scan (&catalog[0].header, "RA1",  "%lf", 1, &Rmax);
    gfits_scan (&catalog[0].header, "DEC1", "%lf", 1, &Dmax);

    AREA = fabs(Dmax - Dmin) * fabs(Rmax - Rmin) * cos (0.5*RAD_DEG*(Dmax + Dmin));
    if (VERBOSE) fprintf (stderr, "extracting from catalog covering region %f,%f to %f,%f for area %f\n", Rmin, Dmin, Rmax, Dmax, AREA);
    assert (AREA > 0);

    Nmax = MaxDensityValue * AREA;

    dMag = 0.1;
    MagMin = 0.0;
    MagMax = 32.0;
    Nbins = (MagMax - MagMin) / dMag;
    ALLOCATE (Nmag, int, Nbins);
    bzero (Nmag, Nbins * sizeof(int));

    for (i = 0; i < catalog[0].Naverage; i++) {
      // n = catalog[0].measure[i].averef;
      mag = NAN;
      if (needMeas) {
        offset = catalog[0].average[i].measureOffset;
        for (m = 0; m < catalog[0].average[i].Nmeasure; m++) {
          if (catalog[0].measure[offset + m].photcode == code) {
            mag = PhotRel (&catalog[0].measure[offset + m], &catalog[0].average[i], &catalog[0].secfilt[i*Nsecfilt], MAG_CLASS_PSF);
            break;
          }
        }
      } else {
        mag = catalog[0].secfilt[i*Nsecfilt + Nsec].MpsfChp;
      }
      if (isnan(mag)) continue;
      if (mag < MinMagValue) continue;

      bin = (mag - MagMin) / dMag;
      if (bin < 0) continue;
      if (bin >= Nbins) continue;

      Nmag[bin] ++;
    }

    if (!MinMagUse) MinMagValue = MagMin;
    bin = (MinMagValue - MagMin) / dMag;
    bin = MIN (MAX (0, bin), Nbins);

    Nsum = 0;
    for (i = bin; (i < Nbins) && (Nsum < Nmax); i++) {
      Nsum += Nmag[i];
    }
    MagLimitValue = i*dMag + MagMin;
    if (VERBOSE) fprintf (stderr, "using %d (Nmax %d) stars in mag range %f - %f\n", Nsum, Nmax, MinMagValue, MagLimitValue);
  }

  for (i = 0; i < catalog[0].Naverage; i++) {
    // n = catalog[0].measure[i].averef;

    R = catalog[0].average[i].R;
    D = catalog[0].average[i].D;

    if (region[0].Rmin > region[0].Rmax) {
        // Rmin > Rmax : R may either be > Rmin or < Rmax:
        if ((R > region[0].Rmin) && (R < region[0].Rmax)) continue;
    } else {
        if (R < region[0].Rmin) continue;
        if (R > region[0].Rmax) continue;
    }
    if (D < region[0].Dmin) continue;
    if (D > region[0].Dmax) continue;

    // If either magnitude limit was specfied it a apply both.
    // If one of the limits is not used it is initialized to an unphysical value so test will succeed
    if (MagLimitUse || MinMagUse) {
      mag = NAN;
      if (needMeas) {
        offset = catalog[0].average[i].measureOffset;
        for (m = 0; m < catalog[0].average[i].Nmeasure; m++) {
          if (catalog[0].measure[offset + m].photcode == code) {
            mag = PhotRel (&catalog[0].measure[offset + m], &catalog[0].average[i], &catalog[0].secfilt[i*Nsecfilt], MAG_CLASS_PSF);
            break;
          }
        }
      } else {
        mag = catalog[0].secfilt[i*Nsecfilt + Nsec].MpsfChp;
      }
      if (isnan(mag) || (mag > MagLimitValue)) continue;
      if (isnan(mag) || (mag < MinMagValue)) continue;
    }

    output[0].average[Nave] = catalog[0].average[i];
    output[0].average[Nave].measureOffset = Nmeas;
    for (j = 0; j < Nsecfilt; j++) {
      output[0].secfilt[Nsecfilt*Nave + j] = catalog[0].secfilt[Nsecfilt*i + j];
    }

    if (needMeas) {
      offset = catalog[0].average[i].measureOffset;
      for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {
	output[0].measure[Nmeas] = catalog[0].measure[offset + j];
	output[0].measure[Nmeas].averef = Nave;
	Nmeas ++;
	CHECK_REALLOCATE (output[0].measure, Measure, NMEAS, Nmeas, 1000);
      }
    }

    Nave ++;
    if (Nave == NAVE) {
      NAVE += 1000;
      REALLOCATE (output[0].average, Average, NAVE);
      REALLOCATE (output[0].secfilt, SecFilt, NAVE*Nsecfilt);
    }
  }
  output[0].Naverage = Nave;
  output[0].Nmeasure = Nmeas;
  output[0].Nsecfilt_mem = Nave*Nsecfilt;

  fprintf (stderr, "output catalog has "OFF_T_FMT" stars ("OFF_T_FMT" measures, %d secfilt)\n",
            output[0].Naverage,  output[0].Nmeasure, output[0].Nsecfilt);
  return (TRUE);
}
