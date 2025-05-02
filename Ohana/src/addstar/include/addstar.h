# include <ohana.h>
# include <dvo.h>
# include <signal.h>
# include <sys/time.h>
# include <time.h>
# include <zlib.h>

/* solaris requires both of these instead of ip.h:
   # include <sys/socket.h>
   # include <netinet/in.h>
*/

/* linux is happy with this, not solaris */
# include <netinet/ip.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <glob.h>

/* used in find_matches, find_matches_refstars */
# define IN_REGION(R,D) ( \
((D) >= region[0].Dmin) && ((D) < region[0].Dmax) && \
((R) >= region[0].Rmin) && ((R) < region[0].Rmax))

/* grab named photcode */
# define NAMED_PHOTCODE(CODE,NAME) \
  CODE = GetPhotcodeCodebyName (NAME); \
  if (!CODE) { \
    fprintf (stderr, "ERROR:  photcode %s not found in photcode table\n", NAME); \
    exit (0); }

# define dCOS(A)   ((double) cos ((double)RAD_DEG*A))
# define dSIN(A)   ((double) sin ((double)RAD_DEG*A))

# define myAbortF(FORMAT,...) { fprintf (stderr, FORMAT, __VA_ARGS__); abort(); }

// things that are needed for a single file
typedef struct {
  char *filename;  // name of the file on disk (full path)
  char *imagename; // name of the image for user reference (eg, base of neb path, full path, etc)
} AddstarFile;

typedef struct {
  char *exthead;
  char *extdata;
  char *extxrad;
  char *exttype;
  int extnum_head;
  int extnum_data;
  int extnum_xrad;
} HeaderSet;

# define IDTYPE int

typedef struct {
  IDTYPE Nimage;
  IDTYPE minID;
  IDTYPE maxID;
  IDTYPE range;
  IDTYPE *imageID;
  IDTYPE *externID;
  char *found;
} ImageIndex;

typedef struct sockaddr_in SockAddress;

enum {ADDSTAR_MODE_NONE, ADDSTAR_MODE_IMAGE, ADDSTAR_MODE_REFLIST, ADDSTAR_MODE_REFCAT, ADDSTAR_MODE_FAKEIMAGE, ADDSTAR_MODE_RESORT, ADDSTAR_MODE_CREATE_ID, ADDSTAR_MODE_REFFITS};
enum {NONE, SIMPLE_CMP, SIMPLE_CMF, SIMPLE_MEF, MOSAIC_CMP, MOSAIC_CMF, MOSAIC_MEF, MOSAIC_PHU, SDSS_OBJ, UKIRT_OBJ};
/* note: MEF implies CMF */

/* globals which define database info / data sources (KEEP) */
char   ImageCat[DVO_MAX_PATH];
char   GSCFILE[256];
char  *CATDIR;
char   CATMODE[16];     /* raw, mef, split, mysql */
char   CATFORMAT[16];   /* internal, elixir, loneos, panstarrs */
char   CATCOMPRESS[16]; /* GZIP_1, NONE */
char   TWO_MASS_DIR_AS[256];
char   TWO_MASS_DIR_DR2[256];
char   GSCDIR[256];
char   USNO_A_DIR[256];
char   USNO_B_DIR[256];
char   TYCHO_DIR[256];
char   SubpixDatafile[256];
char   *USE_NAME;
char   PASSWORD[80];
char   HOSTNAME[80];
int    NVALID_IP, *VALID_IP;
char   SKY_TABLE[256];
int    SKY_DEPTH;  /** XXX EAM : depth of catalog tables, fix usage */
char   CameraLayout[256];
SkyTable *ServerSky;
char  *PMM_CCD_TABLE;

/* used to select entries from header (gstars or parse_time) (KEEP) */
char   DateKeyword[64];
char   DateMode[64];
char   UTKeyword[64];
char   MJDKeyword[64];
char   JDKeyword[64];
char   ExptimeKeyword[64];
char   AirmassKeyword[64];
char   CCDNumKeyword[64];
char   STKeyword[64];
char   ExtnameKeyword[64];
char   ImageIDKeyword[64];
char   SourceIDKeyword[64];

/* these globals modify the behavior of gstars (KEEP) */
double 	SNLIMIT;
int     PHOTFLAG_EXCLUDE;
int    	ACCEPT_ASTROM;  // accept even bad astrometry solutions (NASTRO == 0)
int    	ACCEPT_MOTION;  // accept reference proper motion measurements
int    	ACCEPT_TIME;    // accept time stamp (or 0)
int    	FORCE_SINGLE_TIME;    // use PHU time for all chips
int    	NO_STARS;       // ignore the stars
int    	NO_DUPLICATE_IMAGES; // allow / skip duplicate images 
int    	IMAGE_ID_OVERRIDE; // allow / skip duplicate images 
int    	TEXTMODE;       // force input file to be loaded as RAW
int     SUBPIX;         // apply a subpix correction
char   *DUMP;           // dump out intermediate results
int    	XOVERSCAN;      // used to modify stored image dimensions 
int    	YOVERSCAN;      // used to modify stored image dimensions 
int    	XMIN;           // used to filter loaded star list 
int    	XMAX;           // used to filter loaded star list 
int    	YMIN;           // used to filter loaded star list 
int    	YMAX;           // used to filter loaded star list 
double 	Latitude;       // carried into image structure from config
double 	Longitude;      // carried into image structure from config
double  FAKE_RA;        // boresite coords for fake images
double  FAKE_DEC;       // boresite coords for fake images
double  FAKE_THETA;     // boresite angle for fake images

char    ZERO_POINT_OPTION[64];
char    ZERO_POINT_KEYWORD[64];
float	ZERO_POINT_OFFSET;
float	ZERO_POINT_ERROR;
float   ZPT_OBS_PHU;
float   ZPT_ERR_PHU;

float   OFFSET_ZPT;

int     OLD_RESORT;
int     READ_XRAD_DATA;
int     DIFF_WITH_INV;

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;
int    HOST_ID;
char  *HOSTDIR;

// carries the mosaic into gstars

/* these globals are used separately by both client and server (KEEP) */
double CAL_INSTMAG_MAX;
double CAL_INSTMAG_MIN;
int    VERBOSE;
int    PLOT;
double MAX_CERROR;
double MIN_FWHM_X;
double MIN_FWHM_Y;
int    NTHREADS;

/* modify server behavior (make this an addstar cleanup mode?) */
int    FORCE_READ;

// XXX this should be replaced with 
// 1) an airmass accuracy option
// 2) an alternative CATFORMAT with the sky element correctly defined.

/* these depend on HOW we implement the client/server interaction for CAT/REF modes */
time_t    TIMEREF;    // used by MODE REF
SkyRegion UserPatch;  // used by MODE CAT
char     *SELECT_2MASS_QUALITY;  // used only by get2mass_as
int NREFSTAR_GROUP;
int NSTAR_GROUP;

/*** addstar prototypes ***/

AddstarClientOptions ConfigInit   	  PROTO((int *argc, char **argv));
AddstarClientOptions args         	  PROTO((int argc, char **argv, AddstarClientOptions options));
AddstarClientOptions args_parallel_client PROTO((int argc, char **argv, AddstarClientOptions options));
void FreeConfig PROTO((void));

void       AddToCalibration       PROTO((Average *average, SecFilt *secfilt, Measure *measure, Measure *new, off_t *next, off_t Nstar));
void       FindCalibration        PROTO((Image *image));
FILE      *GetDB                  PROTO((int *state));
void       InitCalibration        PROTO((int mode));
void       SaveCalibration        PROTO((float Mo, float dMo, float Mr, float dMr, float Mi, off_t N));
void       SetProtect             PROTO((int mode));
int        SetSignals             PROTO((void));
int        Shutdown               PROTO((char *message, ...)) OHANA_FORMAT(printf, 1, 2);
void       TrapSignal             PROTO((int sig));
float      airmass                PROTO((float secz_image, double ra, double dec, double st, double latitude));
float      azimuth                PROTO((double ha, double dec, double latitude));
void       SetAirmassQuality      PROTO((int quality));
SkyTable  *SkyTableFromTychoIndex PROTO((char *filename, int VERBOSE));
void       check_permissions      PROTO((char *basefile));
int        edge_check             PROTO((double *x1, double *y1, double *x2, double *y2));
Image     *fakeimage              PROTO((char *rootname, off_t *Nimage, int photcode));

double     get_subpix             PROTO((double x, double y));

int        find_matches_closest   PROTO((SkyRegion *region, Catalog *newcat, Catalog *catalog, AddstarClientOptions options));
int        find_matches        	  PROTO((SkyRegion *region, Catalog *newcat, Catalog *catalog, AddstarClientOptions options));

int        find_matches_refstars  	 PROTO((SkyRegion *region, Catalog *srccat, Catalog *tgtcat, AddstarClientOptions options));
int        find_matches_closest_refstars PROTO((SkyRegion *region, Catalog *srccat, Catalog *tgtcat, AddstarClientOptions options));

Catalog   *greference             PROTO((char *Refcat, SkyRegion *catstats, int photcode));

int        gcatalog               PROTO((Catalog *catalog));
Catalog   *getgsc                 PROTO((SkyRegion *patch));
Catalog   *getusno                PROTO((SkyRegion *catstats));
Catalog   *getusnob               PROTO((SkyRegion *catstats));

// load text-based stars (REF only in the sense of REF photcodes)
Catalog   *grefstars              PROTO((char *file, int photcode));
Catalog   *greffits               PROTO((char *file, int photcode));

Catalog   *rd_gsc                 PROTO((char *filename));

int        replace_match          PROTO((Average *average, Measure *measure, Measure *newmeas, off_t *found));

Catalog   *LoadStars              PROTO((char *file, Image **images, off_t *Nimages, AddstarClientOptions *options));
Header   **LoadHeaders            PROTO((FILE *f, int *mode, int *Nheader));
HeaderSet *MatchHeaders           PROTO((off_t **extsize, off_t *nimage, int mode, Header **headers, int Nheaders));
void       HeaderSetFree          PROTO((HeaderSet *headerSets, off_t NheaderSets));
Catalog   *LoadData               PROTO((FILE *f, AddstarFile *file, Image **images, off_t *nvalid, Header **headers, off_t *extsize, HeaderSet *headerSets, int NheaderSets, SkyRegion *region, AddstarClientOptions *options));
int        GetZeroPointExposure   PROTO((Header **headers, HeaderSet *headerSets, off_t Nimages));

int        in_image               PROTO((double r, double d, Image *image));
int        load_pt_catalog        PROTO((Catalog *catalog, SkyRegion *region));  /*** choose new name ***/
void       load_subpix            PROTO((void));
void       lock_image_db          PROTO((FITS_DB *db, char *filename));
int        main                   PROTO((int argc, char **argv));
double     opening_angle          PROTO((double x1, double y1, double x2, double y2, double x3, double y3));
int        parse_time             PROTO((Header *header));
int        resort_catalogs        PROTO((AddstarClientOptions *options, SkyTable *sky));
int        resort_catalogs_parallel PROTO((AddstarClientOptions *options, SkyList *sky));
int        resort_threaded        PROTO((SkyList *skylist, int ForceSort));
int        resort_unthreaded      PROTO((SkyList *skylist, int ForceSort));
void       resort_catalog         PROTO((Catalog *catalog));
void       resort_catalog_old     PROTO((Catalog *catalog));

// Stars  *ReadStarsTEXT          PROTO((FILE *f, unsigned int *nstars));

int        ReadImageHeader        PROTO((Header *header, Image *image, int photcode));

Catalog   *ReadStarsFITS          PROTO((FILE *f, Header *header, Header *in_theader));
Catalog   *FilterStars            PROTO((Catalog *newcat, Image *image, unsigned int imageID, SkyRegion *region, const AddstarClientOptions *options));
int        ReadXradFITS           PROTO((FILE *f, Header *theader, Catalog *catalog));

Catalog   *LoadDataSDSS           PROTO((FILE *f, char *file, Image **images, off_t *nvalid, Header **headers, off_t *extsize, HeaderSet *headerSets, off_t Nimages));
Catalog   *ReadStarsSDSS          PROTO((FILE *f, char *name, Header *header, Header *in_theader, Image *images, off_t *nimages));

double     scat_subpix            PROTO((double x, double y));
void       update_coords          PROTO((Average *average, Measure *measure, off_t *next));
off_t 	  *init_measure_links     PROTO((Average *average, off_t Naverage, Measure *measure, off_t Nmeasure));
off_t 	  *init_lensing_links     PROTO((Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing));
off_t 	  *init_missing_links     PROTO((Average *average, off_t Naverage, Missing *missing, off_t Nmissing));
int 	   add_meas_link     	  PROTO((Average *average, off_t *next_meas, off_t Nmeasure, off_t NMEASURE));
int 	   add_miss_link     	  PROTO((Average *average, off_t *next_miss, off_t Nmissing));
int        add_lens_link          PROTO((Average *average, off_t *next_lens, off_t Nlensing, off_t NLENSING));
off_t 	  *build_measure_links    PROTO((Average *average, off_t Naverage, Measure *measure, off_t Nmeasure));
off_t     *build_lensing_links    PROTO((Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing));
Measure   *sort_measure     	  PROTO((Average *average, off_t Naverage, Measure *measure, off_t Nmeasure, off_t *next_meas));
Missing   *sort_missing     	  PROTO((Average *average, off_t Naverage, Missing *missing, off_t Nmissing, off_t *next_miss));
Lensing   *sort_lensing           PROTO((Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing, off_t *next_lens));
int        ImageOptions		  PROTO((AddstarClientOptions *options, Image *images, off_t Nimages));
int        GetFileMode		  PROTO((Header *header));
AddstarClientOptions args_client  PROTO((int argc, char **argv, AddstarClientOptions options));
AddstarClientOptions args_load2mass PROTO((int argc, char **argv, AddstarClientOptions options));
AddstarClientOptions args_sedstar PROTO((int argc, char **argv, AddstarClientOptions options));

SkyList   *SkyListExistingSubset  PROTO((SkyList *input, char *path));
SkyList   *SkyListForStars	  PROTO((SkyTable *table, int depth, Catalog *catalog));

// these are all for the addstar client/server which has not been maintained
# if (0)
void	   args_server		  PROTO((int argc, char **argv));
int 	   CheckPassword	  PROTO((int BindSocket));
int 	   NewImage		  PROTO((int BindSocket));
int 	   NewReflist		  PROTO((int BindSocket));
int 	   NewRefcat		  PROTO((int BindSocket));
int 	   InitServerSocket	  PROTO((SockAddress *Address));
int 	   WaitServerSocket	  PROTO((int InitSocket, SockAddress *Address, int *validIP, int Nvalid));
int 	   GetClientSocket	  PROTO((char *hostname));
int 	   UpdateDatabase_Image	  PROTO((AddstarClientOptions *options, Image *images, off_t Nimages, Coords *mosaic, Stars *stars, unsigned int Nstars));
int 	   UpdateDatabase_Reflist PROTO((AddstarClientOptions *options, Stars *stars, unsigned int Nstars));
int 	   UpdateDatabase_Refcat  PROTO((AddstarClientOptions *options, SkyRegion *UserPatch, char *refcat));
int        SkyListSetPath	  PROTO((SkyList *list, char *path));
int 	   InitDataset		  PROTO((void));
int 	   PushDataset		  PROTO((DVO_DATA *data));
DVO_DATA  *PopDataset		  PROTO((void));
void	  *ListenClients_Thread	  PROTO((void *data));
int 	   NewImage_Thread	  PROTO((int BindSocket));
int 	   NewRefcat_Thread	  PROTO((int BindSocket));
int 	   NewReflist_Thread	  PROTO((int BindSocket));
# endif

int args_skycells (int argc, char **argv);
int ConfigInit_skycells (int *argc, char **argv);
int UpdateImageIDs (Catalog *catalog, Image *images, off_t Nimages);

int CheckDuplicateImageIDs (Image *images, off_t Nimages);
int ImageIndexFileInit ();

Catalog *LoadDataPMM (FILE *f, char *file, Image **images, off_t *nvalid);

PhotCode *LoadMetadataPMM (char *datafile, Image *image);
time_t pmm_date_to_sec (char *date, char *time);
double pmm_get_ra (char *RA);
double pmm_get_dec (char *DEC);
PhotCode *pmm_get_photcode (char *emulsion, char *filter);

#define PSPS_ID TRUE
uint64_t CreatePSPSDetectionID(double tobs, int ccdid, int detID);
uint64_t CreatePSPSObjectID(double ra, double dec);
uint64_t CreatePSPSStackDetectionID(int sourceID, int imageID, int detID);

int altaz (double *alt, double *az, double ha, double dec, double latitude);

// this is a gnu extension?? caution!
void *memrchr(const void *s, int c, size_t n);
int addstar_create_ID ();

void initMosaicCoords ();
void saveMosaicCoords (Coords *input);

Catalog *ReadStarsUKIRT (FILE *f, char *name, Header *header, Image *images, off_t *nimages, SkyRegion *region);
Catalog *LoadDataUKIRT (FILE *f, char *file, Image **images, off_t *nimages, Header **headers, off_t *extsize, HeaderSet *headerSets, off_t NheaderSets, SkyRegion *region);

AddstarFile *LoadFilenames (int *nfile, char *filename, AddstarClientOptions *options);
void AddstarFileFree (AddstarFile *file, int Nfile);

Catalog *addstar_catalog_init (int Nstars);

void resort_catalog_measure (Catalog *catalog);
void resort_catalog_lensing (Catalog *catalog);
void resort_catalog_starpar (Catalog *catalog);
void resort_catalog_galphot (Catalog *catalog);

void GetConfig (char *config, char *field, char *format, int N, void *ptr);

/** 
    there is an inconsistency to be resolved: fixed structures (like Image)
    need a fixed bit-length time (e_time), but these functions all use the
    UNIX time_t types, which may be 32 or 64 bits, depending on the machine.
    This can be resolved by using time_t with these functions, but casting 
    between e_time and time_t when necessary (ie, cannot return data to an
    e_time pointer from one of these functions)
**/


/** function for client / server **/ 

