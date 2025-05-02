
typedef struct {
  int state; /* none, close, perfect */
  int crit;  /* which criterion? */
  int image; /* which detrend image? */
} Match;

typedef struct {
  int TimeSelect;    time_t tstart, tstop;
  char *label; 
  char *imageID;
  int order;
  int type;
  char mode;
  int CCDSelect;     int CCD;
  int ExptimeSelect; float Exptime;
  int filter;
} Descriptor;

/* MosaicSelect & ImageSelect define these values */
typedef struct {
  int ModeSelect;    int Mode;
  int TypeSelect;    int Type;
  int CCDSelect;     int CCD;
  int FilterSelect;  int Filter;
  int EntrySelect;   int Entry;
  int LabelSelect;   char *Label;
  int NameSelect;    char *Name;
  int ExptimeSelect; float Exptime;
  int TimeSelect;    time_t tstart, tstop;
  int MatchNumber;
} Criteria;

int Ncriteria;
Criteria *criteria;

struct {
  int Select;
  int Delete;
  int Modify;
  int Altpath;
  int ElixirSmart;
  int TimeMode;
  int Recipe;
  int Close;
  int Criteria;
  int Chipname;

  char *ModifyEntry, *ModifyValue;
  time_t TimeValue;
  
  int verbose;
  char *table;
  char *bintable;
} output;

/* altpath values */
enum {NONE, ADD, DELETE, UPDATE};
enum {START, STOP, REG};
enum {UNLOCK, LOCK, IGNORE};
enum {MATCH_NONE, MATCH_CLOSE, MATCH_PERFECT};

# define DEBUG 0

char **RecipeType;
int Nrecipe;

int SingleIsSplit;
int NoReg;

int regargs (int argc, char **argv, Descriptor *descriptor);
int args (int argc, char **argv);
int DefineImage (char *filename, Descriptor *descriptor);
DetReg DefineEntry (Descriptor descriptor);

char *set_dBFile (void);
char *get_dBPath (void);
int delete_image (DetReg *image);

int SaveEntry (char *input, DetReg *newdata, char *ID);

char **LoadRecipe (char *filter, int *nrecipe);

Match *MatchCriteria (DetReg *image, off_t Nimage, off_t *nmatch);
Match *UniqueSubset (DetReg *image, off_t Nimage, Match *match, off_t *nmatch);
Match *ExptimeCriteria (DetReg *image, off_t Nimage, Match *match, off_t *nmatch);
Match *CloseCriteria (DetReg *image, off_t Nimage, Match *match, off_t *nmatch);

Match CheckCriteria (DetReg *image);

int OutputSubset (DetReg *image, off_t Nimage, Match *match, off_t Nmatch);
int DumpFitsTable (char *filename, DetReg *detdata, Match *match, off_t Nmatch);
int PrintSubset (DetReg *detdata, Match *match, off_t Nmatch);
Match SelectEntry (DetReg *image, off_t Nimage, Match *list, off_t Nlist, Criteria *crit);

int usage (void);
Criteria *MosaicCriteria (Criteria base, char *filename, int *ncrit);
Criteria *ImageCriteria (Criteria base, char *filename, char *ImageExtend, char *ImageMode, int *ncrit);
Criteria *ExpandBase (Criteria base, int *ncrit, time_t *tstart, time_t *tstop, int *filt);
Criteria *ExpandRecipe (Criteria *base, int *Ncrit);

int escape (int mode, char *message);
int DumpFitsBintable (char *filename, DetReg *image, Match *match, off_t Nmatch);
int cmp_crit (Criteria *crit, DetReg *image);
int set_crit (Criteria *crit, DetReg *image);
int ckpathname (char *newpath);
int PrintCriteria (void);

int SetAltpath (FITS_DB *db, DetReg *image, off_t Nimage, Match *match, off_t Nmatch);
int ModifySubset (FITS_DB *db, DetReg *image, off_t Nimage, Match *match, off_t Nmatch);
void DeleteSubset (FITS_DB *db, DetReg *image, off_t Nimage, Match *match, off_t Nmatch);
