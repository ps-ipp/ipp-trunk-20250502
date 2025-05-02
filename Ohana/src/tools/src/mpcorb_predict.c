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

// structure to represent the model of a planet
typedef struct {
  char   ID[8];
  double epoch;
  double mean_anomaly;
  double perihelion;
  double ascend_node;
  double inclination;
  double eccentricity;
  double semimajor_a;
  double Hmag;
  int    Nobs;  // number of observations
  int    Nopp;  // number of oppositions
  double mjdMin;
  double mjdMax;
  double RatMin;
  double RatMax;
  double DatMin;
  double DatMax;
} Planets;

// structure to represent properties of an observed planet
typedef struct {
  char   ID[8];
  double Robs;
  double Dobs;
  double Mobs; // apparent mag
  double dist;
} PlanetDatum;

void mpcorb_trange (int argc, char **argv);
void mpcorb_moment (int argc, char **argv);

PlanetDatum mpcorb_predict (Planets *planet, double mjdObs, int FullCalc);

Planets *mpcorb_read_text (char *filename, int *nplanets);
Planets *mpcorb_read_fits (char *filename, int *nplanets);
void     mpcorb_save_fits (char *filename, Planets *planets, int Nplanets);
void     mpcorb_refs_fits (char *filename, PlanetDatum *planets, int Nplanets);

int Shutdown (char *format, ...);
int mpcorb_parseline (char *line, Planets *planet, int Nmax);

void solar_coords (double mjdObs, double *Rsun, double *Dsun, double *distSun);

# define PS1_LONGITUDE -156.2559041668965 /* degrees East */
# define PS1_LATITUDE    20.7070994446013 /* degrees North */
# define PS1_ALTITUDE  3067.7 /* meters */

# define SKIP_BAD FALSE

// save solar position for apparent mag calc
static double R_SUN = NAN;
static double D_SUN = NAN;
static double DIST_SUN = NAN;

int main (int argc, char **argv) {

  // XXX test
  if (TESTING) {
    double mjdObs   = atof (argv[1]);
    char  *filename = argv[2];
  
    // save the coordinates of the sun:
    solar_coords (mjdObs, &R_SUN, &D_SUN, &DIST_SUN);
    
    // load the planets
    int Nplanets = 0;
    Planets *planets = mpcorb_read_text (filename, &Nplanets);
    for (int i = 0; i < 10; i++) {
      // if it is in the region, use the more accurate calculation
      mpcorb_predict (&planets[i], mjdObs, TRUE);
    }
    exit (0);
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

    // XXX make these quality cuts user options
    if (SKIP_BAD && ((planets[i].Nobs < 20) || (planets[i].Nopp < 2))) continue;

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

  // save the coordinates of the sun:
  solar_coords (mjdObs, &R_SUN, &D_SUN, &DIST_SUN);

  int Nplanets = 0;
  Planets *planets = mpcorb_read_fits (filename, &Nplanets);
  if (VERBOSE) fprintf (stderr, "loaded %d planets\n", Nplanets);

  int NplanetsSave = 0;
  int NPLANETSSAVE = 1000;
  ALLOCATE_PTR (planetsSave, PlanetDatum, NPLANETSSAVE);

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

// mjdObs is NOT in TT
PlanetDatum mpcorb_predict (Planets *planet, double mjdObs, int FullCalc) {

  int jstat;
  
  double dTT = slaDtt (mjdObs);
  double mjdObsTT = mjdObs + dTT / 86400.0;

  Planets tmpPlanet;
  memset (&tmpPlanet, 0, sizeof(Planets));

  PlanetDatum myPlanet;
  myPlanet.Robs = NAN;
  myPlanet.Dobs = NAN;
  myPlanet.dist = NAN;
  myPlanet.Mobs = NAN;
  memset (myPlanet.ID, 0, 8);

  // correct the orbital elements to the perturbed version
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

  // this function returns the topocentric apparent R,D.  It does no atm corrections.

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

  // NOTE: the atm effects seem to be large and incompatible with MPC outputs.
  // Setting the pressure to 0.0 seems to avoid those corrections.
  // Should I set altitude to zero?

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
    slaAmp (Rap, Dap, mjdObsTT, 2000.0, &Rmean, &Dmean);
  } else {Rmean = Rap; Dmean = Dap; }

  myPlanet.Robs = Rmean * DEG_RAD;
  myPlanet.Dobs = Dmean * DEG_RAD;
  myPlanet.Mobs = NAN;

  // calculate the apparent magnitude from the H magnitude:
  if (FullCalc) {
    // find the angle between the sun and the object
    double dRmean = Rmean - R_SUN;
    double Chi = sin(D_SUN)*sin(Dmean) + cos(D_SUN)*cos(Dmean)*cos(dRmean);
    double Psi1 = cos(Dmean)*sin(dRmean);
    double Psi2 = cos(D_SUN)*sin(Dmean) - sin(D_SUN)*cos(Dmean)*cos(dRmean);
    double Psi = sqrt(Psi1*Psi1 + Psi2*Psi2);
    double phi = atan2 (Psi, Chi);

    // place phi in the range 0 360
    if ((phi) <    0.0) (phi) += M_PI;
    if ((phi) > 2*M_PI) (phi) -= M_PI;

    double DsunTgt = sqrt (SQ(DIST_SUN) + SQ(dist) - 2.0*DIST_SUN*dist*cos(phi));

    double phaseArg = (SQ(dist) + SQ(DsunTgt) - SQ(DIST_SUN)) / (2.0*DsunTgt*dist);
    double phase = DEG_RAD*acos(phaseArg); 
    double phcen = phase / 100.0;

    // mercury phase curve from Hilton 2005
    myPlanet.Mobs = planet->Hmag + 5*log10(dist) + 5*log10(DsunTgt) + 4.98*phcen - 4.88*phcen*phcen + 3.02*(phcen*phcen*phcen);

    if (TESTING) fprintf (stderr, "R,D: %12.6f %12.6f : SunDist: %6.4f : phi %6.4f : phase %6.4f : Mapp %6.4f\n",
	     myPlanet.Robs, myPlanet.Dobs, DsunTgt, phi*DEG_RAD, phase, myPlanet.Mobs);
  }

  return myPlanet;
}

void solar_coords (double mjdObs, double *Rsun, double *Dsun, double *distSun) {

# define J2000 51544.5       /* Modified Julian date at standard epoch J2000 */
# define J1900 15019.5       /* Modified Julian at 1900 */

  /* Low precision formulae for the sun, from Astro. Almanac p. C5 (2012) */

  double n = mjdObs - J2000;	      // day number relative to standard epoch
  double L = 280.460 + 0.9856474 * n; // mean solar longitute (corr. for aberration)
  double g = (357.528 + 0.9856003 * n)*RAD_DEG;	// Mean anomaly

  double lambda = L + 1.915 * sin(g) + 0.020 * sin(2*g); // solar longitude in degrees
  double beta   = 0.0;					 // approx latitude 
  double dSun   = 1.00014 - 0.01671*cos(g) - 0.00014*cos(2*g); // earth-to-sun dist in AU

  // year since 1900
  double year = (mjdObs - J1900) / 365.25;

  double T = year / 100.0;

  double phi = RAD_DEG*(23.452294 - 0.013013*T - 0.000001639*T*T + 0.000000503*T*T*T);
  double Xo = 0.0;
  double xo = 0.0;

  // pre-calculated constants:
  double sin_phi_cos_Xo = sin(phi)*cos(Xo);
  double sin_phi_sin_Xo = sin(phi)*sin(Xo);
  double cos_phi        = cos(phi);
  double cos_phi_cos_Xo = cos(phi)*cos(Xo);
  double cos_phi_sin_Xo = cos(phi)*sin(Xo);
  double sin_phi        = sin(phi);
  double cos_Xo 	= cos(Xo);
  double sin_Xo 	= sin(Xo);

  double X = lambda*RAD_DEG;
  double Y = beta  *RAD_DEG;

  double cosY = cos(Y);
  double sinY = sin(Y);
  
  double cosX = cos(X);
  double sinX = sin(X);

  // recast with constants extracted:
  double sin_y = cosY*sinX*sin_phi_cos_Xo - cosY*cosX*sin_phi_sin_Xo + sinY*cos_phi;
  double cos_y = sqrt (1 - sin_y*sin_y);

  double sin_x = cosY*sinX*cos_phi_cos_Xo - cosY*cosX*cos_phi_sin_Xo - sinY*sin_phi;
  double cos_x = cosY*cosX*cos_Xo + cosY*sinX*sin_Xo;
      
  // atan2 returns -pi : +pi
  double RsunRad = atan2 (sin_x, cos_x) + xo;
  if ((RsunRad) <    0.0) (RsunRad) += M_PI;
  if ((RsunRad) > 2*M_PI) (RsunRad) -= M_PI;

  // should be in range -pi/2 : +pi/2
  double DsunRad = atan2 (sin_y, cos_y);

  double epochYear = 2000.0 + (mjdObs - J2000) / 365.25; // J2000 = 51544.5

  slaPreces ("FK5", epochYear, 2000.000000, &RsunRad, &DsunRad);

  // *Rsun = RsunRad * DEG_RAD;
  // *Dsun = DsunRad * DEG_RAD;

  // keep these in radians for calculations in mpcorb_predict
  *Rsun = RsunRad;
  *Dsun = DsunRad;
  *distSun = dSun;
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
  gfits_define_bintable_column (&theader,  "D", "Mref", "Mag",     "magnitudes",  1.0, 0.0);
  gfits_define_bintable_column (&theader,  "D", "dist", "DIST",    "AU",          1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // create intermediate storage arrays
  ALLOCATE_PTR (ID,                     char, Nplanets*8);
  ALLOCATE_PTR (Rref,                 double, Nplanets);
  ALLOCATE_PTR (Dref,                 double, Nplanets);
  ALLOCATE_PTR (Mref,                 double, Nplanets);
  ALLOCATE_PTR (dist,                 double, Nplanets);

  // set intermediate storage arrays
  for (int i = 0; i < Nplanets; i++) {
    Rref[i]        = planets[i].Robs;
    Dref[i]        = planets[i].Dobs;
    dist[i]        = planets[i].dist;
    Mref[i]        = planets[i].Mobs;
    memcpy(&ID[i*8], planets[i].ID, 8);
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "ID",   ID,   Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Rref", Rref, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Dref", Dref, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "Mref", Mref, Nplanets);
  gfits_set_bintable_column (&theader, &ftable, "dist", dist, Nplanets);

  // free intermediate storage arrays
  free (ID);
  free (Rref);
  free (Dref);
  free (Mref);
  free (dist);

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
  gfits_define_bintable_column (&theader,  "D", "HMAG",         "Hmag",         "mag",      1.0, 0.0);

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
  ALLOCATE_PTR (Hmag,    	        double, Nplanets);

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
    Hmag[i]            = planets[i].Hmag;

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
  gfits_set_bintable_column (&theader, &ftable, "HMAG",         Hmag,         Nplanets);

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
  free (Hmag);

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
  GET_COLUMN (Hmag,         "HMAG",          double);

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
    planets[i].Hmag         = Hmag[i];

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
  free (Hmag        );

  *nplanets = Nrow;
  return planets;
}

# define MPC_GETDBL(FIELD,START,END) {FIELD = strtod(&line[START-1], &endptr); if (endptr == &line[START-1]) FIELD = NAN;}
# define MPC_GETINT(FIELD,START,END) {FIELD = strtol(&line[START-1], NULL, 10); }

// parse the data in a single line of the MPCORB.DAT file
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

  char *endptr;
  MPC_GETDBL (planet->Hmag,           9,  13);
  MPC_GETDBL (planet->mean_anomaly,  27,  35);
  MPC_GETDBL (planet->perihelion,    38,  46);
  MPC_GETDBL (planet->ascend_node,   49,  57);
  MPC_GETDBL (planet->inclination,   60,  68);
  MPC_GETDBL (planet->eccentricity,  71,  79);
  MPC_GETDBL (planet->semimajor_a,   93, 103);
  MPC_GETINT (planet->Nobs,         118, 122);
  MPC_GETINT (planet->Nopp,         124, 126);

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

