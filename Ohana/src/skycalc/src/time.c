# include <skycalc_internal.h>

/* fill date structure with current date & time (UT) */
int SC_get_sys_date (struct SC_date_time *date) {

  time_t t;
  struct tm *stm;
  
  t = time (0);
  if (t == -1) return (-1);

  stm = gmtime (&t);
  date->y = (short) (stm->tm_year + 1900);
  date->mo = (short) (stm->tm_mon + 1);
  date->d = (short) (stm->tm_mday);
  date->h = (short) (stm->tm_hour);
  date->mn = (short) (stm->tm_min);
  date->s = (float) (stm->tm_sec);

  return (0); /* success */

}

/* convert a UT date to JD ( 1900 -- 2100?) */
double SC_date_to_jd (struct SC_date_time date) {
  
  short yr1=0;
  short mo1=1;
  long jdzpt = 1720982;
  long jdint, inter;
  double jd, jdfrac;
  
  if ((date.y <= 1900) | (date.y >= 2100)) return (0.0);
  
  if (date.mo <= 2) {
    yr1 = -1;
    mo1 = 13;
  }
  
  jdint = 365.25*(date.y+yr1);  /* truncates */
  inter = 30.6001*(date.mo+mo1);
  jdint = jdint+inter+date.d+jdzpt;
  jd = jdint;
  jdfrac=date.h/24.+date.mn/1440.+date.s/SEC_IN_DAY;
  if (jdfrac < 0.5) {
    jdint--;
    jdfrac=jdfrac+0.5;
  }
  else jdfrac=jdfrac-0.5;
  jd = jdint+jdfrac;
  return (jd);
}

/* convert JD to a UT date & time */
/* XXX can we drop this and use the unix time functions? */
void SC_jd_to_date (double jdin, struct SC_date_time *date) {
  
#define IGREG 2299161

  /* Adapted from Press, Flannery, Teukolsky, &
     Vetterling, Numerical Recipes in C, (Cambridge
     University Press), 1st edn, p. 12. */
  
  int mm, id, iyyy;  /* their notation */
  long ja, jdint, jalpha, jb, jc, jd, je;
  float jdfrac;

  jdin = jdin + 0.5;  /* adjust for 1/2 day */
  jdint = jdin;

  // double x = jdint/7.+0.01; XXX why was this here?
  jdfrac = jdin - jdint;
  date->h = jdfrac * 24; /* truncate */
  date->mn = (jdfrac - ((float) date->h)/24.) * 1440.;
  date->s = (jdfrac - ((float) date->h)/24. -
	     ((float) date->mn)/1440.) * SEC_IN_DAY;

  if(jdint > IGREG) {
    jalpha=((float) (jdint-1867216)-0.25)/36524.25;
    ja=jdint+1+jalpha-(long)(0.25*jalpha);
  }
  else
    ja=jdint;
  jb=ja+1524;
  jc=6680.0+((float) (jb-2439870)-122.1)/365.25;
  jd=365*jc+(0.25*jc);
  je=(jb-jd)/30.6001;
  id=jb-jd-(int) (30.6001*je);
  mm=je-1;
  if(mm > 12) mm -= 12;
  iyyy=jc-4715;
  if(mm > 2) --iyyy;
  if (iyyy <= 0) --iyyy;
  date->y = iyyy;
  date->mo = mm;
  date->d = id;

}


/* returns the local MEAN sidereal time (dec hrs) at julian date jd
   at west longitude long (decimal hours).  Follows
   definitions in 1992 Astronomical Almanac, pp. B7 and L2.
   Expression for GMST at 0h ut referenced to Aoki et al, A&A 105,
   p.359, 1982.  On workstations, accuracy (numerical only!)
   is about a millisecond in the 1990s. */
double SC_lst (double jd, double longit) {
  
  double t, ut, jdmid, jdint, jdfrac, sid_g;
  long sid_int;
  
  jdint = (int) jd;
  jdfrac = jd - jdint;

  if (jdfrac < 0.5) {
    jdmid = jdint - 0.5;
    ut = jdfrac + 0.5;
  }
  else {
    jdmid = jdint + 0.5;
    ut = jdfrac - 0.5;
  }
  t = (jdmid - J2000)/36525;
  sid_g = (24110.54841+8640184.812866*t+0.093104*t*t-6.2e-6*t*t*t)/SEC_IN_DAY;
  sid_int = sid_g;
  sid_g = sid_g - (double) sid_int;
  sid_g = sid_g + 1.0027379093 * ut - longit/24.;
  sid_int = sid_g;
  sid_g = (sid_g - (double) sid_int) * 24.;
  if (sid_g < 0.) sid_g = sid_g + 24.;
  return (sid_g);
}

/* force time domain to be -12h and 12h  */
double SC_adj_time (double x) {
  
  /* ridiculously inefficient - use modulo and fractions.. */
  if(fabs(x) < 100000.) {  
    while(x > 12.) {
      x = x - 24.;
    }
    while(x < -12.) {
      x = x + 24.;
    }
  }
  else printf ("Out of bounds in adj_time!\n");
  return(x);
}

