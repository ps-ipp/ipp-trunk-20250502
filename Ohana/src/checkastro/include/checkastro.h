# include <ohana.h>
# include <dvo.h>
# include <kapa.h>
# include <signal.h>
# include <assert.h>
# include <pthread.h>

// choose off_t or int depending on full-scale relphot analysis resources
// # define IDX_T off_t
# define IDX_T int 

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
  int   *catIDs;	      // lookup table catID <- catalog[i]
  int    maxID;		      // max catID value to date
} CatalogSplitter;

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char   ImageCat[DVO_MAX_PATH];
char   GSCFILE[DVO_MAX_PATH];
char   CATDIR[DVO_MAX_PATH];

char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char   SKY_TABLE[DVO_MAX_PATH];
int    SKY_DEPTH;  /** XXX EAM : depth of catalog tables, fix usage */

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

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

int    VERBOSE;
int    VERBOSE2;

int MaxDensityUse;
double MaxDensityValue;

char          *PHOTCODE_KEEP_LIST, *PHOTCODE_SKIP_LIST, *PHOTCODE_RESET_LIST;
int           NphotcodesKeep,      NphotcodesSkip,      NphotcodesReset;
PhotCode     **photcodesKeep,     **photcodesSkip,     **photcodesReset;

int ImagSelect;
double ImagMin, ImagMax;

int PhotFlagSelect, PhotFlagPoor, PhotFlagBad;

float MinBadQF;

int TimeSelect;
time_t TSTART, TSTOP;

SkyRegion UserPatch;

/*** relphot prototypes ***/
void          ConfigInit          PROTO((int *argc, char **argv));
void          GetConfig           PROTO((char *config, char *field, char *format, int N, void *ptr));

int           args                PROTO((int argc, char **argv));
int           args_client         PROTO((int argc, char **argv));
int           bcatalog            PROTO((Catalog *subcatalog, Catalog *catalog));

void          findImages          PROTO((Catalog *catalog, int Ncatalog));
int           findMosaics         PROTO((Catalog *catalog, int Ncatalog));

void          set_db              PROTO((FITS_DB *in));
int           Shutdown            PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);
void          TrapSignal          PROTO((int sig));
void          SetProtect          PROTO((int mode));
int           SetSignals          PROTO((void));

void          freeImageBins       PROTO((int Ncatalog));
void          free_catalogs       PROTO((Catalog *catalog, int Ncatalog));

void          initImageBins       PROTO((Catalog *catalog, int Ncatalog));
void          initImages          PROTO((Image *input, off_t *line_number, off_t N));

void          initialize          PROTO((int argc, char **argv));
void          initialize_client   PROTO((int argc, char **argv));

Catalog      *load_catalogs       PROTO((SkyList *skylist, int *Ncatalog, int subselect, int hostID, char *hostpath));
int           load_images         PROTO((FITS_DB *db, SkyList *skylist));
Image        *select_images       PROTO((SkyList *skylist, Image *timage, off_t Ntimage, off_t **LineNumber, off_t *Nimage));

void check_permissions (char *basefile);
void lock_image_db (FITS_DB *db, char *filename);
void unlock_image_db (FITS_DB *db);
void create_image_db (FITS_DB *db);
void save_catalogs (Catalog *catalog, int Ncatalog);

int reload_images (FITS_DB *db);

int           main                PROTO((int argc, char **argv));

void          matchImage          PROTO((Catalog *catalog, off_t meas, int cat));

void          set_ZP              PROTO((double ZERO));

off_t getImageByID (off_t ID);

int checkastro_objects (SkyList *skylist, int hostID, char *hostpath);
int checkastro_images (SkyList *skylist);

int LimitDensityCatalog_ByNmeasure (Catalog *subcatalog, Catalog *catalog);
int LimitDensityCatalog_RandomSample (Catalog *subcatalog, Catalog *catalog);

BrightCatalog *BrightCatalogLoad(char *filename);
int BrightCatalogSave(char *filename, BrightCatalog *catalog);
BrightCatalog *BrightCatalogMerge (Catalog *catalog, int Ncatalog);
CatalogSplitter *BrightCatalogSplitInit (int Nsecfilt);
int BrightCatalogSplit (CatalogSplitter *catalogs, BrightCatalog *bcatalog);
int BrightCatalogSplitFree (CatalogSplitter *catalogs);

Catalog *load_catalogs_parallel (SkyList *sky, int *Ncatalog);
void bcatalog_show_skips ();

int MeasFilterTestTiny(MeasureTiny *measure, int applySigmaLim);
int MeasFilterTest(Measure *measure, int applySigmaLim);
