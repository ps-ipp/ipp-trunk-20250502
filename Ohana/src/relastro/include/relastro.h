# include <ohana.h>
# include <dvo.h>
# include <kapa.h>
# include <signal.h>
# include <assert.h>
# include <pthread.h>

# define OLD_METHOD 0

// choose off_t or int depending on full-scale relphot analysis resources
// # define IDX_T off_t
# define IDX_T int 

# ifndef MAX_INT
# define MAX_INT 2147483647
# endif

# define LOGRTIME(MSG,...) {				\
    gettimeofday (&stopTimer, (void *) NULL);		\
    float dtime = DTIME (stopTimer, startTimer);	\
    client_logger_message (MSG, __VA_ARGS__); }

typedef enum {
  MODE_SIMPLE,
  MODE_CHIP,
  MODE_MOSAIC,
} CoordMode;

typedef enum {ERROR_MODE_RA, ERROR_MODE_DEC, ERROR_MODE_POS} ErrorMode;

typedef enum {FIT_NONE, FIT_AVERAGE, FIT_PM_ONLY, FIT_PAR_ONLY, FIT_PM_AND_PAR} FitMode;

typedef enum {OP_NONE, OP_IMAGES, OP_HIGH_SPEED, OP_MERGE_SOURCE, OP_UPDATE_OBJECTS, OP_UPDATE_OFFSETS, OP_LOAD_OBJECTS, OP_HPM, OP_PARALLEL_REGIONS, OP_PARALLEL_IMAGES, OP_REPAIR_STACKS, OP_REPAIR_WARPS, OP_REPAIR_OBJECT_ID} RelastroOp;

typedef enum {TARGET_NONE, TARGET_SIMPLE, TARGET_CHIPS, SET_CHIPS, SET_STACKS, TARGET_MOSAICS} FitTarget;

typedef enum {
  FIT_RESULT_RA,
  FIT_RESULT_DEC,
  FIT_RESULT_uR,
  FIT_RESULT_uD,
  FIT_RESULT_PLX,
} FitAstromResultMode;

typedef enum {
  MARK_MEAS_DEFAULT  = 0x0000,
  MARK_TOO_FEW_MEAS  = 0x0001,
  MARK_NAN_POS_ERROR = 0x0002,
  MARK_NAN_MAG_ERROR = 0x0004,
  MARK_BIG_MAG_ERROR = 0x0008,
  MARK_BIG_OFFSET    = 0x0010,
} MeasurementMask;

typedef struct {
  int minID;
  int maxID;
  int *index;
  int Nindex;
  int NINDEX;
} myIndexType;

typedef struct {
  double R;
  double D;
  unsigned int objID;
  unsigned int catID;
} MeanPos;

typedef struct {
  double R;
  double D;
  unsigned int objID;
  unsigned int catID;
  unsigned int imageID;
} MeasPos;

typedef struct {
  Coords coords;
  float dXpixSys;
  float dYpixSys;
  unsigned int imageID;
  int nFitAstrom;
  int flags;
  float refColorBlue;
  float refColorRed;
} ImagePos;

typedef struct {
  Average     *average;	      // array of (minimal) average data
  MeasureTiny *measure;	      // array of (minimal) measure data 
  SecFilt     *secfilt;	      // array of secfilt data (matched to average by Nsecfilt)
  off_t       Naverage;
  off_t       Nmeasure;
} BrightCatalog;

typedef struct {
  Catalog *catalog;	      // array of catalogs generated
  int NCATALOG;		      // number of catalogs allocated
  int Ncatalog;		      // number of catalogs generated
  int Nsecfilt;		      // number of catalogs generated
  off_t *NAVERAGE; 	      // allocated Averages per catalog
  off_t *NMEASURE;	      // allocated Measures per catalog
  int   *index;		      // lookup table catID -> catalog[i]
  unsigned int *catIDs;	      // lookup table catID <- catalog[i]
  unsigned int  maxID;        // max catID value to date
} CatalogSplitter;

typedef struct {
  double R, D;  /* Sky Coords    - degrees */
  double P, Q;  /* Tangent Plane - pixels  */
  double L, M;  /* Focal Plane   - pixels  */
  double X, Y;  /* Chip Coords   - pixels  */
  float Mag;
  float ColorBlue;
  float ColorRed;
  float dMag;
  float dPos;
  int mask;
  int Nmeas;
} StarData;

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

typedef struct {
  double Ro, dRo;
  double Do, dDo;
  double uR, duR;
  double uD, duD;
  double  p, dp;

  double chisq;
  int Nfit;
  int converged;
  int useWeight;

} FitAstromResult;

typedef struct {
  double **A;
  double **B;
  double **Cov;
  double *Beta;
  double *Beta_prev;
  int Nterms;
  int getChisq;
  int getError;
} FitAstromData;

// XXX do we need doubles for all of these?  I actually only have of order 100 of these
// allocated at a time, so size is not an issue.
typedef struct {
  double X, dX;
  double Y, dY;
  double R, dR;
  double D, dD;
  double T, dT;
  double Qx, Qy; // unmodified error-based weight (1/dX^2)
  double qx, qy; // IRLS-modified error-based weight (1/dX^2)
  double Wx, Wy; // IRLS weight factor
  double rx, ry;
  double u;
  double pR;
  double pD;
  double C_blue;
  double C_red;
  int measure;
  int mask;
} FitAstromPoint;

typedef struct {
  off_t Nave;
  off_t Npm;
  off_t Npar;
  off_t Nskip;
  off_t Noffset;

  double *values;
  FitAstromResult *fit; // use bootstrap resampling to generate Nfit fits to measure the stats
  int Nfit;
  int NfitAlloc;

  double * Xstack;
  double *dXstack;
  double * Ystack;
  double *dYstack;

  FitAstromPoint *points;
  FitAstromPoint *sample;
  FitAstromPoint *nomask;
  int Npoints;
  int NpointsAlloc;

  FitAstromData *fitdataPos;
  FitAstromData *fitdataPM;
  FitAstromData *fitdataPar;

  Coords coords;
  time_t T2000;
} FitStats;

typedef struct {
  unsigned int start;
  unsigned int stop;
  off_t myImage;
  float McalPSF;
  float McalAPER;
  float dMcal;
  float McalChiSq;
  float secz;
  char flags;
  Coords coords;
} Mosaic;

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
  double scale;
  double **Roff;
  double **Doff;
  double  *dR;
  int *Nra;
  int Ndec;
} FrameCorrectionType;

typedef struct {
  // AstromOffsetMap *map;
  Coords *coords; // carries a pointer to the AstromOffsetMap
  FrameCorrectionType *frame;
} FrameCorrectionSet;

typedef struct {
  double *Rave;
  double *Dave;
  double *dRoff;
  double *dDoff;
  int Nicrfobj;
} ICRFobj;

typedef struct {
  int NStackFixExtID;
  int NStackFixImageID;
  int NstackBadCoords;
  int NstackFailRepair_v1;
  int NstackFailRepair_v2;
  int NstackFixCoords;
  int NstackMissImage;
  int NstackMissTime;
  int NstackNoImageID;
} StackRepairResult;

typedef struct {
  int NfixChipID;
  int NfixStackID;
  int NfixWarpID;
  int NfixWarpImageID;
  int NfixWarpCoord;
  int NmissWarp;
  int NmissStack;
  int NbadWarp;
  int NbadWarpTime;
  int NwarpNoImage;
} WarpRepairResult;

typedef struct {
  int NstackNoImageID;
  int NstackBadCoords;
  int NstackBadTime;
  int NstackBadImageCoords;
  int NwarpNoImageID;
  int NwarpBadCoords;
  int NwarpBadTime;
  int NwarpBadImageCoords;
  int NchipNoImageID;
  int NchipBadCoords;
  int NchipBadTime;
  int NchipBadImageCoords;
} CheckMeasureResult;

// # define ID_MEAS_OBJECT_HAS_2MASS ID_MEAS_POOR_PHOTOM

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char   ImageCat[DVO_MAX_PATH];
char   GSCFILE[DVO_MAX_PATH];
char   CATDIR[DVO_MAX_PATH];
char   *HIGH_SPEED_DIR;
char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char   SKY_TABLE[DVO_MAX_PATH];
int    SKY_DEPTH;  /** XXX EAM : depth of catalog tables, fix usage */

// globals for parallel region operations
char  *REGION_FILE;
char  *IMAGE_TABLE;
int    REGION_HOST_ID;
int    PARALLEL_REGIONS_MANUAL;
char  *MANUAL_UNIQUER;
int    CATCH_UP;

int          HOST_ID;
char        *HOSTDIR;
char        *BCATALOG;

unsigned int OBJ_ID_SRC;
unsigned int CAT_ID_SRC;
unsigned int OBJ_ID_DST;
unsigned int CAT_ID_DST;

double SIGMA_LIM;
int SRC_MEAS_TOOFEW; //catalog objects wich fewer detections then this are ignored
double MIN_ERROR;

double MIN_DISTANCE_MOD;  
double MAX_DISTANCE_MOD;  
double MAX_DISTANCE_MOD_ERR;

int    IMFIT_TOO_FEW; // need more than this number of stars to fit an image
int    IMFIT_CLIP_NITER; // number of clipping iterations to perform in FitChip
double IMFIT_CLIP_NSIGMA; // number of sigma to clip in FitChip
double IMFIT_SYS_SIGMA_LIM; // max dMag for objects used to measure systematic scatter

double RADIUS; // match radius for high-speed objects

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;
int    PARALLEL_MANUAL_NO_WAIT;

int    PARALLEL_OUTPUT;

int    VERBOSE_IMAGE;

int    VERBOSE;
int    VERBOSE2;

float  TEST_SCALE;
char  *GALAXY_MODEL;

int    USE_FIXED_PIXCOORDS;
int    USE_GALAXY_MODEL;
int    USE_ICRF_CORRECT;

int    USE_ICRF_LOCAL;
int    USE_ICRF_SHFIT;
int    USE_ICRF_POLE;

int    FIT_STACKS;
int    IMSTATS_ONLY;
int    REPAIR_STACKS;
int    USE_IMAGE_COORDS_FOR_REPAIR;
int    USE_ALL_IMAGES;
int    KEEP_ALL_IMAGES_RA;
int    CHECK_MEASURE_TO_IMAGE;

int    SKIP_PS1_CHIP;
int    SKIP_PS1_STACK;
int    SKIP_HSC;
int    SKIP_CFH;

int    UPDATE_ALL_MEASURE;
int    UPDATE_PS1_STACK_MEASURE;
int    UPDATE_PS1_CHIP_MEASURE;
int    UPDATE_HSC_MEASURE;
int    UPDATE_CFH_MEASURE;

int    APPLY_PROPER_MOTION;

int    RESET;
int    RESET_IMAGES;
int    RESET_BAD_IMAGES;
int    NLOOP;
int    NTHREADS;
int    UPDATE;
int    PLOTSTUFF;
int    SAVEPLOT;
int    SHOW_PARAMS;
char   STATMODE[32];
// int    POS_TOOFEW;
// int    PM_TOOFEW;
double PM_DT_MIN;
double PAR_FACTOR_MIN;
int    PLOTDELAY;
int    CHIPORDER;
int    CHIPMAP;

int    *ChipMapLoop;
int    *ChipOrderLoop;
char   *ChipMapLoopStr;
char   *ChipOrderLoopStr;

int    N_BOOTSTRAP_SAMPLES;

int MaxDensityUse;
double MaxDensityValue;

char          *PHOTCODE_KEEP_LIST, *PHOTCODE_SKIP_LIST, *PHOTCODE_RESET_LIST;
int           NphotcodesKeep,      NphotcodesSkip,      NphotcodesReset;
PhotCode     **photcodesKeep,     **photcodesSkip,     **photcodesReset;

char          *PHOTCODE_A_LIST,  *PHOTCODE_B_LIST;
int           NphotcodesGroupA,  NphotcodesGroupB;
PhotCode     **photcodesGroupA, **photcodesGroupB;
char          WHERE_A[10000],          WHERE_B[10000];
SkyRegionSelection SELECTION;

char         *DCR_BLUE_COLOR_POS,    *DCR_BLUE_COLOR_NEG;
PhotCode     *DCR_BLUE_PHOTCODE_POS, *DCR_BLUE_PHOTCODE_NEG; 
int           DCR_BLUE_NSEC_POS,      DCR_BLUE_NSEC_NEG; 

char         *DCR_RED_COLOR_POS,    *DCR_RED_COLOR_NEG;
PhotCode     *DCR_RED_PHOTCODE_POS, *DCR_RED_PHOTCODE_NEG; 
int           DCR_RED_NSEC_POS,      DCR_RED_NSEC_NEG; 

float *LoopWeight2MASS;
char *LoopWeight2MASSstr;

float *LoopWeightTycho;
char *LoopWeightTychostr;

float *LoopWeightGAIA;
char *LoopWeightGAIAstr;

int ImagSelect;
double ImagMin, ImagMax;

double  PlotMmin, PlotMmax, PlotdMmin, PlotdMmax;

int PhotFlagSelect, PhotFlagPoor, PhotFlagBad;

float MinBadQF;
float MaxMeanOffset;

int TimeSelect;
time_t TSTART, TSTOP;

int FlagOutlier;
int    CLIP_THRESH;
int USE_BASIC_CHECK;
int APPLY_OFFSETS; // in parallel-regions mode, do not launch UpdateObjectOffsets unless -apply-offsets is called

int ExcludeBogus;
double ExcludeBogusRadius;

FitMode FIT_MODE;
int USE_IRLS;
int ALLOW_IRLS;

RelastroOp RELASTRO_OP;
FitTarget FIT_TARGET;

SkyRegion UserPatch;

int DoUpdateObjects;
int DoUpdateSimple;
int DoUpdateChips;
int DoUpdateMosaics;

// StarMap parameters:
int NX_MAP;
int NY_MAP;
double DPOS_MAX;
double ADDSTAR_RADIUS;

/*** relphot prototypes ***/
void          ConfigInit          PROTO((int *argc, char **argv));
void          GetConfig           PROTO((char *config, char *field, char *format, int N, void *ptr));
char         *GetPhotnamebyCode   PROTO((PhotCodeData *photcodes, int code));
void          InterpolateGrid     PROTO((float *buffer, int Nx, int Ny, Coords *ccd, Coords *gcoords));
off_t        *SelectRefMosaic     PROTO((Mosaic **refmosaic, off_t *Nimage));
int           args                PROTO((int argc, char **argv));
int           args_client         PROTO((int argc, char **argv));
int           bcatalog            PROTO((Catalog *subcatalog, Catalog *catalog));
void          clean_images        PROTO((void));
void          clean_measures      PROTO((Catalog *catalog, int Ncatalog, int final));
void          clean_mosaics       PROTO((void));
void          clean_stars         PROTO((Catalog *catalog, int Ncatalog));
int           corner_check        PROTO((double *x1, double *y1, double *x2, double *y2));
void          dumpGrid            PROTO((void));
void          dump_grid           PROTO((void));
int           edge_check          PROTO((double *x1, double *y1, double *x2, double *y2));
void          findImages          PROTO((Catalog *catalog, int Ncatalog, int MATCHCAT));
int           findMosaics         PROTO((Catalog *catalog, int Ncatalog));
Image        *find_images         PROTO((FITS_DB *db, GSCRegion *region, off_t Nregion, off_t *Nimage, off_t **LineNum));
void set_db (FITS_DB *in);
int Shutdown (char *format, ...) OHANA_FORMAT(printf, 1, 2);
void TrapSignal (int sig);
void SetProtect (int mode);
int SetSignals (void);

void relastro_free (SkyTable *sky, SkyList *skylist);
void relastro_client_free (SkyTable *sky, SkyList *skylist);

GSCRegion    *find_regions        PROTO((Image *image, off_t Nimage, int *Nregions, GSCRegion *fullregion));
void          freeGridBins        PROTO((int Ncatalog));
void          freeImageBins       PROTO((int Ncatalog));
void          freeMosaicBins      PROTO((int Ncatalog));
void          free_catalogs       PROTO((Catalog *catalog, int Ncatalog));
int           gcatalog            PROTO((Catalog *catalog, int FINAL));
// Coords       *getCoords           PROTO((off_t meas, int cat));
float         getMcal             PROTO((off_t meas, int cat));
float         getMgrid            PROTO((off_t meas, int cat));
float         getMmos             PROTO((off_t meas, int cat));
float         getMrel             PROTO((Catalog *catalog, off_t meas, int cat));
GSCRegion    *get_regions         PROTO((double minRa, double maxRa, double minDec, double maxDec, off_t *Nregions));
void          getfullregion       PROTO((Image *image, off_t Nimage, GSCRegion *fullregion));
Image        *getimage            PROTO((off_t N));
Image        *getimages           PROTO((off_t *N, off_t **line_number));
void          global_stats        PROTO((Catalog *catalog, int Ncatalog));
void          initGrid            PROTO((int dX, int dY));
void          initGridBins        PROTO((Catalog *catalog, int Ncatalog));
void          initImageBins       PROTO((Catalog *catalog, int Ncatalog, int FULLINIT));
void          initImages          PROTO((Image *input, off_t *line_number, off_t N, int isSubset));
void          freeImages          PROTO((char *dbImagePtr));
void          initMosaicBins      PROTO((Catalog *catalog, int Ncatalog));
void          initMosaicGrid      PROTO((Image *image, off_t Nimage));
void          initMosaics         PROTO((Image *image, off_t Nimage));
void          initMrel            PROTO((Catalog *catalog, int Ncatalog));
void          initialize          PROTO((int argc, char **argv));
void          initialize_client   PROTO((int argc, char **argv));
void          initstats           PROTO((char *mode));
int           liststats           PROTO((double *value, double *dvalue, int N, StatType *stats));
int           liststats_pos       PROTO((double *value, double *dvalue, int N, StatType *stats, int XVERB));
Catalog      *load_catalogs       PROTO((SkyList *skylist, int *Ncatalog, int subselect, int hostID, char *hostpath, char *syncfile));
int           load_images         PROTO((FITS_DB *db, SkyList *skylist, int UseFullOverlap, int UseAllImages));
Image        *select_images       PROTO((SkyList *skylist, Image *timage, off_t Ntimage, off_t **LineNumber, off_t *Nimage, int UseFullOverlap));

void check_permissions (char *basefile);
void lock_image_db (FITS_DB *db, char *filename);
void unlock_image_db (FITS_DB *db);
void create_image_db (FITS_DB *db);
void save_catalogs (Catalog *catalog, int Ncatalog);

int reload_images (FITS_DB *db);

int           main                PROTO((int argc, char **argv));
void          mark_images         PROTO((Image *image, off_t Nimage, Image *timage, off_t Ntimage));
void          matchImage          PROTO((Catalog *catalog, off_t meas, int cat, int MATCHCAT));
void          matchMosaics        PROTO((Catalog *catalog, off_t meas, int cat));
GSCRegion    *name_region         PROTO((char *name, off_t *Nregions));
double        opening_angle       PROTO((double x1, double y1, double x2, double y2, double x3, double y3));
void          plot_chisq          PROTO((Catalog *catalog, int Ncatalog));
void          plot_defaults       PROTO((Graphdata *graphdata));
void          plot_grid           PROTO((Catalog *catalog));
void          plot_images         PROTO((void));
void          plot_list           PROTO((Graphdata *graphdata, double *xlist, double *ylist, int N, char *label, char *file));
void          plot_mosaic_fields  PROTO((Catalog *catalog));
void          plot_mosaics        PROTO((void));
void          plot_scatter        PROTO((Catalog *catalog, int Ncatalog));
void          plot_star_coords    PROTO((Catalog *catalog, int Ncatalog));
void          plot_stars          PROTO((Catalog *catalog, int Ncatalog));
void          reload_catalogs     PROTO((SkyList *skylist));
int           setExclusions       PROTO((Catalog *catalog, int Ncatalog));
void          setMcal             PROTO((Catalog *catalog, int Poor));
void          setMcalFinal        PROTO((void));
int           setMcalOutput       PROTO((Catalog *catalog, int Ncatalog));
void          setMgrid            PROTO((Catalog *catalog));
int           setMmos             PROTO((Catalog *catalog, int Poor));
int           setMrel             PROTO((Catalog *catalog, int Ncatalog));
void          setMrelFinal        PROTO((Catalog *catalog));
int           setMrelOutput       PROTO((Catalog *catalog, int Ncatalog, int mark));
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
StatType      statsStarN          PROTO((Catalog *catalog, int Ncatalog));
StatType      statsStarS          PROTO((Catalog *catalog, int Ncatalog));
StatType      statsStarX          PROTO((Catalog *catalog, int Ncatalog));
void          wcatalog            PROTO((Catalog *catalog));
void          wimages             PROTO((void));
void          write_coords        PROTO((Header *header, Coords *coords));

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
int CoordsGetCenter (CoordFit *fit, double tol, double *xo, double *yo);
CoordFit *CoordsSetCenter (CoordFit *input, double Xo, double Yo);
int FitChip (StarData *raw, StarData *ref, int Nmatch, Image *image);
void FitMosaic (StarData *raw, StarData *ref, int Nmatch, Coords *coords);
void FitSimple (StarData *raw, StarData *ref, int Nmatch, Coords *coords);
void initObjectData (Catalog *catalog, int Ncatalog);
int UpdateObjects (Catalog *catalog, int Ncatalog, int Nloop);
int UpdateSimple (Catalog *catalog, int Ncatalog);
int UpdateChips (Catalog *catalog, int Ncatalog, int Nloop);
int UpdateStacks (Catalog *catalog, int Ncatalog);
int UpdateStacksWithFit (Catalog *catalog, int Ncatalog);
int UpdateMosaic (Catalog *catalog, int Ncatalog);
int UpdateMeasures (Catalog *catalog, int Ncatalog);
void fixImageRaw (Catalog *catalog, int Ncatalog, off_t im);
void FlagOutliers(Catalog *catalog);
int MeasFilterTest(Measure *measure, int applySigmaLim);
int MeasFilterTestTiny(MeasureTiny *measure, int applySigmaLim);

int sun_ecliptic (double jd, double *lambda, double *beta, double *epsilon, double *Radius);
int ParFactor (double *pR, double *pD, double RA, double Dec, double Time);

int FitPM_Basic (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);
int FitPMandPar_Basic (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);
int FitPosPMfixed_Basic (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);

Mosaic *getMosaicForImage (off_t N);

StarData *getImageRef (Catalog *catalog, int Ncatalog, off_t im, off_t *Nstars, CoordMode mode);
StarData *getImageRaw (Catalog *catalog, int Ncatalog, off_t im, off_t *Nstars, CoordMode mode);
int setImageRaw (Catalog *catalog, int Ncatalog, off_t im, StarData *raw, off_t Nraw, CoordMode mode);
off_t getImageByID (off_t ID);
int updateImageRaw (Catalog *catalog, int Ncatalog, off_t im);

Mosaic *getmosaics (off_t *N);
void initMosaics (Image *image, off_t Nimage);
void freeMosaics ();
StarData *getMosaicRaw (Catalog *catalog, int Ncatalog, off_t mos, off_t *Nstars);
StarData *getMosaicRef (Catalog *catalog, int Ncatalog, off_t mos, off_t *Nstars);
Mosaic *getMosaicForImage (off_t im);

double getMeanR (MeasureTiny *measure, Average *average, SecFilt *secfilt);
double getMeanD (MeasureTiny *measure, Average *average, SecFilt *secfilt);
int setMeanR (double ra_fit, MeasureTiny *measure, Average *average, SecFilt *secfilt);
int setMeanD (double dec_fit, MeasureTiny *measure, Average *average, SecFilt *secfilt);
double getMeanR_Big (Measure *measure, Average *average, SecFilt *secfilt);
double getMeanD_Big (Measure *measure, Average *average, SecFilt *secfilt);
int setMeanR_Big (double ra_fit, Measure *measure, Average *average, SecFilt *secfilt);
int setMeanD_Big (double dec_fit, Measure *measure, Average *average, SecFilt *secfilt);

float GetAstromError (Measure *measure, int mode);
float GetAstromErrorTiny (MeasureTiny *measure, int mode);
int relastro_objects (SkyList *skylist, int hostID, char *hostpath);
int relastro_images (SkyList *skylist);
int UpdateObjectOffsets (SkyList *skylist, int hostID, char *hostpath);

int relastroVisualPlotChipFit(StarData *raw, StarData *ref, double dRmax, int numObj);
// int relastroVisualPlotRawRef(StarData *raw, StarData *ref, double dRmax, int numObj);
// int relastroVisualPlotScatter(double values[], double thresh, int npts);
// int relastroVisualPlotOutliers(Catalog *catalog, int offset, int Nmeasure, StatType statsR, StatType statsD, double thresh);
void relastroSetVisual(int state);
int relastroGetVisual(void);

int *getCatlist (int *N, off_t im);

void SaveCoords (Coords *tgt, Coords *src);
void RestoreCoords (Coords *tgt, Coords *src, Image *image);

int high_speed_catalogs (SkyTable *sky, SkyList *skylist, int hostID, char *hostpath);
int high_speed_objects (SkyRegion *region, Catalog *catalog);
int MeasMatchesPhotcode(Measure *measure, PhotCode **photcodeSet, int Nset);

int initStarMaps ();
void freeStarMaps ();
int updateStarMaps(Catalog *catalog);
int createStarMapPoints();
int checkStarMap(int N);
int createStarMap (Catalog *catalog, int Ncatalog);
int printStarMap(int N, char *filename);

int GetScatterRawRef(float *dLsig, float *dMsig, float *dRsig, int *nKeep, StarData *raw, StarData *ref, int Nstars, float SigmaLimit);
int LimitDensityCatalog_ByNmeasure (Catalog *subcatalog, Catalog *catalog);
int LimitDensityCatalog_RandomSample (Catalog *subcatalog, Catalog *catalog);

int initializeConstraints();
int applyConstraintsA(Catalog *catalog, off_t i);
int applyConstraintsB(Catalog *catalog, off_t i);
void setupAreaSelection(SkyRegion *region);

int relastro_merge_source (SkyTable *sky);
void resort_catalog (Catalog *catalog);

BrightCatalog *BrightCatalogLoad(char *filename);
int BrightCatalogSave(char *filename, BrightCatalog *catalog);
BrightCatalog *BrightCatalogMerge (Catalog *catalog, int Ncatalog);
CatalogSplitter *BrightCatalogSplitInit (int Nsecfilt);
int BrightCatalogSplit (CatalogSplitter *catalogs, BrightCatalog *bcatalog);
int BrightCatalogSplitFree (CatalogSplitter *catalogs);
void BrightCatalogFree (BrightCatalog *bcatalog);

PhotCode **ParsePhotcodeList (char *rawlist, int *nphotcodes, int needAve);

int hpm_catalogs (SkyTable *sky, SkyList *skylist, int hostID, char *hostpath);
int hpm_catalogs_parallel (SkyList *skylist);
int hpm_objects (SkyRegion *region, Catalog *catalog);

int launch_region_hosts (RegionHostTable *regionHosts);

int assign_images (FITS_DB *db, RegionHostTable *regionHosts);
int select_images_hostregion (RegionHostTable *regionHosts, Image *image, off_t Nimage);
int calculate_image_bounds (Image *image, double *rmin, double *rmax, double *dmin, double *dmax, double Rmid);
int calculate_host_image_bounds (RegionHostTable *regionHosts);
int find_host_for_coords (RegionHostTable *regionHosts, double Rc, double Dc);

int relastro_parallel_regions ();
int relastro_parallel_images ();

char *make_filename (char *dirname, char *hostname, int hostID, char *tailname);
int check_sync_file (char *filename, int nloop);
int clear_sync_file (char *filename);
int update_sync_file (char *filename, int nloop);

int share_mean_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
int slurp_mean_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
int set_mean_pos (MeanPos *meanpos, Average *average);
MeanPos *merge_mean_pos (MeanPos *target, int *ntarget, MeanPos *source, int Nsource);

int MeanPosSave(char *filename, MeanPos *meanpos, off_t Nmeanpos);
MeanPos *MeanPosLoad(char *filename, off_t *nmeanpos);

int share_meas_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
int slurp_meas_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
int set_meas_pos (MeasPos *measpos, Average *average, MeasureTiny *measure);
MeasPos *merge_meas_pos (MeasPos *target, int *ntarget, MeasPos *source, int Nsource);

int MeasPosSave(char *filename, MeasPos *measpos, off_t Nmeaspos);
MeasPos *MeasPosLoad(char *filename, off_t *nmeaspos);

int indexCatalogs (Catalog *catalog, int Ncatalog);
int catID_and_objID_to_seq (unsigned int catID, unsigned int objID, int *catSeq, off_t *objSeq);
void freeCatalogIndexes (int Ncatalog);

int markObjects (Catalog *catalog, int Ncatalog);

int ImagePosSave(char *filename, ImagePos *image_pos, off_t Nimage_pos);
ImagePos *ImagePosLoad(char *filename, off_t *nimage_pos);

int share_image_pos (RegionHostTable *regionHosts, int nloop);
int slurp_image_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
ImagePos *merge_image_pos (ImagePos *target, int *ntarget, ImagePos *source, int Nsource);
int set_image_pos (ImagePos *image_pos, Image *image);

Image *ImageTableLoad(char *filename, off_t *nimage);
int ImageTableSave (char *filename, Image *images, off_t Nimages);
int select_mosaics_hostregion (RegionHostTable *regionHosts, Image *image, off_t Nimage);

float getColorBlue (off_t meas, int cat);
float getColorRed (off_t meas, int cat);

int areImagesLoaded ();
int areImagesMatched ();

int isGPC1chip (int photcode);
int isGPC1stack (int photcode);
int isGPC1warp (int photcode);
int isHSCchip (int photcode);
int isCFHchip (int photcode);

int save_astrom_table ();
AstromOffsetTable *get_astrom_table ();
void put_astrom_table (AstromOffsetTable *myTable);
void free_astrom_table ();

int fit_map (AstromOffsetMap *map, StarData *raw, StarData *ref, int Npts);

// ICRF QSOs : these must be marked in the database (flag on average, flag on measure)

void ICRFinit ();
int ICRFsave (int cat, int ave, int meas);
int ICRFdata (int n, int *cat, int *ave, int *meas);
int ICRFmax ();
int select_catalog_ICRF (Catalog *catalog, int Ncatalog);

void ICRFobjFree (ICRFobj *icrfobj);
ICRFobj *get_ICRF_data (Catalog *catalog, int Ncatalog);
ICRFobj *merge_icrf_obj (ICRFobj *target, ICRFobj *source);
ICRFobj *slurp_icrf_obj (RegionHostTable *regionHosts, int nloop);
int share_icrf_obj (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);

int ICRFobjSave(char *filename, ICRFobj *icrfobj);
ICRFobj *ICRFobjLoad(char *filename);

int SHfitWithMask (double *R, double *D, double *value, int *mask, int Npts, SHterms *fit);

FrameCorrectionType *FrameCorrectionInit (double scale);
void FrameCorrectionFree (FrameCorrectionType *frame);

int FrameCorrectionParallelMaster (RegionHostTable *regionHosts);
int FrameCorrectionParallelSlave (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop);
int FrameCorrectionSerial (Catalog *catalog, int Ncatalog);

FrameCorrectionSet *FrameCorrectionMeasure (ICRFobj *icrfobj);

int FrameCorrectionFromSH (FrameCorrectionType *frame, SHterms *dR, SHterms *dD);

Coords *FrameCorrectionImageToMap (Header *header, Matrix *matrix, Coords *coords, int raDirection);
int FrameCorrectionMapToImage (Header *header, Matrix *matrix, Coords *coords, int raDirection);
int FrameCorrectionSHtoImage (Header *header, Matrix *matrix, FrameCorrectionType *frame, int raDirection);
FrameCorrectionType *FrameCorrectionImageToSH (Header *header, Matrix *matrix, FrameCorrectionType *frame, int raDirection);
int FrameCorrectionFromSH (FrameCorrectionType *frame, SHterms *dR, SHterms *dD);
void FrameCorrectionFree (FrameCorrectionType *frame);
FrameCorrectionType *FrameCorrectionInit (double scale);
void FrameCorrectionSetFree (FrameCorrectionSet *set);
FrameCorrectionSet *FrameCorrectionSetInit ();

int FrameCorrectionSetSave(char *filename, FrameCorrectionSet *set);
FrameCorrectionSet *FrameCorrectionSetLoad(char *filename);

int FrameCorrectionFitSH (ICRFobj *icrfobj, SHterms *dRc, SHterms *dDc);
int FrameCorrectionFitLocal (ICRFobj *icrfobj, Coords *coords);
int FrameCorrectionApply (Catalog *catalog, int Ncatalog, FrameCorrectionType *frame, Coords *coords);

int dump_stardata_pts (StarData *raw, int Npts, char *filename);
void printNcatTotal ();

int HarvestRegionHosts (RegionHostTable *regionHosts);

void lockUpdateChips ();
void unlockUpdateChips ();

int client_logger_init (char *dirname);
int client_logger_message (char *format,...);

int FitAstromSetChisq (FitAstromResult *fit, FitAstromPoint *points, int Npoints, FitMode mode);
double VectorFractionInterpolate (double *values, float fraction, int Npts);
int BootstrapRobustStats (FitAstromResult *result, FitAstromResult *fit, int Nfit, int mode);
int BootstrapResample (FitAstromPoint *sample, FitAstromPoint *points, int Npoints);
int BootstrapSaveUnmasked (FitAstromPoint *nomask, FitAstromPoint *points, int Npoints);

int CatalogMaxNmeasure (Catalog *catalog, int Ncatalog);
int FitAstromPoints_Project (FitStats *fitStats, double *Tmean, double *Trange, double *parRange);
int UpdateObjects_SelectMeasures (FitStats *fit, Average *average, SecFilt *secfilt, MeasureTiny *measure, Measure *measureBig, int isStack, int *stackEntry);
int UpdateObjects_Stack (Average *average, SecFilt *secfilt, MeasureTiny *measure, Measure *measureBig, int Nsecfilt, FitStats *fitStats);
int UpdateObjects_Chips (Average *average, SecFilt *secfilt, MeasureTiny *measure, Measure *measureBig, int Nsecfilt, FitStats *fitStats, int cat, off_t measOff);

void FitAstromResultInit (FitAstromResult *fit);
void FitAstromPointInit (FitAstromPoint *object);
void FitAstromDataFree (FitAstromData *fit);
FitAstromData *FitAstromDataInit (int Nterms);
void FitStatsFree (FitStats *fitStats);
void FitStatsSum (FitStats *src, FitStats *tgt);
void FitStatsReset (FitStats *tgt);
FitStats *FitStatsInit (int Nmax, int Nboot);
int FitAstromResultSetPM (FitAstromResult *fit, int Nfit, Average *average);
void AstromErrorSetLoop (int Nloop, int isImageMode);
void FitPointsClearMasks (FitAstromPoint *points, int Npoints);

int FitPM_IRLS (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);
int FitPMandPar_IRLS (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);
int FitPosPMfixed_IRLS (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);
int FitPosPMfixed_Single (FitAstromResult *fit, FitAstromPoint *points, int Npoints);
int FitPosStack (FitAstromResult *fit, FitStats *fitstats);

double MedianAbsDeviation(FitAstromPoint *points, int Npoints);

double weight_cauchy (double x);
double dpsi_cauchy (double x);
void my_memdump (char *message);

int RepairWarps (SkyList *skylist, int hostID, char *hostpath);
WarpRepairResult RepairWarpMeasures (Catalog *catalog);

int FindWarpGroups (void);
void FreeWarpGroups (void);
int GetWarpSeq (Image *image, int obstime, unsigned short photcode, double Rave, double Dave, float X, float Y);

myIndexType *myIndexInit ();
int myIndexFree (myIndexType *myIndex);
int myIndexUpdateLimits (myIndexType *myIndex, int value);
int myIndexSetRange (myIndexType *myIndex);
int myIndexSetEntry (myIndexType *myIndex, int value, int entry);
int myIndexGetEntry (myIndexType *myIndex, int value);

uint64_t CreatePSPSObjectID(double ra, double dec);
uint64_t CreatePSPSStackDetectionID(int sourceID, int imageID, int detID);
uint64_t CreatePSPSDetectionID(double tobs, int ccdid, int detID);

int RepairStacks (SkyList *skylist, int hostID, char *hostpath);

int RepairObjectIDs (SkyList *skylist, int hostID, char *hostpath);

void sort_by_ra (double *R, double *D, int *I, int *S, int N);
void FreeStackGroups (void);
int GetStackSeq (Image *image, double Rstk, double Dstk, unsigned short photcode, float X, float Y);
int MakeStackIndex (void);
int RepairStackID (StackRepairResult *result, Image *image, double Rave, double Dave, Measure *measureB, MeasureTiny *measureT);
StackRepairResult RepairStackMeasures (Catalog *catalog);
int CheckMeasureToImage (Catalog *catalog);
