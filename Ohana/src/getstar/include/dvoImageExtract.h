# include <ohana.h>
# include <dvo.h>
# include <signal.h>
# include <glob.h>

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
char *OUTFILE;
Coords *MOSAIC;         // carries the mosaic into ReadImageHeader

int  args_extract    	 PROTO((int argc, char **argv));
int  ConfigInit_extract  PROTO((int *argc, char **argv));
int  Shutdown         	 PROTO((char *format, ...)) OHANA_FORMAT(printf, 1, 2);
void TrapSignal       	 PROTO((int sig));
void SetProtect       	 PROTO((int mode));
int  SetSignals       	 PROTO((void));

int GetFileMode (Header *header);
int edge_check (double *x1, double *y1, double *x2, double *y2);

int  WriteImageFITS (FILE *f, Image *image);
int  WriteImages (char *filename, Image *images, off_t Nimages, off_t *matches, off_t Nmatches);
off_t *SelectImages (char *filename, Image *dbImages, off_t NdbImages, off_t *Nmatch);
