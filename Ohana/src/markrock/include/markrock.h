# include <ohana.h>
# include <dvo.h>

typedef struct {
  Coords coords;
  double *X, *Y;
  int *N;
  double RA[2], DEC[2];
  double Area, density, spacing;
} CatStats;

typedef struct {
  double ra[3];
  double dec[3];
  double X[3];
  double Y[3];
  unsigned int t[3];
  double mag[3];
  int N[3];
} Rocks;

/* global variables set in parameter file */

int    VERBOSE;
int    RESET;
int    FORCE_RUN;

char   GSCFILE[256];
char   GSCDIR[256];
char   CATDIR[256];
char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char   RockCat[256];

double RADIUS;
double MAX_RADIUS;
double MAX_SPEED;
double MAX_DELAY;
double ZERO_POINT;

double BRIGHT_HALO_MAG;
double BRIGHT_HALO_SLOPE;
double ROCK_NEIGHBOR_RADIUS;
int    ROCK_NEIGHBOR_NMAX;

PhotCodeData photcodes;
