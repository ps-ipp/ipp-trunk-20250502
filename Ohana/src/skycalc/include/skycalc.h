
/* This is the header file for the Ohana version of 'libskycalc'.  This library is based on the
 * code provided to the community by John Thorstensen.  See the discussion in the README file
 * and in the file doc/Thorstensen.txt
 */

/* header for skycalc library function calls */

struct SC_coord {
     short sign;  /* carry sign explicitly since -0 not neg. */
     double hh;
     double mm;
     double ss;
}; 

struct SC_date_time {
  short y;
  short mo;
  short d;
  short h;
  short mn;
  float s;
};

double sunset_tonight (struct SC_date_time date, double lat, double longit, double elev);
double sunrise_tonight (struct SC_date_time date, double lat, double longit, double elev);
double moonset_tonight (struct SC_date_time date, double lat, double longit, double elevsea, double elev);
double moonrise_tonight (struct SC_date_time date, double lat, double longit, double elevsea, double elev);
