# include "fakeastro.h"

// region corresponds to the catalog
int make_gaia_measures (Catalog *catalog) {

  int Nsecfilt = GetPhotcodeNsecfilt ();

  Average *average = catalog->average;
  SecFilt *secfilt = catalog->secfilt;
  StarPar *starpar = catalog->starpar;
  Measure *measure = catalog->measure;
  
  // we have starpar.R,D, which represent the true positions at the reference epoch, FAKEASTRO_REF_EPOCH
  // the GAIA stars are generated at locations for the GAIA EPOCH
  time_t timeRef = ohana_date_to_sec(FAKEASTRO_REF_EPOCH);
  time_t tzero_gaia = ohana_date_to_sec(FAKEASTRO_GAIA_EPOCH);

  // tzero, timeRef are in UNIX seconds, Toffset should be in years
  double Toffset = (tzero_gaia - timeRef) / 365.25 / 86400.0;

  // use photcode to get zero point
  PhotCode *codeG = GetPhotcodebyName ("GAIA_G_DR1");

  float ZP = MAX_MAG_GAIA + 6.5; // this is chosen to give stars at the limit a flux of 400.0 counts (crude, yes)
  float SkyCts = 1000.0; // this is chosen to make SN @ limit ~ 10.0 (

  int Nmeasure = catalog->Nmeasure;
  int NMEASURE = catalog->Nmeasure;

  int Nsec = 2; // i-band

  int i;
  for (i = 0; i < catalog->Naverage; i++) {

    if (Nmeasure >= NMEASURE) {
      NMEASURE = Nmeasure + 1000;
      REALLOCATE (catalog[0].measure, Measure, NMEASURE);
      measure = catalog[0].measure;
    }

    int Nstarpar = average[i].starparOffset;

    // make a crude G-i color for now:
    double G_PS1 = secfilt[i*Nsecfilt + Nsec].MpsfChp; // make all stars have G-i = 1.0
    if (G_PS1 > MAX_MAG_GAIA) continue; // only generate GAIA detections for objects with G_PS1 < 21.0

    if (isnan(secfilt[i*Nsecfilt + Nsec].MpsfChp)) {
      // look for a non-NAN secfilt mag and just use that (it is not super important)
      int ns;
      for (ns = 0; ns < Nsecfilt; ns++) {
	if (!isnan(secfilt[i*Nsecfilt + ns].MpsfChp)) break;
      }
      if (ns == Nsecfilt) continue; // no non-nan

      G_PS1 = secfilt[i*Nsecfilt + ns].MpsfChp; // pretend secfilt.M = G
      if (G_PS1 > MAX_MAG_GAIA) continue; // only generate GAIA detections for objects with G_PS1 < 21.0
    }

    double Minst = G_PS1 - ZP; 

    double Counts = pow(10.0, -0.4*Minst);

    double SN = Counts / sqrt(SkyCts + Counts);

    // true position from src catalog
    double Rtru = starpar[Nstarpar].R;
    double Dtru = starpar[Nstarpar].D;

    // observed position is scattered from true position by:
    // * proper motion
    // * gaussian scatter (~ seeing) 
    double uR = starpar[Nstarpar].uRA; // starpar are in arcsec / year
    double uD = starpar[Nstarpar].uDEC;
    
    // uR,uD in linear (arcsec / yr)
    double dRpm = uR*Toffset;
    double dDpm = uD*Toffset;

    // uR,uD in linear arcsec
    double dRsee = ohana_gaussdev_rnd (0.0, 0.01 / SN);
    double dDsee = ohana_gaussdev_rnd (0.0, 0.01 / SN);

    // XXX TEST
    // dDsee = dRsee = 0.0;

    double dRoff = (dRpm + dRsee) / 3600.0;
    double dDoff = (dDpm + dDsee) / 3600.0;

    double Robs = Rtru + dRoff / cos(Dtru*RAD_DEG);
    double Dobs = Dtru + dDoff;

    dvo_measure_init (&measure[Nmeasure]);

    measure[Nmeasure].R = Robs;
    measure[Nmeasure].D = Dobs;

    measure[Nmeasure].M      = G_PS1;
    measure[Nmeasure].dM     = 1.0 / SN;

    measure[Nmeasure].Sky        = SkyCts;
    measure[Nmeasure].dSky       = sqrt(SkyCts);
    measure[Nmeasure].photFlags  = 0;
    measure[Nmeasure].photFlags2 = 0;
    measure[Nmeasure].airmass = 1.0;
    measure[Nmeasure].az      = 0.0; // irrelevant
    measure[Nmeasure].McalPSF = 0.0;
    measure[Nmeasure].McalAPER= 0.0;
    measure[Nmeasure].t       = tzero_gaia;
    measure[Nmeasure].dt      = 0.0;
    measure[Nmeasure].photcode = codeG->code;

    measure[Nmeasure].averef   = i;
    measure[Nmeasure].objID    = average[i].objID;
    measure[Nmeasure].catID    = average[i].catID;

    measure[Nmeasure].imageID = 0;

    // This is may optionally be replaced by the internal sequence (see FilterStars.c)
    measure[Nmeasure].detID      = 0;
    Nmeasure ++;
  }

  if (VERBOSE) fprintf (stderr, "added %d gaia entries to %d stars\n",  (int) Nmeasure, (int) catalog->Naverage);

  catalog->Nmeasure = Nmeasure;
  return TRUE;
}
