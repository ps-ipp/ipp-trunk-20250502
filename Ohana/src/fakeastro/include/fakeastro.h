# include <ohana.h>
# include <dvo.h>
# include <kapa.h>
# include <signal.h>
# include <assert.h>
# include <pthread.h>

# define RESETTIME { gettimeofday (&startTimer, (void *) NULL); }

typedef enum {OP_NONE, OP_GALAXY, OP_IMAGES, OP_2MASS, OP_GAIA} FakeastroOp;

typedef struct {
  int   NfakeImage;
  int   NFAKEIMAGE;
  Image *fakeImage;

  int   NtrueImage;
  int   NTRUEIMAGE;
  Image *trueImage;
} ImageInfo;

typedef struct {
  double R, D;
  StarPar starpar;
  int flag; // in a subset?
  int found; // assigned to an object?
} FakeAstro_Stars;

typedef struct {
  double Rref; // "reference" coordinate of the fake stars
  double Dref;
  Average  average;
  Measure  measure;
  StarPar  starpar;
  int found;
} Stars;

// structure to hold coordinate fitting terms
typedef struct {
    int Npts;
    int Nterms;
    int Norder;
    int Nsums;
    int Nelems;
    double **sum;
    double **xsum;
    double **ysum;
    double **xfit;
    double **yfit;
} CoordFit;

/* used in find_matches, find_matches_refstars */
# define IN_REGION(R,D) ( \
((D) >= region[0].Dmin) && ((D) < region[0].Dmax) && \
((R) >= region[0].Rmin) && ((R) < region[0].Rmax))

# define myAbortF(FORMAT,...) { fprintf (stderr, FORMAT, __VA_ARGS__); abort(); }

/**** global variables set in parameter file ***/

# define DVO_MAX_PATH 1024

char   GSCFILE[DVO_MAX_PATH];
char   CATDIR[DVO_MAX_PATH];
char   SKY_TABLE[DVO_MAX_PATH];
char   MasterPhotcodeFile[DVO_MAX_PATH];

char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
int    SKY_DEPTH;      /* depth of catalog tables, fix usage */

int    HOST_ID;
char  *HOSTDIR;
char  *CPT_FILE;
char  *INPUT;

int    IMAGE_ID;

char  *IMAGES_INPUT;
char  *CATDIR_INPUT;
char  *CATDIR_OUTPUT;

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

int    VERBOSE;
int    VERBOSE2;
int    ONE_BIG_CHIP;

float  TEST_SCALE;
char  *GALAXY_MODEL;

int    FORCE;
int    UNIFORM_RADEC;

int    FAKEASTRO_NLOOP;
int    FAKEASTRO_NSTARS;
int    FAKEASTRO_NQSO_ICRF;
int    FAKEASTRO_NQSO_ZERO;
float  FAKEASTRO_ZGAL; // parsec
float  FAKEASTRO_RGAL; // parsec
char   FAKEASTRO_REF_EPOCH[80];
char   FAKEASTRO_2MASS_EPOCH[80];
char   FAKEASTRO_GAIA_EPOCH[80];

float  RADIUS;
float  MAX_MAG_2MASS;
float  MAX_MAG_GAIA;

SkyRegion UserPatch;

FakeastroOp FAKEASTRO_OP;

/*** fakeastro prototypes ***/

void          ConfigInit          PROTO((int *argc, char **argv));
void          GetConfig           PROTO((char *config, char *field, char *format, int N, void *ptr));
int           args                PROTO((int *argc, char **argv));
int           args_client         PROTO((int *argc, char **argv));

void          set_db              PROTO((FITS_DB *in));
int           Shutdown            PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);
void          TrapSignal          PROTO((int sig));
void          SetProtect          PROTO((int mode));
int           SetSignals          PROTO((void));

void initialize (int argc, char **argv);
void initialize_client (int argc, char **argv);

int fakeastro_galaxy ();
FakeAstro_Stars *make_fakestars (int Nstars);
int sortStars (FakeAstro_Stars *stars, int Nstars);

FakeAstro_Stars *make_subset (FakeAstro_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

int save_fakestars (FakeAstro_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname);

FakeAstro_Stars *fakestar_load_stars (char *filename, int *nstars);
int fakestar_save_stars (char *filename, FakeAstro_Stars *stars, int Nstars);

int harvest_host ();
int save_remote_host (HostInfo *host);
int harvest_all ();
int find_empty_slot ();
int init_remote_hosts ();

int fakestar_catalog (FakeAstro_Stars *stars, int Nstars, SkyRegion *region, char *filename);
int insert_fakestar (SkyRegion *region, FakeAstro_Stars *stars, int Nstars, Catalog *catalog);

double gaussian (double x, double mean, double sigma);
void gauss_init (int Nbin);
double rnd_gauss (double mean, double sigma);
double int_gauss (int i);

int fakeastro_images ();
Image *load_template_images (int *nimage);
Image *make_fake_images (Image *image, int *nfakeImage);

int fakeastro_images_region (ImageInfo *imageInfo, Image *refImage, int NrefImage, SkyTable *skyTableInput, SkyTable *skyTableOutput, SkyRegion *innerRegion);

SkyRegion *get_image_patch (Image *image);
SkyRegion *get_mosaic_patch (Image *image);

int SkyRegionsOverlap (SkyRegion *region, SkyRegion *patch);
int SkyRegionHasPoint (SkyRegion *region, double R, double D);
SkyRegion *SkyRegionExpand (SkyRegion *region, float boundary);

Catalog *load_fake_stars (SkyList *skylist, int *ncatalog);

Stars *make_fake_stars (Catalog *catalog, int Ncatalog, SkyList *skylist, Image *image, Stars *stars, int *nstars);
Stars *make_fake_stars_catalog (Stars *stars, int *nstars, SkyRegion *patch, Catalog *catalog, Image *image);

int save_fake_stars (SkyTable *sky, Image *image, Stars *stars, int Nstars);
int match_fake_stars (Stars *stars, unsigned int NstarsIn, SkyRegion *region, Catalog *catalog);

int InitStar (Stars *star);

float airmass (float secz_image, double ra, double dec, double st, double latitude);
float azimuth (double ha, double dec, double latitude);

int fit_fake_stars (Stars *stars, int Nstars, Image *image);

double **array_init (int Nx, int Ny);
void array_free (double **array, int Nx);
CoordFit *fit_init (int order);
void fit_free (CoordFit *fit);
void fit_add (CoordFit *fit, double x1, double y1, double x2, double y2, double wt);
int fit_eval (CoordFit *fit);
void fit_apply (CoordFit *fit, double *x2, double *y2, double x1, double y1);
double **poly2d_dx (double **poly, int Nx, int Ny);
double **poly2d_dy (double **poly, int Nx, int Ny);
double **poly2d_copy (double **poly, int Nx, int Ny);
double poly2d_eval (double **poly, int Nx, int Ny, double x, double y);
int fit_apply_coords (CoordFit *fit, Coords *coords, int keepRef);

int save_astrom_table ();
AstromOffsetTable *get_astrom_table ();

FakeAstro_Stars *make_fakeqsos (int Nstars, int ICRF);

int fakeastro_2mass ();
int make_2mass_measures (Catalog *catalog);

int fakeastro_gaia ();
int make_gaia_measures (Catalog *catalog);
