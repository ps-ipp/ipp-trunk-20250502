
/* structure for data on a catalog region */
typedef struct {
  char filename[256];
  double RA[2];
  int Nrec;
} WISE_Region;

typedef struct {
  double Rmin, Rmax, Dmin, Dmax;
  int index[20];
  int Nindex;
} WISE_Bands;

typedef struct {
  double R, D;
  int offset;
  int flag;
} WISE_Stars;

short WISE_W1, WISE_W2, WISE_W3, WISE_W4;

double  RA_SYS_OFFRAW;
double  DE_SYS_OFFSET;
double  uRA_SYS_OFFSET;
double  uDE_SYS_OFFSET;

enum {MODE_NONE, MODE_CATWISE, MODE_ALLWISE, MODE_ALLSKY, MODE_PRELIM};
int MODE;

AddstarClientOptions args_loadwise (int *argc, char **argv, AddstarClientOptions options);

int loadwise_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options);

int getWISE_setup ();
int getWISE_coords (char *line, double *R, double *D, int Nmax);
e_time getWISE_date (char *ptr, int Nbound, int Nmax);
e_time getWISE_time (char *ptr, int Nbound, int Nmax);

int getWISE_sortStars (WISE_Stars *tstars, int Ntstars);

int loadwise_star_prelim  (Measure *measure, char *line, int Nmax);
int loadwise_star_allsky  (Measure *measure, char *line, int Nmax);
int loadwise_star_allwise (Measure *measure, char *line, int Nmax);
int loadwise_star_catwise (Average *average, Measure *measure, char *line, int Nmax, WISE_Stars *tstars);

char *nextWISEfield (char *line);
int setWISE_cc_flag  (Measure *measure, char qual);
int setWISE_ph_qual  (Measure *measure, char qual);
int setWISE_rd_flag  (Measure *measure, char qual);
int setWISE_bl_flag  (Measure *measure, char qual);
int setWISE_gal_flag (Measure *measure, char qual);
int setWISE_mp_flag  (Measure *measure, char qual);
int setWISE_dup_flag (Measure *measure, char qual);
int setWISE_use_flag (Measure *measure, char qual);
int setWISE_sat_flag (Measure *measure, char *ptr);

int setCatWISE_sat_flag (Measure *measure, char *ptr, int start, int end);
int setCatWISE_blend_flag (Measure *measure, char *line);

char *skipNbounds (char *line, char bound, int Nbound, int Nbyte);

char *getLineSegment (char *line, int start, int end);
double getDoubleRAW (char *line, int start, int end);
double getDoubleNAN (char *line, int start, int end);
