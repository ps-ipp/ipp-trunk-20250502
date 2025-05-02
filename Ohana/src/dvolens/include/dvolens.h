# include <ohana.h>
# include <dvo.h>
# include <kapa.h>
# include <signal.h>
# include <pthread.h>

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

// a structure to define a sequence lookup
typedef struct {
  int minID;
  int maxID;
  int *value;
  int *sequence;
  int Nsequence;
  int NSEQUENCE;
} mySequenceType;

typedef enum {
  MODE_ERROR = 0,
  MODE_UPDATE_OBJECTS,
} DvoLensMode;

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char  *CATDIR;
char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char   ImageCat[DVO_MAX_PATH];

int    HOST_ID;
char  *HOSTDIR;

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

int    VERBOSE;
int    VERBOSE2;
int    UPDATE;
int    NTHREADS;

int    REPAIR_LENSING_IDS;
int    REPAIR_LENSING_IDS_FROM_WARPS;

DvoLensMode MODE;

SkyRegion UserPatch;
char     *UserCatalog;

/*** dvolens prototypes ***/
void          ConfigInit              PROTO((int *argc, char **argv));
void          GetConfig               PROTO((char *config, char *field, char *format, int N, void *ptr));

DvoLensMode   args                    PROTO((int argc, char **argv));
int           args_client             PROTO((int argc, char **argv));

DvoLensMode   initialize              PROTO((int argc, char **argv));
void          initialize_client       PROTO((int argc, char **argv));

void 	      dvolens_usage           PROTO((void));
void 	      dvolens_help            PROTO((int argc, char **argv));

void 	      dvolens_client_usage    PROTO((void));
void 	      dvolens_client_help     PROTO((int argc, char **argv));

int           Shutdown                PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2)) ;
void          TrapSignal              PROTO((int sig));
void          SetProtect              PROTO((int mode));
int           SetSignals              PROTO((void));

int           main                    PROTO((int argc, char **argv));

void          update_objects          PROTO((void));
int           update_objects_parallel PROTO((SkyList *sky));
int           update_objects_catalog  PROTO((Catalog *catalog));

int  	      client_logger_init      PROTO((char *dirname));
int  	      client_logger_message   PROTO((char *format,...));

myIndexType *myIndexAlloc ();
int myIndexFree (myIndexType *myIndex);
void myIndexInit (myIndexType *myIndex);
int myIndexUpdateLimits (myIndexType *myIndex, int value);
int myIndexSetRange (myIndexType *myIndex);
int myIndexSetEntry (myIndexType *myIndex, int value, int entry);
int myIndexGetEntry (myIndexType *myIndex, int value);

int FindWarpGroups (void);
int RecoverLensingIndex (Average *average, mySequenceType *measureSeq, Lensing *lensing);
void FreeWarpGroups (void);

int isGPC1warp (int photcode);

Image *select_images (SkyList *skylist, Image *timage, off_t Ntimage, off_t *Nimage);

int load_images (SkyList *skylist);
void free_images (void);
Image *getimages (off_t *N);
off_t getImageByID (off_t ID);

mySequenceType *mySequenceAlloc ();
int mySequenceFree (mySequenceType *mySequence);
int mySequenceSetSize (mySequenceType *mySequence, int Nmax);
int mySequenceSetValue (mySequenceType *mySequence, int value, int entry);
int mySequenceSort (mySequenceType *mySequence);
int mySequenceGetEntry (mySequenceType *mySequence, int value);
