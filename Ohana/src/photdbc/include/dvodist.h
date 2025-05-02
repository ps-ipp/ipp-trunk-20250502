# include <ohana.h>
# include <dvo.h>
# include <signal.h>
# include <md5.h>
# include <unistd.h> // needed?

# define PCLIENT_PROMPT "pclient:"

// used by the md5 stuff
# define NDIGEST  16

typedef enum {
    MODE_NONE = 0,
    MODE_OUT  = 1,
    MODE_IN   = 2,
    MODE_FIX  = 3,
    MODE_OUT_BACKUP = 4,
    MODE_USE_BACKUP = 5,
} ModeType;

/* global variables */
int       VERBOSE;
SkyRegion UserPatch;
ModeType  MODE;

char     *srcHostname;
char     *dstHostname;

# define DVO_MAX_PATH 1024

void          usage();
int           args (int argc, char **argv); 
void          initialize (int argc, char **argv); 

void          LockDatabase (char *catdir);
int           CheckHostsAndPaths (HostTable *table);

int           AssignSkyToHost (SkyList *skylist, HostTable *table);
int           CopyToHostLocation (char *catdir, SkyList *skylist, HostTable *table);
int           CopyFromHostLocation(char *catdir, SkyList *skylist, HostTable *table);

int           Shutdown (char *format, ...) OHANA_FORMAT(printf, 1, 2);
void 	      lock_image_db (FITS_DB *db, char *filename);
void 	      TrapSignal (int sig);
void 	      SetProtect (int mode);
int 	      SetSignals (void);

int           get_md5_with_copy (char *input, char *output, md5_byte_t *digest);
int hexchar_to_int (char input);
int get_md5_from_pclient (HostInfo *host, char *filename, md5_byte_t *digest);
int get_md5_from_remote (HostInfo *host, char *filename, md5_byte_t *digest);

int CheckBusyJob (HostInfo *host, IOBuffer *stdout_buf, IOBuffer *stderr_buf);
int GetJobOutput (char *command, HostInfo *host, IOBuffer *output, int size);
int PclientResponse (HostInfo *host, char *response, IOBuffer *buffer);
int PclientCommand (HostInfo *host, char *command, IOBuffer *buffer);

int FixSkyForHost (char *catdir, SkyList *skylist, HostTable *table);

int UseBackupForHost (char *catdir, SkyList *skylist, HostTable *table);
int CopyBackupToHost (char *catdir, SkyList *skylist, HostTable *table);
