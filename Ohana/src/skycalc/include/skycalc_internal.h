/* header for use by the skycalc library files and stand-alone programs, not needed for external calls */

# include <stdio.h>
# include <math.h>
# include <stdlib.h>
# include <time.h>
# include <skycalc.h>

/* some (not all) physical, mathematical, and astronomical constants
   used are defined here. */

#define  PI                3.14159265358979
#define  ARCSEC_IN_RADIAN  206264.8062471
#define  DEG_IN_RADIAN     57.2957795130823
#define  HRS_IN_RADIAN     3.819718634205
#define  KMS_AUDAY         1731.45683633   /* km per sec in 1 AU/day */
#define  SS_MASS           1.00134198      /* solar system mass in solar units */
#define  J2000             2451545.        /* Julian date at standard epoch */
#define  SEC_IN_DAY        86400.
#define  FLATTEN           0.003352813   /* flattening of earth, 1/298.257 */
#define  EQUAT_RAD         6378137.    /* equatorial radius of earth, meters */
#define  ASTRO_UNIT        1.4959787066e11 /* 1 AU in meters */
#define  RSUN              6.96000e8  /* IAU 1976 recom. solar radius, meters */
#define  RMOON             1.738e6    /* IAU 1976 recom. lunar radius, meters */
#define  PLANET_TOL        3.          /* flag if nearer than 3 degrees
						to a major planet ... */

# define dCOS(A)   ((double) cos ((double)RAD_DEG*A))
# define dSIN(A)   ((double) sin ((double)RAD_DEG*A))

/** prototypes of private functions used by the library **/

/* in time.c */
int    SC_get_sys_date (struct SC_date_time *date);
double SC_date_to_jd (struct SC_date_time date);
void   SC_jd_to_date (double jdin, struct SC_date_time *date);
double SC_lst (double jd, double longit);
double SC_adj_time (double x);

/* in geometry.c */
void   SC_xyz_cel (double x, double y, double z, double *r, double *d);
double SC_atan_circ (double x, double y);
double SC_altit (double dec, double ha, double lat, double *az);
double SC_ha_alt (double dec, double lat, double alt);
void   SC_min_max_alt (double lat, double dec, double *min, double *max);
double SC_circulo (double x);

/* in astro.c */
void   SC_precrot (double rorig, double dorig, double orig_epoch, double final_epoch, double *rf, double *df);
void   SC_geocent (double geolong, double geolat, double height, double *x_geo, double *y_geo, double *z_geo);
void   SC_eclrot(double jd, double *x, double *y, double *z);
double SC_etcorr (double jd);
void   SC_set_zenith (struct SC_date_time date, double lat, double longit, double epoch, double *ra, double *dec);

/* in sun.c */
void   SC_lpsun (double jd, double *ra, double *dec);
double SC_jd_sun_alt (double alt, double jdguess, double lat, double longit);
double SC_sunset_tonight (struct SC_date_time date, double lat, double longit, double elev);
double SC_sunrise_tonight (struct SC_date_time date, double lat, double longit, double elev);

/* in moon.c */
void   SC_lpmoon(double jd, double lat, double sid, double* ra, double* dec, double* dist);
void   SC_accumoon (double jd, double geolat, double lst, double elevsea, double *geora, double *geodec, double *geodist, double *topora, double *topodec, double *topodist);
double SC_jd_moon_alt (double alt, double jdguess, double lat, double longit, double elevsea);
double SC_moonset_tonight (struct SC_date_time date, double lat, double longit, double elevsea, double elev);
double SC_moonrise_tonight (struct SC_date_time date, double lat, double longit, double elevsea, double elev);

// are these defined in here or in libohana?
// int dms_to_ddd (double *Value, char *string);
// int str_to_radec (double *ra, double *dec, char *str1, char *str2);
// int chk_time (char *line);
// double sec_to_jd (time_t second);
// time_t jd_to_sec (double jd);
// char *sec_to_date (time_t second);
// time_t date_to_sec (char *date);
// int str_to_time (char *line, time_t *second);
// int str_to_dtime (char *line, double *second);
