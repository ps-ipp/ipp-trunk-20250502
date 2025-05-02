
/* structure for data on a catalog region */
typedef struct {
  char filename[256];
  double RA[2];
  int Nrec;
} TM_Region;

typedef struct {
  double Rmin, Rmax, Dmin, Dmax;
  int index[20];
  int Nindex;
} TMBands;

typedef struct {
  double R, D;
  int offset;
  int flag;
} TMStars;

short TM_J, TM_H, TM_K, TM_B, TM_V;

SkyTable *get2mass_acc (SkyRegion *patch, char *path, char *accel);

// Stars    *get2mass_2DR_data (SkyRegion *region, char *filename, SkyRegion *patch, int photcode, int *nstars);
// Stars    *get2mass_AS_data (SkyRegion *region, char *filename, SkyRegion *patch, int phocode, int *nstars);
// Stars    *get2mass_AS_rawdata (SkyRegion *region, char *filename, SkyRegion *patch, int phocode, int *nstars);

SkyTable *scan2mass_acc (char *path, char *accel);
int       scan2mass_as_data (char *filename);

char     *skipNbounds (char *line, char bound, int Nbound, int Nbyte);
e_time    get2mass_time (char *ptr, int Nbound, int Nbyte);
e_time    get2mass_date (char *ptr, int Nbound, int Nmax);

int       load2mass_as_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options);
SkyTable *load2mass_acc (char *path, char *accel);

// int       get2mass_3star (Stars *star, char *line, int Nmax);
// int       load2mass_catalog (Catalog *catalog, Stars *stars, int Nstars);

int       get2mass_setup (int photcode);
int       get2mass_coords (char *line, double *R, double *D, int Nmax);

// int       get2mass_star (Stars *star, char *line, int Nmax);
// int       get2mass_3star (Stars *star, char *line, int Nmax);

int get2mass_3star_full (Measure *measure, char *line, int *Nmeasure);
char *next2MASSfield (char *line);
int set2MASS_ph_qual (Measure *measure, char qual);
int set2MASS_rd_flag (Measure *measure, char qual);
int set2MASS_cc_flag (Measure *measure, char qual);
int set2MASS_bl_flag (Measure *measure, char qual);
int set2MASS_gal_flag (Measure *measure, char qual);
int set2MASS_mp_flag (Measure *measure, char qual);
int set2MASS_dup_flag (Measure *measure, char qual);
int set2MASS_use_flag (Measure *measure, char qual);
int get2mass_sortStars (TMStars *tstars, int Ntstars);
