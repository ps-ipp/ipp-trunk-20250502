# include <ohana.h>
# include <dvo.h>
# include <signal.h>
# include <sys/time.h>
# include <time.h>
# include <zlib.h>

/* solaris requires both of these instead of ip.h:
   # include <sys/socket.h>
   # include <netinet/in.h>
*/

/* linux is happy with this, not solaris */
# include <netinet/ip.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <glob.h>

# define DEBUG 0

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

char  *CATDIR;

int    HOST_ID;
char  *HOSTDIR;

char  *RESULTS;

int    CHECKSORTED;
int    VERBOSE;
int    NNotSorted;
int    CHECK_TOPLEVEL;
int    CHECK_IMAGE_ID;
int    LIST_MISSING;

int    IGNORE_SORTED_STATE;

SkyRegion UserPatch;

int dvoverify_args (int *argc, char **argv);
int dvoverify_client_args (int *argc, char **argv);

int dvoverify_catalogs (SkyList *skylist, int *Nbad);
int dvoverify_parallel (SkyList *skylist, int *Nbad);

int dvoverify_single (char *filename);

int VerifyTableFile (char *filename);
int CheckCatalogIndexes (char *filename,  SkyRegion *region);

void InitFailures ();
void AddFailures (char *filename);
char **GetFailures (int *N);
void FreeFailures (void);

int LoadImageIDs (char *catdir);
int CheckImageID (Catalog *catalog);

int SaveImageIDsSmall(char *filename);
int LoadImageIDsSmall (char *filename);

void FreeImageIDs (void);

int        SetSignals             PROTO((void));
void       SetProtect             PROTO((int mode));
void       TrapSignal             PROTO((int sig));
int        Shutdown               PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);

