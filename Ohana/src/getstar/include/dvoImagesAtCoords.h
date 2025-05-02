# include <ohana.h>
# include <dvo.h>
# include <ReadImageFiles.h>
# include <signal.h>

enum {NONE, SIMPLE_CMP, SIMPLE_CMF, SIMPLE_MEF, MOSAIC_CMP, MOSAIC_CMF, MOSAIC_MEF, MOSAIC_PHU};

int       VERBOSE;
int       LISTCHIPCOORDS;

char OUTPUT[256];
char GSCFILE[256];
char CATDIR[256];
char CATMODE[16];    /* raw, mef, split, mysql */
char CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char SKY_TABLE[256];
int  SKY_DEPTH;
char   ImageCat[256];
Coords *MOSAIC;         // carries the mosaic into ReadImageHeader

typedef struct {
    int     n;
    double  x;
    double  y;
} Match;

typedef struct {
    int     id;
    double  ra;
    double  dec;
    int     Nmatches;
    Match   *matches;
    int     arrayLength;
} Point;

char *astromFile;
char *coordsFile;
double cmd_line_ra;
double cmd_line_dec;
int fullNames;
int WITH_PHU;
int SOLO_PHU;
int ACCEPT_ASTROM;

// These functions belong to dvoImagesAtCoords
int  args_coords    	 PROTO((int argc, char **argv));
int  ConfigInit_coords PROTO((int *argc, char **argv));
int  Shutdown         PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2));
void TrapSignal       PROTO((int sig));
void SetProtect       PROTO((int mode));
int  SetSignals       PROTO((void));

off_t MatchCoords (Image *dbImages, off_t NdbImages, Point *points, int Npoints);
int GetFileMode (Header *header);
