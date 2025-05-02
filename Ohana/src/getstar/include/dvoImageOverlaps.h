# include <ohana.h>
# include <dvo.h>
# include <ReadImageFiles.h>
# include <signal.h>

enum {NONE, SIMPLE_CMP, SIMPLE_CMF, SIMPLE_MEF, MOSAIC_CMP, MOSAIC_CMF, MOSAIC_MEF, MOSAIC_PHU};

int       VERBOSE;

char OUTPUT[256];
char GSCFILE[256];
char CATDIR[256];
char CATMODE[16];    /* raw, mef, split, mysql */
char CATFORMAT[16];  /* internal, elixir, loneos, panstarrs */
char SKY_TABLE[256];
int  SKY_DEPTH;
char   ImageCat[256];
Coords *MOSAIC;         // carries the mosaic into ReadImageHeader

int WITH_PHU;
int SOLO_PHU;
int ACCEPT_ASTROM;

double MAX_CERROR;

int  args_overlaps    	 PROTO((int argc, char **argv));
int  ConfigInit_overlaps PROTO((int *argc, char **argv));
int  Shutdown         PROTO((char *format, ...) OHANA_FORMAT(printf, 1, 2));
void TrapSignal       PROTO((int sig));
void SetProtect       PROTO((int mode));
int  SetSignals       PROTO((void));

off_t *MatchImage (Image *dbImages, off_t NdbImages, Image *image, off_t *Nmatch);
int ListImageOverlaps (Image *dbImages, Image *image, off_t *matches, off_t Nmatches);

int GetFileMode (Header *header);
int edge_check (double *x1, double *y1, double *x2, double *y2);
double opening_angle (double x1, double y1, double x2, double y2, double x3, double y3);

void initMosaicCoords ();
void saveMosaicCoords (Coords *input);
