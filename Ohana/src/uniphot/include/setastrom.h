# include <ohana.h>
# include <dvo.h>
# include <signal.h>

// the interpolating spline information: this is based on the spline used by opihi/cmd.data/spline.c
typedef struct {
  int Nknots;
  double *xk;
  double *yk;
  double *y2;
} Spline;

// we have one correction (an image) for each filter and chip
typedef struct {
  int Nx;       // number of chips in x
  int Ny;       // number of chips in y
  int Nfilter;  // number of filters
  int Ndir;     // number of correction dimensions

  int Nchips;	// chip offset (Nx*Ny)
  int Ngroup;	// direction offset (Nx*Ny*Nfilters)
  int Nvalues;  // Nx*Ny*Nfilters*Ndir

  int dX;       // superpixel size
  int dY;	// superpixel size

  int NxCCD;	// number of pixels
  int NyCCD;	// number of pixels

  Matrix **matrix; // allocate an array of pointers
  // index = ix + iy*Nx + filter*Nchips + dir*Ngroup
} CamAstromCorrection;

/* global variables set in parameter file */
# define DVO_MAX_PATH 1024
char         ImageCat[DVO_MAX_PATH];
char        *CATDIR;
int          HOST_ID;
char        *HOSTDIR;
int          VERBOSE;
int          UPDATE;
int          PARALLEL;
int          PARALLEL_MANUAL;
int          PARALLEL_SERIAL;
char        *UPDATE_CATFORMAT;
char        *SINGLE_CPT;

char        *KH_FILE;
char        *DCR_FILE;
char        *TYC_FILE;

char        *CAM_ASTROM_FILE;

int          KH_RESET;
int          DCR_RESET;
int          CAM_RESET;
int          TYC_RESET;

SkyRegion    UserPatch;

/***** prototypes ****/
int           main                            PROTO((int argc, char **argv));

void          GetConfig                       PROTO((char *config, char *field, char *format, int N, void *ptr));
void          ConfigInit                      PROTO((int *argc, char **argv));

void          usage_setastrom                 PROTO((void));
void          initialize_setastrom            PROTO((int argc, char **argv));
int           args_setastrom                  PROTO((int argc, char **argv));

void          usage_setastrom_client          PROTO((void));
void          initialize_setastrom_client     PROTO((int argc, char **argv));
int           args_setastrom_client           PROTO((int argc, char **argv));

int           Shutdown                        PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void          TrapSignal                      PROTO((int sig));
void          SetProtect                      PROTO((int mode));
int           SetSignals                      PROTO((void));

int           update_dvo_setastrom            PROTO((void));
int           update_dvo_setastrom_parallel   PROTO((SkyTable *sky));

int           update_catalog_setastrom        PROTO((Catalog *catalog));

// spline correction functions
double spline_apply_dbl (double *x, double *y, double *y2, int N, double X);

int load_kh_correction (char *filename);
int get_kh_correction (int sub, int chip, double *dX, double *dY, float Minst);

int load_dcr_correction (char *filename);
int get_dcr_correction (int filter, double *dX, double *dY, float Color);

int CamAstromCorrectionLoad (char *filename);
int CamAstromCorrectionValue (int chipID, int filter, float Xccd, float Yccd, double *dX, double *dY);

int load_tyc_correction (char *filename);
int get_tyc_correction (double **R, double **D, int *N);
int repair_tycho_setastrom (Catalog *catalog, SkyRegion *region);
