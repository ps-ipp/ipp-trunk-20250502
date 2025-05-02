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

typedef struct {
  int *outref;
  int *outcat;
} AveLinks;

int    FULL_TABLE;
int    VERBOSE;
int    SKIP_EXIST; // do not re-split existing files
char  *OUTDIR;
char  *CATMODE;    /* raw, mef, split, mysql */
char  *CATFORMAT;  /* internal, elixir, loneos, panstarrs */

SkyRegion UserPatch;  // used by MODE CAT

int        main                   PROTO((int argc, char **argv));

int        ConfigInit             PROTO((int *argc, char **argv));
int        SetSignals             PROTO((void));
void       SetProtect             PROTO((int mode));
void       TrapSignal             PROTO((int sig));
int        Shutdown               PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2));
int        args                   PROTO((int argc, char **argv));

Catalog   *open_output_catalogs   PROTO((SkyList *outlist, int catformat, int catmode));
int        split_averages         PROTO((Catalog *incatalog, SkyList *outlist, Catalog *outcatalogs));
int        split_measures         PROTO((Catalog *incatalog, SkyList *outlist, Catalog *outcatalogs, AveLinks *avelinks));
void       GetConfig              PROTO((char *config, char *field, char *format, int N, void *ptr));

void       dvosplit_free          PROTO((SkyTable *sky, SkyList *skylist));
