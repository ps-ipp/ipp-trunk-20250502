# include "setastrom.h"

// XXX this function is dependent in two ways on the specific numerical values of the photcodes
// 1) the examined measurements are limited to GPC1 values based on the photcode range 10000 - 10600
// 2) the chip ID is determined from the numerical value of the photcode (chip = photcode % 100)

int update_catalog_setastrom (Catalog *catalog) {

  off_t i, found;

  time_t timeRef = ohana_date_to_sec("2011/05/11,00:00:00");

  found = 0;

  for (i = 0; i < catalog[0].Nmeasure; i++) {

    Measure *measure = &catalog[0].measure[i];

    // XXX only do GPC1 data for now
    if (measure[0].photcode < 10000) continue;
    if (measure[0].photcode > 10600) continue;
      
    // XXX hardwire the list of chips to correct
    // some notes:
    // XY24 surprisingly does not need correction
    // XY27 has poor astrometry with a weird correction for all mags
    // XY36 surprisingly does not need correction
    // XY67 has poor astrometry with a weird correction for all mags

    int chipID = measure[0].photcode % 100;
    switch (chipID) {
      case 4:
      case 5:
      case 6:
      case 14:
      case 15:
      case 16:
      case 17:
      case 25:
      case 26:
      case 34:
      case 35:
      case 37:
      case 40:
      case 41:
      case 42:
      case 43:
      case 50:
      case 51:
      case 52:
      case 53:
      case 60:
      case 61:
      case 62:
      case 63:
      case 71:
      case 72:
      case 73:
	break;
      default:
	measure[0].Xfix = measure[0].Xccd;
	measure[0].Yfix = measure[0].Yccd;
	continue;
    }

    double Xccd = measure[0].Xccd;
    double Yccd = measure[0].Yccd;
    double pltScale = fabs(measure[0].pltscale);

    // this is not needed, is it?
    // double posAngle = FromShortDegrees(measure[0].posangle);

    // I need to know which chip we have.  use photcode assuming range
    int chip = measure[0].photcode % 100;

    // correction may be Minst or Minst + 5.0*log(fwhm
    float fwhm_maj = FromShortPixels(measure[0].FWx);
    float Minst = PhotInst (measure, MAG_CLASS_PSF);
    float MinstSB = Minst + 5.0*log10(fwhm_maj);

    // I should validate that the spline I'm using is SB corrected not Minst...

    // XXX this is very ad hoc.  the splines are measured based on an extraction from dvo
    // the dvo values of fwhm_maj are sigmas (NOT FWHM!) in pixels.  
    
    // saturate fwhm_maj in the range 1.0 < fwhm_maj < 5.0
    // (images with really odd psf probably have poor astrometry anyway...)
    fwhm_maj = MIN(fwhm_maj,5.0);
    fwhm_maj = MAX(fwhm_maj,1.0);

    double dX;
    double dY;

    // correction is in arcseconds
    if (measure[0].t < timeRef) {
      get_astrom_correction (0, chip, &dX, &dY, MinstSB);
    } else {
      // XXX do not correct new data
      // get_astrom_correction (1, chip, &dX, &dY, MinstSB);
      dX = 0.0;
      dY = 0.0;
    }

    // do not modify the original Xccd,Yccd values
    measure[0].XoffKH = Xccd + dX / pltScale;
    measure[0].YoffKH = Yccd + dY / pltScale;
    measure[0].Xfix   = measure[0].XoffKH;
    measure[0].Yfix   = measure[0].YoffKH;

    found ++;
  }

  if (found) {
    fprintf (stderr, "found "OFF_T_FMT" matches\n", found);
  }

  return (TRUE);
}
