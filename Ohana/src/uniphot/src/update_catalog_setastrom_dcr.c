# include "setastrom.h"

// this DCR code is largely hard-wired for gpc1 data

int update_catalog_setastrom_dcr (Catalog *catalog) {

  off_t i, found;

  Nsecfilt = GetPhotcodeNsecfilt ();
  PhotCode *code_g = GetPhotcodebyName ("g");
  PhotCode *code_i = GetPhotcodebyName ("i");
  PhotCode *code_z = GetPhotcodebyName ("z");
  PhotCode *code_y = GetPhotcodebyName ("y");

  Nsec_g = GetPhotcodeNsec(code_g->code);
  Nsec_i = GetPhotcodeNsec(code_i->code);
  Nsec_z = GetPhotcodeNsec(code_z->code);
  Nsec_y = GetPhotcodeNsec(code_y->code);

  for (i = 0; i < catalog[0].Nmeasure; i++) {

    Measure *measure = &catalog[0].measure[i];

    double Xccd = measure[0].Xccd;
    double Yccd = measure[0].Yccd;
    double pltScale = fabs(measure[0].pltscale);
    double posAngle = FromShortDegrees(measure[0].posangle);

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

    double sn_ha = -cs_alt * sn_az;
    double cs_ha =  sn_alt * cos(LATITUDE) - cs_alt * cs_az * sin(LATITUDE);

    double sn_rot = -cs_az * sn_alt * sn_ha * sin(LATITUDE) + sn_az * sn_alt * cs_ha - sn_ha * cs_alt * cos(LATITUDE);
    double cs_rot = -sn_az *          sn_ha * sin(LATITUDE) - cs_az          * cs_ha;

    float dColor = 0.0;
    float DCRslope = 0.0;

    // XXX hard-wire gpc1 photcodes for now:
    // I should use photcode ops: PhotCode *code = PhotCodeByCode(measure->photcode)
    // int equiv = code->equiv
    // Nsec = GetNsec(equiv)

    int filtCode = (int)(measure->photcode / 100); // eg, 101 = r
    switch (filtCode) {
      case 100:
      case 101:
      case 102:
	float gmag = secfilt[Nsec_g].M;
	float imag = secfilt[Nsec_i].M;
	dColor = average->refColorBlue - (gmag - imag);
	break;
      case 103:
      case 104:
	float zmag = secfilt[Nsec_z].M;
	float ymag = secfilt[Nsec_y].M;
	dColor = average->refColorRed - (zmag - ymag);
	break;
    }
    int filtSeq = filtCode % 100;
    float colorOffset = dColor * tan_dz;

    get_dcr_correction (filtSeq, double *dPx, double *dPy, colorOffset);

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

    double dX =  dR*cos(pos) - dD*sin(pos);
    double dY =  dR*sin(pos) + dD*cos(pos);

    // if parity != sky:
    // double dD =  dY*cos(pos) + dX*sin(pos);
    // double dR =  dY*sin(pos) - dX*cos(pos);
    // double dX = -dR*cos(pos) + dD*sin(pos);
    // double dY =  dR*sin(pos) + dD*cos(pos);

    // do not modify the original Xccd,Yccd values

    // why is KH added but DCR subtracted??

    // ANSWER: both KH and DCR trends are measured by extracted dR,dD from the database
    // (mextract dR dD).  But the KH analysis was done before measure carried R,D when it
    // instead carried dR,dD.  In the current definition in dbExtractMeasures, dR,dD =
    // (measure.R,D - average.R,D) * 3600, but in the old format, measure.dR,dD were
    // defined as (average.R,D - measure.R,D)*3600).

    measure[0].XoffDCR = -dX / pltScale;
    measure[0].YoffDCR = -dY / pltScale;

    found ++;
  }

  if (found) {
    fprintf (stderr, "found "OFF_T_FMT" matches\n", found);
  }

  return (TRUE);
}
