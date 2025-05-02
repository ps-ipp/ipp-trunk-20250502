
typedef enum {UKIRT_MODE_NONE, UKIRT_MODE_UHS, UKIRT_MODE_UGCS, UKIRT_MODE_UGPS, UKIRT_MODE_ULAS, UKIRT_MODE_UHS2022} UkirtMode;

# define NSTARS_MAX 10000000
//# define NSTARS_MAX 5000
// # define NSTARS_MAX 10

# define BUFFER_SIZE 0x100000
// # define BUFFER_SIZE 900

typedef struct {
  Average average;
  Measure *measure;
  int flag; // in a subset?
  int found; // assigned to an object?
} UKIRT_Stars;

int   HOST_ID;
char *HOSTDIR;
char *CPT_FILE;
char *INPUT;
int   UKIRT_NFILTER;

UkirtMode UKIRT_MODE;

AddstarClientOptions args_loadukirt_uhs (int *argc, char **argv, AddstarClientOptions options);
// AddstarClientOptions args_loadukirt_uhs_client (int *argc, char **argv, AddstarClientOptions options);

int loadukirt_uhs_table (SkyList *skylistInput, char *filename, AddstarClientOptions *options);

UKIRT_Stars *loadukirt_uhs_make_subset (UKIRT_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

// int loadukirt_uhs_save_remote (UKIRT_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname, AddstarClientOptions *options);
// int save_remote_host (HostInfo *host);

// int init_remote_hosts (void);
// void free_remote_hosts (void);
// int find_empty_slot (void);
// int harvest_all (void);
// int harvest_host (void);

int loadukirt_uhs_catalog (UKIRT_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options);

// int galactic_to_celestial (double *R, double *D, double l, double b);

int find_matches_ukirt_uhs (SkyRegion *region, UKIRT_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options);

int loadukirt_uhs_save_stars (char *filename, UKIRT_Stars *stars, int Nstars);
// UKIRT_Stars *loadukirt_uhs_load_stars (char *filename, int *nstars);

UKIRT_Stars *loadukirt_uhs_readstars (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars);

UKIRT_Stars *loadukirt_uhs_readstars_ugps (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars);
UKIRT_Stars *loadukirt_uhs_readstars_ugcs (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars);
UKIRT_Stars *loadukirt_uhs_readstars_ulas (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars);
UKIRT_Stars *loadukirt_uhs_readstars_uhs (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars);
UKIRT_Stars *loadukirt_uhs_readstars_uhs2022 (FILE *f, char *buffer, int *nstart, AddstarClientOptions *options, int *nstars);


int loadukirt_uhs_sortStars (UKIRT_Stars *stars, int Nstars);

// int loadukirt_uhs_tmpdir (void);

//  roll these into a function with a structure to carry the Nlast, etc?
char *dparse_csv_rpt (double *X, int Nwant, int Nlast, char *line, int *status);
char *iparse_csv_rpt (int *X,    int Nwant, int Nlast, char *line, int *status);
char *jparse_csv_rpt (uint64_t *X, int Nwant, int Nlast, char *line, int *status);

float psfQFfromXClass (int xClass);
