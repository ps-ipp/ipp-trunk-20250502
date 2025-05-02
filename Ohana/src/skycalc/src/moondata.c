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
  double jdnow, ra, dec, sid;
  double geora, geodec, geodist, dist;
  double RAo, DECo, abx, aby, abz, cs, theta;
  double Rsun, Dsun, days;
  time_t tzero;
  struct tm *stm;

  if (argc != 4) {
    fprintf (stderr, "USAGE: moondata (date) (ra) (dec) [ra & dec in dec. deg.]\n");
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

  /*
  fprintf (stderr, "%4d/%02d/%02d %02d:%02d:%02f\n", date.y, date.mo, date.d, date.h, date.mn, date.s);
  */

  SC_set_site (&longit, &lat, &elevsea, &elev);
  jdnow  = SC_date_to_jd (date);
  /* Calcualte local sidereal time */
  sid = SC_lst(jdnow, longit);
  SC_lpsun (jdnow, &Rsun, &Dsun);

  SC_accumoon(jdnow,lat,sid,elevsea,&geora,&geodec,&geodist,&ra,&dec,&dist);

  /*  fprintf (stdout, "moon @ %f %f\n", 15*ra, dec); */

  abx = cos(RAo*RAD_DEG)*cos(DECo*RAD_DEG)*cos(15*ra*RAD_DEG)*cos(dec*RAD_DEG);
  aby = sin(RAo*RAD_DEG)*cos(DECo*RAD_DEG)*sin(15*ra*RAD_DEG)*cos(dec*RAD_DEG);
  abz = sin(DECo*RAD_DEG)*sin(dec*RAD_DEG);

  cs = abx + aby + abz;
  
  theta = DEG_RAD * acos (cs);

  days = (Rsun - ra - 12)/24.0;
  while (days < -0.5) days += 1.0;
  while (days >  0.5) days -= 1.0;
  days *= 29.5;

  fprintf (stdout, "moon @ %f %f dist %f %f days from full\n", 15*ra, dec, theta, days);
  exit (0);

}

