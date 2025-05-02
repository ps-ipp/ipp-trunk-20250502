# include <signal.h>

enum {
  SPMODE_UKN,				/* unknown mode */
  SPMODE_PHU,				/* one spectrum, primary header unit */
  SPMODE_MEF,				/* spectrum group, extensions */
  SPMODE_EXT,				/* one of a spectrum group, extension */
  SPMODE_N
};

enum {
  SPSTATE_UKN,				/* unknown state */
  SPSTATE_RAW,				/* raw image counts */
  SPSTATE_FLT,				/* flattened image counts */
  SPSTATE_CLN,				/* clean extraction */
  SPSTATE_WAV,				/* wavelength calibrated */
  SPSTATE_FLX,				/* flux calibrated (implies WAV) */
  SPSTATE_N
};

struct {
  int Ntimes;           time_t *tstart, *tstop;
  int ModeSelect;       int Mode;
  int StateSelect;      int State;
  int ExptimeSelect;    float Exptime;
  int FilenameSelect;   char *Filename;
  int ObjectSelect;     char *Object;
  int TelescopeSelect;  char *Telescope;
  int InstrumentSelect; char *Instrument;
  int MatchNumber;
} criteria;

struct {
  int delete;
  int modify;

  int unique;
  int modify_path;
  char *oldpath, *newpath;

  int modify_mode, mode;
  int modify_state, state;

  int HST;
  int verbose;
  char *table;
  char *bintable;
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
int DUMP;

char SpectrumDB[64];
char ObjectKeyword[64];
char TelescopeKeyword[64];

int args (int argc, char **argv);
off_t *match_criteria (Spectrum *spectrum, off_t Nspectrum, off_t *Nmatch);
off_t *unique_entries (Spectrum *spectrum, off_t Nspectrum, off_t *subset, off_t *Nmatch);

void ModifySubset (FITS_DB *db, Spectrum *spectrum, off_t Nspectrum, off_t *match, off_t Nmatch);
void DeleteSubset (FITS_DB *db, Spectrum *spectrum, off_t Nspectrum, off_t *match, off_t Nmatch);
void OutputSubset (Spectrum *spectrum, off_t Nspectrum, off_t *match, off_t Nmatch);

void SetOutputMode (char *mode);
void DumpFitsBintable (char *filename, Spectrum *spectrum, off_t *match, off_t Nmatch);
void DumpFitsTable (char *filename, Spectrum *spectrum, off_t *match, off_t Nmatch);
int PrintSubset (Spectrum *spectrum, off_t *match, off_t Nmatch);
int dump_data (Spectrum *spectrum, off_t Nspectrum);

Spectrum *spinfo (char *filename);
off_t *match_spectrums (Spectrum *subset, off_t Nsubset, off_t *Nmatch);
int SubmitSpectrums (Spectrum *spectrum);
void set_timezone (double dt);

void showinfo (Spectrum *spec);
int escape (int mode, char *message);
int set_spectra (Spectrum *new, off_t Nnew);
Spectrum *get_spectra (off_t *N);

void ConfigInitSpec (int *argc, char **argv);
