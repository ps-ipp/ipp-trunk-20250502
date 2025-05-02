
typedef struct {
  double R, D;
  StarPar starpar;
  int flag; // in a subset?
  int found; // assigned to an object?
} StarPar_Stars;

int   HOST_ID;
char *HOSTDIR;
char *CPT_FILE;
char *INPUT;

AddstarClientOptions args_loadstarpar (int *argc, char **argv, AddstarClientOptions options);
AddstarClientOptions args_loadstarpar_client (int *argc, char **argv, AddstarClientOptions options);

int loadstarpar_table (SkyList *skylistInput, HostTable *hosts, char *filename, AddstarClientOptions *options);

StarPar_Stars *loadstarpar_make_subset (StarPar_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

int loadstarpar_save_remote (StarPar_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname, AddstarClientOptions *options);

int save_remote_host (HostInfo *host);

int init_remote_hosts (void);
void free_remote_hosts (void);
int find_empty_slot (void);
int harvest_all (void);
int harvest_host (void);

int loadstarpar_catalog (StarPar_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options);

int galactic_to_celestial (double *R, double *D, double l, double b);

int find_matches_starpar (SkyRegion *region, StarPar_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options);

int loadstarpar_save_stars (char *filename, StarPar_Stars *stars, int Nstars);
StarPar_Stars *loadstarpar_load_stars (char *filename, int *nstars);

StarPar_Stars *loadstarpar_readstars (char *filename, int *nstars);

int loadstarpar_sortStars (StarPar_Stars *stars, int Nstars);

int loadstarpar_tmpdir (void);
