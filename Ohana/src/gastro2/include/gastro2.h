# include <ohana.h>
# include <dvo.h>
# include <kapa.h>

typedef struct {
  double R, D;
  double P, Q;
  double X, Y;
  double M, dM;
  int type;
} StarData;

typedef struct {
  double dNdM;
  double Mo;
  double Mmin;
  double Mmax;
  double Mz;
} LumStats;

typedef struct {
  double angle;
  double Xoff;
  double Yoff;
  double Chi;
  double dR;
  int    N;
} Answer;

typedef struct {
  Header header;   /* cmp file header */
  LumStats lum;
  Coords coords;   /* current best guess for astrometry */
  Answer answer;

  double Area;
  StarData *stars; /* array with all star data */
  int N;           /* number of stars */
} CmpCatalog;

typedef struct {
  LumStats lum;

  double Area;
  double Moff;
  double R0, R1;
  double D0, D1;
  int N;           /* number of stars */
  
  StarData *stars; /* array with all star data */
} RefCatalog;

typedef struct {
  double RA[2], DEC[2];
  double Area;
  char *name;
} CatStats;

typedef struct {
  double R, D;
  double r, b;
} USNOdata;

/* global variables, from ConfigInit or args */
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
char CATDIR[256];
char CATMODE[16];    /* raw, mef, split, mysql */
char CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char REFCAT[256];
char HEADER[256];
int PLOTSTUFF;
int MAGLIMS;
int NMAX_STARS;
char PhotCodeFile[256];
int GASTRO_MAX_NSTARS;
int TEXTMODE;
int PTOLEMY_FILL_FACTOR;
int MAGMANUAL;
double MAGLIM_MIN;
double MAGLIM_MAX;
int NGRID_PIX;

int    ASCA;
int    FORCE;
double F_RA;
double F_DEC;

double ASEC_PIX;
char ROUGH_ASTROMETRY[64];

/* locations for reference data */
char GSCFILE[256];
char GSCDIR[256];
char CATDIR[256];
char USNO_A_DIR[256];
char USNO_B_DIR[256];
char TWO_MASS_DIR[256];
char ASTROM_CATDIR[256];
char LONEOS_REGION_FILE[256];

StarData *rtext (FILE *f, int *nstars);
StarData *rfits (FILE *f, int *nstars);

StarData *remove_clumps (StarData *instars, int *nstars, int NX, int NY);
void 	  ConfigInit (int *argc, char **argv);
void 	  DonePlotting (Graphdata *graphmode, int N);
void 	  PlotReset (int N);
void 	  PlotVector (int Npts, float *vect, int mode, int N);
void 	  PrepPlotting (int Npts, Graphdata *graphmode, int N);
void 	  XDead (int value);
void 	  add_to_regions (CatStats *area);
void 	  ahelp (void);
void 	  area_of_region (CatStats *region);
double    area_of_skyregion (SkyRegion *region);
void 	  args (int *argc, char **argv, Coords *coords);
void 	  define_region (CatStats *catstats, CmpCatalog *Target);
int 	  dms_to_ddd (double *Value, char *string);
void 	  dump_coords (CmpCatalog *Target);
void 	  fill_lumfunc (StarData *stars, int N, float *lbin, float *bin, int *nb);
int 	  find_dec_bands (CatStats *area);
void 	  fit_add (double x1, double y1, double x2, double y2, double wt);
int 	  fit_adjust (Coords *coords);
void 	  fit_apply (double *x, double *y, double X, double Y);
void 	  fit_eval (void);
void 	  fit_init (int order);
void 	  fit_lum_bin (double *x, double *y, int N, double *C0, double *C1);
void 	  fit_norm (void); 
double    fit_scat (StarData *st, StarData *sr, Coords *coords);
int 	  gaussj (double **a, int n, double **b, int m);
void 	  gcenter (CmpCatalog *Target, RefCatalog *Ref);
int 	  get_luminosity_func (StarData *stars, int N, LumStats *lum);
int 	  getptolemy (CatStats *catstats, RefCatalog *Ref);
int 	  getusno (CatStats *catstats, RefCatalog *Ref);
int 	  getusnob (CatStats *catstats, RefCatalog *Ref, double epoch);
int       getgsc (CatStats *catstats, RefCatalog *Ref);

void 	  gfit (CmpCatalog *Target, RefCatalog *Ref, int order);
void 	  gheader (char *file, CmpCatalog *Target);
void 	  gproject (CmpCatalog *Target, RefCatalog *Ref, RefCatalog *Subset);
void 	  greference (CmpCatalog *Target, RefCatalog *Ref);
void 	  grid (CmpCatalog *Target, RefCatalog *Subset, Answer *answer);
int 	  gridbin (double dX, double dY);
void 	  gridfree (void);
void 	  gridinit (double XMIN, double XMAX, double YMIN, double YMAX, int Nr, int Nt);
void 	  gstars (char *filename, CmpCatalog *Target);
void 	  hh_hms (double hh, int *hr, int *mn, double *sc);
void 	  hms_format (char *line, double value);
void 	  init_regions (void);
int 	  load_ra_blocks (int Ndec, CatStats *area);
int 	  mk_polyterm (int n, int m, int norder);
int 	  mk_vector (int n, int m, int norder);
int 	  open_graph (int N);

void 	  pair_add (int i1, int i2);
void 	  pair_init (void);
int       pair_lists (int **index1, int **index2);

int 	  parse_GSC_line (CatStats *tregion, char *line);
int 	  plot_addpt_gridplot (double x, double y);
void 	  plot_done_gridplot (void);
void 	  plot_fullfield (CmpCatalog *Target, RefCatalog *Ref);
void 	  plot_fullfield_pairs (float *x, float *y, int n);
void 	  plot_gridpts (double *pts, int Npts);
void 	  plot_init_gridplot (void);
void 	  plot_lumfunc (CmpCatalog *Target, RefCatalog *Ref);
void 	  plot_resid (StarData *st, StarData *sr, Coords *coords);
void 	  plot_resid_init (int version, double xmax);
void 	  plot_resid_plot (int version, float *xvect, float *yvect, int Nvect);

void 	  rotate (RefCatalog *Subset, RefCatalog *Ref, double angle);
void 	  set_catalog (char *catdir);
void 	  sort (double *X, int N);
void 	  sort_lists (double *X, double *Y, int *S, int N);
void 	  sort_lum (double *R, double *X, double *Y, int N);
void 	  sort_stars_X (StarData *stars, int N);
void 	  sort_stars_mag (StarData *stars, int N);
int 	  str_to_radec (double *ra, double *dec, char *str1, char *str2);
