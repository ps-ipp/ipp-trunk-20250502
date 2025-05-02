# include <ohana.h>
# include <dvo.h>
# include <kapa.h>

double DEFAULT_RADIUS;
double MINIMUM_RADIUS;
double MAX_ERROR, MAX_NONLINEAR;
double MIN_PRECISE;
double CCD_PC1_1;
double CCD_PC2_2;
double CCD_PC1_2;
double CCD_PC2_1;
double NFIELD;
double SEARCH_RADIUS;
double MMIN;
double ROT_ZERO;
double dROT;
double RA_OFFSET, DEC_OFFSET;
double POLE_RA, POLE_DEC;
int POLAR_ALIGNMENT;
int NROT;
int VERBOSE;
int LONEOS_COORDS;
int CATDUMP;
int MATCHDUMP;
int NOMATCHDUMP;
int NEWPHOTCODE;
int MIN_MATCHES;
char *PHOTCODE;
int FLIPX, FLIPY;
int NPOLYTERMS;
char CDROM[256];
char CATDIR[256];
char CATMODE[16];    /* raw, mef, split, mysql */
char CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char REFCAT[256];
char HEADER[256];
int PLOTSTUFF;
int MAGLIMS;
int NMAX_STARS;
char PhotCodeFile[256];

int    FORCE;
double F_RA;
double F_DEC;

char GSCFILE[256], GSCDIR[256], LONEOS_REGION_FILE[256];
double ASEC_PIX;
char ROUGH_ASTROMETRY[64];

/* simple structure to carry around data on an array of stars */
typedef struct {
  double X;
  double Y;
  double mag;
} SStars;

typedef struct {
  Coords coords;
  float *X, *Y;
  int *N;
  double RA[2], DEC[2];
  double Area, density, spacing;
} CatStats;

typedef struct {
  double R, D;
  double r, b;
} USNOdata;

typedef struct {
  int *match;
  float *X, *Y;
  int *N;
} USNOstats;

/*  this seems to be a problem: is not included from math.h with the -ansi flag */
extern double hypot PROTO((double, double));

SStars   *getptolemy          PROTO((CatStats *catstats, int *NSTARS));
SStars   *getgsc              PROTO((CatStats *catstats, int *NSTARS));
USNOdata *getusno	      PROTO((USNOstats *usnostats, CatStats *catstats, int *Nusno));
SStars   *gstars	      PROTO((char *file, int *NSTARS, Coords *coords, int *NX, int *NY, double *dNdM));
void 	  ConfigInit	      PROTO((int *argc, char **argv));
void 	  DonePlotting	      PROTO((Graphdata *graphmode, int N));
void 	  PlotReset	      PROTO((int N));
void 	  PlotVector	      PROTO((int Npts, float *vect, int mode, int N));
void 	  PrepPlotting	      PROTO((int Npts, Graphdata *graphmode, int N));
void 	  XDead		      PROTO((int value));
void   	  alter_header        PROTO((char *, char **, int, double, double, double, double, double, double, double, double, int));
void 	  area_of_region      PROTO((CatStats *region));
void 	  define_region	      PROTO((CatStats *catstats, Coords *coords, int NX, int NY));
void   	  find_shift          PROTO((SStars *, SStars *, int, int, double, double, int *, double *, double *, double *));
void 	  gargs		      PROTO((int *argc, char **argv, Coords *coords));
int 	  gaussj	      PROTO((double **a, int n, double **b, int m));
int 	  gcenter	      PROTO((SStars *stars1in, SStars *stars2, int N1, int N2, Coords *coords, int NX, int NY, double *dR));
int 	  get_region_coords   PROTO((double *ra, double *dec, int rnumber, char *side));
int 	  gfit		      PROTO((SStars *stars1, SStars *stars2, int N1, int N2, Coords *coords, int NX, int NY, double *Radius, double *DR, int *Nmatch, int mode));
void 	  gfitpoly	      PROTO((SStars *stars1, SStars *stars2, int N1, int N2, Coords *coords, double *Radius, double *DR, int *Nmatch));
void 	  gheader	      PROTO((char *file, Coords coords, double dR, int Nmatch));
void 	  gproject	      PROTO((SStars *catalog, SStars **stars, int Ncat, int *Nstars, Coords *coords, int NX, int NY, double dNdM, int N1));
void 	  granges	      PROTO((SStars *stars1, SStars *stars2, int N1, int N2, int NPIX, double *gx, double *gy, double *gx0, double *gy0));
int 	  greference	      PROTO((SStars **cat, int *Ncat, Coords *coords, int NX, int NY));
void 	  hh_hms	      PROTO((double hh, int *hr, int *mn, double *sc));
void 	  hms_format	      PROTO((char *line, double value));
int       line_fit            PROTO((SStars *, SStars *, int, int, double, double, double, double, double *, double *, double *, double *, double *, double *, double *, double *));
int 	  mk_polyterm	      PROTO((int n, int m, int norder));
int 	  mk_vector	      PROTO((int n, int m, int norder));
int 	  open_graph	      PROTO((int N));
void   	  precess             PROTO((double *, double *, double, double));
void   	  ranges              PROTO((SStars *, SStars *, int, int, double *, double *, double *, double *, double, double));
void 	  rotate	      PROTO((SStars *stars, int Nstars, double angle, int Xo, int Yo));
void 	  sort_stars	      PROTO((SStars *stars, int N));
void      sort_lists          PROTO((double *X, double *Y, int *S, int N));
void   	  stats               PROTO((char *, double *, double *, double *, double *, double));
