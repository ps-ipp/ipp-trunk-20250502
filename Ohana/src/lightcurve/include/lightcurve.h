# include <ohana.h>

# define EMPTY          -1   
# define END            (star_data_type *) -1

# define C_LAMBDA       23.0
# define MCAL(I,dR,dD) (((I.Mcal) + (dR)*(I.McalR) + (dD)*(I.McalD) + (dR)*(dR)*(I.McalR2) + (dD)*(dD)*(I.McalD2) + (dD)*(dR)*(I.McalRD)))

int PRINT;
int MIDAS;
int EXTRASTARS;
int PIXELS;
int NLOOP;
double RADIUS;
double A_LAMBDA;
double SIG;
double COS;
double MCUTOFF;
char OUTFILE[100];
char IMAGES[100];

typedef struct Star {
  double RA, Dec;
  double ap, m, dm;
  struct Star *next_this_unique;
  struct Star *next_this_image;
  int    image_number, star_number, unique_number;
} Star;

typedef struct {
  char    name[50];
  int     Nunique, Nstars;
  int     fixed, empty;
  Star   *first_this_image;
  double  Mcal, dMcal;
  double  exptime, airmass, clouds, Mtime, AmF, dAmF, JD;
  double  RA_O, RA_X, RA_Y, DEC_O, DEC_X, DEC_Y;
  double  McalR, McalD, McalR2, McalD2, McalRD;
} Image;

typedef struct {
  double  Mrel, dMrel;
  int     Nmeasurements;
  Star   *first_this_unique;
} Unique;


/******************** PROTOTYPES ********************/
void args             PROTO((int, char **));
void get_names        PROTO((Image **, int  *));
void get_sources      PROTO((Star **, int *));
void get_stars        PROTO((Star  **, int  *, Image  *, int));
void sort_stars       PROTO((int  **, Star *, int));
void get_unique       PROTO((Unique **, int    *, Star *, int *, int, Star *, int));
void count_unique     PROTO((Image *, int, int));
void set_Mcal         PROTO((Image *, int));
void get_Mrel         PROTO((Unique *, Image *, int)); 
void get_Mcal         PROTO((Image *, Unique *, int));
void ChiSquare        PROTO((Unique *, Image *, int, int));
void get_Alam         PROTO((Unique *, Image *, int, int));
void alter_headers    PROTO((Image *, int));
void make_table       PROTO((Star *, int, Unique *, Image *, int));
void get_info         PROTO((Image *));
char *nextword        PROTO((char *));

extern double hypot PROTO((double, double));

