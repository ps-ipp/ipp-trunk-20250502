# include <skycalc_internal.h>

/* converts a coordinate triplet back to a standard ra and dec */
void SC_xyz_cel (double x, double y, double z, double *r, double *d) {

   /* converts a coordinate triplet back to a standard ra and dec */

   double mod;    /* modulus */
   double xy;     /* component in xy plane */
   double radian_ra, radian_dec;

   /* this taken directly from pl1 routine - no acos or asin available there,
       as it is in c. Easier just to copy, though */

   mod = sqrt(x*x + y*y + z*z);
   x = x / mod;
   y = y / mod;
   z = z / mod;   /* normalize 'em explicitly first. */

   xy = sqrt(x*x + y*y);

   if(xy < 1.0e-10) {
      radian_ra = 0.;  /* too close to pole */
      radian_dec = PI / 2.;
      if(z < 0.) radian_dec = radian_dec * -1.;
   }
   else {
      if(fabs(z/xy) < 3.) radian_dec = atan(z / xy);
	 else if (z >= 0.) radian_dec = PI / 2. - atan(xy / z);
	 else radian_dec = -1. * PI / 2. - atan(xy / z);
      if(fabs(x) > 1.0e-10) {
	 if(fabs(y / x) < 3.) radian_ra = atan(y / x);
	 else if ((x * y ) >= 0.) radian_ra = PI / 2. - atan(x/y);
	 else radian_ra = -1. *  PI / 2. - atan(x / y);
      }
      else {
	 radian_ra = PI / 2.;
	 if((x * y)<= 0.) radian_ra = radian_ra * -1.;
      }
      if(x <0.) radian_ra = radian_ra + PI ;
      if(radian_ra < 0.) radian_ra = radian_ra + 2. * PI ;
   }

   *r = radian_ra * HRS_IN_RADIAN;
   *d = radian_dec * DEG_IN_RADIAN;

}

/* returns radian angle 0 to 2pi for coords x, y -- get that quadrant nright !! */
/* XXX : reimplements atan2() */
double SC_atan_circ (double x, double y) {
  
  double theta;
  
  if(x == 0.) {
    if(y > 0.) theta = PI / 2.;
    else if(y < 0.) theta = 3.* PI / 2.;
    else theta = 0.;   /* x and y zero */
  }
  else theta = atan(y/x);
  if(x < 0.) theta = theta + PI;
  if(theta < 0.) theta = theta + 2.* PI;
  return(theta);
}

/* returns altitude(degr) for dec, ha, lat (decimal degr, hr, degr);
    also computes and returns azimuth through pointer argument. */
double SC_altit (double dec, double ha, double lat, double *az) {

  double x,y,z;

  dec = dec / DEG_IN_RADIAN;
  ha = ha / HRS_IN_RADIAN;
  lat = lat / DEG_IN_RADIAN;
  x = DEG_IN_RADIAN * asin(cos(dec)*cos(ha)*cos(lat) + sin(dec)*sin(lat));
  y =  sin(dec)*cos(lat) - cos(dec)*cos(ha)*sin(lat); /* due N comp. */
  z =  -1. * cos(dec)*sin(ha); /* due east comp. */
  *az = SC_atan_circ(y,z) * DEG_IN_RADIAN;
  return(x);
}

/* returns hour angle at which object at dec is at altitude alt.
   If object is never at this altitude, signals with special
   return values 1000 (always higher) and -1000 (always lower). */
double SC_ha_alt (double dec, double lat, double alt) {

  double x,coalt,min,max;
  
  SC_min_max_alt(lat,dec,&min,&max);
  if(alt < min)
    return(1000.);  /* flag value - always higher than asked */
  if(alt > max)
    return(-1000.); /* flag for object always lower than asked */
  dec = (0.5*PI) - dec / DEG_IN_RADIAN;
  lat = (0.5*PI) - lat / DEG_IN_RADIAN;
  coalt = (0.5*PI) - alt / DEG_IN_RADIAN;
  x = (cos(coalt) - cos(dec)*cos(lat)) / (sin(dec)*sin(lat));
  if (fabs(x) <= 1.) return(acos(x) * HRS_IN_RADIAN);
  else {
    printf  ("Error in ha_alt ... acos(>1).\n");
    return (1000.);
  }
}

/* computes minimum and maximum altitude for a given dec and latitude. */
void SC_min_max_alt (double lat, double dec, double *min, double *max) {

  double x;

  lat = lat / DEG_IN_RADIAN; /* pass by value! */
  dec = dec / DEG_IN_RADIAN;
  x = cos(dec)*cos(lat) + sin(dec)*sin(lat);
  if (fabs(x) <= 1.) {
    *max = asin(x) * DEG_IN_RADIAN;
  }
  else printf ("Error in min_max_alt -- arcsin(>1)\n");

  x = sin(dec)*sin(lat) - cos(dec)*cos(lat);
  if(fabs(x) <= 1.) {
    *min = asin(x) * DEG_IN_RADIAN;
  }
  else printf ("Error in min_max_alt -- arcsin(>1)\n");
}

/* force domain to be 0 - 360 degrees */
double SC_circulo (double x) {
  
  /* fails for negative angles! */

  int n;
  
  n = (int)(x / 360.);
  return(x - 360. * n);
}

