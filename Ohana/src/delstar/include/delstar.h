# include <ohana.h>
# include <dvo.h>
# include <signal.h>

// options for generating the IndexArray used to select images for deletion (delete_duplicate_images.c)
enum {NONE, EXTERN_ID, IMAGE_ID};

typedef struct {
  Coords coords;
  float *X, *Y;
  int *N;
  double RA[2], DEC[2];
  double Area, density, spacing;
} CatStats;

typedef struct {
  off_t minID;
  off_t maxID;
  off_t range;
  off_t *value;
} IndexArray;

// to determine the image ID, we need time (tzero + trate) and photcode
typedef struct {
  unsigned int     imageID;
  unsigned int     externID;
  unsigned short   NX;                   // image width
  unsigned short   NY;                   // image height
  unsigned int     nstar;                // number of stars on the image
  e_time           tzero;                // readout time (row 0)
  e_time           tmin;                 // start exp - 1
  e_time           tmax;                 // stop exp + 1
  unsigned char    trate;                // scan rate (100 usec/pixel)
  short            photcode;
} ImageSubset;

typedef struct {
  double R;
  double D;
  int objID;
  int catID;
  int detID;
  int imageID;
} MeasureEdge;

typedef struct {
  off_t NdelWarp;
  off_t NdelChip;
  off_t NdelStack;
  off_t NdelOther;
  off_t NdelAves;
  off_t NdelMeas;
} DeleteMeasureResult;

/* global variables set in parameter file */
char   ImageCat[DVO_MAX_PATH];
char   ImageTemplate[DVO_MAX_PATH];
char   CatTemplate[DVO_MAX_PATH];
char   GSCFILE[DVO_MAX_PATH];

char  *CATDIR;

char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */

char  *IMAGENAME;
char  *IMAGES;
char  *IMSTATS_FILE;
char  *MEASURE_EDGE_FILE;
char  *EDGE_DELETIONS;

double NSIGMA;
double ALPHA;
int    VERBOSE;
int    VERBOSE2;
int    UPDATE;
int    SKIP_DIFF_PAIRS;
int    IMAGE_DETAILS;
int    IMAGE_DUPLICATES_BY_OBSTIME;
int    IMAGE_ONLY;
int    ORPHAN;
int    MISSED;
char   SKY_TABLE[DVO_MAX_PATH];
int    SKY_DEPTH;  /** XXX EAM : depth of catalog tables, fix usage */

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

int          HOST_ID;
char        *HOSTDIR;
char        *SINGLE_CPT;

int   SAVE_DUPLICATES;
int   SAVE_DELETES;
int   SKIP_IMAGES;
char *BACKUP_EXTNAME;
char *DELLIST_FILENAME;

time_t    START;
time_t    END;
PhotCode *PHOTCODE;

char *PHOTCODE_LIST;
char *UNIQUER;

int    MODE;
enum {MODE_NONE, MODE_IMAGENAME, MODE_IMAGEFILE, MODE_TIME, MODE_ORPHAN, MODE_MISSED, MODE_PHOTCODES, MODE_DUP_IMAGES, MODE_DUP_MEASURES, MODE_DELETE_MEASURES_BY_MATCH, MODE_DELETE_MEASURES_BY_DETID, MODE_FIX_LAP, MODE_FIX_LAP_STATS, MODE_FIX_LAP_EDGES, MODE_FIX_LAP_EDGES_DELETE};

char DateKeyword[64], DateMode[64], UTKeyword[64], MJDKeyword[64], JDKeyword[64];

SkyRegion UserPatch;

// for DELETE_MEASURES_BY_MATCH, these are the ranges to delete:
int DELETE_MIN_DET_ID;
int DELETE_MAX_DET_ID;
int DELETE_MIN_CAT_ID;
int DELETE_MAX_CAT_ID;

int DELETE_MIN_IMAGE_ID;
int DELETE_MAX_IMAGE_ID;
int DELETE_MIN_PHOTCODE;
int DELETE_MAX_PHOTCODE;

int DELETE_MIN_TIME;
int DELETE_MAX_TIME;

/*** delstar prototypes ***/
void       ConfigInit             PROTO((int *argc, char **argv));
int        FindDecBand            PROTO((double dec, double *DEC0, double *DEC1));
FILE      *GetDB                  PROTO((int *state));
Image     *GetImages              PROTO((off_t *nimage));
int        SetImages              PROTO((Image *new, off_t Nnew));
void       SetProtect             PROTO((int mode));
int        SetSignals             PROTO((void));
int        Shutdown               PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);
void       TrapSignal             PROTO((int sig));
int        args                   PROTO((int argc, char **argv));
void       check_permissions      PROTO((char *basefile));
void       delete_imagefile       PROTO((FITS_DB *db));
void       delete_imagename       PROTO((FITS_DB *db));
void       delete_times           PROTO((FITS_DB *db));
int        edge_check             PROTO((double *x1, double *y1, double *x2, double *y2));
off_t     *find_images_data       PROTO((FITS_DB *db, Image *timage, off_t *nlist));
off_t     *find_images_name       PROTO((FITS_DB *db, char *filename, off_t *nlist));
off_t     *find_images_time       PROTO((FITS_DB *db, e_time start, e_time end, PhotCode *code, off_t *nlist));
void       find_matches           PROTO((Catalog *catalog, int photcode, e_time start, e_time end));
int        gcatalog               PROTO((Catalog *catalog));
Image     *gimages                PROTO((char *filename));
Image     *gtimes                 PROTO((off_t *NIMAGE));
void       help                   PROTO((void));
int        load_image_db          PROTO((FITS_DB *db));
void       lock_image_db          PROTO((FITS_DB *db, char *filename));
void       match_images           PROTO((Catalog *catalog, Image *image, off_t Nimage));
double     opening_angle          PROTO((double x1, double y1, double x2, double y2, double x3, double y3));
e_time     parse_time             PROTO((Header *header));
int        save_image_db          PROTO((void));
void       sort_lists             PROTO((float *X, float *Y, int *S, int N));
void       unlock_image_db        PROTO((FITS_DB *db));
void       usage                  PROTO((void));
int        wcatalog               PROTO((Catalog *catalog));

void set_db (FITS_DB *in);

int args_client (int argc, char **argv);

void delstar_args_free ();
void delstar_client_args_free ();

void SortAveMeasMatch (off_t *MEAS, off_t *AVE, off_t N);

int delete_photcodes (void);
int delete_photcodes_parallel (SkyList *sky);
int delete_photcodes_catalog (Catalog *catalog, PhotCode **photcodes, int Nphotcodes);
int delete_image_photcodes (FITS_DB *db);
int delete_photcodes_single (char *cptname);

int delete_duplicate_images (FITS_DB *db);
int delete_duplicate_image_measures (IndexArray *imageID);
int delete_duplicate_image_measures_parallel (SkyList *sky, IndexArray *imageID);
int delete_duplicate_image_measures_catalog (Catalog *catalog, IndexArray *imageID);

IndexArray *find_duplicates (Image *image, off_t Nimage, off_t *Nduplicates);
IndexArray *make_index_array (Image *image, off_t Nimage, int mode);

IndexArray *ImageIDLoad(char *filename);
int ImageIDSave(char *filename, IndexArray *imageID);

int delete_measures_by_match ();
int delete_measures_by_match_parallel (SkyList *sky);
DeleteMeasureResult delete_measures_by_match_catalog (Catalog *catalog);

int delete_duplicate_measures ();
int delete_duplicate_measures_parallel (SkyList *sky);
DeleteMeasureResult delete_duplicate_measures_catalog (Catalog *catalog);

int delete_fix_LAP (ImageSubset *image, off_t Nimage);
int delete_fix_LAP_catalog (Catalog *catalog, ImageSubset *image, off_t Nimage);
int delete_fix_LAP_measures (off_t *measureDrop, Catalog *catalog, ImageSubset *image, off_t Nimage);
int delete_fix_LAP_parallel (SkyList *sky, ImageSubset *image, off_t Nimage);
int delete_fix_LAP_setstats (ImageSubset *image, off_t Nimage);

void initImageIndex (ImageSubset *image, off_t Nimage_init);
int *getImageIndex (int *maxIndex);
int FindIDexp (off_t *ID, off_t *Seq, e_time time, short photcode);
int FindIDstk (off_t *ID, off_t *Seq, short *photcode, off_t extID);

ImageSubset *ImageSubsetLoad(char *filename, off_t *nimage);
int ImageSubsetSave(char *filename, ImageSubset *image, off_t Nimage);
ImageSubset *ImagesToSubset (Image *image, off_t N);

void sort_fullIDs (uint64_t *I, int *S, off_t N);

int ImageValidSave(char *filename);
int ImageValidLoad(char *filename);
void SummaryImageStats (ImageSubset *image, off_t Nimage_comp);
void SummaryImageStats_Full (Image *image, off_t Nimage_comp);
void BumpInvalidImage (int Seq);
void BumpValidImage (int Seq);

MeasureEdge *delete_fix_LAP_edges (off_t *nmeasure_edge);
MeasureEdge *delete_fix_LAP_edges_parallel (SkyList *sky, off_t *nmeasure_edge);
MeasureEdge *delete_fix_LAP_edges_get_measures (Catalog *catalog, MeasureEdge *measure_edge, off_t *Nmeasure_edge_in, off_t *NMEASURE_EDGE_IN);
int          delete_fix_LAP_edges_find_dups (MeasureEdge *measure_edge, off_t Nmeasure_edge);

MeasureEdge *MeasureEdgeMerge (MeasureEdge *measure_edge_all, off_t *Nmeasure_edge_in, off_t *NMEASURE_EDGE_IN, MeasureEdge *measure_edge, off_t Nmeasure_edge);
int MeasureEdgeSave(char *filename, MeasureEdge *measure_edge, off_t Nmeasure_edge);
MeasureEdge *MeasureEdgeLoad(char *filename, off_t *Nmeasure_edge);

int delete_fix_LAP_edges_delete ();
int delete_fix_LAP_edges_drop_measures (Catalog *catalog, MeasureEdge *measure_edge, off_t Nmeasure_edge, int catIDcount);
int delete_fix_LAP_edges_delete_parallel (SkyList *sky);

typedef struct {
  int  catID;
  int *objID;
  int *detID;
  int *imageID;
  int NdetID;
} CatIDGroup;

int dvo_catalog_subset_backup (Catalog *catalog, char *suffix);
void isortthree (int *X, int *Y, int *Z, int N);
int thisCatID (int *catID, int NcatID, int index);
int needCatID (int *catID, int NcatID, int index);
int delete_measures_by_detID_catalog (Catalog *catalog, CatIDGroup *catIDgroup);
int delete_measures_by_detID_parallel (SkyList *sky);
int delete_measures_by_detID ();

