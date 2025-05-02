# include <ohana.h>
# include <dvo.h>
# include <signal.h>

typedef struct {
  unsigned int start;
  unsigned int stop;
  Coords coords;
} Mosaic;

typedef struct {
  unsigned int     imageID;
  unsigned short   NX;                   // image width
  unsigned short   NY;                   // image height
  e_time           tzero;                // readout time (row 0)
  unsigned char    trate;                // scan rate (100 usec/pixel)
  Coords           coords;
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
int          RESET;
int          UPDATE;
int          PARALLEL;
int          PARALLEL_MANUAL;
int          PARALLEL_SERIAL;

SkyRegion    UserPatch;

/***** prototypes ****/
int           main                              PROTO((int argc, char **argv));
void          ConfigInit                        PROTO((int *argc, char **argv));

void          usage_setposangle                 PROTO((void));
void          initialize_setposangle            PROTO((int argc, char **argv));
int           args_setposangle                  PROTO((int argc, char **argv));

void          usage_setposangle_client          PROTO((void));
void          initialize_setposangle_client     PROTO((int argc, char **argv));
int           args_setposangle_client           PROTO((int argc, char **argv));

// int           update_dvo_setposangle_client  PROTO((ImageSubset *image, off_t Nimage, FlatCorrectionTable *flatcorr));

ImageSubset  *ImageSubsetLoad                   PROTO((char *filename, off_t *nimage));
int           ImageSubsetSave                   PROTO((char *filename, ImageSubset *image, off_t Nimage));
Image        *ImagesFromSubset                  PROTO((ImageSubset *subset, off_t N));
ImageSubset  *ImagesToSubset                    PROTO((Image *image, off_t N));

void          lock_image_db                     PROTO((FITS_DB *db, char *filename));
void          unlock_image_db                   PROTO((FITS_DB *db));
void          create_image_db                   PROTO((FITS_DB *db));
void          set_db                            PROTO((FITS_DB *in));
int           Shutdown                          PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void          TrapSignal                        PROTO((int sig));
void          SetProtect                        PROTO((int mode));
int           SetSignals                        PROTO((void));

Image        *load_images_setposangle           PROTO((FITS_DB *db, off_t *Nimage));
int           update_dvo_setposangle            PROTO((ImageSubset *image, off_t Nimage));
int           update_dvo_setposangle_parallel   PROTO((SkyTable *sky, ImageSubset *image, off_t Nimage));

void          update_catalog_setposangle        PROTO((Catalog *catalog, ImageSubset *image, off_t *index, off_t Nimage));
int           setposangle_local_astrometry      PROTO((float *posAngle, float *pltScale, double x, double y, Coords *mosaic, Coords *coords));

off_t         getMosaicByTimes                  PROTO((unsigned int start, unsigned int stop, unsigned int *startMos, unsigned int *stopMos, off_t *indexMos));
void          sort_mosaic_times                 PROTO((unsigned int *S, unsigned int *E, off_t *I, off_t N));
void          initMosaics                       PROTO((ImageSubset *image, off_t Nimage));
Mosaic       *getMosaicForImage                 PROTO((off_t im));
