# include <skycalc_internal.h>
# include <ohana.h>
# define VERBOSE 0

void SC_set_site (double *longit, double *lat, double *elevsea, double *elev) {

  *longit = 10.36478; /*  W longitude in decimal hours */                     
  *lat = 19.8267;     /*  N latitude in decimal degrees */                    
  *elevsea = 4215.;   /* elevation above sea level (for absolute location) */ 
  *elev = 4215.;      /* observatory elevation above horizon, meters */       

}

int main (int argc, char **argv) {

  struct SC_date_time date;
  double longit, lat, elevsea, elev;
  double jdnow, sid, alt, az, sind, sinh, cosh;
  double RAo, DECo, abx, aby, abz, cs, theta;
  double Rsun, Dsun, Hsun;
  time_t tzero;
  struct tm *stm;

  if (argc != 4) {
    fprintf (stderr, "USAGE: sundata (date) (ra) (dec) [ra & dec in dec. deg.]\n");
    exit (1);
  }

  RAo = atof (argv[2]);
  DECo = atof (argv[3]);

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
  jdnow  = SC_date_to_jd (date);

  /* Calculate local sidereal time */
  sid = SC_lst(jdnow, longit);
  SC_lpsun (jdnow, &Rsun, &Dsun);

  if (VERBOSE) fprintf (stderr, "jdnow: %lf, sid: %lf\n", jdnow, sid);

  /* dot product of unit vectors of (RAo,DECo) & (Rsun,Dsun) */
  abx = dCOS(RAo)*dCOS(DECo)*dCOS(15*Rsun)*dCOS(Dsun);
  aby = dSIN(RAo)*dCOS(DECo)*dSIN(15*Rsun)*dCOS(Dsun);
  abz = dSIN(DECo)*dSIN(Dsun);
  cs = abx + aby + abz;
  theta = DEG_RAD * acos (cs);

  /***** get sun altitude *****/
  Hsun = 15.0*(sid - Rsun);
 
  sind = dSIN (Dsun) * dSIN (lat) + dCOS (Dsun) * dCOS (Hsun) * dCOS (lat);
  alt  = DEG_RAD * asin (sind);

  sinh = - dCOS (Dsun) * dSIN (Hsun);
  cosh =   dSIN (Dsun) * dCOS (lat) - dCOS (Dsun) * dCOS (Hsun) * dSIN (lat);
  az = DEG_RAD * atan2 (sinh, cosh);

  fprintf (stdout, "sun @ %f %f dist %f altaz: %f %f\n", 15*Rsun, Dsun, theta, alt, az);
  exit (0);

}

