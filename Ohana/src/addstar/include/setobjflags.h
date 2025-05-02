
typedef struct {
  double R, D;
//e_time time;
//short filter;
  int myBit;
  int flag; // in a subset?
  int found; // assigned to an object?
} MyStars;

int   HOST_ID;
char *HOSTDIR;
char *CPT_FILE;
char *INPUT;

char *SRC_FILES;
int   SRC_MYBIT;

double SRC_RADIUS;

int ConfigInit_setobjflags (int *argc, char **argv);

int args_setobjflags (int *argc, char **argv);
int args_setobjflags_client (int *argc, char **argv);

int setobjflags_table (SkyList *skylistInput, HostTable *hosts);

MyStars *setobjflags_make_subset (MyStars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

int setobjflags_save_remote (MyStars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname);

int save_remote_host (HostInfo *host);

int init_remote_hosts (void);
void free_remote_hosts (void);
int find_empty_slot (void);
int harvest_all (void);
int harvest_host (void);

int setobjflags_catalog (MyStars *stars, int Nstars, SkyRegion *region, char *filename);

int galactic_to_celestial (double *R, double *D, double l, double b);

int find_matches_setobjflags (SkyRegion *region, MyStars *stars, int Nstars, Catalog *catalog);

int setobjflags_save_stars (char *filename, MyStars *stars, int Nstars);
MyStars *setobjflags_load_stars (char *filename, int *nstars);

MyStars *setobjflags_loadfile (int *Nstars);

int setobjflags_sortStars (MyStars *stars, int Nstars);
int setobjflags_tmpdir (void);

