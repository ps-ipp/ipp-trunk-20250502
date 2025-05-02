
struct {
  int PhotcodeSelect; int photcode;
  int NameSelect;     char *Name;
  int Ntimes;         time_t *tstart, *tstop;
  int CodeSelect;     int Code;
} criteria;

struct {
  int   modify;
  char *ModifyValue;
  char *ModifyEntry;
  char *table;
  char *bintable;
} options;

int VERBOSE;
int FORCE_READ;

int db_load (FITS_DB *db);
int args (int argc, char **argv);
int DumpFitsBintable (char *filename, Image *image, off_t *match, off_t Nmatch);
int DumpFitsTable (char *filename, Image *image, off_t *match, off_t Nmatch);
void ModifySubset (FITS_DB *db, Image *image, off_t Nimage, off_t *match, off_t Nmatch);
int PrintSubset (Image *image, off_t *match, off_t Nmatch);
int output (Image *image, off_t *match, off_t Nmatch);
int rfits (FITS_DB *db);
int rtext (FITS_DB *db);
off_t *subset (Image *image, off_t Nimage, off_t *nsubset);
