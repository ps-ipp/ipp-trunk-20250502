# include "addstar.h"

// determine relevant image information from the PHU header
int ReadSDSSHeader (Header *header, Image *image, int photcode) {

  int Nastro, ccdnum, hour, min, Nx, Ny;
  double tmp, sec, Cerror, ZeroPt;
  char *c, photname[64], line[80];

  // I need to convert a single obj file into a set of 5 images

  // XXX how do I define the image boundaries?
  // image[0].coords
  // image[0].NX, NY

  // image[0].tzero : MJD_U,G,R,I,Z in table header

  // test astrometry quality? 
  // image[0].cerror : ??
 
  // set photcodes for the 5 images (SDSS_U,G,R,I,Z)
  photcode = GetPhotcodeCodebyName ("SDSS_U");
  if (photcode == 0) {
    fprintf (stderr, "photcode %s not found in photcode table\n", photname);
    return (FALSE);
  }
  image[0].photcode = photcode;

  // calculate this from : C_OBS, TRACKING, and NY
  image[0].exptime = tmp;
  
  // image[0].apmifit = tmp;
  // image[0].dapmifit = tmp;
  // image[0].detection_limit 
  // image[0].saturation_limit
  // image[0].fwhm_x : SEEING_U, etc in table header
  // image[0].fwhm_y : SEEING_U, etc in table header

  // XXX longitude and latitude are known for SDSS
  // jd = ohana_sec_to_jd (image[0].tzero);
  // image[0].sidtime  = ohana_lst (jd, Longitude);
  // image[0].latitude = Latitude;

  // image[0].trate : from C_OBS
  // image[0].secz : ??
  // image[0].ccdnum : COLNUM?

  // secz is in units milli-airmass
  image[0].McalPSF   = 0.0;
  image[0].McalAPER  = 0.0;
  image[0].McalChiSq = NAN;
  image[0].dMcal     = NAN;
  image[0].code = 0;
  memset (image[0].dummy, 0, sizeof(image[0].dummy));

  // NAXIS2 for table:
  // image[0].nstar

  return (TRUE);
}
