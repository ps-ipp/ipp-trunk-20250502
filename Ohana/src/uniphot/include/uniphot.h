# include <ohana.h>
# include <dvo.h>
# include <signal.h>

# ifdef ANSI
#   define F_SETFL      4   
#   define O_NONBLOCK   0200000  
#   define AF_UNIX      1          
#   define SOCK_STREAM  1          
#   define ENOENT       2       /* No such file or directory    */
# endif /* ANSI */

typedef struct {
  double median;
  double mean;
  double sigma;
  double error;
  double chisq;
  double min;
  double max;
  double total;
  int    Nmeas;
} StatType;

typedef struct {
  void *tgroup;
  void *sgroup;
} ImageLink;

typedef struct {
  char tstart[64];
  char tstop[64];
  char label[64];
  float M;
  float dM;
  float dMsub;
  double v1, v2;
  Image **image;
  ImageLink **imlink;
  int Nimage, Ngood;
} Group;

typedef struct {
    float zpt;
    float zpt_err;
    e_time time;
    int found;
} ZptTable;

typedef struct {
    float fwhm_major;
    float fwhm_minor;
    e_time time;
    int found;
    unsigned short photcode;
} FWHMTable;

/* global variables set in parameter file */
char         ImageCat[256];
char         CATDIR[256];
char         CATMODE[16];    /* raw, mef, split, mysql */
char         CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char         SKY_TABLE[256];
int          SKY_DEPTH;  /** XXX EAM : depth of catalog tables, fix usage */
char         GSCFILE[256];
char         STATMODE[64];
int          VERBOSE;
int          UBERCAL; // load the supplied ubercal zero point fits table (with flat-field corrections)
int          NO_METADATA; // the supplied ubercal data has no descriptive metadata
int          NLOOP;
int          TimeSelect;
int          VERBOSE;
int          UPDATE;
int          IMAGE_BAD;
double       RADIUS;
double       TRANGE;
time_t 	     TSTART;
time_t 	     TSTOP;
PhotCode    *photcode;

enum {black, white, red, orange, yellow, green, blue, indigo, violet};

typedef struct {
  double xmin, xmax, ymin, ymax;
  int style, ptype, ltype, etype, color;
  double lweight, size;
} UPGraphdata;

/***** prototypes ****/
void          ConfigInit          PROTO((int *argc, char **argv));
void          DonePlotting        PROTO((UPGraphdata *graphmode, int N));
void          JpegPlot            PROTO((UPGraphdata *graphmode, int N, char *filename));
void          PSPlot              PROTO((UPGraphdata *graphmode, int N, char *filename));
void          PlotLabel           PROTO((char *string, int N));
void          PlotVector          PROTO((int Npts, double *vect, int mode, int N));
void          PrepPlotting        PROTO((int Npts, UPGraphdata *graphmode, int N));
void          XDead               PROTO((void));
int           args_uniphot        PROTO((int argc, char **argv));
void          dumpresult          PROTO((void));
Group        *find_image_sgroups  PROTO((FITS_DB *db, ImageLink **imlink, int *Nsgroup));
Group        *find_image_tgroups  PROTO((FITS_DB *db, ImageLink **imlink, int *Ntgroup));
void          fit_sgroup          PROTO((Group *sgroup, int Nsgroup));
void          fit_tgroup          PROTO((Group *tgroup, int Ntgroup));
int           gcatalog            PROTO((Catalog *catalog));
void          initialize_uniphot  PROTO((int argc, char **argv));
void          initstats           PROTO((char *mode));
int           liststats           PROTO((double *value, double *dvalue, int N, StatType *stats));
int           load_images_uniphot PROTO((FITS_DB *db));
Image        *load_images_setfwhm PROTO((FITS_DB *db, off_t *Nimage));
int           main                PROTO((int argc, char **argv));
int           open_graph          PROTO((int N));
void          sort                PROTO((unsigned int *X, int N));
void          sortB               PROTO((double *X, double *Y, int N));
void          sortD               PROTO((double *X, double *Y, double *Z, int N));
void          wcatalog            PROTO((Catalog *catalog));
void          wimages             PROTO((Image *image, int Nimage));
void 	      check_permissions   PROTO((char *basefile));
void 	      lock_image_db 	  PROTO((FITS_DB *db, char *filename));
void	      unlock_image_db 	  PROTO((FITS_DB *db));
void	      create_image_db 	  PROTO((FITS_DB *db));
void	      set_db 		  PROTO((FITS_DB *in));
int 	      Shutdown 		  PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2) );
void 	      TrapSignal 	  PROTO((int sig));
void 	      SetProtect 	  PROTO((int mode));
int 	      SetSignals 	  PROTO((void));
int 	      subset_images 	  PROTO((FITS_DB *db));
void 	      update_dvo_uniphot  PROTO((FITS_DB *db, Group *sgroup, int Nsgroup));
void          update_catalog_uniphot PROTO((Catalog *catalog, Group *sgroup, int warn));
void 	      sort_time 	  PROTO((unsigned int *value, int N));

time_t        GetTimeReference      PROTO((char *reference));
int           GetTimeUnits          PROTO((char *name));

void          initialize_setfwhm    PROTO((int argc, char **argv));
int           args_setfwhm          PROTO((int argc, char **argv));
FWHMTable    *load_fwhm_table       PROTO((char *filename, int *nfwhm));
int           match_fwhm_to_images  PROTO((Image *image, off_t Nimage, FWHMTable *fwhm, int Nfwhm));

