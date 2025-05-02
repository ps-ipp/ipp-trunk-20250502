# include <signal.h>

struct {
  int Ntimes;         time_t *tstart, *tstop;
  int PhotCodeSelect; int photcode;
  int LabelSelect;    char *Label;
} criteria;

struct {
  int delete;
  int modify;

  int HST;
  int verbose;
  int convert;
  char *table;
  char *bintable;
  char *cadctable;

  int equiv;
  int offset;
  char *db;
  int photcodenames;
} output;

enum {UNLOCK, LOCK, IGNORE};

int args (int argc, char **argv);
int regargs (int argc, char **argv, PhotPars *);

void DeleteSubset (FITS_DB *db, PhotPars *photpars, off_t Nphotpars, off_t *match, off_t Nmatch);
off_t *match_criteria (PhotPars *photpars, off_t Nphotpars, off_t *Nmatch);

void OutputSubset (PhotPars *photpars, off_t Nphotpars, off_t *match, off_t Nmatch);

void DumpFitsBintable (char *filename, PhotPars *image, off_t *match, off_t Nmatch);
void DumpFitsTable (char *filename, PhotPars *image, off_t *match, off_t Nmatch);
int PrintSubset (PhotPars *image, off_t *match, off_t Nmatch);

void set_timezone (double dt);
void getImageData (char *Image, char *ImageCCD, char *ImageMode);
int escape (int mode, char *message);

PhotPars *PhotParsOld_to_PhotPars (PhotParsOld *input, off_t Nphotpars);

