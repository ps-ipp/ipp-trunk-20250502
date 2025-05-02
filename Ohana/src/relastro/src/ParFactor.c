# include "relastro.h"
# define J2000 2451545.       /* Julian date at standard epoch */

/* Low precision formulae for the sun, from Astro. Almanac p. C5 (2012) */
// jdoff is days since J2000
int sun_ecliptic (double jdoff, double *lambda, double *beta, double *epsilon, double *Radius) {

  double n = jdoff;	      // day number
  double L = 280.460 + 0.9856474 * n; // mean solar longitute (corr. for aberration)
  double g = (357.528 + 0.9856003 * n)*RAD_DEG; // Mean anomaly

  *lambda = L + 1.915 * sin(g) + 0.020 * sin(2*g); // solar longitude in degrees
  *beta = 0.0;					   // approx latitude
  *epsilon = (23.439 - 0.0000004 * n);		   // obliquity of ecliptic in degrees
  *Radius = 1.00014 - 0.01671*cos(g) - 0.00014*cos(2*g); // earth-to-sun dist in AU
  return TRUE;
}

/* given RA, DEC, Time, calculate the parallax factor */
// Time is years since J2000
int ParFactor (double *pR, double *pD, double RA, double DEC, double Time) {

  double lambda, beta, epsilon, Radius;

  /* given a Time in years since J2000, determine the solar longitude S */

  double jdoff = 365.25*Time;

  sun_ecliptic (jdoff, &lambda, &beta, &epsilon, &Radius);

  double lambda_rad = lambda*RAD_DEG;
  double epsilon_rad = epsilon*RAD_DEG;
  double RA_rad = RA*RAD_DEG;
  double DEC_rad = DEC*RAD_DEG;

  double x = Radius*cos(lambda_rad);
  double y = Radius*cos(epsilon_rad)*sin(lambda_rad);
  double z = Radius*sin(epsilon_rad)*sin(lambda_rad);

  // original terms:
  // *pR =  +(cos(e)*sin(s)*cos(r) - cos(s)*sin(r));
  // *pD =  -(cos(e)*sin(s)*sin(r) + cos(s)*cos(r))*sin(d) + sin(e)*sin(s)*cos(d);

  // convert e->eps, s->lam, etc
  // *pR =  +(cos(eps)*sin(lam)*cos(ra) - cos(lam)*sin(ra));
  // *pD =  -(cos(eps)*sin(lam)*sin(ra) + cos(lam)*cos(ra))*sin(dec) + sin(eps)*sin(lam)*cos(dec);

  // convert to x,y,z
  // *pR = +(y*cos(ra) - x*sin(ra));
  // *pD = -(y*sin(ra) + x*cos(ra))*sin(dec) + z*cos(dec);

  // NOTE: seems to be identical to the old values, except I now include the varying solar distance
  *pR = +(y*cos(RA_rad) - x*sin(RA_rad));
  *pD = -(y*sin(RA_rad) + x*cos(RA_rad))*sin(DEC_rad) + z*cos(DEC_rad);

  return TRUE;
}

# if (0)
/* Low precision formulae for the sun, from Almanac p. C24 (1990) */
/* ra and dec are returned as decimal hours and decimal degrees. */
void lpsun (double jd, double *ra, double *dec) {

  double n, L, g, lambda,epsilon,alpha,delta,x,y,z;

  n = jd - J2000;
  L = 280.460 + 0.9856474 * n;
  g = (357.528 + 0.9856003 * n)/DEG_IN_RADIAN;
  lambda = (L + 1.915 * sin(g) + 0.020 * sin(2. * g))/DEG_IN_RADIAN;
  epsilon = (23.439 - 0.0000004 * n)/DEG_IN_RADIAN;

  // this is the conversion from ecliptic to celestial coords
  x = cos(lambda);
  y = cos(epsilon)*sin(lambda);
  z = sin(epsilon)*sin(lambda);

  *ra = (atan_circ(x,y))*HRS_IN_RADIAN;
  *dec = (asin(z))*DEG_IN_RADIAN;
}

/* code borrowed from Skycalc : fix this stuff XXX */
/* Low precision formulae for the sun, from Almanac p. C24 (1990) */
int sun_ecliptic (double jd, double *lambda, double *beta, double *epsilon) {

  double n, L, g;


  n = jd - J2000;
  L = 280.460 + 0.9856474 * n;
  g = (357.528 + 0.9856003 * n)*RAD_DEG;
  *lambda = L + 1.915 * sin(g) + 0.020 * sin(2. * g); // longitude in degrees
  *beta = 0.0;					  // approx latitude
  *epsilon = (23.439 - 0.0000004 * n);		  // obliquity of ecliptic in degrees
  return TRUE;
}
# endif

