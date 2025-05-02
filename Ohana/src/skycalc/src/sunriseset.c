# include <skycalc_internal.h>
# include <ohana.h>
# define VERBOSE 0

// XXX is this set for MKO??
void SC_set_site (double *longit, double *lat, double *elevsea, double *elev) {

  *longit = 10.36478; /*  W longitude in decimal hours */                     
  *lat = 19.8267;     /*  N latitude in decimal degrees */                    
  *elevsea = 4215.;   /* elevation above sea level (for absolute location) */ 
  *elev = 4215.;      /* observatory elevation above horizon, meters */       

}

int main (int argc, char **argv) {

  struct SC_date_time date;
  double longit, lat, elevsea, elev;
  double jdnow, jdset, jdrise;
  time_t tzero;
  struct tm *stm;

  if (argc != 2) {
    fprintf (stderr, "USAGE: sunriseset (date)\n");
    exit (1);
  }

  if (!ohana_str_to_time (argv[1], &tzero)) { 
    fprintf (stderr, "syntax error\n");
    exit (1);
  }
  stm = gmtime (&tzero);
  date.y  = (short) (stm->tm_year + 1900);
  date.mo = (short) (stm->tm_mon + 1);
  date.d  = (short) (stm->tm_mday);
  date.h  = (short) (stm->tm_hour);
  date.mn = (short) (stm->tm_min);
  date.s  = (float) (stm->tm_sec);

  if (VERBOSE) fprintf (stderr, "%4d/%02d/%02d %02d:%02d:%02f\n", date.y, date.mo, date.d, date.h, date.mn, date.s);

  SC_set_site (&longit, &lat, &elevsea, &elev);

  jdnow  = (int)(SC_date_to_jd (date));
  jdset  = SC_sunset_tonight (date, lat, longit, elev);
  jdrise = SC_sunrise_tonight (date, lat, longit, elev);

  fprintf (stdout, "jd %f  rise %f  set %f   day length %f\n", jdnow, 24*60*(jdrise - jdnow), 24*60*(jdset - jdnow), 24*60*(jdset - jdrise));
  exit (0);
}

  /* set_zenith (date, lat, longit, objepoch, &objra, &objdec); */
