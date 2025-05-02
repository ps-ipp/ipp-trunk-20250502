
// measure[0] = g, measure[1] = b, measure[2] = r, 

typedef struct {
  Average average;
  Measure measure[3];
  int flag; // in a subset?
  int found; // assigned to an object?
} Gaia_EDR3_Stars;

int   HOST_ID;
char *HOSTDIR;
char *CPT_FILE;
char *INPUT;

AddstarClientOptions args_loadgaia_edr3 (int *argc, char **argv, AddstarClientOptions options);
// AddstarClientOptions args_loadgaia_edr3_client (int *argc, char **argv, AddstarClientOptions options);

int loadgaia_edr3_table (int Nstart, int Nend, SkyList *skylistInput, HostTable *hosts, char **filename, AddstarClientOptions *options);

Gaia_EDR3_Stars *loadgaia_edr3_make_subset (Gaia_EDR3_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

// int loadgaia_edr3_save_remote (Gaia_EDR3_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname, AddstarClientOptions *options);
// int save_remote_host (HostInfo *host);

// int init_remote_hosts (void);
// void free_remote_hosts (void);
// int find_empty_slot (void);
// int harvest_all (void);
// int harvest_host (void);

int loadgaia_edr3_catalog (Gaia_EDR3_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options);

// int galactic_to_celestial (double *R, double *D, double l, double b);

int find_matches_gaia_edr3 (SkyRegion *region, Gaia_EDR3_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options);

int loadgaia_edr3_save_stars (char *filename, Gaia_EDR3_Stars *stars, int Nstars);
// Gaia_EDR3_Stars *loadgaia_edr3_load_stars (char *filename, int *nstars);

Gaia_EDR3_Stars *loadgaia_edr3_readstars (char *filename, Gaia_EDR3_Stars *stars, int *nstars, AddstarClientOptions *options);

int loadgaia_edr3_sortStars (Gaia_EDR3_Stars *stars, int Nstars);

// int loadgaia_edr3_tmpdir (void);

//  roll these into a function with a structure to carry the Nlast, etc?
char *dparse_csv_rpt (double *X, int Nwant, int Nlast, char *line, int *status);
char *iparse_csv_rpt (int *X,    int Nwant, int Nlast, char *line, int *status);
char *jparse_csv_rpt (uint64_t *X, int Nwant, int Nlast, char *line, int *status);
