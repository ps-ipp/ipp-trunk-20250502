# include <ohana.h>
# include <dvo.h>
# include <signal.h>

typedef struct {
    float zpt;
    float zpt_err;
    e_time time;
    int found;
} ZptTable;

typedef struct {
  float McalPSF;
  float McalAPER;
  float dMcal;
  unsigned int imageID;
  unsigned int photom_map_id;
  unsigned int flags;
} ImageSubset;

// we have one correction (an image) for each filter and chip
typedef struct {
  int Nx;       // number of chips in x
  int Ny;       // number of chips in y
  int Nfilter;  // number of filters
  int Nseason;  // number of correction peridos

  int Nchips;	// chip offset (Nx*Ny)
  int Nflats;	// season offset (Nx*Ny*Nfilters)
  int Nvalues;  // Nx*Ny*Nfilters*Nseason

  int dX;       // superpixel size
  int dY;	// superpixel size

  int NxCCD;	// number of pixels
  int NyCCD;	// number of pixels

  e_time *tstart;
  e_time *tstop;

  Header *phu;
  Matrix **matrix; // allocate an array of pointers
  Header **header; // allocate an array of pointers
  
  // index = ix + iy*Nx + filter*Nchips + dir*Ngroup
} CamPhotomCorrection;

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char         ImageCat[DVO_MAX_PATH];
char        *CATDIR;
int          HOST_ID;
char        *HOSTDIR;
char        *IMAGES;
char        *SINGLE_CPT;

char        *KH_FILE;
char        *DCR_FILE;
char        *CAM_PHOTOM_FILE;
char        *CAM_ASTROM_FILE;

int          KH_RESET;
int          DCR_RESET;
int          CAM_RESET;

char        *SET_GAL_MODEL;

int          VERBOSE;
int          RESET;
int          PHOTCODE_MIN;
int          PHOTCODE_MAX;
int          UBERCAL; // load the supplied ubercal zero point fits table (with flat-field corrections)
int          NO_METADATA; // the supplied ubercal data has no descriptive metadata
int          UPDATE;
int          PARALLEL;
int          PARALLEL_MANUAL;
int          PARALLEL_SERIAL;
int          IMAGES_ONLY;
int          REPAIR_BY_OBJID;
int          SKIP_EXTRA_EXTENSIONS;

SkyRegion    UserPatch;

/***** prototypes ****/
int           main                PROTO((int argc, char **argv));

void          ConfigInit          PROTO((int *argc, char **argv));
void          initialize_setphot  PROTO((int argc, char **argv));
int           args_setphot        PROTO((int argc, char **argv));

void          initialize_setphot_client PROTO((int argc, char **argv));
int           args_setphot_client       PROTO((int argc, char **argv));
int           update_dvo_setphot_client PROTO((ImageSubset *image, off_t Nimage, FlatCorrectionTable *flatcorr));

ImageSubset  *ImageSubsetLoad     PROTO((char *filename, off_t *nimage));
int           ImageSubsetSave     PROTO((char *filename, Image *image, off_t Nimage));
Image        *ImagesFromSubset    PROTO((ImageSubset *subset, off_t N));

void 	      lock_image_db 	  PROTO((FITS_DB *db, char *filename));
void	      unlock_image_db 	  PROTO((FITS_DB *db));
void	      create_image_db 	  PROTO((FITS_DB *db));
void	      set_db 		  PROTO((FITS_DB *in));
int 	      Shutdown 		  PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void 	      TrapSignal 	  PROTO((int sig));
void 	      SetProtect 	  PROTO((int mode));
int 	      SetSignals 	  PROTO((void));

// setphot-specific prototypes
ZptTable     *load_zpt_table         PROTO((char *filename, int *nzpts));
Image        *load_images_setphot    PROTO((FITS_DB *db, off_t *Nimage));
int           match_zpts_to_images   PROTO((Image *image, off_t Nimage, ZptTable *zpts, int Nzpts));
int           update_dvo_setphot     PROTO((Image *image, off_t Nimage, CamPhotomCorrection *camcorr));
void          update_catalog_setphot PROTO((Catalog *catalog, Image *image, off_t *index, off_t Nimage, CamPhotomCorrection *camcorr));
void          update_catalog_setphot_client PROTO((Catalog *catalog, ImageSubset *image, off_t *index, off_t Nimage, FlatCorrectionTable *flatcorr));

ZptTable     *load_zpt_ubercal            PROTO((char *filename, int *nzpts, FlatCorrectionTable *flatcorrTable));
ZptTable     *load_zpt_ubercal_nometadata PROTO((char *filename, int *nzpts, FlatCorrectionTable *flatcorrTable));
int           match_flatcorr_to_images    PROTO((Image *image, off_t Nimage, FlatCorrectionTable *flatcorrTable));

int           parse_zpt_offsets           PROTO((char *ZPT_OFFSET_FILTERS, char *ZPT_OFFSET_VALUES));
float         apply_zpt_offset            PROTO((short code));

int           repair_catalog_by_objID     PROTO((Catalog *catalog));

void          CamPhotomCorrectionFree            PROTO((CamPhotomCorrection *cam));
CamPhotomCorrection *CamPhotomCorrectionAlloc    PROTO((int Nx, int Ny, int Nfilter, int Nseason));
CamPhotomCorrection *CamPhotomCorrectionLoad     PROTO((char *filename));
int            CamPhotomCorrectionSave           PROTO((CamPhotomCorrection *cam, char *filename));
float          CamPhotomCorrectionValue          PROTO((CamPhotomCorrection *cam, int flat_id, float Xccd, float Yccd));

int            match_camcorr_to_images      PROTO((Image *image, off_t Nimage, CamPhotomCorrection *camcorr));
CamPhotomCorrection *merge_flatcorr_and_camcorr   PROTO((FlatCorrectionTable *flatcorr, CamPhotomCorrection *camcorr));

// spline correction functions
double spline_apply_dbl (double *x, double *y, double *y2, int N, double X);

int load_kh_correction (char *filename);
int get_kh_correction (int sub, int chip, double *dX, double *dY, float Minst);

int load_dcr_correction (char *filename);
int get_dcr_correction (int filter, double *dX, double *dY, float Color);

int CamAstromCorrectionLoad (char *filename);
int CamAstromCorrectionValue (int chipID, int filter, float Xccd, float Yccd, double *dX, double *dY);

int           update_catalog_setgalmodel        PROTO((Catalog *catalog));
int           update_catalog_setastrom        PROTO((Catalog *catalog));
