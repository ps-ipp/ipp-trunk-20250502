# include "ohana.h"

/***** convert [-]00:00:00 to 0.0000 ****/
int ohana_dms_to_ddd (double *Value, char *string) {
  
  int valid, neg, status;
  double tmp, value;
  char *p1, *p2, *px;

  valid = FALSE; 
  neg = FALSE;
  stripwhite (string);
  p1 = string;
  px = string + strlen(string);

  if (string[0] == '-') { 
    valid = TRUE; 
    neg = TRUE;
    p1 = &string[1];
  }
  if (string[0] == '+') { 
    valid = TRUE; 
    neg = FALSE;
    p1 = &string[1];
  }
  if (isdigit(string[0])) { 
    valid = TRUE;
    p1 = &string[0];
  }
  if (!valid) { return (FALSE); }

  status = 1;
  tmp = strtod (p1, &p2);
  if (p2 == p1) return (FALSE); /* entry not a number: +fred */
  value = tmp;
  if (p2 == px) goto escape;    /* entry only number: +1.0 */ 
  p1 = p2 + 1;

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;    /* entry not a number: +1:fred */
  status = 2;
  value += tmp / 60.0;
  if (p2 == px) goto escape;    /* entry only number: +1:1 */
  p1 = p2 + 1;

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;    /* entry not a number: +1:1:fred */
  value += tmp / 3600.0;

escape:
  if (neg) {
    value *= -1;
  }
  *Value = value;

  return (status);
}

/**********/
int ohana_str_to_radec (double *ra, double *dec, char *str1, char *str2) {

  double Ra, Dec;

  *ra = *dec = 0;
  switch (ohana_dms_to_ddd (&Ra, str1)) {
    case 0:
      fprintf (stderr, "syntax error in RA\n");
      return (FALSE);
    case 1:
      break;
    case 2:
      Ra = Ra * 15;
      break;
  }
  switch (ohana_dms_to_ddd (&Dec, str2)) {
    case 0:
      fprintf (stderr, "syntax error in DEC\n");
      return (FALSE);
    case 1:
    case 2:
      break;
  }
  *ra = Ra;
  *dec = Dec;
  return (TRUE);
}

/**********/
int ohana_chk_time (char *line) {

  char *p1, *p2;
  double tmp;
  int mode;

  p1 = line;
  tmp = strtod (p1, &p2);
  if (0) { fprintf (stderr, "tmp: %f\n", tmp); }
  mode = TIME_DATE;
  if (p2 == p1 + strlen (p1) - 1) {
    if (*p2 == 'd') {
      mode = TIME_DAYS;
    }
    if (*p2 == 'h') {
      mode = TIME_HOURS;
    }
    if (*p2 == 'm') {
      mode = TIME_MINUTES;
    }
    if (*p2 == 's') {
      mode = TIME_SECONDS;
    }
    if (*p2 == 'j') {
      mode = TIME_JD;
    }
    if (*p2 == 'J') {
      mode = TIME_MJD;
    }
  }
  return (mode);
}

/**********/
int ohana_str_to_time (char *line, time_t *second) {
  
  char *tmpline;
  struct tm *gmt;
  struct timeval now;
  double jd;
  time_t tsec;

  if (!strcasecmp (line, "NOW")) {
    gettimeofday (&now, (struct timezone *) NULL);
    *second = now.tv_sec;
    return (TRUE);
  }
    
  if (!strncasecmp (line, "TODAY", 5)) {
    gettimeofday (&now, (struct timezone *) NULL);
    if (line[5]) { /* line has extra data (ie, hh:mm:ss) */
      tsec = now.tv_sec;
      ALLOCATE (tmpline, char, 64);
      gmt   = gmtime (&tsec);
      snprintf (tmpline, 64, "%04d/%02d/%02d,%s", 1900 + gmt[0].tm_year, gmt[0].tm_mon+1, gmt[0].tm_mday, &line[6]);
      *second = ohana_date_to_sec (tmpline);
      free (tmpline);
      return (TRUE);
    } else {
      *second = 86400 * ((int)(now.tv_sec / 86400));
      return (TRUE); 
    }
  }
    
  switch (ohana_chk_time (line)) {
    case 0:
      return (FALSE);
    case TIME_DATE:
      *second = ohana_date_to_sec (line);
      return (TRUE);
    case TIME_DAYS:
      *second = strtod (line, 0) * 86400.0;
      return (TRUE);
    case TIME_HOURS:
      *second = strtod (line, 0) * 3600.0;
      return (TRUE);
    case TIME_MINUTES:
      *second = strtod (line, 0) * 60.0;
      return (TRUE);
    case TIME_SECONDS:
      *second = strtod (line, 0);
      return (TRUE);
    case TIME_JD:
      jd = strtod (line, 0);
      *second = ohana_jd_to_sec (jd);
      return (TRUE);
    case TIME_MJD:
      jd = strtod (line, 0);
      *second = ohana_mjd_to_sec (jd);
      return (TRUE);
  }
  return (FALSE);
}


/**********/
int ohana_str_to_dtime (char *line, double *second) {
  
  switch (ohana_chk_time (line)) {
    case 0:
    case TIME_JD:
    case TIME_MJD:
    case TIME_DATE:
      return (FALSE);
    case TIME_DAYS:
      *second = strtod (line, 0) * 86400.0;
      return (TRUE);
    case TIME_HOURS:
      *second = strtod (line, 0) * 3600.0;
      return (TRUE);
    case TIME_MINUTES:
      *second = strtod (line, 0) * 60.0;
      return (TRUE);
    case TIME_SECONDS:
      *second = strtod (line, 0);
      return (TRUE);
  }
  return (FALSE);
}

/**********/
double ohana_sec_to_jd (time_t second) {

  double jd;
  
  jd = second/86400.0 + 2440587.5;
  return (jd);
}

/**********/
time_t ohana_jd_to_sec (double jd) {

  time_t second;

  second = (jd - 2440587.5)*86400;
  return (second);
}

/**********/
double ohana_sec_to_mjd (time_t second) {

  double mjd;
  
  mjd = second/86400.0 + 40587.0;
  return (mjd);
}

/**********/
time_t ohana_mjd_to_sec (double mjd) {

  time_t second;

  second = (mjd - 40587.0)*86400;
  return (second);
}

/**********/
char *ohana_sec_to_date (time_t second) {
  
  struct tm *gmt;
  char *line;
  
  ALLOCATE (line, char, 64);
  gmt   = gmtime (&second);
  if (!gmt) {
    snprintf (line, 64, "INVALID DATE");
  } else {
    snprintf (line, 64, "%04d/%02d/%02d,%02d:%02d:%02d", 1900 + gmt[0].tm_year, gmt[0].tm_mon+1, gmt[0].tm_mday, gmt[0].tm_hour, gmt[0].tm_min, gmt[0].tm_sec); 
  }
  return (line);

}

/***** date in format yyyy/mm/dd,hh:mm:ss *****/
time_t ohana_date_to_sec (char *date) {
  
  time_t second;
  double tmp, jd;
  struct tm now;
  char *p1, *p2, *px;
  
  p1 = date;
  if (p1 == NULL) return 0;

  /* ignore standard leading quoting options: " ' ( */
  if (*p1 == 0x22) p1++; /* " */
  if (*p1 == 0x27) p1++; /* ' */
  if (*p1 == 0x28) p1++; /* ( */

  px = p1 + strlen(p1);
  bzero (&now, sizeof(now));

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;
  now.tm_year = tmp;
  if (now.tm_year > 1000) now.tm_year -= 1900;
  if (now.tm_year <   50) now.tm_year += 100;
  if (p2 == px) goto escape;  
  p1 = p2 + 1;

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;
  now.tm_mon = tmp - 1; /* mon runs from 0 - 11 */
  if (p2 == px) goto escape;  
  p1 = p2 + 1;

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;
  now.tm_mday = tmp;
  if (p2 == px) goto escape;  
  p1 = p2 + 1;

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;
  p1 = p2 + 1;
  now.tm_hour = tmp;
  if (p2 == px) goto escape;  

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;
  now.tm_min = tmp;
  if (p2 == px) goto escape;  
  p1 = p2 + 1;

  tmp = strtod (p1, &p2);
  if (p2 == p1) goto escape;
  now.tm_sec = tmp;
  if (p2 == px) goto escape;  
  p1 = p2 + 1;

escape:
  jd = now.tm_mday - 32075 + (int)(1461*(1900 + now.tm_year + 4800 + (int)(((now.tm_mon+1)-14)/12))/4)
    + (int)(367*((now.tm_mon+1) - 2 - (int)(((now.tm_mon+1) - 14)/12)*12)/12)
    - (int)(3*(int)((1900 + now.tm_year + 4900 + (int)(((now.tm_mon+1) - 14)/12))/100)/4) - 0.5;
  
  second = (jd - 2440587.5)*86400 + 3600.0*now.tm_hour + now.tm_min*60.0 + now.tm_sec;

  return (second);
}

/***** short date in format yymmdd *****/
time_t short_date_to_sec (char *date) {
  
  time_t second;
  double jd;
  struct tm now;
  
  bzero (&now, sizeof(now));

  sscanf (date, "%2d%2d%2d", &now.tm_year, &now.tm_mon, &now.tm_mday);

  if (now.tm_year >   51) now.tm_year +=   0;
  if (now.tm_year <   50) now.tm_year += 100;
  now.tm_mon --; /* tm_mon runs from 0 - 11 */

  jd = now.tm_mday - 32075 + (int)(1461*(1900 + now.tm_year + 4800 + (int)(((now.tm_mon+1)-14)/12))/4)
    + (int)(367*((now.tm_mon+1) - 2 - (int)(((now.tm_mon+1) - 14)/12)*12)/12)
    - (int)(3*(int)((1900 + now.tm_year + 4900 + (int)(((now.tm_mon+1) - 14)/12))/100)/4) - 0.5;
  
  second = (jd - 2440587.5)*86400 + 3600.0*now.tm_hour + now.tm_min*60.0 + now.tm_sec;

  return (second);
}

/**********/
int hstgsc_hms_to_deg (double *h0, double *h1, double *d0, double *d1, char *string) {
  
  int flag_d0, flag_d1, flag_h0, flag_h1;
  double tmp;
  
  *d0 = *h0 = *d1 = *h1 = 0;

  flag_h0 = dparse (h0, 1, string);
  flag_h1 = dparse (h1, 4, string);
  flag_d0 = dparse (d0, 7, string);
  flag_d1 = dparse (d1, 9, string);
  *h0 *= flag_h0;
  *h1 *= flag_h1;
  *d0 *= flag_d0;
  *d1 *= flag_d1;

  dparse (&tmp, 2, string);
  *h0 += tmp/60.0;
  dparse (&tmp, 3, string);
  *h0 += tmp/3600.0;
  
  dparse (&tmp, 5, string);
  *h1 += tmp/60.0;
  dparse (&tmp, 6, string);
  *h1 += tmp/3600.0;
  
  dparse (&tmp, 8, string);
  *d0 += tmp/60.0;

  dparse (&tmp, 10, string);
  *d1 += tmp/60.0;

  *h0 *= 15*flag_h0;
  *h1 *= 15*flag_h1;
  *d0 *= flag_d0;
  *d1 *= flag_d1;

  return (TRUE);
}

/* times may be in forms as:
 * 20040200450s (N seconds since 1970.0)
 * 2440900.232j (julian date)
 * 99/02/23,03:22:18 (date string)
 * (separators may be anything except space, +, -)
 * 99:02:15:12:23:30
 * 99:02:15:12h23m30s
 */


/* returns the local MEAN sidereal time (dec hrs) at julian date jd
   at west longitude long (decimal hours).  Follows
   definitions in 1992 Astronomical Almanac, pp. B7 and L2.
   Expression for GMST at 0h ut referenced to Aoki et al, A&A 105,
   p.359, 1982.  On workstations, accuracy (numerical only!)
   is about a millisecond in the 1990s.
   EAM: function from skycalc (thorstensen) 
*/
#define  J2000             2451545.        /* Julian date at standard epoch */
#define  SEC_IN_DAY        86400.

double ohana_lst (double jd, double longitude) {

  double t, ut, jdmid, jdint, jdfrac, sid_g;
  long sid_int;

  jdint = (int) jd;
  jdfrac = jd - jdint;

  if (jdfrac < 0.5) {
    jdmid = jdint - 0.5;
    ut = jdfrac + 0.5;
  } else {
    jdmid = jdint + 0.5;
    ut = jdfrac - 0.5;
  }

  t = (jdmid - J2000)/36525;
  sid_g = (24110.54841+8640184.812866*t+0.093104*t*t-6.2e-6*t*t*t)/SEC_IN_DAY;
  sid_int = sid_g;
  sid_g = sid_g - (double) sid_int;
  sid_g = sid_g + 1.0027379093 * ut - longitude/24.;
  sid_int = sid_g;
  sid_g = (sid_g - (double) sid_int) * 24.;
  if (sid_g < 0.) sid_g = sid_g + 24.;
  return (sid_g);
}

double ohana_normalize_angle (double angle) {

    double result;

    // take an input angle and force the domain to be 0.0 - 360.0
    // there are a few ways to do this:  

    // option 1: This is a potentially very slow method, also subject to round off errors
# if (0)
    while (angle > 360.0) angle -= 360.0;
    while (angle <   0.0) angle += 360.0;
# endif
    
    // option 2: take sin & cos, apply atan2 (y, x) 
# if (1)    
    double x, y;

    x = cos(angle*RAD_DEG);
    y = sin(angle*RAD_DEG);

    result = DEG_RAD*atan2 (y, x);
    if (result < 0.0) result += 360.0;
# endif

# if (0)
    // option 3:
    int nCircle = angle / 360.0;
    result -= 360.0*nCircle;
    if (result < 0.0) result += 360.0;
# endif

    return (result);
}

double ohana_normalize_angle_to_midpoint (double angle, double Rmid) {

    double result;

    // take an input angle and force the domain to be 0.0 - 360.0
    // there are a few ways to do this:  

    // option 1: This is a potentially very slow method, also subject to round off errors
# if (0)
    while (angle > 360.0) angle -= 360.0;
    while (angle <   0.0) angle += 360.0;
# endif
    
    // option 2: take sin & cos, apply atan2 (y, x) 
# if (1)    
    double x, y;

    x = cos(angle*RAD_DEG);
    y = sin(angle*RAD_DEG);

    result = DEG_RAD*atan2 (y, x);
    if (result < Rmid - 180.0) result += 360.0;
    if (result > Rmid + 180.0) result -= 360.0;
# endif

# if (0)
    // option 3:
    int nCircle = angle / 360.0;
    result -= 360.0*nCircle;
    if (result < 0.0) result += 360.0;
# endif

    return (result);
}

short ToShortPixels (float pixels) {

    short value;

    value = 100*pixels;

    return value;
}

short ToShortDegrees (float degrees) {

    short value;

    value = ((float)0xffff/360.0) * degrees;

    return value;
}

float FromShortPixels (short value) {

    float pixels;

    pixels = value / 100.0;

    return pixels;
}

float FromShortDegrees (float value) {

    float degrees;

    degrees = (360.0 / (float)0xffff) * value;

    return degrees;
}
