# include <signal.h>

typedef struct {
  char *buffer;
  int   Nalloc;
  int   Nmaxread;
  int   Nextra;
  int   Nlast;
  int   Nbuffer;
} Fifo;

struct {
  int ModeSelect;    int Mode;
  int TypeSelect;    int Type;
  int CCDSelect;     int CCD;
  int FilterSelect;  char *Filter;
  int EntrySelect;   int Entry;
  int LabelSelect;   char *Label;
  int ExptimeSelect; float Exptime;
  int TimeSelect;    unsigned long Time;
  int NameSelect;    char *Name;
  int ProcSelect;    int Proc;
  int DistSelect;    int Dist;
  int Ntimes;        time_t *tstart, *tstop;
  int MatchNumber;
  int Close;
} criteria;

struct {
  int delete;
  int modify;

  int modify_path;
  char *oldpath, *newpath;

  int unique;
  int mef2split, split2mef;
  int modify_dist, dist;
  int modify_filter, modify_type, type;
  char *filter;

  int HST;
  int verbose;
  char *table;
  char *bintable;
  char *cadctable;
} output;

typedef struct {
  int *ccd;
  int Nccd;
  char name[64];
} MosaicRegion;

typedef struct {
  MosaicRegion center;
  MosaicRegion outer;
  MosaicRegion top;
  MosaicRegion bottom;
  MosaicRegion left;
  MosaicRegion right;
} MosaicLayout;

enum {UNLOCK, LOCK, IGNORE};

int SingleIsSplit;
int NeedType;
int NoReg;
int IMSORT;
int CLIENT;
char *PIDFILE;
int LOOP_DELAY;

int args (int argc, char **argv);

void DeleteSubset (FITS_DB *db, RegImage *image, off_t Nimage, off_t *match, off_t Nmatch);
RegImage *iminfo (char *filename);

off_t *match_criteria (RegImage *image, off_t Nimage, off_t *Nmatch);
off_t *match_images (RegImage *image, off_t Nimage, RegImage *subset, off_t Nsubset, off_t *Nmatch);

RegImage *newimages (RegImage *image, off_t *Nimage);

void ModifySubset (FITS_DB *db, RegImage *image, off_t Nimage, off_t *match, off_t Nmatch);
void SetOutputMode (char *mode);
void OutputSubset (RegImage *image, off_t Nimage, off_t *match, off_t Nmatch);
void DumpFitsBintable (char *filename, RegImage *image, off_t *match, off_t Nmatch);
void DumpFitsTable (char *filename, RegImage *image, off_t *match, off_t Nmatch);
int PrintSubset (RegImage *image, off_t *match, off_t Nmatch);
int dump_data (RegImage *image, off_t Nimage);
int SubmitImages (RegImage *image);
int load_probes (char *filename, unsigned long tzero, int *wantprobe, double *values, int Nprobe);
int define_table (Header *header, Matrix *matrix, Header *theader, FTable *table);
void set_timezone (double dt);

void DumpCADCTable (char *filename, RegImage *image, off_t *match, off_t Nmatch);
off_t *GetObsIDSubset (RegImage *image, off_t start, off_t *index, off_t *entry, off_t Nindex, off_t *Nsubset);
off_t *GetUniqueObsID (RegImage *image, off_t *index, off_t *entry, off_t Nindex, off_t *Nmatch);
void GetObsIDIndex (RegImage *image, off_t *match, off_t Nmatch, off_t **Index, off_t **Entry);
double SigmaClipList (double *list, int N);
double MosaicIQStats (RegImage *image, off_t *match, off_t Nmatch, MosaicRegion *region);
MosaicLayout *CreateCFH12K (void);
MosaicLayout *CreateMegaCam (void);
off_t GetREFCCD (RegImage *image, off_t *index, off_t *entry, off_t Nindex, off_t start);

int imregclient (char *fitsfile, char *statfile, char *datfile);
void SIG_DIE (int sig);
void SIG_PIPE (int sig);
void SetSignals (void);
void KillProcess (char *pidfile);
void StatusProcess (char *pidfile);
int close_lock_db (void);
int ConfigPID (char *pidfile);
int print_db_status (char *message);
int escape (int mode, char *message);

int LoadPID (char *file, pid_t *pid, char *username, char *machine);
int Shutdown (int status);
void RemovePID (void);

int InitFifo (Fifo *fifo, int Nalloc, int Nextra);
int FlushFifo (Fifo *fifo);
int ShiftFifo (Fifo *fifo);
int ReadtoFifo (Fifo *fifo, int sock);
void FreeFifo (Fifo *fifo);
int SockScan (char *string, Fifo *fifo, int sock);

off_t *unique_entries (RegImage *image, off_t Nimage, off_t *subset, off_t *Nmatch);
