# include <skycalc_internal.h>

/* Low precision formulae for the sun, from Almanac p. C24 (1990) */
/* ra and dec are returned as decimal hours and decimal degrees. */
void SC_lpsun (double jd, double *ra, double *dec) {

  double n, L, g, lambda,epsilon,x,y,z;

  n = jd - J2000;
  L = 280.460 + 0.9856474 * n;
  g = (357.528 + 0.9856003 * n)/DEG_IN_RADIAN;
  lambda = (L + 1.915 * sin(g) + 0.020 * sin(2. * g))/DEG_IN_RADIAN;
  epsilon = (23.439 - 0.0000004 * n)/DEG_IN_RADIAN;

  x = cos(lambda);
  y = cos(epsilon) * sin(lambda);
  z = sin(epsilon)*sin(lambda);

  *ra = (SC_atan_circ(x,y))*HRS_IN_RADIAN;
  *dec = (asin(z))*DEG_IN_RADIAN;
}

/* returns jd at which sun is at a given
   altitude, given jdguess as a starting point. Uses
   low-precision sun, which is plenty good enough. */
double SC_jd_sun_alt (double alt, double jdguess, double lat, double longit) {

  double jdout;
  double deriv, err, del = 0.002;
  double ra,dec,ha,alt2,alt3,az;
  short i = 0;
  
  /* first guess */
  
  SC_lpsun (jdguess, &ra, &dec);
  ha = SC_lst (jdguess,longit) - ra;
  alt2 = SC_altit (dec,ha,lat,&az);
  jdguess = jdguess + del;
  SC_lpsun (jdguess,&ra,&dec);
  alt3 = SC_altit(dec,(SC_lst(jdguess,longit) - ra),lat,&az);
  err = alt3 - alt;
  deriv = (alt3 - alt2) / del;
  while((fabs(err) > 0.1) && (i < 10)) {
    jdguess = jdguess - err/deriv;
    SC_lpsun(jdguess,&ra,&dec);
    alt3 = SC_altit(dec,(SC_lst(jdguess,longit) - ra),lat,&az);
    err = alt3 - alt;
    i++;
    if(i == 9) printf ("Sunrise, set, or twilight calculation not converging!\n");
  }
  if(i >= 9) jdguess = -1000.;
  jdout = jdguess;
  return(jdout);
}

/* Given site position, prints Sun info for the given night. */
/* dates are all in UT now */
double SC_sunset_tonight (struct SC_date_time date, double lat, double longit, double elev) {

  double jd, jdmid0, jdmid, stmid;
  double rasun, decsun, horiz;
  double hasunset, jdsunset;
  double dt, lst0, lst1, djd;
  struct SC_date_time date_midnight;

  horiz = sqrt (2. * elev / 6378140.) * DEG_IN_RADIAN;

  /* find offset in hours from longit to greenwich */
  jd = SC_date_to_jd (date);  /* true jd now */
  lst0 = SC_lst (jd, 0.0);    /* lst at long = 0 */
  lst1 = SC_lst (jd, longit); /* local lst now */
  dt = lst0 - lst1;
  if (dt < 0) dt += 24;
	
  /* midnight at greenwich */
  date_midnight = date;
  date_midnight.h = 0;
  date_midnight.mn = 0;
  date_midnight.s = 0;
	
  /* find jd for local midnight, select the *closest* midnight */
  jdmid0 = SC_date_to_jd (date_midnight);
  jdmid = jdmid0 + dt / 24.0;
  djd = jd - jdmid;
  if (djd < -0.5) jdmid -= 1.0;
  if (djd >  0.5) jdmid += 1.0;
  stmid = SC_lst (jdmid,longit); 

  /* sunset / sunrise hour angle */
  SC_lpsun (jdmid, &rasun, &decsun);
  hasunset = SC_ha_alt (decsun, lat, -(0.83+horiz));
  if(hasunset > 900.) {  /* flag for never sets */
    return (-1);
  }
  if(hasunset < -900.) {
    return (-1);
  }

  /* find sunset time */
  jdsunset = jdmid + SC_adj_time(rasun+hasunset-stmid)/24.;
  jdsunset = SC_jd_sun_alt (-(0.83+horiz),jdsunset,lat,longit);

  return (jdsunset);

}

/* Given site position, prints Sun info for the given night. */
/* dates are all in UT now */
double SC_sunrise_tonight (struct SC_date_time date, double lat, double longit, double elev) {

  double jd, jdmid, stmid;
  double rasun, decsun, horiz;
  double hasunset, jdsunrise;
  double dt, lst0, lst1, djd;
  struct SC_date_time date_midnight;

  horiz = sqrt (2. * elev / 6378140.) * DEG_IN_RADIAN;

  /* find offset in hours from longit to greenwich */
  jd = SC_date_to_jd (date);  /* true jd now */
  lst0 = SC_lst (jd, 0.0);    /* lst at long = 0 */
  lst1 = SC_lst (jd, longit); /* local lst now */
  dt = lst0 - lst1;
  if (dt < 0) dt += 24;
	
  /* midnight at greenwich */
  date_midnight = date;
  date_midnight.h = 0;
  date_midnight.mn = 0;
  date_midnight.s = 0;
	
  /* find jd for local midnight, select the *closest* midnight */
  jdmid = SC_date_to_jd (date_midnight) - dt / 24.0;
  djd = jd - jdmid;
  if (djd < -0.5) jdmid -= 1.0;
  if (djd >  0.5) jdmid += 1.0;
  stmid = SC_lst (jdmid,longit); 

  /* sunset / sunrise hour angle */
  SC_lpsun (jdmid, &rasun, &decsun);
  hasunset = SC_ha_alt (decsun, lat, -(0.83+horiz));
  if(hasunset > 900.) {  /* flag for never sets */
    return (-1);
  }
  if(hasunset < -900.) {
    fprintf (stderr, "Sun down all day!\n");
    return (-1);
  }

  /* find sunrise time */
  jdsunrise = jdmid + SC_adj_time(rasun-hasunset-stmid)/24.;
  jdsunrise = SC_jd_sun_alt(-(0.83+horiz),jdsunrise,lat,longit);

  return (jdsunrise);
}
