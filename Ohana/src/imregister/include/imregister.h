# include <ohana.h>
# include <dvo.h>
# include <errno.h>
# include <time.h>
# include <ctype.h>
# include <stdlib.h>

/* config variables from very old versions - remove?
char ImageTemplate[256];
char DBServer[256];
*/

# define MY_MAX_PATH 256

char ImstatFifo[MY_MAX_PATH];
char PtolemyFifo[MY_MAX_PATH];

char ImageDB[MY_MAX_PATH];
char DetrendDB[MY_MAX_PATH];
char PhotDB[MY_MAX_PATH];
char TransDB[MY_MAX_PATH];
char ImPhotDB[MY_MAX_PATH];
char TempDB[MY_MAX_PATH];
char LogFile[MY_MAX_PATH];
char CONNECT[64];

int  NDetrendAltDB;
char **DetrendAltDB;

char PhotCodeFile[MY_MAX_PATH];
char TempLogFile[MY_MAX_PATH];
char FilterList[MY_MAX_PATH];
char CameraConfig[MY_MAX_PATH];
char RecipeFile[MY_MAX_PATH];

char DateKeyword[64];
char DateMode[64];
char UTKeyword[64];
char MJDKeyword[64];
char JDKeyword[64];
char ExptimeKeyword[16];
char ImagetypeKeyword[16];
char CCDnumKeyword[16];
char FilterKeyword[16];
char AirmassKeyword[16];
char FocusKeyword[16];
char RotationKeyword[16];
char DettempKeyword[16];
char Teldata1Keyword[16];
char Teldata2Keyword[16];
char Teldata3Keyword[16];
char RADecDegKeyword[16];
char DECDecDegKeyword[16];
char RASexigKeyword[16];
char DECSexigKeyword[16];
char CameraKeyword[16];
char Camera[64];
char SeeingREFCCD[64];
double ARCSEC_PIXEL;

/* global vars used by camera info */
char **ccds, **ccdn;
int  Nccd;

/* global vars used by filter abstraction */
# define FILTER_ANY  -1 
# define FILTER_NONE 0
int NFILTER;
char **filtername;
char **filterhash;
int   *filternum;

int get_trange_arguments (int *argc, char **argv, time_t **Tstart, time_t **Tstop, int *ntimes);
int get_filter_arguments (int *argc, char **argv, int **Filt, int *Nfilt);

int parse_time (Header *header);
void sortstr (char **S, off_t *X, off_t N);

void ConfigCamera (void);
int MatchCCDNameHeader (Header *header);
int MatchCCDName (char *ID);
void ConfigFilter (void);
int MatchFilterList (char *line);
void ConfigInit (int *argc, char **argv);
void WarnConfig (char *config, char *key, char * mode, int N, void *var);
int WriteFIFO (char *filename, char *line);
double get_fwhm (char *filename);

void get_version (int argc, char **argv, char *version);
int set_db (char *filename);

void warn_scan (Header *header, char *field, char *format, int N, void *var);
int gfits_scan_nchar (Header *header, int size, char *field, int N,...) OHANA_FORMAT(scanf, 3, 5);
void warn_scan_nchar (Header *header, int size, char *field, int N, void *var);

void clean_spaces (char *line);

/* where is this defined ?? */
char *basename (char *);
