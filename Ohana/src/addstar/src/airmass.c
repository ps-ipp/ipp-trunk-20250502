# include "addstar.h"

static int AirmassQuality = TRUE;

void SetAirmassQuality (int quality) {
  AirmassQuality = quality;
}

static int Nbad_airmass = 0;

float airmass (float secz_image, double ra, double dec, double st, double latitude) {

  double hour, cosz, secz;
  double rdec, rlat;

  if (!AirmassQuality) return (secz_image);

  /*** make this optional? we may not have ST... ***/
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
  if (Nbad_airmass > 1) {
    fprintf (stderr, "*** WARNING *** NaN airmass for detection\n");
    fprintf (stderr, "*** ra, dec : %f, %f | st : %f | lat : %f | cosz : %f\n", ra, dec, st, latitude, cosz);
  }
  if (Nbad_airmass > 10) {
    fprintf (stderr, "*** ERROR *** more than 10 NaN airmass values for detections, giving up\n");
    fprintf (stderr, " (use -quick-airmass to use image airmass)\n");
    exit (2);
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
