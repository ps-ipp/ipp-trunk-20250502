# include <ohana.h>
# include <dvo.h>
# include <signal.h>

// to determine the image ID, we need time (tzero + trate) and photcode
typedef struct {
  unsigned int     imageID;
  unsigned short   NX;                   // image width
  unsigned short   NY;                   // image height
  unsigned int     nstar;                // number of stars on the image
  e_time           tzero;                // readout time (row 0)
  unsigned char    trate;                // scan rate (100 usec/pixel)
  short            photcode;
} ImageSubset;

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char         ImageCat[DVO_MAX_PATH];
char        *CATDIR;
int          HOST_ID;
char        *HOSTDIR;
char        *IMAGES;
char        *SINGLE_CPT;
int          VERBOSE;
int          VERBOSE_IMSTATS;
int          SUMMARY_IMSTATS;
char        *IMSTATS_FILE; // location of partial imstats file for remote client
int          UPDATE;
int          PARALLEL;
int          PARALLEL_MANUAL;
int          PARALLEL_SERIAL;

SkyRegion    UserPatch;

/***** prototypes ****/
int           main                              PROTO((int argc, char **argv));
void          ConfigInit                        PROTO((int *argc, char **argv));

void          usage_fiximids                    PROTO((void));
void          initialize_fiximids               PROTO((int argc, char **argv));
int           args_fiximids                     PROTO((int argc, char **argv));
					        
void          usage_fiximids_client             PROTO((void));
void          initialize_fiximids_client        PROTO((int argc, char **argv));
int           args_fiximids_client              PROTO((int argc, char **argv));

ImageSubset  *ImageSubsetLoad                   PROTO((char *filename, off_t *nimage));
int           ImageSubsetSave                   PROTO((char *filename, ImageSubset *image, off_t Nimage));
ImageSubset  *ImagesToSubset                    PROTO((Image *image, off_t N));

void          set_db                            PROTO((FITS_DB *in));
int           Shutdown                          PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void          TrapSignal                        PROTO((int sig));
void          SetProtect                        PROTO((int mode));
int           SetSignals                        PROTO((void));

Image        *load_images_fiximids              PROTO((FITS_DB *db, off_t *Nimage));
int           update_dvo_fiximids               PROTO((ImageSubset *image, off_t Nimage));
int           update_dvo_fiximids_parallel      PROTO((SkyTable *sky, ImageSubset *image, off_t Nimage));
					        
void          update_catalog_fiximids           PROTO((Catalog *catalog));

void  	      sort_image_times 			PROTO((e_time *S, e_time *E, short *C, unsigned int *I, unsigned int *Q, off_t N));
int           FindImageID                       PROTO((off_t *ID, off_t *Seq, e_time time, short photcode));
void  	      initImageIndex   			PROTO((ImageSubset *image, off_t Nimage_init));

void 	      BumpValidImage     		PROTO((int Seq));
void 	      BumpInvalidImage   		PROTO((int Seq));
void 	      CompareImageCounts 		PROTO((ImageSubset *image, off_t Nimage_comp));
void          SummaryImageStats  		PROTO((ImageSubset *image, off_t Nimage_comp));

int 	      ImageValidSave  		        PROTO((char *filename));
int 	      ImageValidLoad		        PROTO((char *filename));
