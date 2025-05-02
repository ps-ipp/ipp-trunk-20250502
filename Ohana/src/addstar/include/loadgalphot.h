
int IMAGE_ID;

typedef struct {
  double **s;
  double **b;
  double **c;
  double **C;
  double  *B;
  double *Cii;
  int order;
  int nterm;
  int wterm;
  int mterm;
  float mean;
  float sigma;
  double c00;
  double c10;
  double c20;
  double c01;
  double c11;
  double c02;
  int ClipNiter;
  float ClipNsigma;
} Fit2D;

typedef struct {
  double R, D;
  GalPhot galphot;
  int flag; // in a subset?
  int found; // assigned to an object?
} GalPhot_Stars;

typedef struct {
  int *ID;
  int *type;
  int  N;
} GalPhotIDset;

AddstarClientOptions args_loadgalphot (int *argc, char **argv, AddstarClientOptions options);

int loadgalphot_table (SkyList *skylistInput, HostTable *hosts, char *filename, AddstarClientOptions *options);

GalPhot_Stars *loadgalphot_readstars (char *filename, int *nstars, AddstarClientOptions *options);

int loadgalphot_sortStars (GalPhot_Stars *stars, int Nstars);

GalPhot_Stars *loadgalphot_make_subset (GalPhot_Stars *stars, int Nstars, int start, SkyRegion *region, int *nsubset);

int loadgalphot_catalog (GalPhot_Stars *stars, int Nstars, SkyRegion *region, char *filename, AddstarClientOptions *options);

int find_matches_galphot (SkyRegion *region, GalPhot_Stars *stars, int Nstars, Catalog *catalog, AddstarClientOptions *options);

int FitChisqMinimum (Fit2D *fit, GalPhot *galphot, 
		     float *chisq, float *flux, float *fluxErr, int Npts, 
		     float MajorMin, float MajorMax, float MajorDel, 
		     float MinorMin, float MinorMax, float MinorDel);

Fit2D *fit2d_init (int order);
void fit2d_free (Fit2D *fit);
int fit2d (Fit2D *fit, float *xval, float *yval, float *zval, float *zfit, char *mask, int Npts);
int fit2d_reset (Fit2D *fit);

int *join_IDs (GalPhotIDset *gal, GalPhotIDset *fit);
int strhash (char *string, int module);

void sort_IDs_seqonly (int *X, int *S, int N);
