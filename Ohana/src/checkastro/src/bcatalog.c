# include "checkastro.h"

static int Nkeep1 = 0;
static int Nkeep2 = 0;
static int Nskip1 = 0;
static int Nskip2 = 0;

// test image: 2013/06/15,13:25:51, GPC1.r.XY50
static int CHECK_TEST_IMAGE = TRUE;
// static unsigned int Tref = 1378812312; 
// static short Cref = 10001;

static unsigned int Tref = 1379570672;
static short Cref = 10341;

int LimitDensityCatalog_ByNmeasureGrid (Catalog *subcatalog, Catalog *oldcatalog);

int bcatalog (Catalog *subcatalog, Catalog *catalog) {

  off_t i, j, offset;
  off_t NAVERAGE, NMEASURE, Naverage, Nmeasure, Nm;
  int Nsecfilt;

  // XXX in the future, use catalog[0].Nsecfilt only?  allow catalogs to have variable Nsecfilt?
  Nsecfilt = GetPhotcodeNsecfilt ();
  assert (catalog[0].Nsecfilt == Nsecfilt);

  /* we are moving only the subset of measurements from catalog[0] to subcatalog[0] */
  memset(subcatalog, 0, sizeof(Catalog));
  NAVERAGE = 50;
  NMEASURE = 1000;
  ALLOCATE (subcatalog[0].average,  Average,     NAVERAGE);
  ALLOCATE (subcatalog[0].secfilt,  SecFilt,     NAVERAGE*Nsecfilt);
  ALLOCATE (subcatalog[0].measureT, MeasureTiny, NMEASURE);
  Nmeasure = Naverage = 0;

  /* exclude stars not in range or with too few measurements */
  for (i = 0; i < catalog[0].Naverage; i++) {
    if (catalog[0].average[i].Nmeasure <= SRC_MEAS_TOOFEW) continue;
    
    /* start with all stars good */
    subcatalog[0].average[Naverage] = catalog[0].average[i];
    subcatalog[0].average[Naverage].measureOffset = Nmeasure;
    for (j = 0; j < Nsecfilt; j++) {
      subcatalog[0].secfilt[Nsecfilt*Naverage+j] = catalog[0].secfilt[Nsecfilt*i+j];
    }

    int foundRef = FALSE;

    Nm = 0;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {

      offset = catalog[0].average[i].measureOffset + j;
      
      // filter objects based on user supplied criteria, including SIGMA_LIM
      if (!MeasFilterTest(&catalog[0].measure[offset], TRUE)) {
	catalog[0].measure[offset].dbFlags &= ~ID_MEAS_USED_CHIP;
	if (CHECK_TEST_IMAGE && (abs(catalog[0].measure[offset].t - Tref) < 10) && (catalog[0].measure[offset].photcode == Cref)) {
	  Nskip1 ++;
	}
	continue;
      }
      if (CHECK_TEST_IMAGE && (abs(catalog[0].measure[offset].t - Tref) < 10) && (catalog[0].measure[offset].photcode == Cref)) {
	Nkeep1 ++;
      }

      // does this measurement match any of the reference photcodes?
      if (!foundRef && NphotcodesReset) {
	int m, found = FALSE;
	for (m = 0; (m < NphotcodesReset) && !found; m++) {
	  if (catalog[0].measure[offset].photcode == photcodesReset[m][0].code) {
	    found = TRUE;
	  }
	}
	if (found) {
	  foundRef = TRUE;
	}
      }

      // subcatalog[0].measure[Nmeasure] = catalog[0].measure[offset];
      CopyMeasureToTiny (&subcatalog[0].measureT[Nmeasure], &catalog[0].measure[offset]);
      subcatalog[0].measureT[Nmeasure].dbFlags &= ~ID_MEAS_SKIP_ASTROM;
      subcatalog[0].measureT[Nmeasure].dbFlags &= ~ID_MEAS_NOCAL;
      subcatalog[0].measureT[Nmeasure].averef   = Naverage;

      Nmeasure ++;
      Nm ++;
      if (Nmeasure == NMEASURE) {
        NMEASURE += 1000;
        REALLOCATE (subcatalog[0].measureT, MeasureTiny, NMEASURE);
      }
    }
    if (foundRef) {
      subcatalog[0].average[Naverage].flags |= 0x10; 
    }
    subcatalog[0].average[Naverage].Nmeasure = Nm;
    Naverage ++;
    if (Naverage == NAVERAGE) {
      NAVERAGE += 50;
      REALLOCATE (subcatalog[0].average, Average, NAVERAGE);
      REALLOCATE (subcatalog[0].secfilt, SecFilt, NAVERAGE*Nsecfilt);
    }
  }
  REALLOCATE (subcatalog[0].average,  Average,     MAX (Naverage, 1));
  REALLOCATE (subcatalog[0].measureT, MeasureTiny, MAX (Nmeasure, 1));
  REALLOCATE (subcatalog[0].secfilt,  SecFilt,     MAX (Naverage, 1)*Nsecfilt);
  subcatalog[0].Naverage = Naverage;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = catalog[0].Nsecfilt;
  subcatalog[0].Nsecfilt = catalog[0].Nsecfilt;
  subcatalog[0].catID    = catalog[0].catID;
  assert (Nsecfilt == catalog[0].Nsecfilt);

  // limit the total number of stars in the catalog
  if (MaxDensityUse) {
    LimitDensityCatalog_ByNmeasureGrid (subcatalog, catalog);
  } else {
    if (VERBOSE2) {
      char *basename = filebasename (catalog[0].filename);
      fprintf (stderr, "subset of "OFF_T_FMT" ("OFF_T_FMT" total) stars, "OFF_T_FMT" ("OFF_T_FMT" total) measures for catalog %s\n", 
	       subcatalog[0].Naverage, catalog[0].Naverage, subcatalog[0].Nmeasure, catalog[0].Nmeasure, basename);
      free (basename);
    }
  }
  if (CHECK_TEST_IMAGE && (Nkeep1 + Nkeep2 + Nskip1 + Nskip2 > 0)) {
    fprintf (stderr, "kept %d %d, skipped %d %d\n", Nkeep1, Nkeep2, Nskip1, Nskip2);
  }

  return (TRUE);
}

void bcatalog_show_skips () {
  fprintf (stderr, "Nskip: %d, %d\n", Nskip1, Nskip2);
  fprintf (stderr, "Nkeep: %d, %d\n", Nkeep1, Nkeep2);
}

// sort by decreasing Nmeasure (X)
void sort_by_Nmeasure (int *X, off_t *Y, off_t N) {

# define SWAPFUNC(A,B){ int tmpI; off_t tmpT;	\
  tmpI = X[A]; X[A] = X[B]; X[B] = tmpI; \
  tmpT = Y[A]; Y[A] = Y[B]; Y[B] = tmpT; \
}
# define COMPARE(A,B)(X[A] > X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* generate a grid in a locally-projected coordinate system, try to select average entries 
   from each grid cell in decending Nmeasure order.
 */ 
int LimitDensityCatalog_ByNmeasureGrid (Catalog *subcatalog, Catalog *oldcatalog) {

  off_t i, j;
  int ix, iy;

  Catalog tmpcatalog;

  double Rmin, Rmax, Dmin, Dmax;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  gfits_scan (&oldcatalog[0].header, "RA0",  "%lf", 1, &Rmin);
  gfits_scan (&oldcatalog[0].header, "DEC0", "%lf", 1, &Dmin);
  gfits_scan (&oldcatalog[0].header, "RA1",  "%lf", 1, &Rmax);
  gfits_scan (&oldcatalog[0].header, "DEC1", "%lf", 1, &Dmax);

  if (VERBOSE2) fprintf (stderr, "extracting from catalog covering region %f,%f to %f,%f\n", Rmin, Dmin, Rmax, Dmax);

  float AREA = fabs(Dmax - Dmin) * fabs(Rmax - Rmin) * cos (0.5*RAD_DEG*(Dmax + Dmin));
  assert (AREA > 0);

  off_t Nmax = MaxDensityValue * AREA;
  if (subcatalog[0].Naverage <= Nmax) {
    if (VERBOSE) {
      fprintf (stderr, "subcatalog has less than the max density\n");
    }
    return (TRUE);
  }

  off_t Naverage = subcatalog[0].Naverage;

  // generate a grid in locally projected space
  double Rc = 0.5*(Rmin + Rmax);
  double Dc = 0.5*(Dmin + Dmax);

  /* project coordinates to a plane centered on the object with units of arcsec */
  Coords coords;
  InitCoords (&coords, "DEC--SIN");
  coords.crval1 = Rc;
  coords.crval2 = Dc;
  coords.cdelt1 = coords.cdelt2 = 1.0 / 3600.0;

  // convert all average R,D values to X,Y:
  double *X, *Y;
  ALLOCATE (X, double, Naverage);
  ALLOCATE (Y, double, Naverage);
  float Xmin = +10000.0, Ymin = +10000.0;
  float Xmax = -10000.0, Ymax = -10000.0;
  for (i = 0; i < Naverage; i++) {
    X[i] = NAN;
    Y[i] = NAN;
    // skip any stars which are outside of nominal catalog range
    if (subcatalog[0].average[i].R < Rmin) continue;
    if (subcatalog[0].average[i].R > Rmax) continue;
    if (subcatalog[0].average[i].D < Dmin) continue;
    if (subcatalog[0].average[i].D > Dmax) continue;
    RD_to_XY (&X[i], &Y[i], subcatalog[0].average[i].R, subcatalog[0].average[i].D, &coords);
    Xmin = MIN (Xmin, X[i]);
    Xmax = MAX (Xmax, X[i]);
    Ymin = MIN (Ymin, Y[i]);
    Ymax = MAX (Ymax, Y[i]);
  }

  // how many grid cells? what is the grid spacing? 
  float dX = Xmax - Xmin;
  float dY = Ymax - Ymin;

  // *** XXX for the moment, I'm using a hard-wired cell size (200 arcsec ~ 3.3 arcmin)
  int NX = (int)(dX / 200) + 1;
  int NY = (int)(dY / 200) + 1;
  // fprintf (stderr, "Density Grid: %d x %d\n", NX, NY);
  // XXX check that NX,NY are sensible (5 degrees / 200 arcsec seems like the absolute max)
  if (NX > 1000) { 
    fprintf (stderr, "serious problem with %s: NX = %d\n", subcatalog[0].filename, NX); 
    exit (3); 
  }
  if (NY > 1000) { 
    fprintf (stderr, "serious problem with %s: NY = %d\n", subcatalog[0].filename, NY); 
    exit (3); 
  }
  
  // kind of ugly : generate a grid of index, Nmeasure arrays
  // to be filled below (I also need NN and Nn to track the number of 
  // entries in each).
  int    **NN_grid;
  int    **Nn_grid;
  int   ***Nm_grid;
  off_t ***idxgrid;
  ALLOCATE (NN_grid, int *, NX);
  ALLOCATE (Nn_grid, int *, NX);
  ALLOCATE (Nm_grid, int **, NX);
  ALLOCATE (idxgrid, off_t **, NX);

  for (ix = 0; ix < NX; ix++) {
    ALLOCATE (NN_grid[ix], int, NY);
    ALLOCATE (Nn_grid[ix], int, NY);
    ALLOCATE (Nm_grid[ix], int *, NY);
    ALLOCATE (idxgrid[ix], off_t *, NY);
    for (iy = 0; iy < NY; iy++) {
      Nn_grid[ix][iy] = 0;
      NN_grid[ix][iy] = 100;
      ALLOCATE (Nm_grid[ix][iy], int,   NN_grid[ix][iy]);
      ALLOCATE (idxgrid[ix][iy], off_t, NN_grid[ix][iy]);
    }
  }

  // assign all of the average entries to a grid cell
  for (i = 0; i < Naverage; i++) {
    if (isnan(X[i])) continue;
    if (isnan(Y[i])) continue;
    ix = (X[i] - Xmin) / 200.0;
    iy = (Y[i] - Ymin) / 200.0;
    int Nn = Nn_grid[ix][iy];
    Nm_grid[ix][iy][Nn] = subcatalog[0].average[i].Nmeasure;
    
    // if we are resetting to a given photcode, we need to have that photcode...
    // XXX this is not relevant for the check...
    if (FALSE && NphotcodesReset) {
      int k;
      int foundReset = FALSE;
      int m = subcatalog[0].average[i].measureOffset;
      MeasureTiny *measure = &subcatalog[0].measureT[m];
      for (j = 0; (j < subcatalog[0].average[i].Nmeasure) && !foundReset; j++) {
	if (CHECK_TEST_IMAGE && (abs(measure[j].t - Tref) < 10) && (measure[j].photcode == Cref)) {
	  fprintf (stderr, ".");
	}
	for (k = 0; (k < NphotcodesReset) && !foundReset; k++) {
	  if (photcodesReset[k][0].code == measure[j].photcode) foundReset = TRUE;
	}
      }
      if (!foundReset) {
	Nm_grid[ix][iy][Nn] = 0;
      }
    }

    idxgrid[ix][iy][Nn] = i;
    Nn_grid[ix][iy] ++;
    if (Nn_grid[ix][iy] >= NN_grid[ix][iy]) {
      NN_grid[ix][iy] += 100;
      REALLOCATE (Nm_grid[ix][iy], int,   NN_grid[ix][iy]);
      REALLOCATE (idxgrid[ix][iy], off_t, NN_grid[ix][iy]);
    }
  }
    
  // sort all of the grid cells
  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      sort_by_Nmeasure (Nm_grid[ix][iy], idxgrid[ix][iy], Nn_grid[ix][iy]);
      NN_grid[ix][iy] = 0; // I'm going to use this array to track which element I've already selected
    }
  }

  // cycle over the grid until we ready Nmax
  off_t *keepidx = NULL;
  ALLOCATE (keepidx, off_t, Naverage);
  memset (keepidx, 0, Naverage*sizeof(off_t));
  int Nkeep = 0;

  for (i = 0; (i < 20) && (Nkeep < Nmax); i++) {
    for (ix = 0; (ix < NX) && (Nkeep < Nmax); ix++) {
      for (iy = 0; (iy < NY) && (Nkeep < Nmax); iy++) {
	if (NN_grid[ix][iy] >= Nn_grid[ix][iy]) continue; // all used up!
	int Nn = NN_grid[ix][iy];
	keepidx[Nkeep] = idxgrid[ix][iy][Nn];
	Nkeep ++;
	NN_grid[ix][iy] ++;
      }
    }
  }

  // count the number of measurements this selection will yield
  off_t ave, NMEASURE = 0;
  for (i = 0; i < Nkeep; i++) {
    ave = keepidx[i];
    NMEASURE += subcatalog[0].average[ave].Nmeasure;
  }

  // test catID : 37262 37261 37257 37258
  int dumpit = FALSE;
  dumpit |= (oldcatalog[0].catID == 37007);
  // dumpit |= (oldcatalog[0].catID == 37261);
  // dumpit |= (oldcatalog[0].catID == 37257);
  // dumpit |= (oldcatalog[0].catID == 37258);
  if (dumpit) {
    char name[64];
    snprintf (name, 64, "cat.%05d.dump.dat", oldcatalog[0].catID);
    FILE *fdump = fopen (name, "w");
    for (i = 0; i < Nkeep; i++) {
      ave = keepidx[i];
      fprintf (fdump, "%10.6f %10.6f %d\n", subcatalog[0].average[ave].R, subcatalog[0].average[ave].D, subcatalog[0].average[ave].Nmeasure);
    }
    fclose (fdump);
  }

  // allocate the output data 
  ALLOCATE (tmpcatalog.average,  Average,     Nkeep);
  ALLOCATE (tmpcatalog.measureT, MeasureTiny, NMEASURE);
  ALLOCATE (tmpcatalog.secfilt,  SecFilt,     Nkeep * Nsecfilt);

  off_t Nmeasure = 0;

  // copy the Nkeep selected entries from subcatalog to tmpcatalog (adjusting links)
  for (i = 0; i < Nkeep; i++) {
    ave = keepidx[i];
    tmpcatalog.average[i] = subcatalog[0].average[ave];
    tmpcatalog.average[i].measureOffset = Nmeasure;
    for (j = 0; j < tmpcatalog.average[i].Nmeasure; j++) {
      off_t offset = subcatalog[0].average[ave].measureOffset + j;
      tmpcatalog.measureT[Nmeasure] = subcatalog[0].measureT[offset];
      tmpcatalog.measureT[Nmeasure].averef = i;
      Nmeasure ++;
    }
    for (j = 0; j < Nsecfilt; j++) {
      tmpcatalog.secfilt[i*Nsecfilt + j] = subcatalog[0].secfilt[ave*Nsecfilt + j];
    }
  }

  if (VERBOSE2) {
    char *basename = filebasename (oldcatalog[0].filename);
    fprintf (stderr, "limited to %d ("OFF_T_FMT" subset, "OFF_T_FMT" total) stars, "OFF_T_FMT" ("OFF_T_FMT" subset, "OFF_T_FMT" total) measures for catalog %s\n", 
	     Nkeep, subcatalog[0].Naverage, oldcatalog[0].Naverage, Nmeasure, subcatalog[0].Nmeasure,  oldcatalog[0].Nmeasure, basename);
    free (basename);
  }

  free (X);
  free (Y);

  for (ix = 0; ix < NX; ix++) {
    for (iy = 0; iy < NY; iy++) {
      free (Nm_grid[ix][iy]);
      free (idxgrid[ix][iy]);
    }
    free (NN_grid[ix]);
    free (Nn_grid[ix]);
    free (Nm_grid[ix]);
    free (idxgrid[ix]);
  }
  free (NN_grid);
  free (Nn_grid);
  free (Nm_grid);
  free (idxgrid);

  free (keepidx);

  free (subcatalog[0].average);
  free (subcatalog[0].measureT);
  free (subcatalog[0].secfilt);

  subcatalog[0].average = tmpcatalog.average;
  subcatalog[0].measureT = tmpcatalog.measureT;
  subcatalog[0].secfilt = tmpcatalog.secfilt;
  subcatalog[0].Naverage = Nkeep;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = oldcatalog[0].Nsecfilt;
  subcatalog[0].Nsecfilt_mem = Naverage * oldcatalog[0].Nsecfilt;

  return (TRUE);
}

/* this version does NOT use AverageTiny, MeasureTiny */ 
int LimitDensityCatalog_ByNmeasure (Catalog *subcatalog, Catalog *oldcatalog) {

  Catalog tmpcatalog;

  double Rmin, Rmax, Dmin, Dmax;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  gfits_scan (&oldcatalog[0].header, "RA0",  "%lf", 1, &Rmin);
  gfits_scan (&oldcatalog[0].header, "DEC0", "%lf", 1, &Dmin);
  gfits_scan (&oldcatalog[0].header, "RA1",  "%lf", 1, &Rmax);
  gfits_scan (&oldcatalog[0].header, "DEC1", "%lf", 1, &Dmax);

  if (VERBOSE2) fprintf (stderr, "extracting from catalog covering region %f,%f to %f,%f\n", Rmin, Dmin, Rmax, Dmax);

  float AREA = fabs(Dmax - Dmin) * fabs(Rmax - Rmin) * cos (0.5*RAD_DEG*(Dmax + Dmin));
  assert (AREA > 0);

  off_t Nmax = MaxDensityValue * AREA;
  if (subcatalog[0].Naverage <= Nmax) {
    if (VERBOSE) {
      fprintf (stderr, "subcatalog has less than the max density\n");
    }
    return (TRUE);
  }

  off_t Naverage = subcatalog[0].Naverage;

  // select a random subset of Nmax stars from subcatalog using Fisher-Yates

  // we are going to select Nmax entries by choosing the brightest objects
  int *value;
  off_t *index, i, j, ave;
  ALLOCATE (index, off_t, Naverage);
  ALLOCATE (value, int, Naverage);
  for (i = 0; i < Naverage; i++) {
    index[i] = i;
    value[i] = subcatalog[0].average[i].Nmeasure;
  }
  sort_by_Nmeasure (value, index, Naverage);

  // count the number of measurements this selection will yield
  off_t NMEASURE = 0;
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    NMEASURE += subcatalog[0].average[ave].Nmeasure;
  }

# if (0)
  if (oldcatalog[0].catID == 59962) {
    FILE *fdump = fopen ("cat.dump.dat", "w");
    for (i = 0; i < Nmax; i++) {
      ave = index[i];
      fprintf (fdump, "%10.6f %10.6f %d\n", subcatalog[0].average[ave].R, subcatalog[0].average[ave].D, subcatalog[0].average[ave].Nmeasure);
    }
    fclose (fdump);
  }
# endif

  // allocate the output data 
  ALLOCATE (tmpcatalog.average,  Average,     Nmax);
  ALLOCATE (tmpcatalog.measureT, MeasureTiny, NMEASURE);
  ALLOCATE (tmpcatalog.secfilt,  SecFilt,     Nmax * Nsecfilt);

  off_t Nmeasure = 0;

  // copy the Nmax selected entries from subcatalog to tmpcatalog (adjusting links)
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    tmpcatalog.average[i] = subcatalog[0].average[ave];
    tmpcatalog.average[i].measureOffset = Nmeasure;
    for (j = 0; j < tmpcatalog.average[i].Nmeasure; j++) {
      off_t offset = subcatalog[0].average[ave].measureOffset + j;
      tmpcatalog.measureT[Nmeasure] = subcatalog[0].measureT[offset];
      tmpcatalog.measureT[Nmeasure].averef = i;
      Nmeasure ++;
    }
    for (j = 0; j < Nsecfilt; j++) {
      tmpcatalog.secfilt[i*Nsecfilt + j] = subcatalog[0].secfilt[ave*Nsecfilt + j];
    }
  }

  if (VERBOSE2) {
    char *basename = filebasename (oldcatalog[0].filename);
    fprintf (stderr, "limited to "OFF_T_FMT" ("OFF_T_FMT" subset, "OFF_T_FMT" total) stars, "OFF_T_FMT" ("OFF_T_FMT" subset, "OFF_T_FMT" total) measures for catalog %s\n", 
	     Nmax, subcatalog[0].Naverage, oldcatalog[0].Naverage, Nmeasure, subcatalog[0].Nmeasure,  oldcatalog[0].Nmeasure, basename);
    free (basename);
  }

  free (index);
  free (value);
  free (subcatalog[0].average);
  free (subcatalog[0].measureT);
  free (subcatalog[0].secfilt);

  subcatalog[0].average = tmpcatalog.average;
  subcatalog[0].measureT = tmpcatalog.measureT;
  subcatalog[0].secfilt = tmpcatalog.secfilt;
  subcatalog[0].Naverage = Nmax;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = oldcatalog[0].Nsecfilt;
  subcatalog[0].Nsecfilt_mem = Naverage * oldcatalog[0].Nsecfilt;

  return (TRUE);
}

/* this version does NOT use AverageTiny, MeasureTiny */ 
int LimitDensityCatalog_RandomSample (Catalog *subcatalog, Catalog *catalog) {

  Catalog tmpcatalog;

  double Rmin, Rmax, Dmin, Dmax;

  int Nsecfilt = GetPhotcodeNsecfilt ();

  gfits_scan (&catalog[0].header, "RA0",  "%lf", 1, &Rmin);
  gfits_scan (&catalog[0].header, "DEC0", "%lf", 1, &Dmin);
  gfits_scan (&catalog[0].header, "RA1",  "%lf", 1, &Rmax);
  gfits_scan (&catalog[0].header, "DEC1", "%lf", 1, &Dmax);

  if (VERBOSE2) fprintf (stderr, "extracting from catalog covering region %f,%f to %f,%f\n", Rmin, Dmin, Rmax, Dmax);

  float AREA = fabs(Dmax - Dmin) * fabs(Rmax - Rmin) * cos (0.5*RAD_DEG*(Dmax + Dmin));
  assert (AREA > 0);

  off_t Nmax = MaxDensityValue * AREA;
  if (subcatalog[0].Naverage <= Nmax) {
    if (VERBOSE2) {
      fprintf (stderr, "subcatalog has less than the max density\n");
    }
    return (TRUE);
  }

  off_t Naverage = subcatalog[0].Naverage;

  // select a random subset of Nmax stars from subcatalog using Fisher-Yates

  // we are going to select Nmax entries by generating a random-sorted index list
  off_t *index, tmp, i, j, ave;
  ALLOCATE (index, off_t, Naverage);
  for (i = 0; i < Naverage; i++) {
    index[i] = i;
  }
  for (i = 0; i < Naverage; i++) {
    j = (Naverage - i) * drand48() + i; // a number between i and Naverage
    tmp = index[j];
    index[j] = index[i];
    index[i] = tmp;
  }

  // count the number of measurements this selection will yield
  off_t NMEASURE = 0;
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    NMEASURE += subcatalog[0].average[ave].Nmeasure;
  }

  // allocate the output data 
  ALLOCATE (tmpcatalog.average,  Average,     Nmax);
  ALLOCATE (tmpcatalog.measureT, MeasureTiny, NMEASURE);
  ALLOCATE (tmpcatalog.secfilt,  SecFilt,     Nmax * Nsecfilt);

  off_t Nmeasure = 0;

  // copy the Nmax selected entries from subcatalog to tmpcatalog (adjusting links)
  for (i = 0; i < Nmax; i++) {
    ave = index[i];
    tmpcatalog.average[i] = subcatalog[0].average[ave];
    tmpcatalog.average[i].measureOffset = Nmeasure;
    for (j = 0; j < tmpcatalog.average[i].Nmeasure; j++) {
      off_t offset = subcatalog[0].average[ave].measureOffset + j;
      tmpcatalog.measureT[Nmeasure] = subcatalog[0].measureT[offset];
      tmpcatalog.measureT[Nmeasure].averef = i;
      Nmeasure ++;
    }
    for (j = 0; j < Nsecfilt; j++) {
      tmpcatalog.secfilt[i*Nsecfilt + j] = subcatalog[0].secfilt[ave*Nsecfilt + j];
    }
  }

  if (VERBOSE2) {
    fprintf (stderr, "limited to "OFF_T_FMT" of "OFF_T_FMT" stars ("OFF_T_FMT" of "OFF_T_FMT" measures) for catalog %s\n", 
	     Nmax, subcatalog[0].Naverage, Nmeasure, subcatalog[0].Nmeasure,  catalog[0].filename);
  }

  free (subcatalog[0].average);
  free (subcatalog[0].measureT);
  free (subcatalog[0].secfilt);

  subcatalog[0].average = tmpcatalog.average;
  subcatalog[0].measureT = tmpcatalog.measureT;
  subcatalog[0].secfilt = tmpcatalog.secfilt;
  subcatalog[0].Naverage = Nmax;
  subcatalog[0].Nmeasure = Nmeasure;
  subcatalog[0].Nsecfilt = catalog[0].Nsecfilt;
  subcatalog[0].Nsecfilt_mem = Naverage * catalog[0].Nsecfilt;

  return (TRUE);
}

