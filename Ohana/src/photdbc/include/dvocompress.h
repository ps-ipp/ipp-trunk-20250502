# include <ohana.h>
# include <dvo.h>
# include <signal.h>

int    PARALLEL;
int    PARALLEL_MANUAL;
int    PARALLEL_SERIAL;

int          HOST_ID;
char        *HOSTDIR;

# define DVO_MAX_PATH 1024

/* global variables */
int    VERBOSE;
char  *UPDATE_CATFORMAT;  /* internal, elixir, loneos, panstarrs */
char  *UPDATE_CATCOMPRESS;  /* ?? */
int    SKIP_COMPRESSED;

SkyRegion REGION;

int           args (int *argc, char **argv); 
int           args_client (int *argc, char **argv); 

int Shutdown (char *format, ...) OHANA_FORMAT(printf, 1, 2);
void check_permissions (char *basefile);
void TrapSignal (int sig);
void SetProtect (int mode);
int SetSignals (void);
void usage();

int dvocompress_catalogs (char *catdir, SkyList *skylist, int hostID);
int dvocompress_parallel (char *catdir, SkyList *skylist);
