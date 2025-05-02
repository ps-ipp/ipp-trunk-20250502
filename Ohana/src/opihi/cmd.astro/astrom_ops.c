# include "astro.h"
# define J2000 51544.5       /* Modified Julian date at standard epoch J2000 */

/* Low precision formulae for the sun, from Astro. Almanac p. C5 (2012) */
int sun_ecliptic (double mjd, double *lambda, double *beta, double *epsilon, double *Radius) {

  double n = mjd - J2000;                          // day number relative to standard epoch
  double L = 280.460 + 0.9856474 * n;	           // mean solar longitute (corr. for aberration)
  double g = (357.528 + 0.9856003 * n)*RAD_DEG;    // Mean anomaly

  *lambda = L + 1.915 * sin(g) + 0.020 * sin(2*g); // solar longitude in degrees
  *beta = 0.0;					   // approx latitude
  *epsilon = (23.439 - 0.0000004 * n);		   // obliquity of ecliptic in degrees
  *Radius = 1.00014 - 0.01671*cos(g) - 0.00014*cos(2*g); // earth-to-sun dist in AU
  return TRUE;
}

/* given RA, DEC, Time, calculate the parallax factor */
// RA,DEC are decimal degrees
// Time is MJD
int ParFactor (double *pR, double *pD, double RA, double DEC, double Time) {

  double lambda, beta, epsilon, Radius;

  // Time must be mjd
  sun_ecliptic (Time, &lambda, &beta, &epsilon, &Radius);

  double lambda_rad = lambda*RAD_DEG;
  double epsilon_rad = epsilon*RAD_DEG;
  double RA_rad = RA*RAD_DEG;
  double DEC_rad = DEC*RAD_DEG;

  double x = Radius*cos(lambda_rad);
  double y = Radius*cos(epsilon_rad)*sin(lambda_rad);
  double z = Radius*sin(epsilon_rad)*sin(lambda_rad);

  *pR = +(y*cos(RA_rad) - x*sin(RA_rad));
  *pD = -(y*sin(RA_rad) + x*cos(RA_rad))*sin(DEC_rad) + z*cos(DEC_rad);

  return TRUE;
}

/* given RA, DEC, Time, calculate the parallax factor */
// RA,DEC are decimal degrees
// Time is MJD
int ParFactor_3d (double *pR, double *pD, double *pZ, double RA, double DEC, double Time) {

  double lambda, beta, epsilon, Radius;

  // Time must be mjd
  sun_ecliptic (Time, &lambda, &beta, &epsilon, &Radius);

  double lambda_rad = lambda*RAD_DEG;
  double epsilon_rad = epsilon*RAD_DEG;
  double RA_rad = RA*RAD_DEG;
  double DEC_rad = DEC*RAD_DEG;

  double x = Radius*cos(lambda_rad);
  double y = Radius*cos(epsilon_rad)*sin(lambda_rad);
  double z = Radius*sin(epsilon_rad)*sin(lambda_rad);

  *pR = +(y*cos(RA_rad) - x*sin(RA_rad));
  *pD = -(y*sin(RA_rad) + x*cos(RA_rad))*sin(DEC_rad) + z*cos(DEC_rad);
  *pZ = -(y*sin(RA_rad) + x*cos(RA_rad))*cos(DEC_rad) + z*sin(DEC_rad);

  return TRUE;
}

// allocate arrays but not the container
int PlxFitDataAlloc (PlxFitData *data, int N) {

  data->Npts = N;
  ALLOCATE (data->X,  double, N);
  ALLOCATE (data->Y,  double, N);
  ALLOCATE (data->dX, double, N);
  ALLOCATE (data->dY, double, N);
  ALLOCATE (data->t,  double, N);
  ALLOCATE (data->pX, double, N);
  ALLOCATE (data->pY, double, N);
  ALLOCATE (data->Wx, double, N);
  ALLOCATE (data->Wy, double, N);
  ALLOCATE (data->Qx, double, N);
  ALLOCATE (data->Qy, double, N);
  ALLOCATE (data->qx, double, N);
  ALLOCATE (data->qy, double, N);
  ALLOCATE (data->index, int, N);
  return TRUE;
}

void PlxFitDataFree (PlxFitData *data) {
  FREE (data->X);
  FREE (data->Y);
  FREE (data->dX);
  FREE (data->dY);
  FREE (data->t);
  FREE (data->pX);
  FREE (data->pY);
  FREE (data->Wx);
  FREE (data->Wy);
  FREE (data->Qx);
  FREE (data->Qy);
  FREE (data->qx);
  FREE (data->qy);
  FREE (data->index);
}

