# include <stdio.h>
# include <math.h>
# include <skycalc.h>

void set_site (double *longit, double *lat, double *elevsea, double *elev) {

  *longit = 10.36478; /*  W longitude in decimal hours */                     
  *lat = 19.8267;     /*  N latitude in decimal degrees */                    
  *elevsea = 4215.;   /* elevation above sea level (for absolute location) */ 
  *elev = 4215.;      /* observatory elevation above horizon, meters */       

}

main()

{

  struct SC_date_time date, tmpdate;
  double longit, lat, elevsea, elev;
  double jd;

  SC_set_site (&longit, &lat, &elevsea, &elev);
  SC_get_sys_date (&date);

  jd = SC_sunset_tonight (date, lat, longit, elev);
  fprintf (stderr, "Sunset (%5.0f m horizon): %f\n", elev, jd);
  SC_jd_to_date (jd, &tmpdate);
  fprintf (stderr, "%4d/%02d/%02d %02d:%02d:%02f\n", tmpdate.y, tmpdate.mo, tmpdate.d, tmpdate.h, tmpdate.mn, tmpdate.s);

  jd = SC_sunrise_tonight (date, lat, longit, elev);
  fprintf (stderr, "Sunrise (%5.0f m horizon): %f\n", elev, jd);
  SC_jd_to_date (jd, &tmpdate);
  fprintf (stderr, "%4d/%02d/%02d %02d:%02d:%02f\n", tmpdate.y, tmpdate.mo, tmpdate.d, tmpdate.h, tmpdate.mn, tmpdate.s);
  

  jd = SC_moonset_tonight (date, lat, longit, elevsea, elev);
  fprintf (stderr, "Moonset (%5.0f m horizon): %f\n", elev, jd);
  SC_jd_to_date (jd, &tmpdate);
  fprintf (stderr, "%4d/%02d/%02d %02d:%02d:%02f\n", tmpdate.y, tmpdate.mo, tmpdate.d, tmpdate.h, tmpdate.mn, tmpdate.s);

  jd = SC_moonrise_tonight (date, lat, longit, elevsea, elev);
  fprintf (stderr, "Moonrise (%5.0f m horizon): %f\n", elev, jd);
  SC_jd_to_date (jd, &tmpdate);
  fprintf (stderr, "%4d/%02d/%02d %02d:%02d:%02f\n", tmpdate.y, tmpdate.mo, tmpdate.d, tmpdate.h, tmpdate.mn, tmpdate.s);

}

  /* set_zenith (date, lat, longit, objepoch, &objra, &objdec); */
