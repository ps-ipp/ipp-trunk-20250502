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

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

int    HOST_ID;
char  *HOSTDIR;

int    PARALLEL_INPUT;

int    VERBOSE;
int    VERIFY;
int    VERIFY_CATALOG_ONLY;
int    IMAGES_ONLY;
int    ACCEPT_MOTION;

int    ACCEPT_ASTROM;
int    RETAIN_AVE_PHOTOMETRY;

char   CATDIR[DVO_MAX_PATH];
char   GSCFILE[DVO_MAX_PATH];
char   ImageCat[DVO_MAX_PATH];
char   CATMODE[256];    /* raw, mef, split, mysql */
char   CATFORMAT[256];  /* internal, elixir, loneos, panstarrs */
float  RADIUS;
int    SKY_DEPTH;
int    NTHREADS;
int    REPLACE_BY_PHOTCODE;
int    REPLACE_TYCHO;
int    FORCE_MERGE;
int    ALLOW_MISSING_INPUT_IMAGES;

int    MAX_CLIENTS;
char  *ALTERNATE_PHOTCODE_FILE;
char  *UPDATE_CATFORMAT;
char  *UPDATE_CATCOMPRESS;

char  *CPTLIST_FILENAME;
char **CPTLIST;
int   NCPTLIST;

int    MATCH_BY_EXTERN_ID;

int    ONLY_MATCHES;

int    MATCHED_TABLES;
int    RESET_STARPAR;
int    RESET_LENSING;

int    SKIP_IMAGES; 
int    SKIP_MEASURE;
int    SKIP_MISSING;
int    SKIP_LENSING;
int    SKIP_LENSOBJ;
int    SKIP_STARPAR;
int    SKIP_GALPHOT;

int    REPAIR_BY_OBJID;

char *SINGLE_CPT;
SkyRegion UserPatch;  // used by MODE CAT

# ifndef MAX_INT
# define MAX_INT 2147483647
# endif

typedef struct {
  int minID;
  int maxID;
  int *index;
  int Nindex;
  int NINDEX;
} myIndexType;

typedef struct {
  unsigned int Nmap;
  unsigned int *old;
  unsigned int *new;
  unsigned int  oldIDmax;
  int *notFoundMeasure;
  int *notFoundLensing;
} IDmapType;

// struct to describe a sequence of dvomerges (Image.dat header)
typedef struct {
  int Nmerge;
  char **IDs;
} dmhImage;

// struct to describe a sequence of dvomerges (Object table : cpt header)
typedef struct {
  int   Nmerge;
  off_t  *size;
  time_t *time;
  char  **date;
} dmhObject;

// data on a single table, populated when a new file is merged
typedef struct {
  off_t  size;
  time_t time;
  char  *date;
} dmhObjectStats;

// struct to describe the current status of a single output file:
// is it on this machine (valid)? have we already merged or not (missed)?
// what is the collection of past merges (history)?  what is the real filename?
typedef struct {
  int valid;		      // is this object table on this machine?
  int missed;		      // did we fail to merge into this table yet?
  dmhObject *history;	      // complete sequence of previous merges
  char *filename;	      // true filename on disk 
} OutputStatus;

int        main                   PROTO((int argc, char **argv));
int        dvomergeCreate         PROTO((int argc, char **argv));
int        dvomergeUpdate         PROTO((int argc, char **argv));

int        ConfigInit             PROTO((int *argc, char **argv));
void       GetConfig              PROTO((char *config, char *field, char *format, int N, void *ptr));

int        SetSignals             PROTO((void));
void       SetProtect             PROTO((int mode));
void       TrapSignal             PROTO((int sig));
int        Shutdown               PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);

void       dvomerge_usage         PROTO((void));
void 	   dvomerge_help          PROTO((int argc, char **argv));
int  	   dvomerge_args 	  PROTO((int *argc, char **argv));
void       dvomerge_args_free     PROTO((void));

void       dvomerge_client_usage  PROTO((void));
void 	   dvomerge_client_help   PROTO((int argc, char **argv));
int  	   dvomerge_client_args   PROTO((int *argc, char **argv));
void       dvomerge_client_args_free PROTO((void));

void       dvoconvert_usage       PROTO((void));
void 	   dvoconvert_help 	  PROTO((int argc, char **argv));
int  	   dvoconvert_args	  PROTO((int *argc, char **argv));

void       dvosecfilt_usage       PROTO((void));
void 	   dvosecfilt_help 	  PROTO((int argc, char **argv));
int  	   dvosecfilt_args	  PROTO((int *argc, char **argv));

void       dvosecfilt_client_usage PROTO((void));
void 	   dvosecfilt_client_help  PROTO((int argc, char **argv));
int  	   dvosecfilt_client_args  PROTO((int *argc, char **argv));

int        dvosecfilt_catalogs     PROTO((int Nsecfilt));
int        dvosecfilt_parallel     PROTO((SkyTable *insky, int Nsecfilt));

int        SkyTablePopulatedRange PROTO((off_t *ns, off_t *ne, SkyTable *sky, off_t Nstart));
int        SkyListPopulatedRange  PROTO((off_t *ns, off_t *ne, SkyList *sky, off_t Nstart));

SkyList   *SkyTablePopulatedList  PROTO((SkyTable *sky));
SkyList   *SkyTablePopulatedList_old  PROTO((SkyTable *sky, off_t Ns, off_t Ne));

int        LoadCatalog            PROTO((Catalog *catalog, SkyRegion *region, char *filename, char *mode, int Nsecfilt));

int        merge_catalogs_new     PROTO((SkyRegion *region, Catalog *output, Catalog *input, int *secflitMap));
int        merge_catalogs_old     PROTO((SkyRegion *region, Catalog *output, Catalog *input, double RADIUS, int *secflitMap));

off_t 	  *build_measure_links    PROTO((Average *average, off_t Naverage, Measure *measure, off_t Nmeasure));
off_t 	  *init_measure_links     PROTO((Average *average, off_t Naverage, Measure *measure, off_t Nmeasure));
int   	   add_measure_link    	  PROTO((Average *average, off_t *next, off_t Nmeasure, off_t NMEASURE));
Measure   *sort_measure     	  PROTO((Average *average, off_t Naverage, Measure *measure, off_t Nmeasure, off_t *next));

off_t 	  *build_lensing_links    PROTO((Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing));
off_t 	  *init_lensing_links     PROTO((Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing));
int   	   add_lensing_link    	  PROTO((Average *average, off_t *next, off_t Nlensing, off_t NLENSING));
Lensing   *sort_lensing     	  PROTO((Average *average, off_t Naverage, Lensing *lensing, off_t Nlensing, off_t *next));

off_t 	  *build_lensobj_links    PROTO((Average *average, off_t Naverage, Lensobj *lensobj, off_t Nlensobj));
off_t 	  *init_lensobj_links     PROTO((Average *average, off_t Naverage, Lensobj *lensobj, off_t Nlensobj));
int   	   add_lensobj_link    	  PROTO((Average *average, off_t *next, off_t Nlensobj, off_t NLENSOBJ));
Lensobj   *sort_lensobj     	  PROTO((Average *average, off_t Naverage, Lensobj *lensobj, off_t Nlensobj, off_t *next));

off_t 	  *build_starpar_links    PROTO((Average *average, off_t Naverage, StarPar *starpar, off_t Nstarpar));
off_t 	  *init_starpar_links     PROTO((Average *average, off_t Naverage, StarPar *starpar, off_t Nstarpar));
int   	   add_starpar_link     	  PROTO((Average *average, off_t *next, off_t Nstarpar, off_t NSTARPAR));
StarPar   *sort_starpar     	  PROTO((Average *average, off_t Naverage, StarPar *starpar, off_t Nstarpar, off_t *next));

off_t 	  *build_galphot_links    PROTO((Average *average, off_t Naverage, GalPhot *galphot, off_t Ngalphot));
off_t 	  *init_galphot_links     PROTO((Average *average, off_t Naverage, GalPhot *galphot, off_t Ngalphot));
int   	   add_galphot_link     	  PROTO((Average *average, off_t *next, off_t Ngalphot, off_t NGALPHOT));
GalPhot   *sort_galphot     	  PROTO((Average *average, off_t Naverage, GalPhot *galphot, off_t Ngalphot, off_t *next));

off_t 	  *init_missing_links     PROTO((Average *average, off_t Naverage, Missing *missing, off_t Nmissing));
int   	   add_missing_link     	  PROTO((Average *average, off_t *next, off_t Nmissing));
Missing   *sort_missing     	  PROTO((Average *average, off_t Naverage, Missing *missing, off_t Nmissing, off_t *next_miss));

uint64_t   CreatePSPSDetectionID  PROTO((double tobs, int ccdid, int detID));
uint64_t   CreatePSPSObjectID     PROTO((double ra, double dec));

int        dvomergeImagesCreate   PROTO((IDmapType *IDmap1, char *input1, IDmapType *IDmap2, char *input2, char *output));
int        dvomergeImagesUpdate   PROTO((IDmapType *IDmap, char *input, char *output));
int        dvo_image_merge_dbs    PROTO((IDmapType *IDmap, FITS_DB *out, FITS_DB *in));
off_t 	   dvo_map_image_ID       PROTO((IDmapType *IDmap, off_t oldID));
int   	   dvo_update_image_IDs   PROTO((IDmapType *IDmap, Catalog *catalog));
void       dvo_image_map_free     PROTO((IDmapType *IDmap));
void       dvo_image_map_init     PROTO((IDmapType *IDmap));

// dvorepair prototypes
Image     *LoadImages         	  PROTO((FITS_DB *db, char *filename, off_t *Nimage));
Image     *MatchImage         	  PROTO((Image *image, off_t Nimage, unsigned int time, short int source, unsigned int imageID));
off_t      match_image        	  PROTO((Image *image, off_t Nimage, unsigned int T, short int S));

int        SaveImages             PROTO((FITS_DB *oldDB, char *filename, Image *imagesOut, off_t Nout));

int 	   dvorepairFixCPT        PROTO((int argc, char **argv));
int 	   dvorepairImagesVsMeasures PROTO((int argc, char **argv));
int 	   dvorepairDeleteImageList PROTO((int argc, char **argv));
int 	   dvorepairFixImages     PROTO((int argc, char **argv));

int       *ReadDeleteList         PROTO((char *filename, int *nindex));
int 	   RepairTableCPT         PROTO((char *cptFilenameSrc, char *cptFilenameTgt, char *cpsFilenameSrc, char *cpsFilenameTgt, Measure *measure, off_t Nmeasure, Image *image, off_t Nimage, char catformat));
void       dvorepair_help         PROTO((int argc, char **argv));

off_t      getTgtIndex            PROTO((e_time start, e_time stop, short photcode, off_t *TgtIndex, e_time *TgtTimes, short *TgtCodes, off_t NimagesTgt));
void       SortTgtByTimes         PROTO((e_time *S, off_t *I, short *C, off_t N));
int 	   dvomergeImagesGetMap   PROTO((IDmapType *IDmap, char *input, char *output));
int        dvomergeFromList       PROTO((int argc, char **argv));
int 	   dvomergeUpdate_threaded PROTO((int argc, char **argv));
int        dvomergeUpdate_catalogs PROTO((char *input, char *output, SkyList *inlist, SkyTable *outsky, int NsecfiltInput, int NsecfiltOutput, IDmapType *IDmap, int *secfiltMap));

int 	   dvo_image_match_dbs_by_time_and_photcode PROTO((IDmapType *IDmap, FITS_DB *tgt, FITS_DB *src));
int 	   dvo_image_match_dbs_by_extern_id         PROTO((IDmapType *IDmap, FITS_DB *tgt, FITS_DB *src));

int        replace_match           PROTO((Average *average_out, Measure *measure_out, off_t *next_meas, Average *average_in, Measure *measure_in));

int        IDmapSave               PROTO((char *filename, IDmapType *IDmap));
IDmapType *IDmapLoad               PROTO((char *filename));
int        create_IDmap_lookup     PROTO((IDmapType *IDmap));
void       dvo_report_image_IDs    PROTO((IDmapType *IDmap));

// dvomerge history functions
OutputStatus *OutputStatusInit (int N);
int OutputStatusFree (OutputStatus *outstat, int N);

int dmhObjectAdd (dmhObject *history, Header *header, dmhObjectStats *inStats);
int dmhObjectCheck (dmhObject *history, dmhObjectStats *inStats);
dmhObject *dmhObjectRead (char *filename);
dmhObject *dmhObjectAlloc (void);
void dmhObjectFree (dmhObject *history);

void dmhObjectStatsFree (dmhObjectStats *stats);
dmhObjectStats *dmhObjectStatsRead (char *filename);

int dmhImageAdd (FITS_DB *db, dmhImage *history, char *dbID);
dmhImage *dmhImageRead (FITS_DB *db);
int dmhImageCheck (dmhImage *history, char *dbID);
void dmhImageFree(dmhImage *history);

char *dmhImageReadID (FITS_DB *db);
int dvoCreateID (char *catdir);

myIndexType *myIndexInit ();
int myIndexFree (myIndexType *myIndex);
int myIndexUpdateLimits (myIndexType *myIndex, int value);
int myIndexSetRange (myIndexType *myIndex);
int myIndexSetEntry (myIndexType *myIndex, int value, int entry);
int myIndexGetEntry (myIndexType *myIndex, int value);

int dvorepairDeleteImagesByExternID (int argc, char **argv);
int RepairTableCPT_V1(char *cptFilenameSrc, char *cptFilenameTgt, char *cpsFilenameSrc, char *cpsFilenameTgt, Measure *measure, off_t Nmeasure, Image *image, off_t Nimage, myIndexType *imageIDindex, char catformat);
int *ReadDeleteListExternID(char *filename, int *nindex);

int dvorepairDeleteImagesByExternID_v2 (int argc, char **argv);
int *ReadDeleteListExternID_v2(char *filename, int *nindex);

void replace_tycho_init ();
int  replace_tycho (Average *averageInp, Measure *measureInp, off_t *next_meas, Average *averageOut, Measure *measureOut);
int repair_catalog_by_objID (Catalog *catalog);

int dvorepair_by_objID (int argc, char **argv);
int dvorepairFixWarpIDs (int argc, char **argv);
int dvorepairFixStackIDs (int argc, char **argv);

char **load_cptlist (char *filename, int *nlist);
int ResetStarPar (Catalog *catalog);
int ResetLensing (Catalog *catalog);
