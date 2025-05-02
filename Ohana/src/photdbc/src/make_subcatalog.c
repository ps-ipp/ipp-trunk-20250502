# include "photdbc.h"

// copy a catalog to a new subcatalog, applying some filters
// the supplied subcatalog must already be locked, opened, and created
int make_subcatalog (Catalog *subcatalog, Catalog *catalog, SkyRegion *region) {
  
  int found;
  off_t i, j, k, offset;
  off_t Nm, Nsecfilt;
  double mag, minMag, minSigma;
  int keep, *secKeep;
  PhotCode *photcode;
  
  Nsecfilt = GetPhotcodeNsecfilt ();
  assert (catalog[0].Nsecfilt == Nsecfilt);

  // set up a list of SEC entries to ignore when evaluating a source
  ALLOCATE (secKeep, int, Nsecfilt);
  for (i = 0; i < Nsecfilt; i++) {
      secKeep[i] = FALSE;
      photcode = GetPhotcodebyNsec(i);
      for (k = 0; k < NphotcodesKeep; k++) {
	  if (photcodesKeep[k][0].code != photcode[0].code) continue; 
	  secKeep[i] = TRUE;
      }
  }

  /* we are moving only the subset of measurements from catalog[0] to subcatalog[0] */
  off_t NAVERAGE = 50;   off_t Naverage = 0;
  off_t NMEASURE = 1000; off_t Nmeasure = 0;
  off_t NLENSING = 1000; off_t Nlensing = 0;
  off_t NLENSOBJ = 1000; off_t Nlensobj = 0;
  off_t NSTARPAR = 1000; off_t Nstarpar = 0;
  off_t NGALPHOT = 1000; off_t Ngalphot = 0;

  REALLOCATE (subcatalog[0].average, Average, NAVERAGE);
  REALLOCATE (subcatalog[0].secfilt, SecFilt, NAVERAGE*Nsecfilt);
  REALLOCATE (subcatalog[0].measure, Measure, NMEASURE);
  REALLOCATE (subcatalog[0].lensing, Lensing, NLENSING);
  REALLOCATE (subcatalog[0].lensobj, Lensobj, NLENSOBJ);
  REALLOCATE (subcatalog[0].starpar, StarPar, NSTARPAR);
  REALLOCATE (subcatalog[0].galphot, GalPhot, NGALPHOT);

  for (i = 0; i < catalog[0].Naverage; i++) {
    // perform exclusions based on average properties

    // XXX: temporary check make sure that this object belongs in this region
    // used to fix the pole area in the reference catalog
    if (0) {
        double R = catalog[0].average[i].R;
        double D = catalog[0].average[i].D;
        if (R <= region->Rmin) continue;
        if (R >  region->Rmax) continue;
        if (D <= region->Dmin) continue;
        if (D >  region->Dmax) continue;
    }

    // exclude stars with too few measurements
    if (NMEAS_MIN && (catalog[0].average[i].Nmeasure < NMEAS_MIN)) continue; 

    if (AVE_SIGMA_LIM) {
      // if all of the average magnitude errors are >AVE_SIGMA_LIM, drop the object
      keep = FALSE;
      for (j = 0; !keep && (j < Nsecfilt); j++) {
	if (catalog[0].secfilt[Nsecfilt*i+j].dMpsfChp < AVE_SIGMA_LIM) {
	  keep = TRUE;
	}
      }
      if (!keep) continue;
    }

    // exclude stars with too few measurements
    if (NCODE_MIN) {
      // drop if all of the allowed average photcodes have ncode < NCODE_MIN values
      keep = FALSE;
      for (j = 0; !keep && (j < Nsecfilt); j++) {
	  if (secKeep[j]) continue;
	  if (catalog[0].secfilt[Nsecfilt*i+j].Ncode >= NCODE_MIN) {
	      keep = TRUE;
	}
      }
      if (!keep) continue;
    }

    /* assign average and secfilt values */
    subcatalog[0].average[Naverage] = catalog[0].average[i];
    subcatalog[0].average[Naverage].measureOffset = Nmeasure;
    for (j = 0; j < Nsecfilt; j++) {
      subcatalog[0].secfilt[Nsecfilt*Naverage+j] = catalog[0].secfilt[Nsecfilt*i+j];
    }

    // if the input catalog is an old type, generate the catID entries:
    if (catalog[0].catformat < DVO_FORMAT_PS1_V1) {
      subcatalog[0].average[Naverage].catID = catalog[0].catID;
    }

    minMag   = 32;
    minSigma = 32;
    Nm = 0;
    for (j = 0; !SKIP_MEASURE && (j < catalog[0].average[i].Nmeasure); j++) {

      offset = catalog[0].average[i].measureOffset + j;

      # if 0
      if (DropNonStellar && catalog[0].measure[offset].dophot != 1) continue;
      # endif

      // remove certain photcodes from the output measurements
      if (NphotcodesDrop > 0) {
	  found = FALSE;
	  for (k = 0; (k < NphotcodesDrop) && !found; k++) {
	      if (photcodesDrop[k][0].code == catalog[0].measure[offset].photcode) found = TRUE;
	      if (photcodesDrop[k][0].code == GetPhotcodeEquivCodebyCode(catalog[0].measure[offset].photcode)) found = TRUE;
	  }
	  if (found) continue;
      }  
  
      // ignore certain photcodes to assess the measurements
      if (NphotcodesKeep > 0) {
	  found = FALSE;
	  for (k = 0; (k < NphotcodesKeep) && !found; k++) {
	      if (photcodesKeep[k][0].code == catalog[0].measure[offset].photcode) found = TRUE;
	      if (photcodesKeep[k][0].code == GetPhotcodeEquivCodebyCode(catalog[0].measure[offset].photcode)) found = TRUE;
	  }
	  if (found) goto keep;
      }  
  
      // exclude measurements by measurement error -- drop exactly this measurement 
      if (SIGMA_MAX && (catalog[0].measure[offset].dM > SIGMA_MAX)) continue;

      // select measurements by mag limit -- drop exactly this measurement  
      if (ExcludeByInstMag) {
	mag = PhotInst (&catalog[0].measure[offset], MAG_CLASS_PSF);
	if (mag < INST_MAG_MIN) continue;
	if (mag > INST_MAG_MAX) continue;
      }

      // check measurements for this object -- drop object if no measurements pass
      if (ExcludeByMinSigma) {
	  minSigma = MIN (minSigma, catalog[0].measure[offset].dM);
      }

      // check measurements for this object -- drop object if no measurements pass
      if (ExcludeByMaxMinMag) {
	mag = PhotSys (&catalog[0].measure[offset], &catalog[0].average[i], &catalog[0].secfilt[i*Nsecfilt], MAG_CLASS_PSF);
	minMag = MIN (minMag, mag);
      }

    keep:

      subcatalog[0].measure[Nmeasure]        = catalog[0].measure[offset];
      subcatalog[0].measure[Nmeasure].averef = Naverage;

      // if the input catalog is an old type, generate the catID entries:
      if (catalog[0].catformat < DVO_FORMAT_PS1_V1) {
	subcatalog[0].measure[Nmeasure].catID = catalog[0].catID;
      }

      Nmeasure ++;
      Nm ++;
      if (Nmeasure == NMEASURE) {
	NMEASURE += 1000;
	REALLOCATE (subcatalog[0].measure, Measure, NMEASURE);
      }
    }
    // End of average[i] loop

    // exclude faint objects
    if (ExcludeByMaxMinMag && (minMag > MAX_MIN_MAG)) {
      Nmeasure -= Nm;
      continue; 
    }

    // exclude faint objects
    if (ExcludeByMinSigma && (minSigma > SIGMA_MIN_KEEP)) {
      Nmeasure -= Nm;
      continue; 
    }

    // after measurement exclusion, exclude stars with too few measurements
    if (NMEAS_MIN_FILTERED && (Nm < NMEAS_MIN_FILTERED)) {
      Nmeasure -= Nm;
      continue; 
    }

    subcatalog[0].average[Naverage].Nmissing = 0;
    subcatalog[0].average[Naverage].Nmeasure = Nm;

    // **** lensing
    Nm = 0;
    subcatalog[0].average[Naverage].lensingOffset = Nlensing;
    for (j = 0; !SKIP_LENSING && (j < catalog[0].average[i].Nlensing); j++) {

      offset = catalog[0].average[i].lensingOffset + j;

      subcatalog[0].lensing[Nlensing]        = catalog[0].lensing[offset];
      subcatalog[0].lensing[Nlensing].averef = Naverage;

      Nlensing ++;
      Nm ++;
      if (Nlensing == NLENSING) {
	NLENSING += 1000;
	REALLOCATE (subcatalog[0].lensing, Lensing, NLENSING);
      }
    }
    subcatalog[0].average[Naverage].Nlensing = Nm;

    // **** lensobj
    Nm = 0;
    subcatalog[0].average[Naverage].lensobjOffset = Nlensobj;
    for (j = 0; !SKIP_LENSOBJ && (j < catalog[0].average[i].Nlensobj); j++) {

      offset = catalog[0].average[i].lensobjOffset + j;

      subcatalog[0].lensobj[Nlensobj]        = catalog[0].lensobj[offset];

      Nlensobj ++;
      Nm ++;
      if (Nlensobj == NLENSOBJ) {
	NLENSOBJ += 1000;
	REALLOCATE (subcatalog[0].lensobj, Lensobj, NLENSOBJ);
      }
    }
    subcatalog[0].average[Naverage].Nlensobj = Nm;

    // **** starpar
    Nm = 0;
    subcatalog[0].average[Naverage].starparOffset = Nstarpar;
    for (j = 0; !SKIP_STARPAR && (j < catalog[0].average[i].Nstarpar); j++) {

      offset = catalog[0].average[i].starparOffset + j;

      subcatalog[0].starpar[Nstarpar]        = catalog[0].starpar[offset];
      subcatalog[0].starpar[Nstarpar].averef = Naverage;

      Nstarpar ++;
      Nm ++;
      if (Nstarpar == NSTARPAR) {
	NSTARPAR += 1000;
	REALLOCATE (subcatalog[0].starpar, StarPar, NSTARPAR);
      }
    }
    subcatalog[0].average[Naverage].Nstarpar = Nm;

    // **** galphot
    Nm = 0;
    subcatalog[0].average[Naverage].galphotOffset = Ngalphot;
    for (j = 0; !SKIP_GALPHOT && (j < catalog[0].average[i].Ngalphot); j++) {

      offset = catalog[0].average[i].galphotOffset + j;

      subcatalog[0].galphot[Ngalphot]        = catalog[0].galphot[offset];
      subcatalog[0].galphot[Ngalphot].averef = Naverage;

      Ngalphot ++;
      Nm ++;
      if (Ngalphot == NGALPHOT) {
	NGALPHOT += 1000;
	REALLOCATE (subcatalog[0].galphot, GalPhot, NGALPHOT);
      }
    }
    subcatalog[0].average[Naverage].Ngalphot = Nm;

    Naverage ++;
    if (Naverage == NAVERAGE) {
      NAVERAGE += 50;
      REALLOCATE (subcatalog[0].average, Average, NAVERAGE);
      REALLOCATE (subcatalog[0].secfilt, SecFilt, NAVERAGE*Nsecfilt);
    }
  }
  REALLOCATE (subcatalog[0].average, Average, MAX (Naverage, 1));
  REALLOCATE (subcatalog[0].measure, Measure, MAX (Nmeasure, 1));
  REALLOCATE (subcatalog[0].secfilt, SecFilt, Nsecfilt*MAX (Naverage, 1));
  REALLOCATE (subcatalog[0].lensing, Lensing, MAX (Nlensing, 1));
  REALLOCATE (subcatalog[0].lensobj, Lensobj, MAX (Nlensobj, 1));
  REALLOCATE (subcatalog[0].starpar, StarPar, MAX (Nstarpar, 1));
  REALLOCATE (subcatalog[0].galphot, GalPhot, MAX (Ngalphot, 1));

  subcatalog[0].Naverage = Naverage;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = Nsecfilt;
  subcatalog[0].Nlensing = Nlensing;
  subcatalog[0].Nlensobj = Nlensobj;
  subcatalog[0].Nstarpar = Nstarpar;
  subcatalog[0].Ngalphot = Ngalphot;

  subcatalog[0].Nsecfilt_mem = Naverage * Nsecfilt;

  // XXX for now, don't copy the missing entries (these should be re-computed)
  ALLOCATE (subcatalog[0].missing, Missing, 1);
  subcatalog[0].Nmissing = 0;

  if (VERBOSE) {
    fprintf (stderr, OFF_T_FMT": using "OFF_T_FMT" stars ("OFF_T_FMT" measures) for catalog\n", i, 
	     subcatalog[0].Naverage, subcatalog[0].Nmeasure);
  }
  return (TRUE);
}

/** 
    the purpose of this function is to create a subset database.  there are several ways this could be done:

    * reject all measurements from all objects that fail to meet some criteria (objects may lose measurements and/or be dropped)

    * only reject an object if all measurements fail to meet some criteria (ie, keep all measurements if any measurement passes)

    * only apply the keep / reject criteria to certain photcodes


    ***

    * options:

    minimum value for nmeas : NMEAS_MIN
    minimum value for nmeas : NMEAS_MIN_FILTERED
    minimum value for ncode : NCODE_MIN (drop if all ncode < NCODE_MIN) (respect photcodes)
    reject faint meas       : SIGMA_MAX
    reject faint source     : SIGMA_MIN_KEEP (keep source source if its minimum dMag < this limit)

    **/
