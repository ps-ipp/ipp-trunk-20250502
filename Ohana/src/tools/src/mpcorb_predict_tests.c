# include <ohana.h>
# include <gfitsio.h>
# include <convert.h>
# include <slalib.h>

/* convert an MPCORB.DAT file to a useful format that can be loaded by mknight */

/* The MPCORB file is NOT fixed bytes/row.  We need to handle fractional lines at the end
 * of each read block.
 */

/* read in chunks of ~64MB */
# define NBYTE 0x400000
# define DEBUG 0
# define VERBOSE 1
# define TESTING 0

typedef struct {
  char   ID[8];
  double epoch;
  double mean_anomaly;
  double perihelion;
  double ascend_node;
  double inclination;
  double eccentricity;
  double semimajor_a;
  double mjdMin;
  double mjdMax;
  double RatMin;
  double RatMax;
  double DatMin;
  double DatMax;
} Planets;

typedef struct {
  char   ID[8];
  double Robs;
  double Dobs;
  double Rvel;
  double Dvel;
  double dist;
} PlanetDatum;

int Shutdown (char *format, ...);
int mpcorb_parseline (char *line, Planets *planet, int Nmax);
Planets *mpcorb_read_text (char *filename, int *nplanets);
Planets *mpcorb_read_fits (char *filename, int *nplanets);
void     mpcorb_save_fits (char *filename, Planets *planets, int Nplanets);
void     mpcorb_refs_fits (char *filename, PlanetDatum *planets, int Nplanets);

PlanetDatum mpcorb_predict (Planets *planet, double mjdObs, int FullCalc);

void mpcorb_trange (int argc, char **argv);
void mpcorb_moment (int argc, char **argv);

void mpcorb_testing (int argc, char **argv);

void mpcorb_predict_test ();
void mpcorb_predict_test_calc (Planets *planet, double mjdObs, double *Robs, double *Dobs);

# define PS1_LONGITUDE -156.2559041668965 /* degrees East */
# define PS1_LATITUDE    20.7070994446013 /* degrees North */
# define PS1_ALTITUDE  3067.7 /* meters */

// some options (for testing)
int USE_AMP                  = FALSE; // otherwise use precess and/or nutation
int USE_PLANETARY_ABERRATION = FALSE; 
int USE_MANUAL_PRECESS       = FALSE; // otherwise use slaPreces
int USE_PRENUT               = FALSE; // otherwise slaPrec

int main (int argc, char **argv) {

  int N;

  if (TESTING) mpcorb_testing (argc, argv);

  // -none option so I can always supply an option in dvo script
  if ((N = get_argument (argc, argv, "-none"))) {
    remove_argument (N, &argc, argv);
  }

  // turn on / off some options
  if ((N = get_argument (argc, argv, "-use-amp"))) {
    USE_AMP = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-apply-planet-ab"))) {
    USE_PLANETARY_ABERRATION = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-use-manual-precess"))) {
    if (USE_AMP) Shutdown ("-use-amp and -use-manual-precess are incompatible");
    USE_MANUAL_PRECESS = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-use-prenut"))) {
    if (USE_AMP) Shutdown ("-use-amp and -use-prenut are incompatible");
    if (!USE_MANUAL_PRECESS) Shutdown ("-use-prenut requires -use-manual-precess");
    USE_PRENUT = TRUE;
    remove_argument (N, &argc, argv);
  }

  if ((argc != 9) && (argc != 10)) goto usage;

  if (!strcasecmp (argv[1], "trange")) mpcorb_trange (argc, argv);
  if (!strcasecmp (argv[1], "moment")) mpcorb_moment (argc, argv);

usage:
  fprintf (stderr, "USAGE: %s trange (MPCORB.DAT) (Tmin) (Tmax) (Rmin) (Rmax) (Dmin) (Dmax) (range.fits)\n", argv[0]);
  fprintf (stderr, "USAGE: %s moment (range.fits) (Tobs) (Rmin) (Rmax) (Dmin) (Dmax) (output)\n", argv[0]);

  fprintf (stderr, " trange : generate a table of asteroid orbital elements which fall within the region at Tmin or Tmax (in MJD)\n");
  fprintf (stderr, " moment : generate a table of asteroid positions which fall within the region at Tobs (in MJD)\n");

  exit (2);
}

// generate a table of asteroid orbital elements which fall within the region at Tmin or Tmax (in MJD)
void mpcorb_trange (int argc, char **argv) {

  if (argc != 10) Shutdown ("USAGE: %s trange (MPCORB.DAT) (Tmin) (Tmax) (Rmin) (Rmax) (Dmin) (Dmax) (range.fits)\n", argv[0]);

  char  *filename = argv[2];
  double mjdMin   = atof (argv[3]);
  double mjdMax   = atof (argv[4]);
  double Rmin     = atof (argv[5]);
  double Rmax     = atof (argv[6]);
  double Dmin     = atof (argv[7]);
  double Dmax     = atof (argv[8]);
  char  *output   = argv[9];

  int Nplanets = 0;
  Planets *planets = mpcorb_read_text (filename, &Nplanets);
  if (VERBOSE) fprintf (stderr, "loaded %d planets\n", Nplanets);

  int NplanetsSave = 0;
  int NPLANETSSAVE = 1000;
  ALLOCATE_PTR (planetsSave, Planets, NPLANETSSAVE);

  // transform all objects and identify those in the target region:
  for (int i = 0; i < Nplanets; i++) {
    if (i % 1000 == 0) fprintf (stderr, ".");

    // is the object in the region for the start of the period?
    PlanetDatum plMin = mpcorb_predict (&planets[i], mjdMin, FALSE);
    int inRangeMin = TRUE;
    inRangeMin = inRangeMin && (plMin.Robs > Rmin);
    inRangeMin = inRangeMin && (plMin.Robs < Rmax);
    inRangeMin = inRangeMin && (plMin.Dobs > Dmin);
    inRangeMin = inRangeMin && (plMin.Dobs < Dmax);
    
    // is the object in the region for the end of the period?
    PlanetDatum plMax = mpcorb_predict (&planets[i], mjdMax, FALSE);
    int inRangeMax = TRUE;
    inRangeMax = inRangeMax && (plMax.Robs > Rmin);
    inRangeMax = inRangeMax && (plMax.Robs < Rmax);
    inRangeMax = inRangeMax && (plMax.Dobs > Dmin);
    inRangeMax = inRangeMax && (plMax.Dobs < Dmax);

    // NOTE: if an object is moving so fast that it traverses the entire region, we will
    // miss it.  I'm going to ignore this case for now.
    if (!inRangeMin && !inRangeMax) continue;

    planetsSave[NplanetsSave] = planets[i];
    planetsSave[NplanetsSave].mjdMin = mjdMin;
    planetsSave[NplanetsSave].RatMin = plMin.Robs;
    planetsSave[NplanetsSave].DatMin = plMin.Dobs;
    planetsSave[NplanetsSave].mjdMax = mjdMax;
    planetsSave[NplanetsSave].RatMax = plMax.Robs;
    planetsSave[NplanetsSave].DatMax = plMax.Dobs;
    
    NplanetsSave++;
    CHECK_REALLOCATE (planetsSave, Planets, NPLANETSSAVE, NplanetsSave, 1000);
  }
  fprintf (stderr, "\n");

  // NOTE : here we are saving the full orbital elements
  mpcorb_save_fits (output, planetsSave, NplanetsSave);

  exit (0);
}

# define VERBOSE_TEST FALSE

// generate a table of asteroid positions which fall within the region at Tobs (in MJD)
void mpcorb_moment (int argc, char **argv) {

  if (argc != 9) Shutdown ("USAGE: %s moment (range.fits) (Tobs) (Rmin) (Rmax) (Dmin) (Dmax) (output)\n", argv[0]);

  char  *filename = argv[2];
  double mjdObs   = atof (argv[3]);
  double Rmin     = atof (argv[4]);
  double Rmax     = atof (argv[5]);
  double Dmin     = atof (argv[6]);
  double Dmax     = atof (argv[7]);
  char  *output   = argv[8];

  int Nplanets = 0;
  Planets *planets = mpcorb_read_fits (filename, &Nplanets);
  if (VERBOSE) fprintf (stderr, "loaded %d planets\n", Nplanets);

  int NplanetsSave = 0;
  int NPLANETSSAVE = 1000;
  ALLOCATE_PTR (planetsSave, PlanetDatum, NPLANETSSAVE);

  if (VERBOSE_TEST) {
    // XXX for a test, I print out coords at steps in myAmpqk
    for (int i = 0; i < 10; i++) {
      mpcorb_predict (&planets[i], mjdObs, TRUE);
    }
    exit (0);
  }

  // transform all objects and identify those in the target region:
  for (int i = 0; i < Nplanets; i++) {
    if (i % 1000 == 0) fprintf (stderr, ".");

    // is the object in the region for the start of the period?
    // first, use the fast, but inaccurate, calculation
    PlanetDatum planetFast = mpcorb_predict (&planets[i], mjdObs, FALSE);
    if (planetFast.Robs < Rmin) continue;
    if (planetFast.Robs > Rmax) continue;
    if (planetFast.Dobs < Dmin) continue;
    if (planetFast.Dobs > Dmax) continue;
    
    // if it is in the region, use the more accurate calculation
    PlanetDatum planetSlow = mpcorb_predict (&planets[i], mjdObs, TRUE);

    // fprintf (fout, "%8s %12.6f %12.6f\n", planets[i].ID, Robs, Dobs);

    // save this prediction, with R,D in both @min & @max
    planetsSave[NplanetsSave] = planetSlow;
    strcpy (planetsSave[NplanetsSave].ID, planets[i].ID); // ID is not passed to mpcorb_predict
    
    NplanetsSave++;
    CHECK_REALLOCATE (planetsSave, PlanetDatum, NPLANETSSAVE, NplanetsSave, 1000);
  }
  fprintf (stderr, "\n");

  mpcorb_refs_fits (output, planetsSave, NplanetsSave);

  exit (0);
}

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...) if (!(STATUS)) { Shutdown (MSG, __VA_ARGS__); }

// write minor planet positions (Rref,Dref,Mref) to a FITS table
void mpcorb_refs_fits (char *filename, PlanetDatum *planets, int Nplanets) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "DATA");

  gfits_define_bintable_column (&theader, "8A", "ID",   "name",    "none",        1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "Rref", "RA",      "degrees",     1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "Dref", "DEC",     "degrees",     1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "Rvel", "RA_VEL",  "arcsec/min",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "Dvel", "DEC_VEL", "arcsec/min",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "dist", "DIST",    "AU",          1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "Mref", "Mag",     "magnitudes",  1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // create intermediate storage arrays
  ALLOCATE_PTR (ID,                     char, Nplanets*8);
  ALLOCATE_PTR (Rref,                 double, Nplanets);
  ALLOCATE_PTR (Dref,                 double, Nplanets);
  ALLOCATE_PTR (Rvel,                 double, Nplanets);
  ALLOCATE_PTR (Dvel,                 double, Nplanets);
  ALLOCATE_PTR (dist,                 double, Nplanets);
  ALLOCATE_PTR (Mref,                 double, Nplanets);

  // set intermediate storage arrays
  for (int i = 0; i < Nplanets; i++) {
    Rref[i]        = planets[i].Robs;
    Dref[i]        = planets[i].Dobs;
    Rvel[i]        = planets[i].Rvel;
    Dvel[i]        = planets[i].Dvel;
    dist[i]        = planets[i].dist;
    Mref[i]        = 16.0; // need to add this to prediction
    memcpy(&ID[i*8], planets[i].ID, 8);
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "ID",   ID,   Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Rref", Rref, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Dref", Dref, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Rvel", Rvel, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Dvel", Dvel, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "dist", dist, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Mref", Mref, Nplanets);

  // free intermediate storage arrays
  free (ID);
  free (Rref);
  free (Dref);
  free (Rvel);
  free (Dvel);
  free (dist);
  free (Mref);

  // open the output file
  FILE *f = fopen (filename, "w");
  if (!f) Shutdown ("ERROR: cannot open mpcorb file for output %s\n", filename);

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for mpcorbs %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for mpcorbs %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for mpcorbs %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for mpcorbs %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file mpcorbs %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file mpcorbs %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing mpcorbs file %s\n", filename);

  return;
}

// write minor planet orbital elements (plus (R,D,T)[min,max]) to a FITS table
void mpcorb_save_fits (char *filename, Planets *planets, int Nplanets) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "MPCORB");

  gfits_define_bintable_column (&theader, "8A", "ID",           "name",         "none",     1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "EPOCH",        "epoch",        "MJD",      1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "MEAN_ANOMALY", "mean_anomaly", "degrees",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "PERIHELION",   "perihelion",   "degrees",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "ASCEND_NODE",  "ascend_node",  "degrees",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "INCLINATION",  "inclination",  "degrees",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "ECCENTRICITY", "eccentricity", "unitless", 1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "SEMIMAJOR_A",  "semimajor_a",  "AU",       1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "MJD_MIN",      "mjdMin",       "MJD",      1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "MJD_MAX",      "mjdMax",       "MJD",      1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "RAT_MIN",      "RatMin",       "degrees",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "RAT_MAX",      "RatMax",       "degrees",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "DAT_MIN",      "DatMin",       "degrees",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "DAT_MAX",      "DatMax",       "degrees",  1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // create intermediate storage arrays
  ALLOCATE_PTR (ID,                     char,   Nplanets*8);
  ALLOCATE_PTR (epoch,                  double, Nplanets);
  ALLOCATE_PTR (mean_anomaly,	        double, Nplanets);
  ALLOCATE_PTR (perihelion,	        double, Nplanets);
  ALLOCATE_PTR (ascend_node,	        double, Nplanets);
  ALLOCATE_PTR (inclination,	        double, Nplanets);
  ALLOCATE_PTR (eccentricity,	        double, Nplanets);
  ALLOCATE_PTR (semimajor_a,	        double, Nplanets);
  ALLOCATE_PTR (mjdMin, 	        double, Nplanets);
  ALLOCATE_PTR (mjdMax, 	        double, Nplanets);
  ALLOCATE_PTR (RatMin, 	        double, Nplanets);
  ALLOCATE_PTR (RatMax, 	        double, Nplanets);
  ALLOCATE_PTR (DatMin, 	        double, Nplanets);
  ALLOCATE_PTR (DatMax, 	        double, Nplanets);

  // set intermediate storage arrays
  for (int i = 0; i < Nplanets; i++) {
    epoch[i]           = planets[i].epoch;
    mean_anomaly[i]    = planets[i].mean_anomaly;
    perihelion[i]      = planets[i].perihelion;
    ascend_node[i]     = planets[i].ascend_node;
    inclination[i]     = planets[i].inclination;
    eccentricity[i]    = planets[i].eccentricity;
    semimajor_a[i]     = planets[i].semimajor_a;
    mjdMin[i]          = planets[i].mjdMin;
    mjdMax[i]          = planets[i].mjdMax;
    RatMin[i]          = planets[i].RatMin;
    RatMax[i]          = planets[i].RatMax;
    DatMin[i]          = planets[i].DatMin;
    DatMax[i]          = planets[i].DatMax;

    memcpy(&ID[i*8], planets[i].ID, 8);
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "ID",           ID,           Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "EPOCH",        epoch,        Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "MEAN_ANOMALY", mean_anomaly, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "PERIHELION",   perihelion,   Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "ASCEND_NODE",  ascend_node,  Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "INCLINATION",  inclination,  Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "ECCENTRICITY", eccentricity, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "SEMIMAJOR_A",  semimajor_a,  Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "MJD_MIN",      mjdMin,       Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "MJD_MAX",      mjdMax,       Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "RAT_MIN",      RatMin,       Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "RAT_MAX",      RatMax,       Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "DAT_MIN",      DatMin,       Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "DAT_MAX",      DatMax,       Nplanets);

  // free intermediate storage arrays
  free (ID);
  free (epoch);
  free (mean_anomaly);
  free (perihelion);
  free (ascend_node);
  free (inclination);
  free (eccentricity);
  free (semimajor_a);
  free (mjdMin);
  free (mjdMax);
  free (RatMin);
  free (RatMax);
  free (DatMin);
  free (DatMax);

  // open the output file
  FILE *f = fopen (filename, "w");
  if (!f) Shutdown ("ERROR: cannot open mpcorb file for output %s\n", filename);

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for mpcorbs %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for mpcorbs %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for mpcorbs %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for mpcorbs %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file mpcorbs %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file mpcorbs %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing mpcorbs file %s\n", filename);

  return;
}

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

// load planet data from a fits table
Planets *mpcorb_read_fits (char *filename, int *nplanets) {

  int Ncol;
  off_t Nrow;
  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  FILE *f = fopen (filename, "r");
  if (!f) Shutdown ("ERROR: cannot open mpcorb fits file %s\n", filename);

  // load in PHU segment (ignore)
  if (!gfits_fread_header (f, &header)) Shutdown ("can't read mpcorb fits file header %s\n", filename);
  if (!gfits_fread_matrix (f, &matrix, &header)) Shutdown ("can't read mpcorb fits file matrix %s\n", filename);
  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) Shutdown ("can't read mpcorb fits file table header %s\n", filename);
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) Shutdown ("can't read mpcorb fits file table data %s\n", filename);

  // we read the entire block of data, then extract the columns, then set the Planets structure values.
  // I free the FITS table data after extracting the colums to avoid having 3 copies in memory.

  char type[16]; // used by the functions called by GET_COLUMN

  GET_COLUMN (ID,           "ID",            char);
  GET_COLUMN (epoch,        "EPOCH",         double);
  GET_COLUMN (mean_anomaly, "MEAN_ANOMALY",  double);
  GET_COLUMN (perihelion,   "PERIHELION",    double);
  GET_COLUMN (ascend_node,  "ASCEND_NODE",   double);
  GET_COLUMN (inclination,  "INCLINATION",   double);
  GET_COLUMN (eccentricity, "ECCENTRICITY",  double);
  GET_COLUMN (semimajor_a,  "SEMIMAJOR_A",   double);
  GET_COLUMN (mjdMin,       "MJD_MIN",       double);
  GET_COLUMN (mjdMax,       "MJD_MAX",       double);
  GET_COLUMN (RatMin,       "RAT_MIN",       double);
  GET_COLUMN (RatMax,       "RAT_MAX",       double);
  GET_COLUMN (DatMin,       "DAT_MIN",       double);
  GET_COLUMN (DatMax,       "DAT_MAX",       double);

  // free the memory associated with the FITS files
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  ALLOCATE_PTR (planets, Planets, Nrow);
  for (int i = 0; i < Nrow; i++) {
    planets[i].epoch        = epoch[i];
    planets[i].mean_anomaly = mean_anomaly[i];
    planets[i].perihelion   = perihelion[i];
    planets[i].ascend_node  = ascend_node[i];
    planets[i].inclination  = inclination[i];
    planets[i].eccentricity = eccentricity[i];
    planets[i].semimajor_a  = semimajor_a[i];
    planets[i].mjdMin       = mjdMin[i];
    planets[i].mjdMax       = mjdMax[i];
    planets[i].RatMin       = RatMin[i];
    planets[i].RatMax       = RatMax[i];
    planets[i].DatMin       = DatMin[i];
    planets[i].DatMax       = DatMax[i];

    memcpy(planets[i].ID, &ID[i*8], 8);
    planets[i].ID[7] = 0;
  }
  fprintf (stderr, "loaded data for %lld planets\n", (long long) Nrow);

  free (ID          );
  free (epoch       );
  free (mean_anomaly);
  free (perihelion  );
  free (ascend_node );
  free (inclination );
  free (eccentricity);
  free (semimajor_a );
  free (mjdMin      );
  free (mjdMax      );
  free (RatMin      );
  free (RatMax      );
  free (DatMin      );
  free (DatMax      );

  *nplanets = Nrow;
  return planets;
}

void mpcorb_testing (int argc, char **argv) {

  if (argc != 3) Shutdown ("USAGE: %s (MPCORB.DAT) (epoch)\n", argv[0]);

  char  *filename = argv[1];
  double mjdObs   = atof (argv[2]);

  int Nplanets = 0;
  Planets *planets = mpcorb_read_text (filename, &Nplanets);
  if (VERBOSE) fprintf (stderr, "loaded %d planets\n", Nplanets);

  if (TESTING == 1) { mpcorb_predict_test(planets, Nplanets); exit (0); }

  if (TESTING == 2) {
    for (int i = 0; i < 10; i++) {
      PlanetDatum myPlanet = mpcorb_predict (&planets[i], mjdObs, TRUE);
      char ra_string[64], de_string[64];
      hms_format (ra_string, 64, myPlanet.Robs/15.0);
      hms_format (de_string, 64, myPlanet.Dobs);
      fprintf (stderr, "result: %12.6f %12.6f : %s %s\n", myPlanet.Robs, myPlanet.Dobs, ra_string, de_string);
    }
    exit (0);
  }
  fprintf (stderr, "ERROR: invalid value of TESTING : %d\n", TESTING);
  exit (2);
}
    
// Supply this test with the full list of minor planets from MPCORB.DAT.
// It will compare the positions for the first 10 minor planets with the
// reference data downloaded from the MPC for:
// MJD = 59396.0 @ (Long,Lat,Alt) = (0.0, 0.0, 0.0)

void mpcorb_predict_test (Planets *planets, int Nplanets) {
  
  int    Nref = 10;
  double Rref[10] = { 49.14708333, 352.90125000, 251.95416667, 175.74666667, 266.25541667, 298.38000000,  52.84458333, 121.94916667, 192.29500000, 144.20583333 };
  double Dref[10] = { 11.48944444,   8.68722222,  -4.19666667,   9.37111111, -16.97583333,  -8.30500000,  22.13472222,  21.50888889,  -1.52000000,  11.57138889 };

  for (int i = 0; (i < Nref) && (i < Nplanets); i++) {

    if (VERBOSE) fprintf (stderr, "%8s %9.5f %9.5f %9.5f %9.5f %9.5f %9.7f %12.7f\n",
	     planets[i].ID,
	     planets[i].epoch,
	     planets[i].mean_anomaly,
	     planets[i].perihelion,
	     planets[i].ascend_node,
	     planets[i].inclination,
	     planets[i].eccentricity,
	     planets[i].semimajor_a);
    
    // generate predictions for (Long,Lat,Alt) = (0.0, 0.0, 0.0)
    double Rpre, Dpre;
    mpcorb_predict_test_calc (&planets[i], 59396.0, &Rpre, &Dpre);

    fprintf (stderr, "result: %8s : %12.6f %12.6f : %6.2f %6.2f\n", planets[i].ID, Rpre, Dpre, 3600*(Rref[i]-Rpre)*cos(Dref[i]*RAD_DEG), 3600*(Dref[i]-Dpre));
  }
}

// attempt to predict as a test for Long,Lat = 0,0, with all other corrections
// this attempt uses slaPertel to calculate perturbed versions of the orbital elements
// NOTE this test seems to have worked: the calculated positions of the first 10 asteroids reported
// by MPC match to within 2 arcsec.  This is much larger than the Planetary abberation,
// so slaPlante must already be accounting for that effect.

void mpcorb_predict_test_calc (Planets *planet, double mjdObs, double *Robs, double *Dobs) {

  int jstat;
  
  double dTT = slaDtt (mjdObs);
  double mjdObsTT = mjdObs + dTT / 86400.0;

  Planets tmpPlanet;

  // correct the orbital elements to the perturbed version
  slaPertel (2,
	     planet->epoch,
	     mjdObsTT,
	     planet->epoch,
	     planet->inclination*RAD_DEG,
	     planet->ascend_node*RAD_DEG,
	     planet->perihelion*RAD_DEG,
	     planet->semimajor_a,
	     planet->eccentricity,
	     planet->mean_anomaly*RAD_DEG,
	     &tmpPlanet.epoch,
	     &tmpPlanet.inclination,  // returned as radians and used as radians below
	     &tmpPlanet.ascend_node,  // returned as radians and used as radians below
	     &tmpPlanet.perihelion,   // returned as radians and used as radians below
	     &tmpPlanet.semimajor_a,
	     &tmpPlanet.eccentricity,
	     &tmpPlanet.mean_anomaly, // returned as radians and used as radians below
	     &jstat);

  // this function returns the topocentric apparent R,D.  I assume it
  // has no atm corrections.

  double Rtopo, Dtopo, dist;
  slaPlante (mjdObsTT,
	     0.0, // PS1_LONGITUDE*RAD_DEG,
	     0.0, // PS1_LATITUDE*RAD_DEG,
	     2,
	     tmpPlanet.epoch,
	     tmpPlanet.inclination,
	     tmpPlanet.ascend_node,
	     tmpPlanet.perihelion,
	     tmpPlanet.semimajor_a,
	     tmpPlanet.eccentricity,
	     tmpPlanet.mean_anomaly,
	     0.0, &Rtopo, &Dtopo, &dist, &jstat);

  if (VERBOSE) fprintf (stderr, "Plante   : %12.6f %12.6f\n", Rtopo*DEG_RAD, Dtopo*DEG_RAD);

  // NOTE: the atm effects seem to be large and incompatible with MPC outputs.
  // Setting the pressure to 0.0 seems to avoid those corrections.

  double Rap, Dap;
  slaOap ("R",
	  Rtopo,
	  Dtopo,
	  mjdObs,
	  -0.4,
	  0.0, // LONGITUDE
	  0.0, // LATITUDE
	  0.0, // ALTITUDE,
	  0.0, // xp
	  0.0, // yp
	  273.15, // T(K)
	  0.0, // P(mB) p = 1013.25 * exp ( -hm / ( 29.3 * tsl ) )
	  0.0, // RH(%)
	  0.7, // Wavelength (um)
	  0.0, // lapse rate
	  &Rap, &Dap);

  if (VERBOSE) fprintf (stderr, "Oap   : %12.6f %12.6f : %6.2f %6.2f\n", Rap*DEG_RAD, Dap*DEG_RAD, 3600*(Rap-Rtopo)*DEG_RAD*cos(Dtopo), 3600*(Dap-Dtopo)*DEG_RAD);

  double Rmean, Dmean;
  slaAmp (Rap, Dap, mjdObsTT, 2000.0, &Rmean, &Dmean);

  if (VERBOSE) fprintf (stderr, "Amp   : %12.6f %12.6f : %6.2f %6.2f\n", Rmean*DEG_RAD, Dmean*DEG_RAD, 3600*(Rmean-Rap)*DEG_RAD*cos(Dap), 3600*(Dmean-Dap)*DEG_RAD);

  if (0) {
    // slaAmp applies both precession/nutation as well as abberation
    // but, I think abberation is not appropriate to solar system bodies.

    // here, I instead calculate just the precession and nutation with slalib

    double Rpre, Dpre;
    Rpre = Rap;
    Dpre = Dap;
    slaPreces ("FK5", 2021.5, 2000.000000, &Rpre, &Dpre);
    if (VERBOSE) fprintf (stderr, "Prec     : %12.6f %12.6f : %6.2f %6.2f\n", Rpre*DEG_RAD, Dpre*DEG_RAD, 3600*(Rpre-Rap)*DEG_RAD*cos(Dap), 3600*(Dpre-Dap)*DEG_RAD);
  }

  *Robs = Rmean * DEG_RAD;
  *Dobs = Dmean * DEG_RAD;
}

void myAmp ( double ra, double da, double date, double eq, double *rm, double *dm );

// mjdObs is NOT in TT
PlanetDatum mpcorb_predict (Planets *planet, double mjdObs, int FullCalc) {

  int jstat;
  
  double dTT = slaDtt (mjdObs);
  double mjdObsTT = mjdObs + dTT / 86400.0;

  Planets tmpPlanet;
  PlanetDatum myPlanet;
  myPlanet.Robs = NAN;
  myPlanet.Dobs = NAN;
  myPlanet.Rvel = NAN;
  myPlanet.Dvel = NAN;
  myPlanet.dist = NAN;

  // correct the orbital elements to the perturbed version
  tmpPlanet = *planet;
  if (FullCalc) { slaPertel (2,
	     planet->epoch,
	     mjdObsTT,
	     planet->epoch,
	     planet->inclination*RAD_DEG,
	     planet->ascend_node*RAD_DEG,
	     planet->perihelion*RAD_DEG,
	     planet->semimajor_a,
	     planet->eccentricity,
	     planet->mean_anomaly*RAD_DEG,
	     &tmpPlanet.epoch,
	     &tmpPlanet.inclination,  // returned as radians and used as radians below
	     &tmpPlanet.ascend_node,  // returned as radians and used as radians below
	     &tmpPlanet.perihelion,   // returned as radians and used as radians below
	     &tmpPlanet.semimajor_a,
	     &tmpPlanet.eccentricity,
	     &tmpPlanet.mean_anomaly, // returned as radians and used as radians below
	     &jstat);
  } else {
    // the function calls below require elements in radians
    tmpPlanet.epoch        = planet->epoch;
    tmpPlanet.inclination  = planet->inclination *RAD_DEG;
    tmpPlanet.ascend_node  = planet->ascend_node *RAD_DEG;
    tmpPlanet.perihelion   = planet->perihelion  *RAD_DEG;
    tmpPlanet.semimajor_a  = planet->semimajor_a ;
    tmpPlanet.eccentricity = planet->eccentricity;
    tmpPlanet.mean_anomaly = planet->mean_anomaly*RAD_DEG;
  }

  // this function returns the topocentric apparent R,D.  I assume it
  // has no atm corrections.

  double Rtopo, Dtopo, dist;
  slaPlante (mjdObsTT,
	     PS1_LONGITUDE*RAD_DEG,
	     PS1_LATITUDE*RAD_DEG,
	     2,
	     tmpPlanet.epoch, 
	     tmpPlanet.inclination,
	     tmpPlanet.ascend_node,
	     tmpPlanet.perihelion,
	     tmpPlanet.semimajor_a,
	     tmpPlanet.eccentricity,
	     tmpPlanet.mean_anomaly,
	     0.0, &Rtopo, &Dtopo, &dist, &jstat);
  myPlanet.dist = dist;

  double dRobs = 0.0; // planetary aberration, if FullCalc
  double dDobs = 0.0; // planetary aberration, if FullCalc
  if (FullCalc) {
    // calculate the velocity components
    
    // find position one minute later
    double Rtop2, Dtop2, dist2;
    slaPlante (mjdObsTT + 60/86400.0,
	       PS1_LONGITUDE*RAD_DEG,
	       PS1_LATITUDE*RAD_DEG,
	       2,
	       tmpPlanet.epoch, 
	       tmpPlanet.inclination,
	       tmpPlanet.ascend_node,
	       tmpPlanet.perihelion,
	       tmpPlanet.semimajor_a,
	       tmpPlanet.eccentricity,
	       tmpPlanet.mean_anomaly,
	       0.0, &Rtop2, &Dtop2, &dist2, &jstat);

    myPlanet.Rvel = 3600.0*(Rtop2 - Rtopo)*DEG_RAD*cos(Dtopo); // this vector is wrong by the precession rotation
    myPlanet.Dvel = 3600.0*(Dtop2 - Dtopo)*DEG_RAD;            // this vector is wrong by the precession rotation

    // include the correction for planetary aberration
    // XXX make these constants more accurate
    double dist_km = dist * 1.5e8;
    double time_min = dist_km / 299792.459/ 60.0;
    double Rlag = myPlanet.Rvel * time_min; // velocity is in arcsec / min
    double Dlag = myPlanet.Dvel * time_min; // velocity is in arcsec / min
    
    dRobs = Rlag / 3600.0 / cos(Dtopo); // RA degrees
    dDobs = Dlag / 3600.0;              // DEC degrees
  }

  // NOTE: the atm effects seem to be large and incompatible with MPC outputs.
  // Setting the pressure to 0.0 seems to avoid those corrections.

  double Rap, Dap;
  if (FullCalc) {
    slaOap ("R",
	  Rtopo,
	  Dtopo,
	  mjdObs,
	  -0.4,
	  PS1_LONGITUDE*RAD_DEG,
	  PS1_LATITUDE*RAD_DEG,
	  PS1_ALTITUDE,
	  0.0, // xp
	  0.0, // yp
	  273.15, // T(K)
	  0.0, // P(mB) p = 1013.25 * exp ( -hm / ( 29.3 * tsl ) )
	  0.0, // RH(%)
	  0.7, // Wavelength (um)
	  0.0065, // lapse rate
	  &Rap, &Dap);
  } else { Rap = Rtopo; Dap = Dtopo; }

  double Rmean, Dmean;
  if (FullCalc) {
    if (USE_AMP) {
      // slaAmp (Rap, Dap, mjdObsTT, 2000.0, &Rmean, &Dmean);
      myAmp (Rap, Dap, mjdObsTT, 2000.0, &Rmean, &Dmean);
    } else {
      // use slaPreces and do not correct for stellar aberration
      // convert mjdObsTT to year
      double epochYear = 2000.0 + (mjdObsTT - 51544.5) / 365.25; // J2000 = 51544.5
      Rmean = Rap;
      Dmean = Dap;

      // use slalib precession and nutation:
      if (USE_MANUAL_PRECESS) {
	double pm[3][3], v1[3], v2[3];

	/* Convert RA,Dec to x,y,z */
	slaDcs2c ( Rmean, Dmean, v1 );

	// calculate the precession & nutation combined matrix:
	if (USE_PRENUT) {
	  // slaPrenut ( epochYear, 51544.5, pm );
	  slaPrenut ( 2000.0, mjdObsTT, pm ); // prenut is always mean-to-apparent
	  slaDimxv ( pm, v1, v2 );            // apply the inverse transformation
	} else {
	  slaPrec ( epochYear, 2000.0, pm );  // precess from apparent-to-mean
	  slaDmxv ( pm, v1, v2 );	      // apply the forward transformatio
	}

	/* Back to RA,Dec */
	slaDcc2s ( v2, &Rmean, &Dmean );

	// renorm to 0-360
	Rmean = slaDranrm ( Rmean );
      } else {
	slaPreces ("FK5", epochYear, 2000.000000, &Rmean, &Dmean);
      }
      // fprintf (stderr, "year: %f\n", epochYear);
    }
  } else {Rmean = Rap; Dmean = Dap; }

  if (USE_PLANETARY_ABERRATION) {
    myPlanet.Robs = Rmean * DEG_RAD + dRobs;
    myPlanet.Dobs = Dmean * DEG_RAD + dDobs;
  } else {
    myPlanet.Robs = Rmean * DEG_RAD;
    myPlanet.Dobs = Dmean * DEG_RAD;
  }
  return myPlanet;
}

# define DISABLE_ABERRATION FALSE
# define DISABLE_DEFLECTION FALSE

void myAmpqk ( double ra, double da, double amprms[21], double *rm, double *dm ) {
   double gr2e;    /* (grav rad Sun)*2/(Sun-Earth distance) */
   double ab1;     /* sqrt(1-v*v) where v=modulus of Earth vel */
   double ehn[3];  /* Earth position wrt Sun (unit vector, FK5) */
   double abv[3];  /* Earth velocity wrt SSB (c, FK5) */
   double p[3], p1[3], p2[3], p3[3];  /* work vectors */
   double ab1p1, p1dv, p1dvp1, w, pde, pdep1;
   int i, j;

/* Unpack some of the parameters */
   gr2e = amprms[7];
   ab1  = amprms[11];
   for ( i = 0; i < 3; i++ ) {
      ehn[i] = amprms[i + 4];
      abv[i] = amprms[i + 8];
   }

   // invert and print out:
   if (VERBOSE_TEST) {
     double Rmean, Dmean;
     Rmean = ra * DEG_RAD;
     Dmean = da * DEG_RAD;
     fprintf (stderr, "%12.6f %12.6f ", Rmean, Dmean);
   }

/* Apparent RA,Dec to Cartesian */
   slaDcs2c ( ra, da, p3 );

   // invert and print out:
   if (VERBOSE_TEST) {
     double Rtmp, Dtmp, Rmean, Dmean;
     slaDcc2s ( p3, &Rtmp, &Dtmp );
     Rtmp = slaDranrm ( Rtmp );
     Rmean = Rtmp * DEG_RAD;
     Dmean = Dtmp * DEG_RAD;
     fprintf (stderr, "C: %12.6f %12.6f ", Rmean, Dmean);
   }

/* Precession and nutation */
   slaDimxv ( (double(*)[3]) &amprms[12], p3, p2 );

   // invert and print out:
   if (VERBOSE_TEST) {
     double Rtmp, Dtmp, Rmean, Dmean;
     slaDcc2s ( p2, &Rtmp, &Dtmp );
     Rtmp = slaDranrm ( Rtmp );
     Rmean = Rtmp * DEG_RAD;
     Dmean = Dtmp * DEG_RAD;
     fprintf (stderr, "P: %12.6f %12.6f ", Rmean, Dmean);
   }

   if (DISABLE_ABERRATION) {
     // XXX : save result from p2 in p1
     for ( i = 0; i < 3; i++ ) {
       p1[i] = p2[i];
     }
   } else {
     /* Aberration */
     ab1p1 = ab1 + 1.0;
     for ( i = 0; i < 3; i++ ) {
       p1[i] = p2[i];
     }
     for ( j = 0; j < 2; j++ ) {
       p1dv = slaDvdv ( p1, abv );
       p1dvp1 = 1.0 + p1dv;
       w = 1.0 + p1dv / ab1p1;
       for ( i = 0; i < 3; i++ ) {
         p1[i] = ( p1dvp1 * p2[i] - w * abv[i] ) / ab1;
       }
       slaDvn ( p1, p3, &w );
       for ( i = 0; i < 3; i++ ) {
         p1[i] = p3[i];
       }
     }
   }

   // invert and print out:
   if (VERBOSE_TEST) {
     double Rtmp, Dtmp, Rmean, Dmean;
     slaDcc2s ( p1, &Rtmp, &Dtmp );
     Rtmp = slaDranrm ( Rtmp );
     Rmean = Rtmp * DEG_RAD;
     Dmean = Dtmp * DEG_RAD;
     fprintf (stderr, "A: %12.6f %12.6f ", Rmean, Dmean);
   }

/* Light deflection */
   for ( i = 0; i < 3; i++ ) {
      p[i] = p1[i];
   }

// disable deflection:
   if (DISABLE_DEFLECTION) {
     
   } else {
     for ( j = 0; j < 5; j++ ) {
       pde = slaDvdv ( p, ehn );
       pdep1 = 1.0 + pde;
       w = pdep1 - gr2e * pde;
       for ( i = 0; i < 3; i++ ) {
         p[i] = ( pdep1 * p1[i] - gr2e * ehn[i] ) / w;
       }
       slaDvn ( p, p2, &w );
       for ( i = 0; i < 3; i++ ) {
         p[i] = p2[i];
       }
     }
   }

   // invert and print out:
   if (VERBOSE_TEST) {
     double Rtmp, Dtmp, Rmean, Dmean;
     slaDcc2s ( p, &Rtmp, &Dtmp );
     Rtmp = slaDranrm ( Rtmp );
     Rmean = Rtmp * DEG_RAD;
     Dmean = Dtmp * DEG_RAD;
     fprintf (stderr, "D: %12.6f %12.6f\n", Rmean, Dmean);
   }

/* Mean RA,Dec */
   slaDcc2s ( p, rm, dm );
   *rm = slaDranrm ( *rm );
}

void myAmp ( double ra, double da, double date, double eq, double *rm, double *dm ) {

   double amprms[21];    /* Mean-to-apparent parameters */

   slaMappa ( eq, date, amprms );
    myAmpqk ( ra, da, amprms, rm, dm );
}

# define MPC_DOUBLE(FIELD,START,END) {FIELD = strtod(&line[START-1], NULL);}

// part the data in a single line of the MPCORB.DAT file
int mpcorb_parseline (char *line, Planets *planet, int Nmax) {

  // fields I need:
 
  // epoch       : 21 - 25
  // inclination : 60 - 68
  // longitude of ascending node (J2000) : 49 - 57
  // argument of perihelion (J2000) : 38 - 46
  // semimajor axis (AU) : 93 - 103
  // eccentricity : 71 - 79
  // Mean anomaly : 27 - 35
  
  // these fields are saved in the data file with whitespace between, so I can
  // set the byte after the range to NULL and scanf the values.  or just use strtod as needed:

  strncpy_nowarn (planet->ID, line, 7);

  MPC_DOUBLE (planet->mean_anomaly, 27, 35);
  MPC_DOUBLE (planet->perihelion,   38, 46);
  MPC_DOUBLE (planet->ascend_node,  49, 57);
  MPC_DOUBLE (planet->inclination,  60, 68);
  MPC_DOUBLE (planet->eccentricity, 71, 79);
  MPC_DOUBLE (planet->semimajor_a,  93, 103);

  // the epoch is stored in a compressed format:
  int year = 1800 + 100*(line[20] - 'I') + 10*(line[21] - '0') + line[22] - '0';
  char mchar = line[23];
  int month = (mchar > 64) ? mchar - 'A' + 10 : mchar - '0';
  char dchar = line[24];
  int day = (dchar > 64) ? dchar - 'A' + 10 : dchar - '0';

  double jd = day - 32075 + (int)(1461*(year + 4800 + (int)((month-14)/12))/4)
    + (int)(367*(month - 2 - (int)((month - 14)/12)*12)/12)
    - (int)(3*(int)((year + 4900 + (int)((month - 14)/12))/100)/4) - 0.5;

  planet->epoch = jd - 2400000.5;;
  return TRUE;
}

// read the full MPCORB file, storing desired information in Planets structure
Planets *mpcorb_read_text (char *filename, int *nplanets) {

  ALLOCATE_PTR (buffer, char, NBYTE);

  // scan through the entire MPCORB file
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't open data file: %s", filename);

  int Nbyte  = 0;  // number of bytes read on each pass
  int Nextra = 0;  // number excess bytes from last partial row
  int Nline  = 0;  // number of lines read so far in file

  int Nplanets = 0;
  int NPLANETS = 10000;
  ALLOCATE_PTR (planets, Planets, NPLANETS);

  while ((Nbyte = fread (&buffer[Nextra], 1, NBYTE-Nextra, f)) != 0) {
    if (Nbyte == -1) Shutdown ("error reading from data file %s", filename);
    if (DEBUG) fprintf (stderr, "read %d bytes", Nbyte);

    Nbyte += Nextra;

    if (VERBOSE) fprintf (stderr, "read .. ");

    /* find bounds on first complete line */
    char *p = buffer;
    char *q = memchr (p, '\n', Nbyte);
    if (q == NULL) Shutdown ("incomplete line at end of file\n");
    int offset = p - buffer; // offset within this scan

    // scan through entire buffer for star coords
    while (1) {

      Nline ++;
      if ((Nline > 43) && (q - p > 5)) {
	mpcorb_parseline (p, &planets[Nplanets], Nbyte - offset);
	Nplanets ++;
	CHECK_REALLOCATE (planets, Planets, NPLANETS, Nplanets, 10000);
      }

      /* start of the next line */
      p = q + 1;
      offset = p - buffer; // offset within this scan
      if (offset == Nbyte) {
	// last buffer read is a complete line
	Nextra = 0;
	break;
      }

      /* end of the next line */
      q = memchr (p, '\n', Nbyte - offset);
      if (q == NULL) {
	// last, incomplete line in buffer
	Nextra = Nbyte - offset;
	break;
      } 
    }

    if (VERBOSE) fprintf (stderr, "scan %d planets (%s).. ", Nplanets, planets[Nplanets-1].ID);

    // at end, p points at the start of last, partial line
    if (Nextra) memmove (buffer, p, Nextra);

    // TESTING:
    if (TESTING && (Nplanets > 50)) goto skip;

    // XX temporary:
    // if (Nplanets > 100000) goto skip;

  }
  if (VERBOSE) fprintf (stderr, "\n");
  
skip:

  fclose (f);
  free (buffer);

  *nplanets = Nplanets;
  return planets;
}

// print an error message and exit
int Shutdown (char *format, ...) {  
  va_list argp;
  char formatplus[1024];
  
  strcpy (formatplus, format);
  strcat (formatplus, "\n");

  va_start (argp, format);
  vfprintf (stderr, formatplus, argp);
  va_end (argp);

  fprintf (stderr, "ERROR: mpcorb_convert halted\n");
  exit (1);
}

// The following test case provided by Serge can be perfectly reproduced, but only if I
// use Dtt for the epoch, and this is clearly wrong.
void mpcorb_predict_test_from_serge () {

  double epochUT = 57435.5;

  Planets testPlanet;
  // testPlanet.epoch        = epochUT + slaDtt(epochUT) / 86400.0; // XXX this is what the epoch should be
  testPlanet.epoch        = slaDtt(epochUT); // XXX this is the value that works with the test values
  testPlanet.inclination  = 0.043306854729735354 * DEG_RAD;
  testPlanet.ascend_node  = 4.728915154005972    * DEG_RAD;
  testPlanet.perihelion   = 2.2756608097151414   * DEG_RAD;
  testPlanet.mean_anomaly = 2.888163341284418    * DEG_RAD;
  testPlanet.eccentricity = 0.09528339999999975;
  testPlanet.semimajor_a  = 2.782049599999998;
		
  PlanetDatum myPlanet = mpcorb_predict (&testPlanet, epochUT, TRUE);

  fprintf (stderr, "%f vs %f : %f\n",  3.92105439135726600*DEG_RAD, myPlanet.Robs, 3600*(3.92105439135726600*DEG_RAD - myPlanet.Robs));
  fprintf (stderr, "%f vs %f : %f\n", -0.33499850519808244*DEG_RAD, myPlanet.Dobs, 3600*(-0.33499850519808244*DEG_RAD - myPlanet.Dobs));
  exit (0);
}

// The following test code uses unperturbed orbital elements and gets the
// wrong answer by 20 - 60 arcsec for the top 10 minor planets.
void mpcorb_predict_t0 (Planets *planet, double mjdObs, double *Robs, double *Dobs) {

  int jstat;
  
  double dTT = slaDtt (mjdObs);
  double mjdObsTT = mjdObs + dTT / 86400.0;

  // this function returns the topocentric apparent R,D.  I assume it
  // has no atm corrections.

  double Rrad, Drad, dist;
  slaPlante (mjdObsTT,
	     0.0, // PS1_LONGITUDE*RAD_DEG,
	     0.0, // PS1_LATITUDE*RAD_DEG,
	     2,
	     planet->epoch,
	     planet->inclination*RAD_DEG,
	     planet->ascend_node*RAD_DEG,
	     planet->perihelion*RAD_DEG,
	     planet->semimajor_a,
	     planet->eccentricity,
	     planet->mean_anomaly*RAD_DEG,
	     0.0, &Rrad, &Drad, &dist, &jstat);

  fprintf (stderr, "Plante   : %12.6f %12.6f\n", Rrad*DEG_RAD, Drad*DEG_RAD);

  // I'm not certain if I should use realistic (STD) atm parameters or not.
  // If I set them to zero, is the correction not applied?
 
  double Rap, Dap;
  slaOap ("R",
	  Rrad,
	  Drad,
	  mjdObs,
	  -0.4,
	  0.0, // LONGITUDE
	  0.0, // LATITUDE
	  0.0, // ALTITUDE,
	  0.0, // xp
	  0.0, // yp
	  273.15, // T(K)
	  1013.25, // P(mB) p = 1013.25 * exp ( -hm / ( 29.3 * tsl ) )
	  0.0, // RH(%)
	  0.7, // Wavelength (um)
	  0.0065, // lapse rate
	  &Rap, &Dap);

  // XXX the atm effects seem to be large and incompatible with MPC outputs
  // fprintf (stderr, "Oap (STD): %12.6f %12.6f\n", Rap*DEG_RAD, Dap*DEG_RAD);

  slaOap ("R",
	  Rrad,
	  Drad,
	  mjdObs,
	  -0.4,
	  0.0, // LONGITUDE
	  0.0, // LATITUDE
	  0.0, // ALTITUDE,
	  0.0, // xp
	  0.0, // yp
	  273.15, // T(K)
	  0.0, // P(mB) p = 1013.25 * exp ( -hm / ( 29.3 * tsl ) )
	  0.0, // RH(%)
	  0.7, // Wavelength (um)
	  0.0, // lapse rate
	  &Rap, &Dap);

  fprintf (stderr, "Oap (0.0): %12.6f %12.6f : %6.2f %6.2f\n", Rap*DEG_RAD, Dap*DEG_RAD, 3600*(Rap-Rrad)*DEG_RAD*cos(Drad), 3600*(Dap-Drad)*DEG_RAD);

  double Rmean, Dmean;
  slaAmp (Rap, Dap, mjdObsTT, 2000.0, &Rmean, &Dmean);

  fprintf (stderr, "Amp1     : %12.6f %12.6f : %6.2f %6.2f\n", Rmean*DEG_RAD, Dmean*DEG_RAD, 3600*(Rmean-Rap)*DEG_RAD*cos(Dap), 3600*(Dmean-Dap)*DEG_RAD);

  // apply Amp to output from Plante
  double Ralt, Dalt;
  slaAmp (Rrad, Drad, mjdObsTT, 2000.0, &Ralt, &Dalt);

  fprintf (stderr, "Amp2     : %12.6f %12.6f : %6.2f %6.2f\n", Ralt*DEG_RAD, Dalt*DEG_RAD, 3600*(Ralt-Rrad)*DEG_RAD*cos(Drad), 3600*(Dalt-Drad)*DEG_RAD);


  // slaAmp applies both precession/nutation as well as abberation
  // but, I think abberation is not appropriate to solar system bodies.

  // here, I instead calculate just the precession and nutation with slalib

  double Rpre, Dpre;
  Rpre = Rap;
  Dpre = Dap;
  slaPreces ("FK5", 2021.5, 2000.000000, &Rpre, &Dpre);
  fprintf (stderr, "Prec     : %12.6f %12.6f : %6.2f %6.2f\n", Rpre*DEG_RAD, Dpre*DEG_RAD, 3600*(Rpre-Rap)*DEG_RAD*cos(Dap), 3600*(Dpre-Dap)*DEG_RAD);

  *Robs = Rmean * DEG_RAD;
  *Dobs = Dmean * DEG_RAD;
}

