# include <skycalc_internal.h>
# include <ohana.h>
# define VERBOSE 0
# define TEST 0

# define MKO_LONGITUDE 10.36478 /*  W longitude in decimal hours */                     
# define MKO_LATITUDE  19.82670 /*  N latitude in decimal degrees */                    
# define MKO_ELEVATION   4215.0 /* elevation above sea level (for absolute location) */ 

double angular_separation (double *bearing, double ra, double dec, double RA, double DEC);

void test_angular_separation (double ra, double dec, double RA, double DEC) {

    double bearing;
    double dist = angular_separation (&bearing, ra, dec, RA, DEC);
    fprintf (stderr, "test: %f, %f : %f, %f -> %f, %f\n", ra, dec, RA, DEC, dist, bearing);
    return;
}

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

  if (TEST) {

    double r1 = atof (argv[1]);
    double d1 = atof (argv[2]);
    double r2 = atof (argv[3]);
    double d2 = atof (argv[4]);

    test_angular_separation (r1, d1, r2, d2);
    exit (0);

    double r;
    for (r = 0; r < 360; r += 5.0) {
      test_angular_separation (0.0, 0.0, r, 10.0);
    }

    // some simple tests to check the angular_separation return values:
    test_angular_separation (0.0, 0.0, 0.0, 15.0);
    test_angular_separation (0.0, 0.0, 0.0, 45.0);
    test_angular_separation (0.0, 0.0, 0.0, 90.0);
    test_angular_separation (0.0, 0.0, 0.0, -15.0);
    test_angular_separation (0.0, 0.0, 0.0, -45.0);
    test_angular_separation (0.0, 0.0, 0.0, -90.0);

    test_angular_separation (0.0, 0.0, 15.0, 0.0);
    test_angular_separation (0.0, 0.0, 45.0, 0.0);
    test_angular_separation (0.0, 0.0, 90.0, 0.0);

    test_angular_separation (0.0, 0.0, 180.0, 0.0);
    test_angular_separation (0.0, 0.0, 185.0, 0.0);
    test_angular_separation (0.0, 0.0, 185.0, 1.0);

    test_angular_separation (0.0, 0.0, 175.0, 0.0);
    test_angular_separation (0.0, 0.0, 175.0, 1.0);

    test_angular_separation (0.0, 0.0, 190.0, 0.0);
    test_angular_separation (0.0, 0.0, 270.0, 0.0);
    test_angular_separation (0.0, 0.0, 300.0, 0.0);
    test_angular_separation (0.0, 0.0, 350.0, 0.0);

    test_angular_separation (0.0, 0.0, 181.0, 40.0);

    test_angular_separation (0.0, 0.0, 5.0, 45.0);
    test_angular_separation (0.0, 0.0, 15.0, 15.0);
    test_angular_separation (0.0, 0.0, 45.0, 5.0);

    test_angular_separation (0.0, 0.0, 355.0, 45.0);
    test_angular_separation (0.0, 0.0, 345.0, 15.0);
    test_angular_separation (0.0, 0.0, 300.0, 5.0);

    test_angular_separation (0.0, 0.0, 5.0, -45.0);
    test_angular_separation (0.0, 0.0, 15.0, -15.0);
    test_angular_separation (0.0, 0.0, 45.0, -5.0);

    test_angular_separation (0.0, 0.0, 355.0, -45.0);
    test_angular_separation (0.0, 0.0, 345.0, -15.0);
    test_angular_separation (0.0, 0.0, 300.0, -5.0);

    exit (0);
  }

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

  double sun_bearing = NAN;
  sun_angle = angular_separation (&sun_bearing, RAo, DECo, sun_ra, sun_dec);

  if (VERBOSE) fprintf (stderr, "sun @ ra,dec = %f %f : alt, az = %f %f\n", sun_ra, sun_dec, sun_alt, sun_az);

  // get the moon coordintes
  SC_accumoon (jdnow, latitude, sid, elevation, &geo_ra, &geo_dec, &geo_dist, &moon_ra, &moon_dec, &moon_dist);
  moon_ha = (sid - moon_ra);
  moon_ra *= 15.0; // convert to degrees
  moon_alt = SC_altit (moon_dec, moon_ha, latitude, &moon_az);

  double moon_bearing = NAN;
  moon_angle = angular_separation (&moon_bearing, RAo, DECo, moon_ra, moon_dec);

  if (VERBOSE) fprintf (stderr, "moon @ ra,dec = %f %f : alt, az = %f %f\n", moon_ra, moon_dec, moon_alt, moon_az);

  phase = (moon_ra - sun_ra)/360.0;
  while (phase <  0.0) phase += 1.0;
  while (phase >  1.0) phase -= 1.0;

  if (VERBOSE) fprintf (stderr, "sun @ %f %f dist %f\n", sun_ra, sun_dec, sun_angle);
  if (VERBOSE) fprintf (stderr, "moon @ %f %f angle %f phase %f\n", moon_ra, moon_dec, moon_angle, phase);

  fprintf (stdout, "sun_alt %f sun_angle %f sun_bearing %f moon_alt %f moon_angle %f moon_bearing %f moon_phase %f\n", sun_alt, sun_angle, sun_bearing, moon_alt, moon_angle, moon_bearing, phase);
  exit (0);
}

// I got these formulae from:
// http://www.movable-type.co.uk/scripts/latlong.html
// and checked them against their Javascript version
double angular_separation (double *bearing, double r1, double d1, double r2, double d2) {

  // Bearing = atan2(sin dLambda cos phi_1, cos phi_2 sin phi_2 - sin phi_1 cos phi_2 cos dLambda)

  // law of cosines distance:
  // Dist  = acos (sin phi_1 sin phi_2 + cos phi_1 cos phi_2 cos dLambda)

  double sd1 = sin (d1*RAD_DEG);
  double cd1 = cos (d1*RAD_DEG);
  double sd2 = sin (d2*RAD_DEG);
  double cd2 = cos (d2*RAD_DEG);

  double dR  = RAD_DEG*(r2 - r1);
  double sdR = sin (dR);
  double cdR = cos (dR);

  double Dist = DEG_RAD * acos (sd1*sd2 + cd1*cd2*cdR);
  // double Dist = 6371 * acos (sd1*sd2 + cd1*cd2*cdR);

  // this is the initial bearing starting from poing r1,d1 and heading to r2,d2
  *bearing = DEG_RAD * atan2 (+sdR*cd2, cd1*sd2 - sd1*cd2*cdR);

  return Dist;
}
