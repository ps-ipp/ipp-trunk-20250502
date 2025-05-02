# include <ohana.h>
# include <dvo.h>

enum {DOPHOT, CHAD, SEXTRACT};

int    MODE;
int    FITS_OUTPUT;
int    VERBOSE;
int    RESET;
int    FORCE_RUN;
int    PROVIDE_ASTROM;
int    NEWPHOTCODE;

/* global variables set in parameter file */
char   *PHOTCODE;
char   PhotCodeFile[256];
char   AstromFile[256];

double DEFAULT_ERROR;
double RADIUS;
double TRAIL_WIDTH;
int    NBINS;
int    NPTSINLINE;
double MIN_DENSITY;
double NSIGMA;

double RA, DEC, ZERO_POINT, MIN_SN_FSTAT;
int CHAR_LINE, TYPE_FIELD, AP_FIELD, PSF_FIELD, HEADER_COORDS;

int FIX_KEYWORD;
char **KEYWORD, **KEYVALU, **KEYFMT;

SMPData *LoadStarsDophot (char *filename, int *nstars, Header *header);
SMPData *LoadStarsChad (char *filename, int *nstars, Header *header);
SMPData *LoadStarsSex (char *filename, int *nstars, Header *header);

void ConfigInit (int *argc, char **argv);
void AdjustHeader (Header *header);
void sort_stars (SMPData *X, int N);
void find_trails (SMPData *stars, int Nstars);
int find_group (SMPData *stars, char *mark, int Npts, int i, double *ANGLE);
void wstars (char *filename, SMPData *stars, int Nstars, Header *header);

void fix_total (SMPData *stars, int Nstars, Header *header);
int find_line (SMPData *stars, char *mark, int Npts, int i, double *M, double *B, double Angle);
void find_better_line (SMPData *stars, char *mark, int Npts, int i, double *M, double *B, int axis);

void help (void);
void args (int argc, char **argv);
void wfits (char *filename, SMPData *stars, int Nstars, Header *header);
