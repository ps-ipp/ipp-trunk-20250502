# include <ohana.h>
# include <dvo.h>

# define PSASTRO_MODE 0

typedef struct {
  double R, D;  /* Sky Coords    - degrees */
  double P, Q;  /* Tangent Plane - pixels  */
  double L, M;  /* Focal Plane   - pixels  */
  double X, Y;  /* Chip Coords   - pixels  */
  double Mag, dMag;
  int mask;
} StarData;

typedef struct {
  double RA[2], DEC[2];
  double Area;
  char *name;
} CatStats;

typedef struct {
  char *file;
  int Nstars;
  int NX, NY;
  StarData *stars;
  Header header;
  Header theader;
  Matrix matrix;

  char *buffer;
  int Nbuffer;
  int FITS;

  int Nmatch;
  StarData *raw, *ref;

  Coords coords;
  Coords map;
} Chip;

typedef struct {
  double Ro, Do, PSx, PSy, To;
  double Rmin, Rmax, Dmin, Dmax;
  int    Norder;
  double **A;
  double **D;
  Coords project;
  Coords distort;
  int    fit;
} Field;

typedef struct {
  double *dPdL;
  double *dPdM;
  double *dQdL;
  double *dQdM;
  double *Lo;
  double *Mo;
  int Npts;
} Gradients;

typedef struct {
  double Rraw, Draw; /* Sky Coords    - degrees */
  float Praw, Qraw;  /* Tangent Plane - pixels  */
  float Lraw, Mraw;  /* Focal Plane   - pixels  */
  float Xraw, Yraw;  /* Chip Coords   - pixels  */

  double Rref, Dref; /* Sky Coords    - degrees */
  float Pref, Qref;  /* Tangent Plane - pixels  */
  float Lref, Mref;  /* Focal Plane   - pixels  */
  float Xref, Yref;  /* Chip Coords   - pixels  */

  float Mcat, dMcat;
  float Minst, dMinst;
  char mask;
} MatchData;

int  ChipOrder;
int  Nchip;
Chip *chip;
Field field;
double Year;  /** carried for precession - probably put this in chip data **/
double RADIUS; /** raw / ref matching radius (pixels on Focal Plane) **/
double SIGMA;
double SIGMA_LIM;
double IMAG_MIN;
double IMAG_MAX;
double INST_BRIGHT;
double ZERO_POINT;

char REFCAT[256];
char CATMODE[16];    /* raw, mef, split, mysql */
char CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char ExptimeKeyword[256];
char DateKeyword[256];
char DateMode[256];
char UTKeyword[256];
char MJDKeyword[256];
char JDKeyword[256];
int VERBOSE;
int NO_CHIPS;
int SAVE_RESID;

char CATDIR[256];
char GSCFILE[256];
char GSCDIR[256];
char USNO_A_DIR[256];
char USNO_B_DIR[256];
char TWO_MASS_DIR[256];
char ASTROM_CATDIR[256];
char StoneRegions[256];

char *FIELD;
char *CHIPS;
char *OUTPUT;
char *DUMP;
char *FOCAL_PLANE;

/*** mosastro prototypes ***/
void       ChipToFP           PROTO((StarData *stars, int Nstars, Coords *coords));
void       ChipToSky          PROTO((StarData *stars, int Nstars, Coords *coords));
int        ClipOnFP           PROTO((double Nsigma));
void       ConfigInit         PROTO((int *argc, char **argv));
int        ConvertMatch       PROTO((MatchData *data, int size, int nitems));
void       FPtoChip           PROTO((StarData *stars, int Nstars, Coords *coords));
void       FPtoTP             PROTO((StarData *stars, int Nstars, Coords *coords));
void       FitChip            PROTO((StarData *raw, StarData *ref, int Nmatch, Coords *coords));
void       FitChipLinear      PROTO((StarData *raw, StarData *ref, int Nmatch, Coords *coords));
void       FitChipResid       PROTO((StarData *raw, StarData *ref, int Nmatch, Coords *coords));
void       FitChips           PROTO((int Norder));
void       FitGradients       PROTO((Gradients *grad));
void       GetConfig          PROTO((char *config, char *field, char *format, int N, void *ptr));
Gradients *GetGradients       PROTO((void));
double     GetScatter         PROTO((int *Nscatter, double *DL, double *DM, int bright));
int        LoadStars          PROTO((int Nfile, char **file));
void       SaveResiduals      PROTO((FILE *f, Header *header));
void       SkyToTP            PROTO((StarData *stars, int Nstars, Coords *coords));
void       TPtoFP             PROTO((StarData *stars, int Nstars, Coords *coords));
void       TPtoSky            PROTO((StarData *stars, int Nstars, Coords *coords));
void       add_to_regions     PROTO((CatStats *area));
void       area_of_region     PROTO((CatStats *region));
void       args               PROTO((int *argc, char **argv));
int        deproject_raw      PROTO((void));
int        deproject_stars    PROTO((void));
int        dump_grads         PROTO((Gradients *grad, char *filename));
int        dump_match         PROTO((void));
int        dump_rawstars      PROTO((void));
int        dump_refcat        PROTO((StarData *refcat, int Nrefcat));
int        dump_stars         PROTO((FILE *f, StarData *stars, int Nstars));
int        fake_field         PROTO((double RA, double DEC));
void       field_combine      PROTO((void));
void       field_stats        PROTO((void));
int        find_dec_bands     PROTO((CatStats *area));
void       fit_add            PROTO((double x1, double y1, double x2, double y2));
void       fit_apply_coords   PROTO((Coords *coords));
void       fit_apply_grads    PROTO((Coords *distort, Coords *project, int term));
void       fit_correct_grads  PROTO((Gradients *in, Gradients *out, int term));
void       fit_eval           PROTO((void));
void       fit_free           PROTO((void));
void       fit_init           PROTO((int order));
int        gaussj             PROTO((double **a, int n, double **b, int m));
StarData  *gcatalog           PROTO((char *filename, int *Nstars));
StarData  *get2mass           PROTO((CatStats *catstats, int *NSTARS));
StarData  *getgsc             PROTO((CatStats *catstats, int *NSTARS));
StarData  *getptolemy         PROTO((CatStats *catstats, int *NSTARS));
StarData  *getstone           PROTO((CatStats *input, int *nstars));
StarData  *getusno            PROTO((CatStats *catstats, int *Nstars));
StarData  *getusnob           PROTO((CatStats *catstats, int *Nstars));
StarData  *gptolemy           PROTO((char *filename, int *NSTARS));
StarData  *greference         PROTO((int *Nrefcat));
CatStats  *gregions           PROTO((CatStats *patch, int *nregion));
int        init_chips         PROTO((void));
int        init_field         PROTO((void));
void       init_regions       PROTO((void));
int        load_chips         PROTO((char *filename));
int        load_field         PROTO((char *filename));
int        load_ra_blocks     PROTO((int Ndec, CatStats *area));
int        match              PROTO((StarData *refcat, int Nrefcat));
Header    *mkheader           PROTO((int Nx, int Ny, int Nstars, Coords *coords));
Header    *mkmosaic           PROTO((int Nx, int Ny, int Nstars, Coords *coords));
int        mkpolyterm         PROTO((int n, int m));
int        mkvector           PROTO((int n, int m, int norder));
void       output             PROTO((char *ext, char *phu));
int        parse_GSC_line     PROTO((CatStats *tregion, char *line));
e_time     parse_time         PROTO((Header *header));
void       print_help         PROTO((void));
int        project_ref        PROTO((void));
int        project_refcat     PROTO((StarData *refcat, int Nrefcat));
int        project_stars      PROTO((void));
int        rfits              PROTO((Chip *mychip));
int        rtext              PROTO((Chip *mychip));
void       set_catalog        PROTO((char *catdir));
int        sortthree          PROTO((double *X, double *Y, int *Z, int N));
void       uppercase          PROTO((char *string));
void       wchip              PROTO((char *filename, Chip *data));
void       wfits              PROTO((char *filename, SMPData *stars, int Nstars, Header *header));
void       wstars             PROTO((char *filename, SMPData *stars, int Nstars, Header *header));
