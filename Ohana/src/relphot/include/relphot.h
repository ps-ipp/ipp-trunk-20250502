# include <ohana.h>
# include <dvo.h>
# include <kapa.h>
# include <signal.h>
# include <pthread.h>

# define ID_SECF_STACK_PRIMARY 0x00004000

# define NBOOTSTRAP 100

// MEAS_BAD is used in ImageOps.c, MosaicOps.c, TGroupOps.c to skip specific measurements.
// ID_MEAS_AREA is raised for measurements outside the user-specified region of the chips
// ID_MEAS_NOCAL is raised for measurements outside the valid time range or for inactive photcodes
// Note: these bits are only raised on the temporary MeasureTiny elements and are not saved
// to the database files.
# define MEAS_BAD (ID_MEAS_AREA | ID_MEAS_NOCAL)

# ifndef MAX_INT
# define MAX_INT 2147483647
# endif

// choose off_t or int depending on full-scale relphot analysis resources
// # define IDX_T off_t
# define IDX_T int 

typedef struct {
  int minID;
  int maxID;
  int *index;
  int Nindex;
  int NINDEX;
} myIndexType;

typedef enum {
  ZPT_STARS,
  ZPT_TGROUP,
  ZPT_MOSAIC,
  ZPT_IMAGES,
} ZptFitModeType;

typedef enum {
  STAGE_CHIP  = 0x01,
  STAGE_WARP  = 0x02,
  STAGE_STACK = 0x04,
} RelphotStages;

typedef enum {
  MODE_ERROR = 0,
  UPDATE_IMAGES,
  UPDATE_AVERAGES,
  PARALLEL_REGIONS,
  PARALLEL_IMAGES,
  APPLY_OFFSETS,
  SYNTH_PHOT,
} RelphotMode;

typedef enum {
    MODE_NONE           = 0,
    MODE_LOAD           = 1,
    MODE_UPDATE         = 2,
    MODE_UPDATE_OBJECTS = 3,
    MODE_SYNTH_PHOT     = 4,
} ModeType;

// NOTE: this is only used in special cases where we limit photcode ranges
typedef enum {
  PS1_none, 
  PS1_g = 1, 
  PS1_r = 2, 
  PS1_i = 3, 
  PS1_z = 4, 
  PS1_y = 5,
  PS1_w = 6,
} PS1filters;

typedef enum {
  GRID_MEAN = 0,
  GRID_STDEV = 1,
  GRID_NPTS = 2,
} GridOutputMode;

typedef struct {
  Header PHU;
  Header header[5]; // grizy (matches Nsec in photcodes)
  Matrix matrix[5];
  Coords coords;
  int Nx;
  int Ny;
} SynthZeroPoints;

typedef struct {
  unsigned int start;
  unsigned int stop;
  short photcode;
  float McalPSF;
  float McalAPER;
  float dMcal;
  float dMsys;
  float stdev;
  float dMmin;
  float dMmax; 
  float McalChiSq;
  float secz;
  float ubercalDist;
  unsigned int nFitPhotom;
  unsigned int nValPhotom;
  unsigned int flags;
  char skipCal;		      // if TRUE, this mosaic is incomplete and should not be calibrated
  char inTGroup;
  Coords coords;	      // this is only used to set the mosaic center for assign_images used by region hosts
} Mosaic; 

typedef struct {
  short photcode;
  float McalPSF;
  float McalAPER;
  float dKlam;
  float dMcal;
  float dMsys;
  float stdev;
  float dMmin;
  float dMmax; 
  float McalChiSq;
  unsigned int nFitPhotom;
  unsigned int nValPhotom;
  unsigned int flags;

  off_t Nimage;
  off_t NIMAGE;
  off_t *image;

  off_t Nmeasure;
  off_t NMEASURE;
  off_t *measure;
  off_t *catalog;

  off_t Nmosaic;
  off_t NMOSAIC;
  Mosaic **mosaic; // pointer to the mosaic structures

  void *parent;
} TGroup; 

// we have an array of TGroup times, each pointing to N sets of data values
typedef struct {
  unsigned int start;
  unsigned int stop;
  TGroup *byCode; // each of these contains the collection of images for the time and photcode
  int      nCode;
} TGTimes;

typedef struct {
  unsigned short photcode;
  float **Mgrid; // grid of average corrections
  float **dMgrid; // grid of stdev of corrections
    int **nMgrid; // grid of number of stars for corrections
  int Nx; // bin = int(Xchip * (Nx / NxChip))
  int Ny; // bin = int(Ychip * (Ny / NyChip))
  float dX; // bin = int(Xchip * dX), dX = Nx / NxChip
  float dY; // bin = int(Ychip * dY), dY = Ny / NyChip
  // NxChip, NyChip = 4900,4900 for now

    int **nAlloc; // allocated vector length
  double ***dMval; // values used to calculate corrections
} GridCorrectionType;

typedef enum {
  // these modes are primary and mutually exclusive:
  STATS_NONE               = 0x0000, 
  STATS_MEAN               = 0x0001, 
  STATS_MEDIAN             = 0x0002, 
  STATS_WT_MEAN            = 0x0003, 
  STATS_INNER_MEAN         = 0x0004, 
  STATS_INNER_WTMEAN       = 0x0005, 
  STATS_CHI_INNER_MEAN     = 0x0006, 
  STATS_CHI_INNER_WTMEAN   = 0x0007, 
  // these modes are additional options
  STATS_PRIMARY            = 0x0007, // use this to mask out the optional bits
  STATS_VARSTATS           = 0x0010, 
} ListStatsMode;

typedef struct {
  double median;
  double mean;
  double sigma;
  double error;
  double chisq;
  double min;
  double max;
  double Upper90;
  double Upper80;
  double Lower20;
  double Lower10;
  double Upper90Nsig;
  double Upper80Nsig;
  double Lower20Nsig;
  double Lower10Nsig;
  double total;
  int    Nmeas;
  ListStatsMode statmode;
} StatType;

typedef struct {
  double *flxlist;           // list of measure.mag values for a given star
  double *errlist;	      // mag errors for a star
  double *wgtlist;	      // weights to use for mean mags
  int    *ranking;	      // weights to use for mean mags
  int    *measSeq;	      // weights to use for mean mags
  int    *msklist;	      // mask modifications
  int     Nlist;

  double *values;
  double *wtvals;
  double *wtlist;
  double *ykeep;
  double *dykeep;
  double *wtkeep;
  double *ysample;
  double *dysample;
  double *wtsample;
  double *bvalue;

} StatDataSet;

typedef struct {
  double *xVector;	      // complete list of values in independent variable (optional)
  double *yVector;	      // complete list of values available
  double *dyVector;	      // complete list of errors available
} FitDataType;
  
// this structure carries the data and pre-allocated arrays for
// 1D fitting with arbitrary order for data with external weights,
// optional priors, irls iterations, and bootstrap resampling
typedef struct {
  int     order;	      // order of fit (e.g., y = C0 + C1*x has order = 1)
  int     nterm;	      // number of fitted parameters (order + 1)
  int     mterm;	      // number of summations needed for OLS (2*order + 1)

  int     Nlist;	      // total number of measurements in list (may be more than the number to use)
  int     Nbootstrap;	      // number of bootstrap iterations
  int     MaxIterations;      // number of IRLS iterations

  // input data & parameters: 
  FitDataType *alldata;	      // full data set

  double *bPriorValue; 	      // prior value
  double *bPriorSigma;	      // prior sigma

  // internal / temporary arrays:
  FitDataType *keepdata;      // unclipped data for bootstrap analysis
  FitDataType *sample;	      // sample for a bootstrap iteration

  double *yOffVector;	      // difference between yFit and yObs

  double *tmpVector;	      // internal copy of values to use in, e.g., calculation of median
  double *wtIRLS; 	      // IRLS weights (distance from model value weighted by standard error)

  double **cArray;
  double  *sumVector;	      // pre-allocated to mterm (2*order + 1)

  double **bArray;
  double **bSaveArray;
  double **bBootArray;	      // values generated by the bootstrap resampling 

  // output results:
  double *bSigma;	      // 1 sigma range of parameters

  double  min;		      // min of bSigma[0] values
  double  max;		      // max of bSigma[0] values
  double  sigma;	      // (sample) standard deviation of bSigma[0] values
  double  chisq;	      // chisq of fit (unmasked values only)
  int     Nmeas;	      // number of unmasked values used in fit
} FitDataSet;

typedef struct {
  int Nfew;
  int Ncode;
  int Nsys;
  int Nbad;
  int Ncal;
  int Nmos;
  int Ngrp;
  int Ngrid;

  // NOTE: the following arrays are (possibly) pre-allocated and carried down to each
  // thread.  The psfData are used in all relphot analyses; the others are only used on
  // the final output steps.

  int Nsecfilt;

  StatDataSet  *psfData; // one is allocated for each primary (average) photcode
  StatDataSet *aperData;
  StatDataSet *kronData;

  double *psfqf_list;	      // psfqf for all filters
  double *psfqfperf_list;     // psfqfperf for all filters
  double *stargal_list;	      // stargal for all filters
  uint32_t *photflag_list;      // photflags for all filters

  int Nstargal;
  int Nphotflags;

  int 	*havePS1;	   // this secfilt has synthetic mags
  int 	*haveSYN;	   // this secfilt has synthetic mags
  int 	*measSYN;	   // this measurement is the synthetic mag for this secfilt
  int 	*needSYN;	   // this secfilt mag should use synthetic mags
  float *minSYN;	   // minimum synthetic mag below which synthetic should be forced
  
  float *psfQfMax;	      // max psfQf value for this secfilt
  float *psfQfPerfMax;	      // max psfQfperf value for this secfilt 

  int *Nmeas;		      // count of PS1 exposure (chip) measurements for this secfilt
  int *NmeasGood;	      // count of PS1 exposure (chip) measurements for this secfilt
  int *Next;		      // count of PS1 exposure (chip) measurements for this secfilt
  int *NexpPS1;		      // count of PS1 exposure (chip) measurements for this secfilt
  int *haveUbercal;	      // does this secfilt have any ubercal data?

  int *tessID;		      // tess,proj,skycell to use for warp and diff analysis
  int *projID;
  int *skycellID;

  float *minUbercalDist;

  StatType psfstats;
  StatType apstats;
  StatType kronstats;
} SetMrelInfo;

typedef struct {
  float M;
  float dM;
  float Mchisq;
  int Nsec;
  unsigned int objID;
  unsigned int catID;
  int photcode;
} MeanMag;

typedef struct {
  float McalPSF;
  float McalAPER;
  float dMcal;
  float dMagSys;
  float McalChiSq;
  int nFitPhotom;
  int flags;
  unsigned int imageID;
  short ubercalDist;
} ImageMag;

typedef struct {
  AverageTiny *average;	      // array of (minimal) average data
  MeasureTiny *measure;	      // array of (minimal) measure data 
  SecFilt     *secfilt;	      // array of secfilt data (matched to average by Nsecfilt)
  off_t       Naverage;
  off_t       Nmeasure;
} BrightCatalog;

typedef struct {
  float McalPSF;
  float McalAPER;
  float dMcal;
  unsigned int imageID;
  unsigned int photom_map_id;
  unsigned int flags;
  int tessID;
  int projID;
  int skycellID;
  unsigned int tzero;
  unsigned char trate;
  short ubercalDist;
} ImageSubset;

typedef struct {
  Catalog *catalog;	      // array of catalogs generated
  int NCATALOG;		      // number of catalogs allocated
  int Ncatalog;		      // number of catalogs generated
  int Nsecfilt;		      // number of catalogs generated
  off_t *NAVERAGE; 	      // allocated Averages per catalog
  off_t *NMEASURE;	      // allocated Measures per catalog
  int   *index;		      // lookup table catID -> catalog[i]
  int   *catIDs;	      // lookup table catID <- catalog[i]
  int    maxID;		      // max catID value to date
} CatalogSplitter;

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char   ImageCat[DVO_MAX_PATH];
char   ImageTemplate[DVO_MAX_PATH];
char   CatTemplate[DVO_MAX_PATH];
char   GSCFILE[DVO_MAX_PATH];
char  *CATDIR;
char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char   CameraConfig[DVO_MAX_PATH];
char   CAMERA[64];    /* eg, gpc1 */
char   SKY_TABLE[DVO_MAX_PATH];
int    SKY_DEPTH;  /** XXX EAM : depth of catalog tables, fix usage */
char  *SYNTH_ZERO_POINTS;
char  *GRID_MEANFILE;

// globals for parallel region operations
char  *REGION_FILE;
char  *IMAGE_TABLE;
int    REGION_HOST_ID;

int          HOST_ID;
char        *HOSTDIR;
char        *IMAGES;
char        *BCATALOG;
char        *BOUNDARY_TREE;
ModeType     MODE;

char        *BOUNDARY_TREE;

// XXX deprecate int SET_MREL_VERSION;
int IS_DIFF_DB;

double MAG_LIM;
double SIGMA_LIM;
double IMAGE_SCATTER;
double IMAGE_OFFSET;
double NIGHT_SCATTER;
double NIGHT_CHISQ;
double MOSAIC_SCATTER;
double MOSAIC_CHISQ;
double STAR_SCATTER;
double STAR_CHISQ;
double MIN_ERROR;
double IMFIT_SYS_SIGMA_LIM;
double CLOUD_TOLERANCE;
int    VARIABILITY_STATS;
int    USE_OLS_FOR_AVERAGES;

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;
char  *MANUAL_UNIQUER;

int    SKIP_PARALLEL_GROUPS;

int    PARALLEL_REGIONS_MANUAL;

int    NTHREADS;

int    VERBOSE;
int    VERBOSE2;
int    VERBOSE_IMAGE;

int    MOSAIC_ZEROPT;
int    TGROUP_ZEROPT;
int    GRID_ZEROPT;

int    GRID_BIN_HSC;
int    GRID_BIN_CFH;
int    GRID_BIN_GPC1;
int    GRID_BIN_GPC2;

int    FREEZE_IMAGES;
int    FREEZE_MOSAICS;

int    TGROUP_FIT_AIRMASS;

int    TEST_IMAGE1;
int    TEST_IMAGE2;

int    MANUAL_ITERATION;
int    NLOOP;
int    RESET;
int    RESET_ZEROPTS;
int    RESET_FLATCORR;
int    REPAIR_WARPS;
int    PRESERVE_PS1;
int    REQUIRE_PSFFIT;
int    USE_APER_FOR_STARGAL;
int    UPDATE;
int    SAVE_IMAGE_UPDATES;
int    PLOTSTUFF;
int    SAVEPLOT;
int    SHOW_PARAMS;
char   MOSAICNAME[256];
char   STATMODE[32];
int    STAR_TOOFEW;
int    GRID_TOOFEW;
int    IMAGE_TOOFEW;
double IMAGE_GOOD_FRACTION;
int    CALIBRATE_STACKS_AND_WARPS;

int    KEEP_UBERCAL;
char  *OUTROOT;
char  *UPDATE_CATFORMAT;
int    UPDATE_XRAD;
int    PLOTDELAY;
int    UpdateAverages;
int    ApplyOffsets;
int    SyntheticPhotometry;

int    USE_MCAL_PSF_FOR_STACK_APER;

char  *PhotcodeList;

int      *photseclist;
int      Nphotcodes;
PhotCode **photcodes;
// int            PhotSec;
// int            PhotNsec;

PhotCode *refPhotcode;
int       USE_REFERENCE_WEIGHT;

int MaxDensityUse;
double MaxDensityValue;

int AreaSelect;
double AreaXmin, AreaXmax, AreaYmin, AreaYmax;

int ImagSelect;
double ImagMin, ImagMax;

int DophotSelect, DophotValue;

double  PlotMmin, PlotMmax, PlotdMmin, PlotdMmax;
enum {black, white, red, orange, yellow, green, blue, indigo, violet};

int TimeSelect;
time_t TSTART, TSTOP;

SkyRegion UserPatch;
char *UserCatalog;

enum {GRID_ZPT_MODE_NONE, GRID_ZPT_MODE_ALL, };
int GRID_ZPT_MODE;

enum {TGROUP_ZPT_MODE_NONE, TGROUP_ZPT_MODE_GOOD_NIGHT, TGROUP_ZPT_MODE_ALL, };
int TGROUP_ZPT_MODE;

enum {MOSAIC_ZPT_MODE_NONE, MOSAIC_ZPT_MODE_BAD_NIGHT, MOSAIC_ZPT_MODE_GOOD_MOSAIC, MOSAIC_ZPT_MODE_BAD_NIGHT_GOOD_MOSAIC, MOSAIC_ZPT_MODE_ALL, };
int MOSAIC_ZPT_MODE;

enum {IMAGE_ZPT_MODE_NONE, IMAGE_ZPT_MODE_BAD_NIGHT, IMAGE_ZPT_MODE_BAD_MOSAIC, IMAGE_ZPT_MODE_BAD_NIGHT_BAD_MOSAIC, IMAGE_ZPT_MODE_ALL, };
int IMAGE_ZPT_MODE;

int USE_ALL_IMAGES;
int USE_BASIC_CHECK;
int USE_FULL_OVERLAP;

RelphotStages STAGES;

/*** relphot prototypes ***/
void          ConfigInit          PROTO((int *argc, char **argv));
void          GetConfig           PROTO((char *config, char *field, char *format, int N, void *ptr));
char         *GetPhotnamebyCode   PROTO((PhotCodeData *photcodes, int code));
void          InterpolateGrid     PROTO((float *buffer, int Nx, int Ny, Coords *ccd, Coords *gcoords));
off_t        *SelectRefMosaic     PROTO((Mosaic **refmosaic, off_t *Nimage));
RelphotMode   args                PROTO((int argc, char **argv));
int           args_client         PROTO((int argc, char **argv));
int           bcatalog            PROTO((Catalog *subcatalog, Catalog *catalog, int Ncat));
void          clean_images        PROTO((void));
void          clean_measures      PROTO((Catalog *catalog, int Ncatalog, int final));
void          clean_mosaics       PROTO((void));
void          clean_tgroups       PROTO((void));
void          clean_stars         PROTO((Catalog *catalog, int Ncatalog));
int           corner_check        PROTO((double *x1, double *y1, double *x2, double *y2));
void          dumpGrid            PROTO((void));
void          dump_grid           PROTO((void)); 
int           edge_check          PROTO((double *x1, double *y1, double *x2, double *y2));
void          findImages          PROTO((Catalog *catalog, int Ncatalog, int doImageList));
int           findMosaics         PROTO((Catalog *catalog, int Ncatalog, int doMosaicList));
int           findTGroups         PROTO((Catalog *catalog, int Ncatalog));

void clearImages (void);
void checkImages (char *name);
int dumpMags (FILE *fout, Catalog *catalog, int Ncatalog);

void makeMosaics (Image *image, off_t Nimage, int mergeMcal);
Mosaic *getMosaicForImage (off_t im);
void setMosaicCenters (Image *image, off_t Nimage);

void set_db (FITS_DB *in);
int Shutdown (char *format, ...) OHANA_FORMAT(printf, 1, 2) ;
void TrapSignal (int sig);
void SetProtect (int mode);
int SetSignals (void);

void          freeGridBins        PROTO((void));
void          freeImageBins       PROTO((int Ncatalog, int doImageList));
void          freeMosaicBins      PROTO((int Ncatalog, int doMosaicList));
void          freeTGroupBins      PROTO((int Ncatalog));

void          free_catalogs       PROTO((Catalog *catalog, int Ncatalog));
int           gcatalog            PROTO((Catalog *catalog, int FINAL));
Coords       *getCoords           PROTO((off_t meas, int cat));
off_t         getImageEntry       PROTO((off_t meas, int cat));

float         getMcal             PROTO((off_t meas, int cat, dvoMagClassType class));
float         getMflat            PROTO((off_t meas, int cat, Catalog *catalog));
float         getMgridTiny        PROTO((MeasureTiny *measure));
float         getMgrid            PROTO((Measure *measure));
float         getMmos             PROTO((off_t meas, int cat));
float         getMgrp             PROTO((off_t meas, int cat, float airmass, float *dZpt));
float         getMrel             PROTO((Catalog *catalog, off_t meas, int cat, dvoMagClassType class, dvoMagSourceType source));
short         getUbercalDist      PROTO((off_t meas, int cat));
float         getCenterOffset     PROTO((off_t meas, int cat, Measure *measure, unsigned int *myID));
Image        *getimage            PROTO((off_t N));
Image        *getimages           PROTO((off_t *N, off_t **LineNumber));
ImageSubset  *getimages_subset    PROTO((off_t *N));
void          global_stats        PROTO((Catalog *catalog, int Ncatalog, int nloop));
void          initGrid            PROTO((void));
void          setMflatFromGrid    PROTO((Catalog *catalog));
void          initGridBins        PROTO((void));
GridCorrectionType *getGridCorrByCode PROTO((int code));
GridCorrectionType *newGridCorrByCode PROTO((int code));
GridCorrectionType *getGridCorrNext   PROTO((int *Nlast));
int           GridCorrectionSave  PROTO((void));
int           GridCorrectionSaveFile  PROTO((char *filename, GridOutputMode mode));
void          GridCorrectionLoad  PROTO((char *filename));
void          initImageBins       PROTO((Catalog *catalog, int Ncatalog, int doImageList));
void          initImagesSubset    PROTO((ImageSubset *input, off_t *line_number, off_t N));
void          initImages          PROTO((Image *input, off_t *LineNumber, off_t N));
void          initMosaicBins      PROTO((Catalog *catalog, int Ncatalog, int doMosaicList));
void          initMosaicMcal      PROTO((Image *image, off_t Nimage));
void          initMosaics         PROTO((Image *subset, off_t Nsubset, Image *image, char *inSubset, off_t Nimage));
void          initTGroups         PROTO((Image *subset, off_t Nsubset));
void          freeTGroups         PROTO((void));
void          initTGroupBins      PROTO((Catalog *catalog, int Ncatalog));
void          initMrel            PROTO((Catalog *catalog, int Ncatalog));
RelphotMode   initialize          PROTO((int argc, char **argv));
void          initialize_client   PROTO((int argc, char **argv));
void          liststats_setmode   PROTO((StatType *stats, char *strmode));
int           liststats           PROTO((double *value, double *dvalue, double *wvalue, int N, StatType *stats));
int           liststats_init      PROTO((StatType *stats));
int           liststats_irls      PROTO((StatDataSet *dataset, int Npoints, StatType *stats));
int           liststats_fit1d     PROTO((double *value, double *err, double *x, int Npts, StatType *stats, double *dk));
double        weight_cauchy       PROTO((double x));
double        VectorFractionInterpolate PROTO((double *values, float fraction, int Npoints));

// fit1d_irls:
int           fit1d_irls          PROTO((FitDataSet *dataset, int Npoints));
void          FitDataSetFree      PROTO((FitDataSet *dataset));
void          FitDataSetAlloc     PROTO((FitDataSet *dataset, int Nmax, int order, int Nbootstrap));
void          FitDataSetAddPriors PROTO((FitDataSet *dataset));
StatType      FitDataSetSoften    PROTO((FitDataSet *dataset, int Nvalues));

unsigned int *ReadTGroupFile      PROTO((FILE *f, int *nelem));
void          loadTGroups         PROTO((char *filename));
void          initTGroupsMcal     PROTO((void));
TGroup       *getTGroupForImage   PROTO((off_t im));
TGroup       *findTGroup          PROTO((unsigned int start, int photcode));

Catalog      *load_catalogs       PROTO((SkyList *skylist, int *Ncatalog, int hostID, char *hostpath, char *syncfile));
Catalog      *load_catalogs_parallel PROTO((SkyList *sky, int *Ncatalog, char *syncfile));

int           load_images         PROTO((FITS_DB *db, SkyList *skylist, SkyRegion *region, int unlockImages, int UseAllImages));
Image        *select_images       PROTO((SkyList *skylist, Image *timage, off_t Ntimage, char *inSubset, off_t **LineNumber, off_t *Nimage, SkyRegion *region));

int           main                PROTO((int argc, char **argv));
void          mark_images         PROTO((Image *image, off_t Nimage, Image *timage, off_t Ntimage));
void          matchImage          PROTO((Catalog *catalog, off_t meas, int cat, int doImageList));
void          matchMosaics        PROTO((Catalog *catalog, off_t meas, int cat, int doMosaicList));
void          matchTGroups        PROTO((Catalog *catalog, off_t meas, int cat));

double        opening_angle       PROTO((double x1, double y1, double x2, double y2, double x3, double y3));
void          plot_chisq          PROTO((Catalog *catalog, int Ncatalog));
void          plot_defaults       PROTO((Graphdata *graphdata));
void          plot_images         PROTO((void));
void          plot_list           PROTO((Graphdata *graphdata, double *xlist, double *ylist, int N, char *label, char *format, ...) OHANA_FORMAT(printf, 6, 7) );
void          plot_mosaic_fields  PROTO((Catalog *catalog));
void          plot_mosaics        PROTO((void));
void          plot_scatter        PROTO((Catalog *catalog, int Ncatalog));
void          plot_star_coords    PROTO((Catalog *catalog, int Ncatalog));
void          plot_stars          PROTO((Catalog *catalog, int Ncatalog));
void          plot_setMcal        PROTO((double *list, int Npts));

void          plot_list_add       PROTO((Graphdata *graphdata, double *xlist, double *ylist, int N));
int           get_graph           PROTO((int N));
int           open_graph          PROTO((int N));

void          reload_catalogs     PROTO((SkyList *skylist, int hostID, char *hostpath));
int           reload_catalogs_parallel PROTO((SkyList *sky));
int           reload_images       PROTO((FITS_DB *db));
int           setExclusions       PROTO((Catalog *catalog, int Ncatalog, int verbose));
void          setMgrid            PROTO((Catalog *catalog, int Ncatalog));
void          resetMgrid          PROTO((void));

void          setMcalFromMosaics  PROTO((void));
void          setMcalFromTGroups  PROTO((void));
int           setMcalOutput       PROTO((Catalog *catalog, int Ncatalog));

void          setMcal             PROTO((Catalog *catalog));
int           setMmos             PROTO((Catalog *catalog));
int           setMgrp             PROTO((Catalog *catalog));

int           setMrel             PROTO((Catalog *catalog, int Ncatalog));
void          setMrelFinal        PROTO((Catalog *catalog, int simpleAverage));
int           setMrelOutput       PROTO((Catalog *catalog, int Ncatalog));

void          setMcalTest         PROTO((Catalog *catalog));

int           setMave             PROTO((Catalog *catalog, int Ncatalog));
void          set_ZP              PROTO((double ZERO));
int           setrefcode          PROTO((Image *image, off_t Nimage)); 
void          skip_measurements   PROTO((Catalog *catalog, int pass));
void          sortA               PROTO((double *X, int N));
void          sortB               PROTO((double *X, double *Y, int N));
void          sortC               PROTO((double *X, double *Y, double *F1, double *F2, int N));
void          sortD               PROTO((double *X, double *Y, double *Z, int N));
StatType      statsImageM         PROTO((Catalog *catalog));
StatType      statsImageN         PROTO((Catalog *catalog));
StatType      statsImageX         PROTO((Catalog *catalog));
StatType      statsImagedM        PROTO((Catalog *catalog));
StatType      statsMosaicM        PROTO((Catalog *catalog));
StatType      statsMosaicN        PROTO((Catalog *catalog));
StatType      statsMosaicX        PROTO((Catalog *catalog));
StatType      statsMosaicdM       PROTO((Catalog *catalog));
StatType      statsTGroupM        PROTO((Catalog *catalog));
StatType      statsTGroupX        PROTO((Catalog *catalog));
StatType      statsTGroupdM       PROTO((Catalog *catalog));

StatType      statsStarN          PROTO((Catalog *catalog, int Ncatalog, int Nsec, int seccode));
StatType      statsStarS          PROTO((Catalog *catalog, int Ncatalog, int Nsec));
StatType      statsStarX          PROTO((Catalog *catalog, int Ncatalog, int Nsec));
void          wcatalog            PROTO((Catalog *catalog));
void          wimages             PROTO((void));
void          write_coords        PROTO((Header *header, Coords *coords));
int relphot_objects (SkyList *skylist, int hostID, char *hostpath);
int relphot_images (SkyList *skylist);

void relphot_usage (int argc, char **argv);
void relphot_help (int argc, char **argv);

void relphot_client_usage (void);
void relphot_client_help (int argc, char **argv);

off_t getImageByID (off_t ID);

int rationalize_mosaics PROTO((Catalog *catalog, int Ncatalog));
int LimitDensityCatalog (Catalog *subcatalog, Catalog *catalog);

BrightCatalog *BrightCatalogLoad(char *filename);
int BrightCatalogSave(char *filename, BrightCatalog *catalog);
BrightCatalog *BrightCatalogMerge (Catalog *catalog, int Ncatalog);
CatalogSplitter *BrightCatalogSplitInit (int Nsecfilt);
int BrightCatalogSplitFree (CatalogSplitter *catalogs);
int BrightCatalogSplit (CatalogSplitter *catalogs, BrightCatalog *bcatalog);

int ImageSubsetSave(char *filename, ImageSubset *image, off_t Nimage);
ImageSubset *ImageSubsetLoad(char *filename, off_t *nimage);

int client_logger_init (char *dirname);
int client_logger_message (char *format,...);

int MatchImageName (off_t meas, int cat, char *name);
int MatchImageSkycellID (off_t meas, int cat, int myTessID, int myProjectionID, int mySkycellID);
int FindImageSkycellID (off_t meas, int cat, int *myTessID, int *myProjectionID, int *mySkycellID);

int load_tess (char *treefile);
void free_tess (void);
int get_tess_ids (int *tessID, int *projID, int *skycellID, double ra, double dec);
int TessellationIDsByImageName (int *tessID, int *projID, int *skycellID, char *name);

// int BoundaryTreePrimaryCell (char *primaryCellName, double ra, double dec);
// int BoundaryTreePrimaryCellIDs (int *projID, int *skycellID, double ra, double dec);

int print_measure_set_alt (Average *average, SecFilt *secfilt, Measure *measure);

// in setMrelCatalog.c:
int setMrelCatalog (Catalog *catalog, int Nc, int isSetMrelFinal, SetMrelInfo *results, int Nsecfilt);
int setMrelAverageExposure (Catalog *catalog, int cat, off_t ave, int Nsecfilt, int isSetMrelFinal, SetMrelInfo *results);
int setMrelAverageStack (Catalog *catalog, int cat, off_t ave, int Nsecfilt);
int setMrelAverageForcedWarp (Catalog *catalog, int cat, off_t ave, int Nsecfilt, SetMrelInfo *results);

int setGlobalObjStats (Average *average, Measure *measure);

void SetMrelInfoInit (SetMrelInfo *results, int allocLists);
void SetMrelInfoFree (SetMrelInfo *results);
void SetMrelInfoReset (SetMrelInfo *results);
void SetMrelInfoResetObject (SetMrelInfo *results);

void StatDataSetFree (StatDataSet *dataset, int Nsecfilt);
StatDataSet *StatDataSetAlloc (int Nsecfilt, int Nmax);

int init_synthetic_mags (void);
int add_synthetic_mags (AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, off_t *Nmeasure, off_t *Nm);

int relphot_parallel_regions (SkyTable *sky);
int relphot_parallel_images (SkyTable *sky);

int assign_images (FITS_DB *db, RegionHostTable *regionHosts);
int select_images_hostregion (RegionHostTable *hosts, Image *image, off_t Nimage);
int find_host_for_coords (RegionHostTable *regionHosts, double R, double d);
int calculate_image_bounds (Image *image, double *rmin, double *rmax, double *dmin, double *dmax, double Rmid);

int launch_region_hosts (RegionHostTable *regionHosts);

Image *ImageTableLoad(char *filename, off_t *nimage);
int ImageTableSave (char *filename, Image *images, off_t Nimages);

int indexCatalogs (Catalog *catalog, int Ncatalog);
int catID_and_objID_to_seq (int catID, int objID, int *catSeq, off_t *objSeq);
void freeCatalogIndexes (int Ncatalog);

int check_sync_file (char *filename, int nloop);
int clear_sync_file (char *filename);
int update_sync_file (char *filename, int nloop);
char *make_filename (char *dirname, char *hostname, int hostID, char *tailname);

int share_mean_mags (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
int slurp_mean_mags (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
int set_mean_mags (MeanMag *meanmags, AverageTiny *average, SecFilt *secfilt, int Nsec);
MeanMag *merge_mean_mags (MeanMag *target, int *ntarget, MeanMag *source, int Nsource);

MeanMag *MeanMagLoad(char *filename, off_t *nmeanmags);
int MeanMagSave(char *filename, MeanMag *meanmags, off_t Nmeanmags);

int share_image_mags (RegionHostTable *regionHosts, int nloop);
int slurp_image_mags (RegionHostTable *regionHosts, int nloop);
int set_image_mags (ImageMag *image_mags, Image *image);
ImageMag *merge_image_mags (ImageMag *target, int *ntarget, ImageMag *source, int Nsource);

ImageMag *ImageMagLoad(char *filename, off_t *nimage_mags);
int ImageMagSave(char *filename, ImageMag *image_mags, off_t Nimage_mags);

int markObjects (Catalog *catalog, int Ncatalog);

// in extra.c
int isMosaicChip  (int photcode);

int isGPC1chip  (int photcode);
int isGPC2chip  (int photcode);
int isGPC1stack (int photcode);
int isGPC1warp  (int photcode);
int isGPC1synth (int photcode);
int whichGPC1filter (int photcode);
int is2MASS (int photcode);
int isTYCHO (int photcode);

int isHSCchip  (int photcode);
int isCFHchip  (int photcode);

int magStatsByRanking (StatDataSet *dataset, StatType *stats);

int SynthZeroPointsLoad (char *filename);
SynthZeroPoints *SynthZeroPointsGet (void);

int relphot_synthphot (SkyList *sky, int hostID, char *hostpath);
int relphot_synthphot_parallel (SkyList *sky);

int relphot_synthphot_catalog (Catalog *catalog, SynthZeroPoints *zpts);
int relphot_synthphot_average (Average *average, SecFilt *secfilt, Measure *measure, SynthZeroPoints *zpts);

void setMeasureRank (Catalog *catalog);
int getImageFlags (off_t meas, int cat);
int getMosaicFlags (off_t meas, int cat);

void relphot_free (SkyTable *sky, SkyList *skylist);
void freeImages (char *dbImagePtr);
void freeMosaics (void);
void relphot_client_free (SkyTable *sky, SkyList *skylist);
void BrightCatalogFree (BrightCatalog *bcatalog);

void put_astrom_table (AstromOffsetTable *myTable);
void free_astrom_table (void);

uint64_t CreatePSPSObjectID(double ra, double dec);
uint64_t CreatePSPSStackDetectionID(int sourceID, int imageID, int detID);
uint64_t CreatePSPSDetectionID(double tobs, int ccdid, int detID);

void sort_by_ra (double *R, double *D, int *I, int *S, int N);

void dump_tgroups (Catalog *catalog, int Npass);
void dump_catalog (Catalog *catalog, off_t c, int Npass);
void dump_tgroup_imstats (int Npass);

void SetZptIteration (int current);
int GetZptIteration (void);

void SetZeroPointModes (Catalog *catalog, int Ncatalog);
int UseStandardOLS (ZptFitModeType mode);
int GetActivePhotcodeIndex (int photcode);

int save_images_backup (FITS_DB *db);
int save_images_updates (FITS_DB *db);

int MagResidSave(char *filename, Catalog *catalog);
int setXradAverages (Catalog *catalog);

void ResetAverageAndMeasure (Catalog *catalog);
void ResetAverageObjects (Catalog *catalog);
void ResetImages (Image *subset, off_t Nsubset);
void ResetMeasureZeroPoints (MeasureTiny *measure, off_t Nmeasure, off_t Ncat);
void ResetAverageActivePhotcodes (SecFilt *secfilt);

void   rationalize_zeropoints (int Niter);
double get_median_zpt_images (short photcode);
void   set_median_zpt_images (short photcode, double zpt);
double get_median_zpt_mosaics (short photcode);
void   set_median_zpt_mosaics (short photcode, double zpt);
double get_median_zpt_tgroups (short photcode);
void   set_median_zpt_tgroups (short photcode, double zpt);

int DumpAllMags(char *filename, Catalog *catalog, int Ncatalog);
