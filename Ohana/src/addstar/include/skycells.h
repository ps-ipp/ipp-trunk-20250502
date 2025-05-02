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

enum {SQUARES, TRIANGLES, LOCAL, RINGS, TAMAS, CFIS};
enum {TETRAHEDRON, CUBE, OCTOHEDRON, DODECAHEDRON, ICOSAHEDRON};

typedef struct {
  double x, y, z;
} Point;

typedef struct {
  Point vertex[3];	      // triangle vertices (3d)
  Point center;		      // triangle center (3d)
  double r, d;		      // triangle center (2d)
  double rv[3], dv[3];	      // triangle center (2d)
} SkyTriangle;

typedef struct {
  Coords coords;
  int NX;
  int NY;
  int photcode;
  char name[64];
} SkyRectangle;

/* globals which define database info / data sources (KEEP) */
char   ImageCat[512];
char   GSCFILE[256];
char   CATDIR[256];
char   CATMODE[16];    /* raw, mef, split, mysql */
char   CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char   PASSWORD[80];
char   HOSTNAME[80];
int    NVALID, *VALID_IP;
char   SKY_TABLE[256];
int    SKY_DEPTH;  /** XXX EAM : depth of catalog tables, fix usage */
char   CameraLayout[256];
SkyTable *ServerSky;

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

int    VERBOSE;
int    MODE;
int    SOLID;
int    FIX_NS;
int    NMAX;
int    NX_SUB, NY_SUB;
int    X_PARITY;
double SCALE;
double PADDING;
double CELLSIZE;
int    LEVEL;
char   PROJECTION_NUMBER[8];
char   *SKYCELLS_MDC;
int    READONLY;

double CENTER_RA, CENTER_DEC;
double RANGE_RA,  RANGE_DEC;
double OVERLAP_RA, OVERLAP_DEC;

double EULER_A;
double EULER_B;

void         SetProtect                	    PROTO((int mode));
int          SetSignals                	    PROTO((void));
int          Shutdown                  	    PROTO((char *message, ...)) OHANA_FORMAT(printf, 1, 2);
void         TrapSignal                	    PROTO((int sig));

int 	     args_skycells 	       	    PROTO((int argc, char **argv));
int 	     ConfigInit_skycells       	    PROTO((int *argc, char **argv));
int 	     sky_tessellation 	       	    PROTO((FITS_DB *db, int level, int Nmax, int mode, double scale));
int          sky_tessellation_init          PROTO((double scale));

int 	     sky_tessellation_local         PROTO((FITS_DB *db, int level, int Nmax));
int 	     sky_tessellation_triangles     PROTO((FITS_DB *db, int level, int Nmax));
int 	     sky_tessellation_squares       PROTO((FITS_DB *db, int level, int Nmax));
int          sky_tessellation_rings         PROTO((FITS_DB *db, int level, int Nmax));
int          sky_tessellation_tamas         PROTO((FITS_DB *db, int level, int Nmax));
int          sky_tessellation_cfis          PROTO((FITS_DB *db, int level, int Nmax));

int 	     sky_triangle_to_image     	    PROTO((Image *image, SkyTriangle *triangle));
int 	     sky_triangle_to_rectangle 	    PROTO((SkyRectangle *image, SkyTriangle *triangle));

int          sky_rectangle_local            PROTO((SkyRectangle *rectangle));
int 	     sky_subdivide_image       	    PROTO((Image *output, SkyRectangle *input, int Nx, int Ny));
int 	     sky_triangle_coords       	    PROTO((SkyTriangle *triangle));

SkyRectangle *sky_rectangle_ring            PROTO((float dec, float dDEC, int *nring, char *format));
SkyRectangle *sky_rectangle_tamas           PROTO((double *Dec, double dm, double halfa, double halftheta, int *nring, char *format));
SkyRectangle *sky_rectangle_cfis            PROTO((double dec, int *nring, char *format));

SkyTriangle *sky_divide_triangles      	    PROTO((SkyTriangle *in, int *ntriangles));
SkyTriangle *sky_base_triangles        	    PROTO((int *ntriangles));
SkyTriangle *sky_base_triangles_icosahedron PROTO((int *ntriangles));
int          sky_base_rotation         	    PROTO((SkyTriangle *base, int Nbase));
Point        sky_divide_edge           	    PROTO((Point v1, Point v2));

void         skycells_to_mdc                PROTO((FILE *output, int simple, char *tess_id, FITS_DB *db)); 

// XXX migrate to libdvo eventually
int dvo_image_clear_vtable             PROTO((FITS_DB *db));

