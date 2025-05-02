# include "setastrom.h"

// this code does both DCR and KH corrections, if both are specified.  

// XXX this function is dependent in two ways on the specific numerical values of the photcodes
// 1) the examined measurements are limited to GPC1 values based on the photcode range 10000 - 10600
// 2) the chip ID is determined from the numerical value of the photcode (chip = photcode % 100)

int update_catalog_setastrom (Catalog *catalog) {

  off_t i;

  // this is the special date when KH corrections stopped being needed
  time_t timeRef = ohana_date_to_sec("2011/05/11,00:00:00");

  int Nsecfilt = GetPhotcodeNsecfilt ();
  PhotCode *code_g = GetPhotcodebyName ("g");
  PhotCode *code_i = GetPhotcodebyName ("i");
  PhotCode *code_z = GetPhotcodebyName ("z");
  PhotCode *code_y = GetPhotcodebyName ("y");

  int Nsec_g = GetPhotcodeNsec(code_g->code);
  int Nsec_i = GetPhotcodeNsec(code_i->code);
  int Nsec_z = GetPhotcodeNsec(code_z->code);
  int Nsec_y = GetPhotcodeNsec(code_y->code);

  off_t found = 0;

  for (i = 0; i < catalog[0].Nmeasure; i++) {

    Measure *measure = &catalog[0].measure[i];

    // only do GPC1 data for now (true for both DCR and KH)
    if (measure[0].photcode < 10000) continue;
    if (measure[0].photcode > 10600) continue;
      
    int averef = measure->averef;
    Average *average = &catalog[0].average[averef];
    SecFilt *secfilt = &catalog[0].secfilt[averef*Nsecfilt];

    int chipID = measure[0].photcode % 100;

    // XXX hardwire the list of chips to KH correct
    // some notes:
    // XY17 surprisingly does not need correction
    // XY24 surprisingly does not need correction
    // XY36 surprisingly does not need correction

    int doKH = FALSE;
    switch (chipID) {
      case 4:
      case 5:
      case 6:
      case 14:
      case 15:
      case 16:
      case 25:
      case 26:
      case 27:
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
      case 67:
      case 71:
      case 72:
      case 73:
	doKH = TRUE;
	break;
      default:
	break;
    }

    double pltScale = fabs(measure[0].pltscale);
    double posAngle = FromShortDegrees(measure[0].posangle);

    /**** KH section ****/

    double dX_KH = 0.0;
    double dY_KH = 0.0;
    if (KH_FILE && doKH && (measure[0].t < timeRef)) {

      // correction may be Minst or Minst + 5.0*log(fwhm
      float fwhm_maj = FromShortPixels(measure[0].FWx);
      float Minst = PhotInst (measure, MAG_CLASS_PSF);
      // float MinstSB = Minst + 5.0*log10(fwhm_maj);

      // NOTE: the spline correction was measured on the nightly-science database using measurements
      // from the period in which fwhm_maj & fwhm_min were a factor of sqrt(2) too small.  I am making 
      // this correction here (but ideally I would re-measure the KH trend from PV2 data and re-generate 
      // the splines
      float MinstSB = Minst + 5.0*log10(fwhm_maj);

      // I should validate that the spline I'm using is SB corrected not Minst..
      // NOTE: it is SB, not raw Minst

      // XXX this is very ad hoc.  the splines are measured based on an extraction from dvo
      // the dvo values of fwhm_maj are sigmas (NOT FWHM!) in pixels.  
    
      // saturate fwhm_maj in the range 1.0 < fwhm_maj < 5.0
      // (images with really odd psf probably have poor astrometry anyway...)
      fwhm_maj = MIN(fwhm_maj,5.0);
      fwhm_maj = MAX(fwhm_maj,1.0);

      // correction is in arcseconds
      get_kh_correction (0, chipID, &dX_KH, &dY_KH, MinstSB);
    }

    /**** CAM section ****/

    double dX_CAM = 0.0;
    double dY_CAM = 0.0;
    if (CAM_ASTROM_FILE) {

      // camera systematic correction ("astroflat") depends on only the X,Y coordinate,
      // the filter, and the chip
      
      // 10134 : r, XY34 -> filtCode = 1
      int filtCode = (int)((measure->photcode % 1000) / 100); 

      // correction is in arcseconds
      CamAstromCorrectionValue (chipID, filtCode, measure->Xccd, measure->Yccd, &dX_CAM, &dY_CAM);
    }

    /**** DCR section ****/

    double dX_DCR = 0.0;
    double dY_DCR = 0.0;
    if (DCR_FILE) {

      // check if the color*airmass term is not-nan (Skip otherwise)

      // XXX hard-wire gpc1 photcodes for now:
      // I should use photcode ops: PhotCode *code = PhotCodeByCode(measure->photcode)
      // int equiv = code->equiv
      // Nsec = GetNsec(equiv)

      float dColor = 0.0;
      int filtCode = (int)(measure->photcode / 100); // eg, 101 = r
      switch (filtCode) {
	case 100:
	case 101:
	case 102: {
	  float gmag = secfilt[Nsec_g].MpsfChp;
	  float imag = secfilt[Nsec_i].MpsfChp;
	  dColor = average->refColorBlue - (gmag - imag);
	  break;
	}
	case 103:
	case 104: {
	  float zmag = secfilt[Nsec_z].MpsfChp;
	  float ymag = secfilt[Nsec_y].MpsfChp;
	  dColor = average->refColorRed - (zmag - ymag);
	  break;
	}
      }
      int filtSeq = filtCode % 100;
      if (!isfinite(dColor)) {
	goto skip_DCR;
      }

      // I need to get the parallactic angle, but I only have alt & az, s:o 
    
      // airmass = 1.0 / cos(90 - alt)
      // cos(90 - alt) = cos(zd) = sin(alt) = 1.0 / airmass
      // airmass = 1.0 / sin(alt)

      // airmass = 1.0 / cos(zd)
      // cos(zd) = 1.0 / airmass == sn_alt
      // sin(zd) = sqrt(1.0 - cos(zd)^2) == cs_alt
      // tan(zd) = sin(zd) / cos(zd) = cs_alt / sn_alt

      double sn_alt = 1.0 / measure->airmass;
      double cs_alt = sqrt(1.0 - SQ(sn_alt));
      double tan_zd = cs_alt / sn_alt;

      double cs_az  = cos(RAD_DEG*measure->az);
      double sn_az  = sin(RAD_DEG*measure->az);

# define LATITUDE (20.7070999146*RAD_DEG)
      double sn_lat = sin(LATITUDE);
      double cs_lat = cos(LATITUDE);

      // double sind = sn_alt * sn_lat + cs_alt * cs_az * cs_lat;
      // *dec  = DEG_RAD * asin (sind);

      // WARNING: these are NOT sin(ha),cos(ha) but are scaled
      // double ha = DEG_RAD * atan2 (sinh, cosh);

      // EAM 20151217: this is the old version of the code.  it is mathematically identical but does some extra work:
      // double sinh = -cs_alt * sn_az;
      // double cosh =  sn_alt * cs_lat - cs_alt * cs_az * sn_lat;
      // double r_ha = hypot(sinh, cosh);
      // double sn_ha = sinh / r_ha;
      // double cs_ha = cosh / r_ha;

      // sinh = -cs_az * sn_alt * sn_ha * sn_lat + sn_az * sn_alt * cs_ha - sn_ha * cs_alt * cs_lat;
      // cosh = -sn_az          * sn_ha * sn_lat - cs_az          * cs_ha;
      // double r_rot = hypot(sinh, cosh);
      // double sn_rot = -sinh / r_rot;
      // double cs_rot = +cosh / r_rot;

      // EAM 20151217: this is the new version:
      double sn_ha = -cs_alt * sn_az;
      double cs_ha =  sn_alt * cs_lat - cs_alt * cs_az * sn_lat;

      double sn_rot_r = -cs_az * sn_alt * sn_ha * sn_lat + sn_az * sn_alt * cs_ha - sn_ha * cs_alt * cs_lat;
      double cs_rot_r = -sn_az          * sn_ha * sn_lat - cs_az          * cs_ha;
      double r_rot = hypot(sn_rot_r, cs_rot_r);

      double sn_rot = -sn_rot_r / r_rot;
      double cs_rot =  cs_rot_r / r_rot;

      double dPx = 0.0; 
      double dPy = 0.0;

      float colorOffset = dColor * tan_zd;
      get_dcr_correction (filtSeq, &dPx, &dPy, colorOffset);

      // if we had x-terms, we would have:
      // double dPx = dRx *cs_rot + dD_s*sn_rot;
      // double dPy = dD_s*cs_rot - dRx *sn_rot;
      // double dR = dPx*cs_rot - dPy*sn_rot;
      // double dD = dPx*sn_rot + dPy*cs_rot;

      // since dPx = 0.0, instead we have: dPx, dPy as measured in dvo.dcr.sh
      double dR = -dPy*sn_rot;
      double dD = +dPy*cs_rot;

      // dR = (measure->R - average->R) * 3600 * dcos(dec) [ie, in the projected plane]
      // dD = (measure->D - average->D) * 3600
      // average->D = measure->D - dD/3600

      // now we need to rotate dR,dD into the chip frame and correction Xccd,Yccd

      // are the plate-scale and posangle correctly set in the db?

      // XXX the koppenhoefer code implies pos if angle from y -> D, (ie negative of the
      // below).  if so, apply sin(pos) -> -sin(pos)

      // if parity = sky:
      // double dR =  dX*cos(pos) + dY*sin(pos);
      // double dD = -dX*sin(pos) + dY*cos(pos);

      // XXX signs set here to reflect pos = -pos
      float posAngRad = posAngle * RAD_DEG;
      dX_DCR =  dR*cos(posAngRad) + dD*sin(posAngRad);
      dY_DCR = -dR*sin(posAngRad) + dD*cos(posAngRad);
    }

  skip_DCR:

    // do not modify the original Xccd,Yccd values
    // dR,dD are measured as measure.D - average.D
    // thus we correct back to the truth (average) with
    // average.D = measure.D - dD
    if (KH_FILE) {
      measure[0].XoffKH = -dX_KH / pltScale;
      measure[0].YoffKH = -dY_KH / pltScale;
    }
    if (KH_RESET) {
      measure[0].XoffKH = 0.0;
      measure[0].YoffKH = 0.0;
    }
    if (DCR_FILE) {
      measure[0].XoffDCR = -dX_DCR / pltScale;
      measure[0].YoffDCR = -dY_DCR / pltScale;
    }
    if (DCR_RESET) {
      measure[0].XoffDCR = 0.0;
      measure[0].YoffDCR = 0.0;
    }
    if (CAM_ASTROM_FILE) {
      measure[0].XoffCAM = -dX_CAM / pltScale;
      measure[0].YoffCAM = -dY_CAM / pltScale;
    }
    if (CAM_RESET) {
      measure[0].XoffDCR = 0.0;
      measure[0].YoffDCR = 0.0;
    }

    measure[0].Xfix = measure[0].Xccd;
    measure[0].Yfix = measure[0].Yccd;
    if (isfinite(measure[0].XoffKH) && isfinite(measure[0].YoffKH)) {
      measure[0].Xfix += measure[0].XoffKH;
      measure[0].Yfix += measure[0].YoffKH;
    }
    if (isfinite(measure[0].XoffDCR) && isfinite(measure[0].YoffDCR)) {
      measure[0].Xfix += measure[0].XoffDCR;
      measure[0].Yfix += measure[0].YoffDCR;
    }
    if (isfinite(measure[0].XoffCAM) && isfinite(measure[0].YoffCAM)) {
      measure[0].Xfix += measure[0].XoffCAM;
      measure[0].Yfix += measure[0].YoffCAM;
    }
    found ++;
  }

  if (found) {
    fprintf (stderr, "found "OFF_T_FMT" matches for setastrom\n", found);
  }

  return (TRUE);
}
