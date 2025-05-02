# include "fakeastro.h"

static int Nbad_airmass = 0;

float airmass (float secz_image, double ra, double dec, double st, double latitude) {
  OHANA_UNUSED_PARAM(secz_image);

  double hour, cosz, secz;
  double rdec, rlat;

  /* ra, dec, latitude in dec deg; st in dec hours */

  /* hour : hour angle in degrees */
  rdec = RAD_DEG*dec;
  rlat = RAD_DEG*latitude;
  hour = 15.0*st - ra;
  cosz = sin (rdec) * sin (rlat) + cos (rdec) * cos (RAD_DEG*hour) * cos (rlat);
  secz = 1.000 / cosz;
  
  if (!isfinite(secz)) {
    Nbad_airmass ++;
  }
  if ((Nbad_airmass > 10) && ((Nbad_airmass % 1000) == 10)) {
    fprintf (stderr, "*** WARNING *** %d NaN airmass values : what is the problem?\n", Nbad_airmass);
  }
  if (Nbad_airmass > 1) {
    fprintf (stderr, "*** WARNING *** NaN airmass for detection\n");
    fprintf (stderr, "*** ra, dec : %f, %f | st : %f | lat : %f | cosz : %f\n", ra, dec, st, latitude, cosz);
  }

  return (secz);
}

// ha/dec -> az
float azimuth (double ha, double dec, double latitude) {

  double rdec, rlat, rha;
  double sinh, cosh;
  float az;

  rdec = RAD_DEG*dec;
  rha = RAD_DEG*ha;
  rlat = RAD_DEG*latitude;

  sinh = - cos (rdec) * sin (rha);
  cosh =   sin (rdec) * cos (rlat) - cos (rdec) * cos (rha) * sin (rlat);

  az = DEG_RAD * atan2 (sinh, cosh);

  return az;
}
