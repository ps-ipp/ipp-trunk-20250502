# include <ohana.h>
# include <dvo.h>
# include <signal.h>

typedef struct {
  int measure;          // pointer to measure entry
  unsigned int averef;  // old measures come from multiple averages
  float R, D;           // actual coordinates of this measure
} Mpointer;

typedef struct {
  double median;
  double mean;
  double sigma;
  double error;
  double chisq;
  double min;
  double max;
  double total;
  int    Nmeas;
} StatType;

typedef struct {
  double Xc[5];
  double Yc[5];
  double Rc;
  double Dc;
} SkyRegionCoords;

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;
char  *PARALLEL_OUTHOSTS;

int          HOST_ID;
char        *HOSTDIR;
char        *HOSTDIR_OUTPUT;

// need to get RADIUS from Config 

# define DVO_MAX_PATH 1024

/* global variables */
int    SKIP_IMAGES;
int    ONLY_IMAGES;

int    SKIP_MEASURE;
int    SKIP_MISSING;
int    SKIP_LENSING;
int    SKIP_LENSOBJ;
int    SKIP_STARPAR;
int    SKIP_GALPHOT;

int    SHOW_PARAMS;
int    VERBOSE;
double JOIN_RADIUS;
double UNIQ_RADIUS;
double DMCAL_MIN;
char   ImageCat[DVO_MAX_PATH];
char   GSCFILE[DVO_MAX_PATH];
char   CATDIR[DVO_MAX_PATH];
char  *CATMODE;    /* raw, mef, split, mysql */
char  *CATFORMAT;  /* internal, elixir, loneos, panstarrs */
char  *CATCOMPRESS;  /* ?? */
char   PhotCodeFile[DVO_MAX_PATH];

double RMIN;
double RMAX;
double DMIN;
double DMAX;
 
double XMIN;
double XMAX;
double YMIN;
double YMAX;
double MMIN;
double MMAX;

double DMSYS;
double DMGAIN;
double CHISQ_MAX;
double SIGMA_MIN_KEEP;
double SIGMA_MAX;
double AVE_SIGMA_LIM;
int    NMEAS_MIN;
int    NMEAS_MIN_FILTERED;
int    NCODE_MIN;
double ZERO_POINT;

int ExcludeByMinSigma;

int ExcludeByInstMag;
double INST_MAG_MIN;
double INST_MAG_MAX;

int ExcludeByMaxMinMag;
double MAX_MIN_MAG;

SkyRegion REGION;
PhotCodeData photcodes;

char          *PHOTCODE_DROP_LIST, *PHOTCODE_KEEP_LIST;
int           NphotcodesDrop,      NphotcodesKeep;
PhotCode     **photcodesDrop,     **photcodesKeep;

# define FLAG_AREA            0X0001
# define FLAG_MINST           0X0002
# define FLAG_DMCAL           0x0004
# define FLAG_SIGCLIP         0X0008
# define FLAG_CHISQ           0x0010
# define FLAG_TOOFEW          0x0020
# define FLAG_DUPMEAS         0x0040

Image        *find_images (FITS_DB *db, GSCRegion *region, int Nregion, int *Nimage, int **LineNum);
GSCRegion    *find_regions (Image *image, int Nimage, int *Nregions, GSCRegion *fullregion); 
GSCRegion    *get_regions (double minRa, double maxRa, double minDec, double maxDec, int *Nregions); 
Catalog      *load_catalogs (GSCRegion *region, int Nregion); 
GSCRegion    *load_images (FITS_DB *db, char *seed, int *nregion);
GSCRegion    *name_region (char *name, int *Nregions); 
void          ConfigInit (int *argc, char **argv); 
int           args (int argc, char **argv); 
int           corner_check (double *x1, double *y1, double *x2, double *y2); 
int           edge_check (double *x1, double *y1, double *x2, double *y2); 
void          flag_measures (FITS_DB *db, Catalog *catalog); 
int           gcatalog (Catalog *catalog); 
void          get_mags (Catalog *catalog); 
void          getfullregion (Image *image, int Nimage, GSCRegion *fullregion); 
void          initialize (int argc, char **argv); 
void          initstats (char *mode); 
void          join_stars (Catalog *catalog); 
int           liststats (double *value, double *dvalue, int N, StatType *stats); 
double        opening_angle (double x1, double y1, double x2, double y2, double x3, double y3); 
void          set_ZP (double ZERO); 
void          sortA (double *X, int N); 
void          sortB (double *X, double *Y, int N); 
void          sortC (double *X, double *Y, double *F1, double *F2, int N); 
void          sortD (double *X, double *Y, double *Z, int N); 
void          sort_lists (float *X, float *Y, int *index, int N); 
void          sort_time (unsigned int *X, int *Y, int N); 
void          unique_measures (Catalog *catalog);
void          wcatalog (Catalog *catalog);
int           make_subcatalog (Catalog *subcatalog, Catalog *catalog, SkyRegion *region);
void          check_directory (char *basefile);

int Shutdown (char *format, ...) OHANA_FORMAT(printf, 1, 2);
void set_db (FITS_DB *in);
void lock_image_db (FITS_DB *db, char *filename);
void unlock_image_db (FITS_DB *db);
void check_permissions (char *basefile);
void TrapSignal (int sig);
void SetProtect (int mode);
int SetSignals (void);
int copy_images (char *outdir, SkyList *skylist);
void usage();

void dsortindex (double *X, off_t *Y, int N);
off_t getRegionStartByRA (double R, double *Rref, off_t Nregions);

Image *select_images (SkyList *skylist, Image *timage, off_t Ntimage, off_t **LineNumber, off_t *Nimage, int UseFullOverlap);

int photdbc_catalogs (char *outroot, SkyList *skylist, int hostID);
int photdbc_parallel (char *outroot, SkyList *skylist);
int args_client (int argc, char **argv);
void initialize_client (int argc, char **argv);
