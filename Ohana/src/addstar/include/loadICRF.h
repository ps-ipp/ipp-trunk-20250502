
typedef struct {
  double R, D;
  Measure measure;
  int found; // TRUE == matched to an input star
  int flag; // TRUE == included in a subset
} ICRF_Stars;

int   HOST_ID;
char *HOSTDIR;
char *CPT_FILE;
char *INPUT;

AddstarClientOptions args_loadICRF (int *argc, char **argv, AddstarClientOptions options);
AddstarClientOptions args_loadICRF_client (int *argc, char **argv, AddstarClientOptions options);

int loadICRF_table (SkyList *skylistInput, HostTable *hosts, char *filename, AddstarClientOptions *options);

ICRF_Stars *loadICRF_make_subset (ICRF_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

int loadICRF_save_remote (ICRF_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname, AddstarClientOptions *options);

int save_remote_host (HostInfo *host);

int init_remote_hosts ();
int find_empty_slot ();
int harvest_all ();
int harvest_host ();

int loadICRF_catalog (ICRF_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options);

int galactic_to_celestial (double *R, double *D, double l, double b);

int find_matches_ICRF (SkyRegion *region, ICRF_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options);

int loadICRF_save_stars (char *filename, ICRF_Stars *stars, int Nstars);
ICRF_Stars *loadICRF_load_stars (char *filename, int *nstars);

ICRF_Stars *loadICRF_readstars (char *filename, int *nstars);

int loadICRF_sortStars (ICRF_Stars *stars, int Nstars);
int InitICRF_Star (ICRF_Stars *star);
