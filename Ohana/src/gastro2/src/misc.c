# include "gastro2.h"

# define SIGN(X)  (((X) == 0) ? 0 : ((fabs((double)(X))) / (X)))

void hh_hms (double hh, int *hr, int *mn, double *sc) {

  int flag;

  flag = SIGN(hh);
  hh *= flag;
  hh = 24.0*(hh/24.0 - (int)(hh/24.0));
  *sc = 60.0*(60.0*hh - (int)(60.0*hh));
  *mn = 60.0*(hh - (int)hh);
  *hr = (int) hh;
  *hr *= flag;

}
 
void hms_format (char *line, double value) {

  int hr, mn;
  double sc;

  hh_hms (value, &hr, &mn, &sc);
  hr = (int) value;
  if (isnan (value))
    sprintf (line, "xx:xx:xx.xx");
  else {
    if (value < 0) {
      sprintf (line, "-%02d:%02d:%05.2f", abs(hr), mn, sc);
    } else {
      sprintf (line, "%02d:%02d:%05.2f", hr, mn, sc);
    }
  }      
}

void area_of_region (CatStats *region) {
  
  double area;

  area = DEG_RAD*(region[0].RA[1] - region[0].RA[0])*(sin(region[0].DEC[1]*RAD_DEG) - sin(region[0].DEC[0]*RAD_DEG));
  region[0].Area = area;
}

double area_of_skyregion (SkyRegion *region) {
  
  double area;

  area = DEG_RAD*(region[0].Rmax - region[0].Rmin)*(sin(region[0].Dmax*RAD_DEG) - sin(region[0].Dmin*RAD_DEG));
  return (area);
}

void dump_coords (CmpCatalog *Target) {
  if (VERBOSE) {
    fprintf (stderr, "%s\n", Target[0].coords.ctype);
    fprintf (stderr, "%f %f\n", Target[0].coords.crval1, Target[0].coords.crval2);
    fprintf (stderr, "%f %f\n", Target[0].coords.crpix1, Target[0].coords.crpix2);
    fprintf (stderr, "%f %f\n", Target[0].coords.pc1_1,  Target[0].coords.pc1_2);
    fprintf (stderr, "%f %f\n", Target[0].coords.pc2_1,  Target[0].coords.pc2_2);
    fprintf (stderr, "%f %f\n", Target[0].coords.cdelt1, Target[0].coords.cdelt2);
    fprintf (stderr, "\n");
  }
}
