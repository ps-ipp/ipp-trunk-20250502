# include <ohana.h>
# include <dvo.h>

int    VERBOSE;
int    RESET;
int    FORCE_RUN;

/* global variables set in parameter file */
char   CATDIR[256];
char   GSCDIR[256];
char   ImageCat[256];
char   GSCFILE[256];

double RADIUS;
double TRAIL_WIDTH;
int    NBINS;
int    NPTSINLINE;
double MIN_DENSITY;
double NSIGMA;

double BRIGHT_HALO_MAG;
double BRIGHT_HALO_SLOPE;
double BRIGHT_XTRAIL_WIDTH;
double BRIGHT_XTRAIL_MAG;
double BRIGHT_XTRAIL_SLOPE;
double BRIGHT_YTRAIL_WIDTH;
double BRIGHT_YTRAIL_MAG;
double BRIGHT_YTRAIL_SLOPE;

double GHOST_MAG;
double GHOST_RADIUS;
double OPTICAL_AXIS1;
double OPTICAL_AXIS2;

typedef struct {
  Coords coords;
  double *X, *Y;
  int *N;
  double RA[2], DEC[2];
  double Area, density, spacing;
} CatStats;

