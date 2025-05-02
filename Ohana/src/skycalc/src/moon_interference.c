/*
   This program segment is extracted from the skycalc program written
   by John Thorstensen from Dartmouth College.  For question please
   contact: John.Thorstensen@Dartmouth.edu
   This is one long and ugly, but accurate program.  Ideally I should
   have split this into separate modules, but this may be replaced by
   Bernt's algorithm anyway... so "let's wait and see what happens"   
   -Rosemary Alles 04/18/2001
   */

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* some (not all) physical, mathematical, and astronomical constants
   used are defined here. */
#define  DEG_IN_RADIAN     57.2957795130823
#define  HRS_IN_RADIAN     3.819718634205
#define  EQUAT_RAD         6378137.    /* equatorial radius of earth, meters */
#define  TWOPI             6.28318530717959
#define  J2000             2451545.    /* Julian date at standard epoch */
#define  SEC_IN_DAY        86400.

struct coord {
    short sign;  /* carry sign explicitly since -0 not neg. */
    double hh;
    double mm;
    double ss;
};

struct date_time {
    short y;
    short mo;
    short d;
    short h;
    short mn;
    float s;
};

/* Gloabals */
int update_on = 0; 
double update_delta = 0.;

/* Prototypes */
void load_site(double* longit, double* lat, double* stdz,
               short* use_dst, char* zone_name, char* zabr,
               double* elevsea, double* elev, double* horiz,
               char* site_name);
double zone(short use_dst, double stdz,
            double jd, double jdb, double jde);
void caldat(double jdin, struct date_time* date, short *dow);
short day_of_week(double jd);
double day_of_year(double jd);
void find_dst_bounds(short yr, double stdz, short use_dst,
                     double *jdb, double* jde);
int get_sys_date(struct date_time *date, short use_dst, short enter_ut,
                 short night_date, double stdz, double toffset, double* jd);
double lst(double jd, double longit);
double date_to_jd(struct date_time date);
void lpmoon(double jd, double lat, double sid,
            double* ra, double* dec, double* dist);
void put_coords(double deci, int precision, int showsign);
double get_coord(void);
void dec_to_bab (double deci, struct coord* bab);
double atan_circ(double x, double y);
double adj_time(double x);

    
void
load_site(double* longit, double* lat, double* stdz,
          short* use_dst, char* zone_name, char* zabr,
          double* elevsea, double* elev, double* horiz,
          char* site_name) {
              
/* sets the site-specific quantities; these are
   longit     = W longitude in decimal hours
   lat        = N latitude in decimal degrees
   stdz       = standard time zone offset, hours
   elevsea    = elevation above sea level (for absolute location)
   elev       = observatory elevation above horizon, meters
   horiz      = (derived) added zenith distance for rise/set due
                to elevation
   use_dst    = 0 don't use it
                1 use USA convention
                2 use Spanish convention
                < 0 Southern hemisphere (reserved, unimplimented)
   zone_name  = name of time zone, e. g. Eastern
   zabr       = single-character abbreviation of time zone
   site_name  = name of site.  */
    
    strcpy(site_name, "Mauna Kea, Hawaii");
    strcpy(zone_name, "Hawaiian");
    *zabr = 'H';
    *use_dst = 0;
    *longit = 10.36478;
    *lat = 19.8267;
    *elevsea = 4215.;
    *elev = 4215.;  /* yow! */
    *stdz = 10.;

    /* now compute derived quantity "horiz" = depression of horizon.*/
    *horiz = sqrt(2. * *elev / EQUAT_RAD) * DEG_IN_RADIAN;   
}

double
zone(short use_dst, double stdz,
     double jd, double jdb, double jde) {
    
    /* Returns zone time offset when standard time zone is stdz,
       when daylight time begins (for the year) on jdb, and ends
       (for the year) on jde.  This is parochial to the northern
       hemisphere.  */
    /* Extension -- specifying a negative value of use_dst reverses
       the logic for the Southern hemisphere; then DST is assumed for
       the Southern hemisphere summer (which is the end and beginning
       of the year. */
    
    if(use_dst == 0) return(stdz);
    else if((jd > jdb) && (jd < jde) && (use_dst > 0)) return(stdz-1.);
    /* next line .. use_dst < 0 .. for Southern Hemisphere sites. */
    else if(((jd < jdb) || (jd > jde)) && (use_dst < 0)) return(stdz-1.);
    else return(stdz);
}

void
caldat(double jdin, struct date_time* date, short *dow) {

    /* from Jean Meeus, Astronomical Formulae for Calculators,
       published by Willman-Bell Inc.
       Avoids a copyrighted routine from Numerical Recipes.
       Tested and works properly from the beginning of the 
       Gregorian calendar era (1583) to beyond 3000 AD. */

    double jdtmp;
    long alpha;
    long Z;
    long A, B, C, D, E;
    double F; 
    double x;   /* for day-of-week calculation */

    jdtmp = jdin + 0.5;

    Z = (long) jdtmp;

    x = Z/7.+0.01;
    *dow = 7.*(x - (long) x);   /* truncate for day of week */

    F = jdtmp - Z;

    if(Z < 2299161) A = Z;
    else {
        alpha = (long) ((Z - 1867216.25) / 36524.25);
        A = Z + 1 + alpha - (long) (alpha / 4);
    }

    B = A + 1524;
    C = ((B - 122.1) / 365.25);
    D =  (365.25 * C);\
    E =  ((B - D) / 30.6001);

    date->d = B - D - (long)(30.6001 * E);
    if(E < 13.5) date->mo = E - 1;
    else date->mo = E - 13;
    if(date->mo  > 2.5)  date->y = C - 4716;
    else date->y = C - 4715;
	
    date->h = F * 24.;  /* truncate */
    date->mn = (F - ((float) date->h)/24.) * 1440.;
    date->s = (F - ((float) date->h)/24. -
               ((float) date->mn)/1440.) * 86400;
	
}

short
day_of_week(double jd) { 

    /* returns day of week for a jd, 0 = Mon, 6 = Sun. */
    
    double x;
    long i;
    short d;
    
    jd = jd+0.5;
    i = jd; /* truncate */
    x = i/7.+0.01; 
    d = 7.*(x - (long) x);   /* truncate */
    return(d);
}

double
day_of_year(double jd) {
    
    double jdjan0;
    struct date_time date;
    short dow;
    
    caldat(jd,&date,&dow);
    /* find jd of "jan 0" = Dec 31 of previous year */
    date.y = date.y - 1;
    date.mo = 12;
    date.d = 31;
    date.h = 0;
    date.mn = 0;
    date.s = 0.;
    jdjan0 = date_to_jd(date);
    return(jd - jdjan0);
}

void
find_dst_bounds(short yr, double stdz, short use_dst,
                double *jdb, double* jde) {
    
	/* finds jd's at which daylight savings time begins 
	    and ends.  The parameter use_dst allows for a number
	    of conventions, namely:
		0 = don't use it at all (standard time all the time)
		1 = use USA convention (1st Sun in April to
		     last Sun in Oct after 1986; last Sun in April before)
		2 = use Spanish convention (for Canary Islands)
		-1 = use Chilean convention (CTIO).
		-2 = Australian convention (for AAT).
	    Negative numbers denote sites in the southern hemisphere,
	    where jdb and jde are beginning and end of STANDARD time for
	    the year. 
	    It's assumed that the time changes at 2AM local time; so
	    when clock is set ahead, time jumps suddenly from 2 to 3,
	    and when time is set back, the hour from 1 to 2 AM local 
	    time is repeated.  This could be changed in code if need be. */

	struct date_time trial;

	if((use_dst == 1) || (use_dst == 0)) { 
	    /* USA Convention, and including no DST to be defensive */
	    /* Note that this ignores various wrinkles such as the
		brief Nixon administration flirtation with year-round DST,
		the extended DST of WW II, and so on. */
		trial.y = yr;
		trial.mo = 4;
		if(yr >= 1986) trial.d = 1;
		else trial.d = 30; 
		trial.h = 2;
		trial.mn = 0;
		trial.s = 0;

		/* Find first Sunday in April for 1986 on ... */
		if(yr >= 1986) 
			while(day_of_week(date_to_jd(trial)) != 6) 
				trial.d++;
			
		/* Find last Sunday in April for pre-1986 .... */
		else while(day_of_week(date_to_jd(trial)) != 6) 
				trial.d--;

		*jdb = date_to_jd(trial) + stdz/24.;    

		/* Find last Sunday in October ... */
		trial.mo = 10;
		trial.d = 31;
		while(day_of_week(date_to_jd(trial)) != 6) {
			trial.d--;
		}
		*jde = date_to_jd(trial) + (stdz - 1.)/24.;             
	}
	else if (use_dst == 2) {  /* Spanish, for Canaries */
		trial.y = yr;
		trial.mo = 3;
		trial.d = 31; 
		trial.h = 2;
		trial.mn = 0;
		trial.s = 0;

		while(day_of_week(date_to_jd(trial)) != 6) {
			trial.d--;
		}
		*jdb = date_to_jd(trial) + stdz/24.;    
		trial.mo = 9;
		trial.d = 30;
		while(day_of_week(date_to_jd(trial)) != 6) {
			trial.d--;
		}
		*jde = date_to_jd(trial) + (stdz - 1.)/24.;             
	}               
	else if (use_dst == -1) {  /* Chilean, for CTIO, etc.  */
	   /* off daylight 2nd Sun in March, onto daylight 2nd Sun in October */
		trial.y = yr;
		trial.mo = 3;
		trial.d = 8;  /* earliest possible 2nd Sunday */
		trial.h = 2;
		trial.mn = 0;
		trial.s = 0;

		while(day_of_week(date_to_jd(trial)) != 6) {
			trial.d++;
		}
		*jdb = date_to_jd(trial) + (stdz - 1.)/24.;
			/* note jdb is beginning of STANDARD time in south,
				hence use stdz - 1. */  
		trial.mo = 10;
		trial.d = 8;
		while(day_of_week(date_to_jd(trial)) != 6) {
			trial.d++;
		}
		*jde = date_to_jd(trial) + stdz /24.;           
	}                       
	else if (use_dst == -2) {  /* For Anglo-Australian Telescope  */
	   /* off daylight 1st Sun in March, onto daylight last Sun in October */
		trial.y = yr;
		trial.mo = 3;
		trial.d = 1;  /* earliest possible 1st Sunday */
		trial.h = 2;
		trial.mn = 0;
		trial.s = 0;

		while(day_of_week(date_to_jd(trial)) != 6) {
			trial.d++;
		}
		*jdb = date_to_jd(trial) + (stdz - 1.)/24.;
			/* note jdb is beginning of STANDARD time in south,
				hence use stdz - 1. */  
		trial.mo = 10;
		trial.d = 31;
		while(day_of_week(date_to_jd(trial)) != 6) {
			trial.d--;
		}
		*jde = date_to_jd(trial) + stdz /24.;           
	}               
}

int
get_sys_date(struct date_time *date, short use_dst, short enter_ut,
             short night_date, double stdz, double toffset, double* jd) {

    /* Reads the system clock; loads up the date structure
       to conform to the prevailing conventions for the interpretation
       of times.  Optionally adds "toffset" minutes to the system
       clock, as in x minutes in the future. */

    time_t t, *tp;
    struct tm *stm;
    double jdb, jde;
    short dow;

    tp = &t;  /* have to initialize pointer variable for it to
                 serve as an argument. */

    t = time(tp);
    if(t == -1) {
        printf("error: system time unavailable during calculation of moon interference\n");
        return(-1);
    }
    stm = localtime(&t);
    date->y = (short) (stm->tm_year + 1900);
    date->mo = (short) (stm->tm_mon + 1);
    date->d = (short) (stm->tm_mday);
    date->h = (short) (stm->tm_hour);
    date->mn = (short) (stm->tm_min);
    date->s = (float) (stm->tm_sec);

    if(toffset != 0.) {
        *jd = date_to_jd(*date);
        *jd = *jd + toffset / 1440.;
        caldat(*jd,date,&dow);
    }

    if(enter_ut == 1)  { /* adjust if needed */
        find_dst_bounds(date->y,stdz,use_dst,&jdb,&jde);
        *jd = date_to_jd(*date);
        *jd = *jd + zone(use_dst,stdz,*jd,jdb,jde)/24.;
        caldat(*jd,date,&dow);
    }
    if((night_date == 1) && (date->h < 12)) {
        date->d = date->d - 1;
    }

    return(0); /* success */
}

double
lst(double jd, double longit) {

    /* returns the local MEAN sidereal time (dec hrs) at julian date jd
       at west longitude long (decimal hours).  Follows
       definitions in 1992 Astronomical Almanac, pp. B7 and L2. 
       Expression for GMST at 0h ut referenced to Aoki et al, A&A 105,
       p.359, 1982.  On workstations, accuracy (numerical only!)
       is about a millisecond in the 1990s. */

    double t, ut, jdmid, jdint, jdfrac, sid_g;
    long jdin, sid_int;

    jdin = jd;         /* fossil code from earlier package which 
                          split jd into integer and fractional parts ... */
    jdint = jdin;
    jdfrac = jd - jdint;
    if(jdfrac < 0.5) {
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
    if(sid_g < 0.) sid_g = sid_g + 24.;
    return(sid_g);
}


double
date_to_jd(struct date_time date) {
    
/* From Meeus' Astronomical Formulae for Calculators.  The two JD
   conversion routines routines were replaced 1998 November 29 to
   avoid inclusion of copyrighted "Numerical Recipes" code.  A test
   of 1 million random JDs between 1585 and 3200 AD gave the same
   conversions as the NR routines. */


    double jd;
    int y, m;
    long A, B;

    if(date.mo <= 2) {
        y = date.y - 1;
        m = date.mo + 12;
    }
    else {
        y = date.y;
        m = date.mo;
    }

    A = (long) (y / 100.);
    B = 2 - A + (long) (A / 4.);

    jd = (long) (365.25 * y) + (long) (30.6001 * (m + 1)) + date.d + 
        1720994.5;

    jd += date.h / 24. + date.mn / 1440. + date.s / 86400.;

    if(date.y > 1583) return(jd + B);  
    else return(jd);
    /* Not quite right, since Gregorian calendar first
       adopted around Oct 1582.  But fine for modern. */
}

void
lpmoon(double jd, double lat, double sid,
       double* ra, double* dec, double* dist) {


/* implements "low precision" moon algorithms from
   Astronomical Almanac (p. D46 in 1992 version).  Does
   apply the topocentric correction. 
   Units are as follows
   jd,lat, sid;   decimal hours 
   *ra, *dec,   decimal hours, degrees 
   *dist;      earth radii */


    double T, lambda, beta, pie, l, m, n, x, y, z, alpha, delta,
        rad_lat, rad_lst, distance, topo_dist;
    char dummy[40];  /* to fix compiler bug on IBM system */

    T = (jd - J2000) / 36525.;  /* jul cent. since J2000.0 */

    lambda = 218.32 + 481267.883 * T 
        + 6.29 * sin((134.9 + 477198.85 * T) / DEG_IN_RADIAN)
        - 1.27 * sin((259.2 - 413335.38 * T) / DEG_IN_RADIAN)
        + 0.66 * sin((235.7 + 890534.23 * T) / DEG_IN_RADIAN)
        + 0.21 * sin((269.9 + 954397.70 * T) / DEG_IN_RADIAN)
        - 0.19 * sin((357.5 + 35999.05 * T) / DEG_IN_RADIAN)
        - 0.11 * sin((186.6 + 966404.05 * T) / DEG_IN_RADIAN);
    lambda = lambda / DEG_IN_RADIAN;
    beta = 5.13 * sin((93.3 + 483202.03 * T) / DEG_IN_RADIAN)
        + 0.28 * sin((228.2 + 960400.87 * T) / DEG_IN_RADIAN)
        - 0.28 * sin((318.3 + 6003.18 * T) / DEG_IN_RADIAN)
        - 0.17 * sin((217.6 - 407332.20 * T) / DEG_IN_RADIAN);
    beta = beta / DEG_IN_RADIAN;
    pie = 0.9508 
        + 0.0518 * cos((134.9 + 477198.85 * T) / DEG_IN_RADIAN)
        + 0.0095 * cos((259.2 - 413335.38 * T) / DEG_IN_RADIAN)
        + 0.0078 * cos((235.7 + 890534.23 * T) / DEG_IN_RADIAN)
        + 0.0028 * cos((269.9 + 954397.70 * T) / DEG_IN_RADIAN);
    pie = pie / DEG_IN_RADIAN;
    distance = 1 / sin(pie);

    l = cos(beta) * cos(lambda);
    m = 0.9175 * cos(beta) * sin(lambda) - 0.3978 * sin(beta);
    n = 0.3978 * cos(beta) * sin(lambda) + 0.9175 * sin(beta);

    x = l * distance; 
    y = m * distance; 
    z = n * distance;  /* for topocentric correction */


    /* lat isn't passed right on some IBM systems unless you do this
       or something like it! */
    sprintf(dummy,"%f",lat);

    rad_lat = lat / DEG_IN_RADIAN;
    rad_lst = sid / HRS_IN_RADIAN;

    x = x - cos(rad_lat) * cos(rad_lst);
    y = y - cos(rad_lat) * sin(rad_lst);
    z = z - sin(rad_lat);


    topo_dist = sqrt(x * x + y * y + z * z);

    l = x / topo_dist; 
    m = y / topo_dist; 
    n = z / topo_dist;

    alpha = atan_circ(l,m);
    delta = asin(n);

    *ra = alpha * HRS_IN_RADIAN;

    *dec = delta * DEG_IN_RADIAN;
    
    *dist = topo_dist;

}


void
put_coords(double deci, int precision, int showsign) {


/* prints out a struct coord in a nice format; precision
   is a code for how accurate you want it.  The options are:
     precision = 0;   minutes rounded to the nearest minute
     precision = 1;   minutes rounded to the nearest tenth.
     precision = 2;   seconds rounded to the nearest second
     precision = 3;   seconds given to the tenth
     precision = 4;   seconds given to the hundredth
   The program assumes that the line is ready for the coord
   to be printed and does NOT deliver a new line at the end
   of the output. */

   
   double minutes;  /* for rounding off if necess. */
   struct coord out_coord, coords;
   char out_string[20];  /* for checking for nasty 60's */

   dec_to_bab(deci,&coords);  /* internally convert to coords*/

   if(coords.sign == -1) printf("-");
	else printf(" "); /* to preserve alignment */

   if(precision == 0) {   /* round to nearest minute */
      minutes = coords.mm + coords.ss / 60.;
           /* check to be sure minutes aren't 60 */
      sprintf(out_string,"%.0f %02.0f",coords.hh,minutes);
      sscanf(out_string,"%lf %lf",&out_coord.hh,&out_coord.mm);
      if(fabs(out_coord.mm - 60.) < 1.0e-7) {
         out_coord.mm = 0.;
         out_coord.hh = out_coord.hh + 1.;
      }
      printf("%2.0f:%02.0f",out_coord.hh,out_coord.mm);
   }

   else if(precision == 1) {    /* keep nearest tenth of a minute */
      minutes = coords.mm + coords.ss / 60.;
           /* check to be sure minutes are not 60 */
      sprintf(out_string,"%.0f %04.1f",coords.hh,minutes);
      sscanf(out_string,"%lf %lf",&out_coord.hh, &out_coord.mm);
      if(fabs(out_coord.mm - 60.) < 1.0e-7) {
         out_coord.mm = 0.;
         out_coord.hh = out_coord.hh + 1.;
      }
      printf("%2.0f:%04.1f", out_coord.hh, out_coord.mm);
   }
   else if(precision == 2) {
          /* check to be sure seconds are not 60 */
      sprintf(out_string,"%.0f %02.0f %02.0f",coords.hh,coords.mm,coords.ss);
      sscanf(out_string,"%lf %lf %lf",&out_coord.hh,&out_coord.mm,
           &out_coord.ss);
      if(fabs(out_coord.ss - 60.) < 1.0e-7) {
          out_coord.mm = out_coord.mm + 1.;
          out_coord.ss = 0.;
          if(fabs(out_coord.mm - 60.) < 1.0e-7) {
              out_coord.hh = out_coord.hh + 1.;
              out_coord.mm = 0.;
          }
      }
      printf("%2.0f:%02.0f:%02.0f",out_coord.hh,out_coord.mm,out_coord.ss);
   }
   else if(precision == 3) {
          /* the usual shuffle to check for 60's */
      sprintf(out_string,"%.0f %02.0f %04.1f",coords.hh, coords.mm, coords.ss);
      sscanf(out_string,"%lf %lf %lf",&out_coord.hh,&out_coord.mm,
           &out_coord.ss);
      if(fabs(out_coord.ss - 60.) < 1.0e-7) {
          out_coord.mm = out_coord.mm + 1.;
          out_coord.ss = 0.;
          if(fabs(out_coord.mm - 60.) < 1.0e-7) {
             out_coord.hh = out_coord.hh + 1.;
             out_coord.mm = 0.;
          }
      }
      printf("%2.0f:%02.0f:%04.1f",out_coord.hh,out_coord.mm,out_coord.ss);
   }
   else {
      sprintf(out_string,"%.0f %02.0f %05.2f",coords.hh,coords.mm,coords.ss);
      sscanf(out_string,"%lf %lf %lf",&out_coord.hh,&out_coord.mm,
           &out_coord.ss);
      if(fabs(out_coord.ss - 60.) < 1.0e-6) {
         out_coord.mm = out_coord.mm + 1.;
         out_coord.ss = 0.;
         if(fabs(out_coord.mm - 60.) < 1.0e-6) {
            out_coord.hh = out_coord.hh + 1.;
            out_coord.mm = 0.;
         }
      }
      printf("%2.0f:%02.0f:%05.2f",out_coord.hh, out_coord.mm, out_coord.ss);
   }
}


double
get_coord(void) {

/* Reads a string from the terminal and converts it into
   a double-precision coordinate.  This is trickier than 
   it appeared at first, since a -00 tests as non-negative; 
   the sign has to be picked out and handled explicitly. */
/* Prompt for input in the calling routine.*/

   short sign;
   double hrs, mins, secs;
   char hh_string[6];  /* string with the first coord (hh) */
   char hh1[1];
   short i = 0;

   /* read and handle the hour (or degree) part with sign */

   scanf("%s",hh_string);
   hh1[0] = hh_string[i];

   while(hh1[0] == ' ') {
       /* discard leading blanks */
       i++;
       hh1[0] = hh_string[i];
   }

   if(hh1[0] == '-') sign = -1;

     else sign = 1;

   sscanf(hh_string,"%lf", &hrs);
   if(sign == -1) hrs = -1. * hrs;

   /* read in the minutes and seconds normally */
   scanf("%lf %lf",&mins,&secs);

   return(sign * (hrs + mins / 60. + secs / 3600.));
}

void
dec_to_bab (double deci, struct coord* bab) {

    /* function for converting decimal to babylonian hh mm ss.ss */
    int hr_int, min_int;
    
    if (deci >= 0.) bab->sign = 1; 
    else {
        bab->sign = -1;
        deci = -1. * deci;
    }
    hr_int = deci;   /* use conversion conventions to truncate */
    bab->hh = hr_int;
    min_int = 60. * (deci - bab->hh);
    bab->mm = min_int;
    bab->ss = 3600. * (deci - bab->hh - bab->mm / 60.);
}

double
atan_circ(double x, double y) {
    
    /* returns radian angle 0 to 2pi for coords x, y --
       get that quadrant right !! */
    
    double theta;
    
    if((x == 0.) && (y == 0.)) return(0.);  /* guard ... */
    
    theta = atan2(y,x);  /* turns out there is such a thing in math.h */
    while(theta < 0.) theta += TWOPI;
    return(theta);
}


double
adj_time(double x) {

    /* adjusts a time (decimal hours) to be between -12 and 12, 
       generally used for hour angles.  */
    
    if(fabs(x) < 100000.) {  /* too inefficient for this! */
        while(x > 12.) {
            x = x - 24.;
        }
        while(x < -12.) {
            x = x + 24.;
        }
    }

    else printf("warning: Out of bounds in adj_time in moon interference routine!\n");

    return(x);
}

double
altit(double dec, double ha, double lat,
      double* az, double *parang) {
            
/*
  returns altitude(degr) for dec, ha, lat (decimal degr, hr, degr); 
  also computes and returns azimuth through pointer argument,
  and as an extra added bonus returns parallactic angle (decimal degr)
  through another pointer argument.
  */

    double x,y,z;
    double sinp, cosp;  /* sin and cos of parallactic angle */
    double cosdec, sindec, cosha, sinha, coslat, sinlat;
    /* time-savers ... */
    
    dec = dec / DEG_IN_RADIAN;
    ha = ha / HRS_IN_RADIAN;
    lat = lat / DEG_IN_RADIAN;  /* thank heavens for pass-by-value */
    cosdec = cos(dec); sindec = sin(dec);
    cosha = cos(ha); sinha = sin(ha);
    coslat = cos(lat); sinlat = sin(lat);
    x = DEG_IN_RADIAN * asin(cosdec*cosha*coslat + sindec*sinlat);
    y =  sindec*coslat - cosdec*cosha*sinlat; /* due N comp. */
    z =  -1. * cosdec*sinha; /* due east comp. */
    *az = atan2(z,y);   
    
    /* as it turns out, having knowledge of the altitude and 
       azimuth makes the spherical trig of the parallactic angle
       less ambiguous ... so do it here!  Method uses the 
       "astronomical triangle" connecting celestial pole, object,
       and zenith ... now know all the other sides and angles,
       so we can crush it ... */
    
    if(cosdec != 0.) { /* protect divide by zero ... */ 
        sinp = -1. * sin(*az) * coslat / cosdec;
        /* spherical law of sines .. note cosdec = sin of codec,
           coslat = sin of colat .... */
        cosp = -1. * cos(*az) * cosha - sin(*az) * sinha * sinlat;
        /* spherical law of cosines ... also transformed to local
           available variables. */
	   *parang = atan2(sinp,cosp) * DEG_IN_RADIAN;
           /* let the library function find the quadrant ... */
    }
    else { /* you're on the pole */
        if(lat >= 0.) *parang = 180.;
        else *parang = 0.;
    }
    
    *az *= DEG_IN_RADIAN;  /* done with taking trig functions of it ... */ 
    *az = ohana_normalize_angle (*az);
    
    return(x);
}


int
main(void) {

    /* Site specific parameters */
    double longit, lat;
    double stdz;
    short use_dst;
    char zabr;
    double elevsea;
    double elev, horiz;

    char site_name[45];  /* initialized later with
                            strcpy for portability */
    char zone_name[25];  /* this too */

    struct date_time date;
    double jd, sid;    

    /* Position of moon */    
    double ra, dec, dist;    
    double ha, az, par, alt;    
    
    /* Misc stuff */
    short enter_ut = 1;
    short night_date = 0;
    
    /* Load site specific information */
    load_site(&longit,&lat,&stdz,&use_dst,zone_name,&zabr,
          &elevsea,&elev,&horiz,site_name);

    /* Get system date and calculate julian date */
    if (get_sys_date(&date, use_dst, enter_ut, night_date, stdz,
                     update_delta, &jd) != 0) { 
        printf("error: Can't get system date! \n");
        return(-1);
    }

    /* Calcualte local sidereal time */
    sid = lst(jd, longit);

    /* Calculate position of moon */
    lpmoon(jd, lat, sid, &ra, &dec, &dist);

    ha = adj_time(sid - ra);
    
    alt=altit(dec, ha, lat, &az, &par);

    /* Debug section
      printf("moon ra = ");    
      put_coords(ra, 2, 1);    
      printf("\n"); 
      printf("moon dec = ");  
      put_coords(dec, 2, 1); 
      printf("\n");
      printf("moon ha = "); 
      put_coords(ha, 2, 1); 
      printf("\n");
      printf("moon altitude = %.2f\n", alt); 
      printf("moon zenith-distance = %.2f\n", 90 - alt);    
      */
      
    /* Calculation of where to move */
    /* If zenith distance is within 40 degrees then we move.
       In order to do so the following logic will be applied:

       1) Is zenith distance within 40 degrees (inclusive)
       2) If so:
             if dec > 20 then
                move dec by -40
             else
                move dec by +40        
          
    */                      

    if (fabs(90.0 - alt) <= 40) {
        double adj_dec;        
        /* Zenith distance within 40 degress */        
        if (dec > lat) {
            adj_dec = dec - 40.0;                              
        }
        else {
            adj_dec = dec + 40.0;                     
        }
        printf("TRUE ");
        put_coords(adj_dec, 2, 1);
        printf("\n");    
    }
    else {
        printf("FALSE\n");        
    }
    
    return(0);    
    
}


