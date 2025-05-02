
typedef struct {
  double R, D;
  Measure measure;
  int flag; // in a subset?
  int found; // assigned to an object?
} Gaia_Stars;

int   HOST_ID;
char *HOSTDIR;
char *CPT_FILE;
char *INPUT;

AddstarClientOptions args_loadgaia (int *argc, char **argv, AddstarClientOptions options);
AddstarClientOptions args_loadgaia_client (int *argc, char **argv, AddstarClientOptions options);

int loadgaia_table (int Nstart, int Nend, SkyList *skylistInput, HostTable *hosts, char **filename, AddstarClientOptions *options);

Gaia_Stars *loadgaia_make_subset (Gaia_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

int loadgaia_save_remote (Gaia_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname, AddstarClientOptions *options);

int save_remote_host (HostInfo *host);

int init_remote_hosts (void);
void free_remote_hosts (void);
int find_empty_slot (void);
int harvest_all (void);
int harvest_host (void);

int loadgaia_catalog (Gaia_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options);

int galactic_to_celestial (double *R, double *D, double l, double b);

int find_matches_gaia (SkyRegion *region, Gaia_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options);

int loadgaia_save_stars (char *filename, Gaia_Stars *stars, int Nstars);
Gaia_Stars *loadgaia_load_stars (char *filename, int *nstars);

Gaia_Stars *loadgaia_readstars (char *filename, Gaia_Stars *stars, int *nstars, AddstarClientOptions *options);

int loadgaia_sortStars (Gaia_Stars *stars, int Nstars);

int loadgaia_tmpdir (void);
