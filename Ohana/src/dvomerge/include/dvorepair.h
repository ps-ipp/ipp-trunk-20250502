# include <ohana.h>
# include <dvo.h>
# include <signal.h>
# include <sys/time.h>
# include <time.h>
# include <zlib.h>

/* linux is happy with this, not solaris */
# include <netinet/ip.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <glob.h>

# ifndef MAX_INT
# define MAX_INT 2147483647
# endif

typedef enum {
  DVOREPAIR_MODE_NONE = 0,
  DVOREPAIR_MODE_FixWarpIDs,
  DVOREPAIR_MODE_FixStackIDs,
  DVOREPAIR_MODE_FixCPT,
  DVOREPAIR_MODE_BY_OBJ_ID,
  DVOREPAIR_MODE_ImagesVsMeasures,
  DVOREPAIR_MODE_DeleteImageList,
  DVOREPAIR_MODE_DeleteImagesByExternID,
  DVOREPAIR_MODE_DeleteImagesByExternID_v2,
  DVOREPAIR_MODE_FixImages,
} dvorepairModes;

typedef struct {
  int minID;
  int maxID;
  int *index;
  int Nindex;
  int NINDEX;
} myIndexType;

typedef struct {
  myIndexType *imageIDindex;
  int *deleteImage;
  int Nimage;
} DeleteImageDataType;

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

int    HOST_ID;
char  *HOSTDIR;

int    VERBOSE;
char  *CATDIR;

// SkyRegion UserPatch[2];  // used by MODE CAT
// int      nUserPatch;

dvorepairModes MODE;

int          main                    PROTO((int argc, char **argv));
	  
void         dvorepair_help          PROTO((int argc, char **argv));
int  	     dvorepair_args 	     PROTO((int *argc, char **argv));
	  
void 	     dvorepair_client_help   PROTO((int argc, char **argv));
int  	     dvorepair_client_args   PROTO((int *argc, char **argv, SkyRegion *UserPatch));
	  
int 	     dvorepairFixCPT           PROTO((int argc, char **argv));
int 	     dvorepairImagesVsMeasures PROTO((int argc, char **argv));
int 	     dvorepairDeleteImageList  PROTO((int argc, char **argv));
int 	     dvorepairFixImages        PROTO((int argc, char **argv));
	  
int         *ReadDeleteList          PROTO((char *filename, int *nindex));
int 	     RepairTableCPT          PROTO((char *cptFilenameSrc, char *cptFilenameTgt, char *cpsFilenameSrc, char *cpsFilenameTgt, Measure *measure, off_t Nmeasure, Image *image, off_t Nimage, char catformat));

myIndexType *myIndexInit             PROTO((void));
int          myIndexFree             PROTO((myIndexType *myIndex));
int          myIndexUpdateLimits     PROTO((myIndexType *myIndex, int value));
int          myIndexSetRange         PROTO((myIndexType *myIndex));
int          myIndexSetEntry         PROTO((myIndexType *myIndex, int value, int entry));
int          myIndexGetEntry         PROTO((myIndexType *myIndex, int value));
           
int          RepairTableCPT_V1       PROTO((char *cptFilenameSrc, char *cptFilenameTgt, char *cpsFilenameSrc, char *cpsFilenameTgt, Measure *measure, off_t Nmeasure, Image *image, off_t Nimage, myIndexType *imageIDindex, char catformat));
int         *ReadDeleteListExternID  PROTO((char *filename, int *nindex));
           
int          dvorepair_by_objID      PROTO((int argc, char **argv));
int          dvorepairFixWarpIDs     PROTO((int argc, char **argv));
int          dvorepairFixStackIDs    PROTO((int argc, char **argv));


int          dvorepairDeleteImagesByExternID    PROTO((int argc, char **argv));
int          dvorepairDeleteImagesByExternID_v2 PROTO((int argc, char **argv));
int         *ReadDeleteListExternID_v2          PROTO((char *filename, int *nindex));
           
int          DeleteImagesSave                   PROTO((char *filename, Image *image, off_t Nimage, int *deleteImage));
int          DeleteImagesLoad                   PROTO((char *filename, DeleteImageDataType *deleteImageData));

int          FindDeleteRegion                   PROTO((SkyRegion *UserPatch, Image *image, off_t Nimage, int *deleteImage));

int          dvorepairDeleteImagesByExternID_catalogs PROTO((SkyRegion *UserPatch, int nUserPatch, Image *image, off_t Nimage, int *deleteImage, myIndexType *imageIDindex));
int          dvorepairDeleteImagesByExternID_parallel PROTO((SkyRegion *UserPatch, int nUserPatch, Image *image, off_t Nimage, int *deleteImage));

int        SetSignals             PROTO((void));
void       SetProtect             PROTO((int mode));
void       TrapSignal             PROTO((int sig));
int        Shutdown               PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);

Image     *LoadImages         	  PROTO((FITS_DB *db, char *filename, off_t *Nimage));
uint64_t   CreatePSPSDetectionID  PROTO((double tobs, int ccdid, int detID));
uint64_t   CreatePSPSObjectID     PROTO((double ra, double dec));

