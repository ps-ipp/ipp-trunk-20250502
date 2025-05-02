# include <skycalc_internal.h>
# define UNUSED_PARAM(x)(void)(x)

/* Takes a coordinate pair and precesses it using matrix procedures
   as outlined in Taff's Computational Spherical Astronomy book.
   This is the so-called 'rigorous' method which should give very
   accurate answers all over the sky over an interval of several
   centuries.  Naked eye accuracy holds to ancient times, too.
   Precession constants used are the new IAU1976 -- the 'J2000'
   system. Angles in degrees, epochs in years */

void SC_precrot (double rorig, double dorig, double orig_epoch, double final_epoch, double *rf, double *df) {
  
  double ti, tf, zeta, z, theta;  /* all as per  Taff */
  double cosz, coszeta, costheta, sinz, sinzeta, sintheta;  /* ftns */
  double p11, p12, p13, p21, p22, p23, p31, p32, p33;
  /* elements of the rotation matrix */
  double radian_ra, radian_dec;
  double orig_x, orig_y, orig_z;
  double fin_x, fin_y, fin_z;   /* original and final unit ectors */

  ti = (orig_epoch - 2000.) / 100.;
  tf = (final_epoch - 2000. - 100. * ti) / 100.;

  zeta = (2306.2181 + 1.39656 * ti + 0.000139 * ti * ti) * tf +
    (0.30188 - 0.000344 * ti) * tf * tf + 0.017998 * tf * tf * tf;
  z = zeta + (0.79280 + 0.000410 * ti) * tf * tf + 0.000205 * tf * tf * tf;
  theta = (2004.3109 - 0.8533 * ti - 0.000217 * ti * ti) * tf
    - (0.42665 + 0.000217 * ti) * tf * tf - 0.041833 * tf * tf * tf;

  /* convert to radians */

  zeta = zeta / ARCSEC_IN_RADIAN;
  z = z / ARCSEC_IN_RADIAN;
  theta = theta / ARCSEC_IN_RADIAN;

  /* compute the necessary trig functions for speed and simplicity */

  cosz = cos(z);
  coszeta = cos(zeta);
  costheta = cos(theta);
  sinz = sin(z);
  sinzeta = sin(zeta);
  sintheta = sin(theta);

  /* compute the elements of the precession matrix */

  p11 = coszeta * cosz * costheta - sinzeta * sinz;
  p12 = -1. * sinzeta * cosz * costheta - coszeta * sinz;
  p13 = -1. * cosz * sintheta;

  p21 = coszeta * sinz * costheta + sinzeta * cosz;
  p22 = -1. * sinzeta * sinz * costheta + coszeta * cosz;
  p23 = -1. * sinz * sintheta;

  p31 = coszeta * sintheta;
  p32 = -1. * sinzeta * sintheta;
  p33 = costheta;

  /* transform original coordinates */

  radian_ra = rorig / HRS_IN_RADIAN;
  radian_dec = dorig / DEG_IN_RADIAN;

  orig_x = cos(radian_dec) * cos(radian_ra);
  orig_y = cos(radian_dec) *sin(radian_ra);
  orig_z = sin(radian_dec);
  /* (hard coded matrix multiplication ...) */
  fin_x = p11 * orig_x + p12 * orig_y + p13 * orig_z;
  fin_y = p21 * orig_x + p22 * orig_y + p23 * orig_z;
  fin_z = p31 * orig_x + p32 * orig_y + p33 * orig_z;

  /* convert back to spherical polar coords */

  SC_xyz_cel(fin_x, fin_y, fin_z, rf, df);

}


/* computes the geocentric coordinates from the geodetic
   (standard map-type) longitude, latitude, and height.
   These are assumed to be in decimal hours, decimal degrees, and
   meters respectively.  Notation generally follows 1992 Astr Almanac,
   p. K11 */

void SC_geocent (double geolong, double geolat, double height, double *x_geo, double *y_geo, double *z_geo) {

  double denom, C_geo, S_geo;

  geolat = geolat / DEG_IN_RADIAN;
  geolong = geolong / HRS_IN_RADIAN;
  denom = (1. - FLATTEN) * sin(geolat);
  denom = cos(geolat) * cos(geolat) + denom*denom;
  C_geo = 1. / sqrt(denom);
  S_geo = (1. - FLATTEN) * (1. - FLATTEN) * C_geo;
  C_geo = C_geo + height / EQUAT_RAD;  /* deviation from almanac
					  notation -- include height here. */
  S_geo = S_geo + height / EQUAT_RAD;
  *x_geo = C_geo * cos(geolat) * cos(geolong);
  *y_geo = C_geo * cos(geolat) * sin(geolong);
  *z_geo = S_geo * sin(geolat);
}

/* rotates ecliptic rectangular coords x, y, z to
   equatorial (all assumed of date.) */
void SC_eclrot(double jd, double *x, double *y, double *z) {
  UNUSED_PARAM(x);

  double incl;
  double ypr,zpr;
  double T;

  T = (jd - J2000) / 36525;  /* centuries since J2000 */

  incl = (23.439291 + T * (-0.0130042 - 0.00000016 * T))/DEG_IN_RADIAN;
/* 1992 Astron Almanac, p. B18, dropping the
   cubic term, which is 2 milli-arcsec! */
  ypr = cos(incl) * *y - sin(incl) * *z;
  zpr = sin(incl) * *y + cos(incl) * *z;
  *y = ypr;
  *z = zpr;
/* x remains the same. */
}

/* Given a julian date in 1900-2100, returns the correction
   delta t which is:
   TDT - UT (after 1983 and before 1993)
   ET - UT (before 1983)
   an extrapolated guess  (after 1993).

   For dates in the past (<= 1993) the value is linearly
   interpolated on 5-year intervals; for dates after the present,
   an extrapolation is used, because the true value of delta t
   cannot be predicted precisely.  Note that TDT is essentially the
   modern version of ephemeris time with a slightly cleaner
   definition.

   Where the algorithm shifts there is an approximately 0.1 second
   discontinuity.  Also, the 5-year linear interpolation scheme can
   lead to errors as large as 0.5 seconds in some cases, though
   usually rather smaller. */

double SC_etcorr (double jd) {

  double dates[20] = {1900,1905,1910,1915,1920,1925,1930,1935,1940,1945,
		      1950,1955,1960,1965,1970,1975,1980,1985,1990,1993};
  double delts[20]={-2.72,3.86,10.46,17.20,21.16,23.62,24.02,23.93,24.33,26.77,
		    29.15,31.07,33.15,35.73,40.18,45.48,50.54,54.34,56.86,59.12};
  double year, delt;
  short i;

  delt = 0.0;
  year = 1900. + (jd - 2415019.5) / 365.25;

  if(year < 1993.0 && year >= 1900.) {
    i = (year - 1900) / 5;
    delt = delts[i] +
      ((delts[i+1] - delts[i])/(dates[i+1] - dates[i])) * (year - dates[i]);
  }

  else if (year > 1993. && year < 2100.)
    delt = 33.15 + (2.164e-3) * (jd - 2436935.4);  /* rough extrapolation */

  else if (year < 1900) {
    delt = 0.;
  }

  else if (year >= 2100.) {
    delt = 180.; /* who knows? */
  }

  return (delt);

}

/* sets RA and DEC at zenith as defined by given time and date  */
void SC_set_zenith (struct SC_date_time date, double lat, double longit, double epoch, double *ra, double *dec)
{
  double jd, current_epoch;

  jd = SC_date_to_jd (date);

  if (jd < 0.) return;  /* nonexistent time. */

  *ra = SC_lst (jd, longit);

  *dec = lat;

  current_epoch = 2000. + (jd - J2000) / 365.25;

  SC_precrot (*ra,*dec,current_epoch,epoch,ra,dec);

}

