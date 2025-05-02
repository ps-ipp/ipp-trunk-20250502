# include <skycalc_internal.h>
# include <ohana.h>
# define VERBOSE 0

# define MKO_LONGITUDE 10.36478 /*  W longitude in decimal hours */                     
# define MKO_LATITUDE  19.82670 /*  N latitude in decimal degrees */                    
# define MKO_ELEVATION   4215.0 /* elevation above sea level (for absolute location) */ 

double angular_separation (double ra, double dec, double RA, double DEC);

int main (int argc, char **argv) {

  int N;
  struct SC_date_time date;
  double longitude, latitude, elevation;
  double jdnow, sid;
  double moon_ra, moon_dec, moon_angle, moon_alt, moon_az, moon_ha, moon_dist;
  double sun_ra, sun_dec, sun_angle, sun_alt, sun_az, sun_ha;
  double geo_ra, geo_dec, geo_dist;
  double RAo, DECo;
  double phase;
  time_t tzero;
  struct tm *stm;

  longitude = MKO_LONGITUDE; // MKO longitude in decimal hours
  if ((N = get_argument (argc, argv, "-longitude"))) {
    remove_argument (N, &argc, argv);
    longitude = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  latitude = MKO_LATITUDE; // MKO latitude in decimal degrees
  if ((N = get_argument (argc, argv, "-latitude"))) {
    remove_argument (N, &argc, argv);
    latitude = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  elevation = MKO_ELEVATION; // MKO elevation in meters
  if ((N = get_argument (argc, argv, "-elevation"))) {
    remove_argument (N, &argc, argv);
    elevation = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    fprintf (stderr, "USAGE: sunmoon [-longitude long] [-latitude lat] [-elevation elev] (date) (ra) (dec)\n");
    fprintf (stderr, " longitude : W long in decimal HOURS\n");
    fprintf (stderr, " latitude  : N lat in decimal DEGREES\n");
    fprintf (stderr, " elevation : meters above sea level\n");
    fprintf (stderr, " ra & dec  : decimal degrees\n");
    fprintf (stderr, " date      : YYYY/MM/DD,HH:MM:SS.SSS -- least significant elements are optional\n");
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

  // get JD
  jdnow  = SC_date_to_jd (date);

  // Calculate local sidereal time
  sid = SC_lst(jdnow, longitude);

  if (VERBOSE) fprintf (stderr, "jdnow: %lf, sid: %lf\n", jdnow, sid);

  // get the sun coordinates
  SC_lpsun (jdnow, &sun_ra, &sun_dec);
  sun_ha = (sid - sun_ra);
  sun_ra *= 15.0;
  sun_alt = SC_altit (sun_dec, sun_ha, latitude, &sun_az);
  sun_angle = angular_separation (sun_ra, sun_dec, RAo, DECo);

  if (VERBOSE) fprintf (stderr, "sun @ ra,dec = %f %f : alt, az = %f %f\n", sun_ra, sun_dec, sun_alt, sun_az);

  // get the moon coordintes
  SC_accumoon (jdnow, latitude, sid, elevation, &geo_ra, &geo_dec, &geo_dist, &moon_ra, &moon_dec, &moon_dist);
  moon_ha = (sid - moon_ra);
  moon_ra *= 15.0; // convert to degrees
  moon_alt = SC_altit (moon_dec, moon_ha, latitude, &moon_az);
  moon_angle = angular_separation (moon_ra, moon_dec, RAo, DECo);

  if (VERBOSE) fprintf (stderr, "moon @ ra,dec = %f %f : alt, az = %f %f\n", moon_ra, moon_dec, moon_alt, moon_az);

  phase = (moon_ra - sun_ra)/360.0;
  while (phase <  0.0) phase += 1.0;
  while (phase >  1.0) phase -= 1.0;

  if (VERBOSE) fprintf (stderr, "sun @ %f %f dist %f\n", sun_ra, sun_dec, sun_angle);
  if (VERBOSE) fprintf (stderr, "moon @ %f %f angle %f phase %f\n", moon_ra, moon_dec, moon_angle, phase);

  fprintf (stdout, "-sun_alt %f -sun_angle %f -moon_alt %f -moon_angle %f -moon_phase %f\n", sun_alt, sun_angle, moon_alt, moon_angle, phase);
  exit (0);
}

double angular_separation (double ra, double dec, double RA, double DEC) {

  double abx, aby, abz, cs, theta;

  abx = cos(RA*RAD_DEG)*cos(DEC*RAD_DEG)*cos(ra*RAD_DEG)*cos(dec*RAD_DEG);
  aby = sin(RA*RAD_DEG)*cos(DEC*RAD_DEG)*sin(ra*RAD_DEG)*cos(dec*RAD_DEG);
  abz = sin(DEC*RAD_DEG)*sin(dec*RAD_DEG);

  cs = abx + aby + abz;
  
  theta = DEG_RAD * acos (cs);

  return theta;
}
