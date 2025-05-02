# include <stdio.h>
# include <fcntl.h>
# include <math.h>
# include <float.h>
# include <errno.h>
# include <time.h>
# include <stdlib.h>
# include <stdint.h>
# include <string.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/uio.h>
# include <sys/un.h>
# include <unistd.h>
# include <stdarg.h> 
# include <readline/history.h>
# include <readline/readline.h>

// comment this out to avoid the internal Ohana memory management code
# define OHANA_MEMORY

// XXX I was including these before, but RHL claims they are not needed
// # include <malloc.h>
// # include <memory.h>

/* OHANA included stuff */
# ifndef OHANA_H
# define OHANA_H

# ifndef TRUE
# define TRUE (1)
# endif

# ifndef FALSE
# define FALSE (0)
# endif 

// XXX these should probably use safe name-spaces (eg, OHANA_MIN)
# ifndef SIGN
# define SIGN(X) (((X) == 0) ? 0 : ((fabs((double)(X))) / (X)))
# endif

# ifndef ROUND
# define ROUND(X) ((int) ((X) + 0.5*SIGN(X)))
# endif

# ifndef SQR
# define SQR(X)   (double) (((double)(X))*((double)(X)))
# endif

# ifndef SQ
# define SQ(X)    (double) (((double)(X))*((double)(X)))
# endif

# ifndef MIN
# define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
# endif

# ifndef MAX
# define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
# endif

# ifndef SWAP
# define SWAP(X,Y) {double tmp=(X); (X) = (Y); (Y) = tmp;}
# endif

# ifndef DTIME
# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))
# endif

# define MARKTIME(MSG,...) {			\
    gettimeofday (&stopTimer, (void *) NULL);	\
    float dtime = DTIME (stopTimer, startTimer);	\
    fprintf (stderr, MSG, __VA_ARGS__); }

# define INITTIME \
  struct timeval startTimer, stopTimer; \
  gettimeofday (&startTimer, (void *) NULL);

// several snprintf statements below my truncate their output
// gcc (since 8.1) warns if the output may be truncated.
// If we do not care, the following snprintf_nowarn can
// replace these snprintf calls. (myNOOP in string.c)

# define snprintf_nowarn(...) (snprintf(__VA_ARGS__) < 0 ? myNOOP() : 0)

#ifdef __GNUC__
#define OHANA_FORMAT(style, fmt, varargs) __attribute__((format(style, fmt, varargs)))
#else // __GNUC__
#define OHANA_FORMAT(style, fmt, varargs)
#endif // __GNUC__

enum {
 LCK_UNLOCK,        /* file is unlocked */
 LCK_INVALID,       /* invalid locktype requested */
 LCK_MISSING,       /* soft requested and file missing (error) */
 LCK_ACCESS,        /* can't get access to file */
 LCK_TIMEOUT,       /* timeout setting lock */
 LCK_HARDOPEN,      /* cannot open hard lockfile */
 LCK_HARDLOCK,      /* cannot lock hard lockfile */
 LCK_HARDLOCKHARD,  /* hard lockfile is locked */
 LCK_HARDCLOSE,     /* cannot write to hardlock */

 LCK_EMPTY,         /* file is locked and empty */
 LCK_FULL,          /* file is locked and not empty */
 LCK_UNKNOWN,       /* file is locked, but can't get size */

 LCK_SOFT,          /* soft lock */
 LCK_HARD,          /* hard lock */
 LCK_XCLD,          /* exclusive soft lock */
};

typedef enum {GP_LOG, GP_ERR} gpDest;

/* Some notes on the Ohana BYTE_SWAP macros:

   1) BYTE_SWAP is set in this file based on the environment variable 'ARCH', which is in turn
   set by the psconfig system (or is set manually by the builder).  

   2) BYTE_SWAP is only used when building the Ohana tree: code which links against Ohana
   (eg, libkapa or others) does not need to have this value correctly set.
   
   3) the libohana build tests for the validity of the BYTE_SWAP choice by running the test
   program 'typestest' and raising an error if the tests fail.

   4) if your build fails due to the typestest program, check your value of 'ARCH' and if
   necessary add it to the list below.

*/

# ifndef BYTE_SWAP
# ifdef linux
# define BYTE_SWAP
# endif

# ifdef lin64
# define BYTE_SWAP
# endif

# ifdef sid
# define BYTE_SWAP
# endif

# if defined(darwin_x86) || defined(_DARWIN_C_SOURCE) || defined(__APPLE__)
# define BYTE_SWAP
# endif

# ifdef dec
# define BYTE_SWAP
# endif
# endif /* BYTE_SWAP */

/* other Ohana components use these two values to check that ohana.h was correctly included */
# ifndef BYTE_SWAP
# define NOT_BYTE_SWAP
# endif

# ifndef NAN
# ifndef BYTE_SWAP
#  define __nan_bytes           { 0x7f, 0xc0, 0, 0 }
# else
#  define __nan_bytes           { 0, 0, 0xc0, 0x7f }
# endif
static union { unsigned char __c[4]; float __d; } __nan_union
    __attribute_used__ = { __nan_bytes };
# define NAN    (__nan_union.__d)
# endif

/* if your build crashes on OFF_T_MODE, you probably need to add your 64bit hardware to this list */
# ifdef _LARGEFILE_SOURCE
#  define OFF_T_FMT "%jd"
# endif

# ifdef lin64
#  define OFF_T_FMT "%jd"
# endif

// mac is annoying
# ifdef _DARWIN_C_SOURCE
#  define OFF_T_FMT "%lld"
# endif
# ifdef darwin_x86
#  define OFF_T_FMT "%lld"
# endif
# ifdef darwin
#  define OFF_T_FMT "%lld"
# endif

// for mac os x 10.7 (lion): they don't use darwin, and i want to make sure it grabs for 10.7 and 64 bit (are they all 64 bit?)
# if defined(__APPLE__) && defined(x86_64)
#  define OFF_T_FMT "%lld"
# endif

# ifndef OFF_T_FMT
#  define OFF_T_FMT "%ld"
# endif

# ifndef isfinite
# define isfinite(A) (!isnan(A) && !isinf(A))
# endif

# ifndef M_PI
# define M_PI 3.14159265358979323846264
# endif

# define DEG_RAD 57.295779513082322
# define RAD_DEG  0.017453292519943

# ifndef PROTO
#   define PROTO(A) A
# endif

/* 
# else
#   ifndef PROTO
#   define PROTO(A) ()
#   endif
# include <varargs.h>
# endif
*/

/* ohana_allocate.h provides ohana memory tools.  to use them
   you must # define OHANA_MEMORY before including ohana.h */
# include <ohana_allocate.h>

# ifndef FOPEN 
# define FOPEN(F,NAME) \
  F = fopen (NAME, "r"); \
  if (F == NULL) { \
    fprintf (stderr, "failed to open %s\n", NAME); \
    exit (0); \
  }
# endif /* FOPEN */

/*
  isspace is c99 : do we require c99 now? 
  isspace()
   checks  for white-space characters.  In the "C" and "POSIX" locales, these are: space, form-feed ('\f'), newline ('\n'),
   carriage return ('\r'), horizontal tab ('\t'), and vertical tab ('\v').
   horiz. tab: 0x09, vert. tab: 0x0b, newline: 0x0a, form-feed: 0x0c, return: 0x0d, space: 0x20, 
*/
# define OHANA_WHITESPACE(c)(((c) == 0x09) || ((c) == 0x0a) || ((c) == 0x0b) || ((c) == 0x0b) || ((c) == 0x0c) || ((c) == 0x0d) || ((c) == 0x20))
# define OHANA_ASSERT(LOGIC,...) { if (!(LOGIC)) { fprintf (stderr, __VA_ARGS__); abort(); } }
# define myAssert(LOGIC,...) { if (!(LOGIC)) { fprintf (stderr, __VA_ARGS__); abort(); } }
# define myAbort(MSG) { fprintf (stderr, "%s\n", MSG); abort(); }

// a no-op to mark unused parameters in a function
# define OHANA_UNUSED_PARAM(x)(void)(x)

// sorting is now defined as a macro call
# include <ohana_sort.h>

// vector statistics options
typedef enum {
  VSTATS_NONE, 
  VSTATS_MEAN, 
  VSTATS_MEDIAN, 
  VSTATS_WT_MEAN, 
  VSTATS_INNER_MEAN, 
  VSTATS_INNER_WTMEAN, 
  VSTATS_CHI_INNER_MEAN, 
  VSTATS_CHI_INNER_WTMEAN
} VStatsMode;

typedef struct {
  double median;
  double mean;
  double sigma;
  double error;
  double chisq;
  double min;
  double max;
  double Upper80;
  double Lower20;
  double total;
  int    Nmeas;
  VStatsMode statmode;
} VStatsType;

/* socket / pipe communication buffer */
typedef struct {
  char *buffer;
  int   Nalloc;
  int   Nreset;
  int   Nblock;
  int   Nbuffer;
} IOBuffer;

extern double hypot(double,double);

/* in string.c */
int     stripwhite             PROTO((char *string));
int     strnumcmp              PROTO((char *str1, char *str2));
char   *strcreate              PROTO((char *string));
char   *strncreate             PROTO((char *string, int n));
char   *strncpy_nowarn         PROTO((char *dest, char *src, size_t n));
int     strextend              PROTO((char **input, char *format,...)) OHANA_FORMAT(printf, 2, 3);
int     scan_line              PROTO((FILE *f, char *line)); 
int     scan_line_maxlen       PROTO((FILE *f, char *line, int maxlen)); 
char   *parse_nextword         PROTO((char *string));
char   *parse_nextword_csv     PROTO((char *string));
int     dparse                 PROTO((double *X, int NX, char *line));
int     dparse_csv             PROTO((double *X, int NX, char *line));
int     iparse                 PROTO((int *X, int NX, char *line));
int     iparse_csv             PROTO((int *X, int NX, char *line));
int     charparse_csv          PROTO((char *X, int NX, char *line));
char   *ptrparse               PROTO((int NX, char *line));
char   *ptrparse_csv           PROTO((int NX, char *line));
int     charparse_csv          PROTO((char *X, int NX, char *line));
int     charparse              PROTO((char *X, int NX, char *line));
int     tparse                 PROTO((time_t *X, int NX, char *line));
int     tparse_csv             PROTO((time_t *X, int NX, char *line));
int     fparse                 PROTO((float *X, int NX, char *line));
int     get_argument           PROTO((int argc, char **argv, char *arg));
int     remove_argument        PROTO((int N, int *argc, char **argv));
void    uppercase              PROTO((char *string));
char   *strip_version          PROTO((char *input));
char   *strsubs                PROTO((char *string, char *match, char *with));

char   *getword                PROTO((char *string));
char   *skipword               PROTO((char *string));
void    myNOOP                 PROTO((void));

/* in findexec.c */
char   *pathname               PROTO((char *name));
char   *filebasename           PROTO((char *name));
char   *filerootname           PROTO((char *name));
char   *fileextname            PROTO((char *file));
char   *findexec               PROTO((int argc, char **argv));
int     mkdirhier              PROTO((char *path, int mode));
void    make_backup            PROTO((char *filename));
int     check_file_access      PROTO((char *basefile, int backup, int readwrite, int verbose));
int     check_dir_access       PROTO((char *path, int verbose));
int     check_file_exec        PROTO((char *filename));
char   *abspath                PROTO((char *oldpath, int maxlength));

/* in glockfile.c */
FILE   *fsetlockfile           PROTO((char *filename, double timeout, int type, int *state));
int     fclearlockfile         PROTO((char *filename, FILE *f, int type, int *state));
int     fchecklockfile         PROTO((char *filename, int type, int *state));

/* in config.c */
char   *SelectConfigFile       PROTO((int *argc, char **argv, char *progname));
char   *LoadConfigFile         PROTO((char *filename));
void    FreeConfigFile         PROTO((void));
char   *ScanConfig             PROTO((char *config, char *field, char *mode, int N,...)) OHANA_FORMAT(scanf, 3, 5);
char   *expandline             PROTO((char *line, char *config));
char   *fileextname            PROTO((char *file));
char   *LoadRawConfigFile      PROTO((char *, int));

/* others */
int     Fseek                  PROTO((FILE *f, off_t offset, int whence));
char   *ohana_version          PROTO((void));

int     dgaussjordan_pivot     PROTO((double **A, double **B, int N, int M, double minPivot));
int     dgaussjordan           PROTO((double **A, double **B, int N, int M));
int     fgaussjordan           PROTO((float **A, float **B, int n, int m));

/* in time.c */
enum {TIME_NONE, TIME_DATE, TIME_DAYS, TIME_HOURS, TIME_MINUTES, TIME_SECONDS, TIME_JD, TIME_MJD};

int     ohana_chk_time         PROTO((char *line));
time_t  ohana_date_to_sec      PROTO((char *date));
int     ohana_dms_to_ddd       PROTO((double *Value, char *string));
time_t  ohana_jd_to_sec        PROTO((double jd));
time_t  ohana_mjd_to_sec       PROTO((double mjd));
char   *ohana_sec_to_date      PROTO((time_t second));
double  ohana_sec_to_jd        PROTO((time_t second));
int     ohana_str_to_dtime     PROTO((char *line, double *second));
int     ohana_str_to_time      PROTO((char *line, time_t *second));
double  ohana_sec_to_mjd       PROTO((time_t second));
double  ohana_lst              PROTO((double jd, double longitude));
int     ohana_str_to_radec     PROTO((double *ra, double *dec, char *str1, char *str2));
double  ohana_normalize_angle  PROTO((double angle));
double  ohana_normalize_angle_to_midpoint  PROTO((double angle, double Rmid));

int     hstgsc_hms_to_deg      PROTO((double *h0, double *h1, double *d0, double *d1, char *string));

short 	ToShortPixels          PROTO((float pixels));
short 	ToShortDegrees         PROTO((float degrees));
float 	FromShortPixels        PROTO((short value));
float 	FromShortDegrees       PROTO((float value));

/* IO Buffer functions */
int     InitIOBuffer   	       PROTO((IOBuffer *buffer, int Nalloc));
int 	FlushIOBuffer  	       PROTO((IOBuffer *buffer));
int 	ReadtoIOBuffer 	       PROTO((IOBuffer *buffer, int fd));
int 	EmptyIOBuffer  	       PROTO((IOBuffer *buffer, int Nmax, int fd));
void    FreeIOBuffer   	       PROTO((IOBuffer *buffer));
int 	PrintIOBuffer  	       PROTO((IOBuffer *buffer, char *format, ...)) OHANA_FORMAT(printf, 2, 3);
int 	vPrintIOBuffer 	       PROTO((IOBuffer *buffer, char *format, va_list argp));
int     WriteToIOBuffer        PROTO((IOBuffer *buffer, char *input, int Ninput));

/* communication functions */
int 	ExpectMessage 	       PROTO((int device, double timeout, IOBuffer *message));
int 	ExpectCommand 	       PROTO((int device, int length, double timeout, IOBuffer *buffer));
int 	SendMessage   	       PROTO((int device, char *format, ...)) OHANA_FORMAT(printf, 2, 3);
int 	SendMessageFixed       PROTO((int device, int length, char *messge));
int 	SendCommand   	       PROTO((int device, int length, char *format, ...)) OHANA_FORMAT(printf, 3, 4);
int 	SendCommandV  	       PROTO((int device, int length, char *format, va_list argp));

char   *CheckForMessage        PROTO((IOBuffer *buffer));

off_t   Fread                  PROTO((void *ptr, off_t size, off_t nitems, FILE *f, char *type));
off_t   Fwrite                 PROTO((void *ptr, off_t size, off_t nitems, FILE *f, char *type));

char   *memstr                 PROTO((char *m1, char *m2, int n));
int     write_fmt              PROTO((int fd, char *format, ...)) OHANA_FORMAT(printf, 2, 3);

char   **isolate_elements      PROTO((unsigned int argc, char **argv, unsigned int *nstack));

// functions used to manage "last error message"
int    init_error                PROTO((void));
int    push_error                PROTO((char *line));
int    print_error               PROTO((void));
void   free_error                PROTO(());

// gprint gets a stub implementation in libohana. opihi implements the real one.
int    gprint                  PROTO((gpDest dest, char *format, ...) OHANA_FORMAT(printf, 2, 3));

// rconnect is used to run remote programs with a pipe for communication
typedef enum {
  RCONNECT_ERR_NONE    = 0,
  RCONNECT_ERR_EXEC    = 1,
  RCONNECT_ERR_PIPE    = 2,
  RCONNECT_ERR_CONNECT = 3,
} RconnectErrors;

void rconnect_set_timeout (int timeout);
int rconnect (char *command, char *hostname, char *shell, int *stdio, int *errorInfo, int doHandshake);

int vstats_setmode (VStatsType *stats, char *mode);
int vstats_getstats (double *value, double *dvalue, double *weight, int N, VStatsType *stats);
int vstats_getstats_f (float *value, float *dvalue, float *weight, int N, VStatsType *stats);

/*
#   define F_SETFL         4   
#   define O_NONBLOCK      0200000  
#   define AF_UNIX         1          
#   define SOCK_STREAM     1
*/

/*
# ifndef ANSI
# include <varargs.h>
# else
# include <stdarg.h> 
# include <cfuncs.h> 
# endif
*/


/** gsl-based functions for legendre and related polynomials **/

double ohana_gsl_sf_log_1plusx (double x, double *err);
double ohana_gsl_sf_lnpoch(const double a, const double x, double *err);
double ohana_gsl_lnpoch_pos (const double a, const double x, double *err);
double ohana_gsl_pochrel_smallx(const double a, const double x, double *err);

double legendre_Pl (int l, double x, double *err);
double legendre_Plm (int l, int m, double x, double *err);
double legendre_Plm_sphere(int l, int m, double x, double *err);

typedef struct {
  double *Fr;
  double *Fi;
  int    *l;
  int    *m;
  int lmax;
  int Nterms;
} SHterms;

SHterms *SHtermsInit (int lmax);
void SHtermsFree (SHterms *terms);
void SHtermsForRD (SHterms *terms, double R, double D);
void SHtermsForLM (SHterms *terms, double R, double D, int l, int m);

typedef struct {
  double *dR_B;
  double *dR_E;
  double *dD_B;
  double *dD_E;
  int    *l;
  int    *m;
  int lmax;
  int Nterms;
} VSHterms;

VSHterms *VSHtermsInit (int lmax);
void VSHtermsFree (VSHterms *terms);
void VSHtermsForRD (VSHterms *terms, double R, double D);
void VSHtermsForLM (VSHterms *terms, double R, double D, int l, int m);

/* in bisection.c */
int ohana_bisection_double (double *values, int Nvalues, double threshold);
int ohana_bisection_int (int *values, int Nvalues, int threshold);

unsigned int sprintf_float (char *output, float value);
unsigned int sprintf_double (char *output, double value);

/* in gaussdev.c */
double ohana_gaussian (double x, double mean, double sigma);
void ohana_gaussdev_init (void);
void ohana_gaussdev_free (void);
double ohana_gaussdev_rnd (double mean, double sigma);

# endif
