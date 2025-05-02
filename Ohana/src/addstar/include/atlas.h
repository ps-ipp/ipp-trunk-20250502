
// measure[0] = g, measure[1] = r, measure[2] = i,  measure[3] = z, measure[4] = y, measure[5] = J, measure[6] = H, measure[7] = K

typedef struct {
  Average average;
  Measure measure[8];
  int flag; // in a subset?
  int found; // assigned to an object?
} Atlas_Stars;

int   HOST_ID;
char *HOSTDIR;
char *CPT_FILE;
char *INPUT;

AddstarClientOptions args_loadatlas (int *argc, char **argv, AddstarClientOptions options);
// AddstarClientOptions args_loadatlas_client (int *argc, char **argv, AddstarClientOptions options);

int loadatlas_table (int Nstart, int Nend, SkyList *skylistInput, HostTable *hosts, char **filename, AddstarClientOptions *options);

Atlas_Stars *loadatlas_make_subset (Atlas_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

// int loadatlas_save_remote (Atlas_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname, AddstarClientOptions *options);
// int save_remote_host (HostInfo *host);

// int init_remote_hosts (void);
// void free_remote_hosts (void);
// int find_empty_slot (void);
// int harvest_all (void);
// int harvest_host (void);

int loadatlas_catalog (Atlas_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options);

// int galactic_to_celestial (double *R, double *D, double l, double b);

int find_matches_atlas (SkyRegion *region, Atlas_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options);

int loadatlas_save_stars (char *filename, Atlas_Stars *stars, int Nstars);
// Atlas_Stars *loadatlas_load_stars (char *filename, int *nstars);

Atlas_Stars *loadatlas_readstars (char *filename, Atlas_Stars *stars, int *nstars, AddstarClientOptions *options);

int loadatlas_sortStars (Atlas_Stars *stars, int Nstars);

// int loadatlas_tmpdir (void);

//  roll these into a function with a structure to carry the Nlast, etc?
char *dparse_csv_rpt (double *X, int Nwant, int Nlast, char *line, int *status);
char *iparse_csv_rpt (int *X,    int Nwant, int Nlast, char *line, int *status);
char *jparse_csv_rpt (uint64_t *X, int Nwant, int Nlast, char *line, int *status);
