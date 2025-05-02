# include "addstar.h"

static double RA_offset_RINGS_V3[] = {360.000000,   40.000000,  24.000000,  17.142857,  13.333333,  10.909091,   9.230769,   8.000000,   7.200000,   
				        6.545455,   6.000000,   5.625000,   5.294118,   5.000000,   4.736842,   4.556962,   4.390244,   
				        4.285714,   4.186047,   4.090909,   4.044944,   4.044944,   4.000000,   4.000000,   4.044944,   
				        4.044944,   4.090909,   4.186047,   4.285714,   4.390244,   4.556962,   4.736842,   5.000000,   
				        5.294118,   5.625000,   6.000000,   6.545455,   7.200000,   8.000000,   9.230769,  10.909091,  
				       13.333333,  17.142857,  24.000000,  40.000000, 360.000000};

// in the general case, projection cell centers are arbitrary
// in a more specific case, DEC[i] = DEC_origin + DEC_offset*zone
// in an even more specific case, RA[i,zone] = RA_origin[zone] + RA_offset[zone]

enum {TREE_NONE, TREE_MAKE, TREE_CFIS, TREE_LOCAL, TREE_USE};
enum {REGION_NONE, REGION_USER, REGION_MIN, REGION_MAX};

void usage (void) {
  fprintf (stderr, "USAGE: findcell -mktree (tree) (catdir) [-nx Nx] [-ny Ny]\n");
  fprintf (stderr, "USAGE: findcell -mklocal (file) (catdir) [-nx Nx] [-ny Ny]\n");
  fprintf (stderr, "USAGE: findcell -tree (tree) (datafile)\n");
  fprintf (stderr, "   (datafile) should contain a list of RA,DEC pairs\n");
  exit (2);
}

int mktree (char *treefile, char *catdir);
int mkcfis (char *treefile, char *catdir);
int mklocal (char *treefile, char *catdir);
int apply_tree (char *treefile, char *datafile);

float SCALE  = 1.0;
float NX_SUB = 1.0;
float NY_SUB = 1.0;

double R_MIN = 0.0;
double R_MAX = 0.0;
double D_MIN = 0.0;
double D_MAX = 0.0;
int REGION_OPTION = REGION_NONE;

char *BASENAME = NULL;
int projectIDoff = -1;
int skycellIDoff = -1;

int APPEND = FALSE;

int main (int argc, char **argv) {

  int N;
  char *treefile = NULL;

  // what does this program do?

  // 1) load the image table for a tessellation and generate the boundary tree

  // 2) convert RA,DEC (or list?) to cell ID 

  if (get_argument (argc, argv, "-help")) usage ();
  if (get_argument (argc, argv, "-h")) usage ();

  if ((N = get_argument (argc, argv, "-nx"))) {
    remove_argument (N, &argc, argv);
    NX_SUB = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-ny"))) {
    remove_argument (N, &argc, argv);
    NY_SUB = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* pixel scale (arcsec/pixel) */
  if ((N = get_argument (argc, argv, "-scale"))) {
    remove_argument (N, &argc, argv);
    SCALE = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* pixel scale (arcsec/pixel) */
  if ((N = get_argument (argc, argv, "-append"))) {
    remove_argument (N, &argc, argv);
    APPEND = TRUE;
  }

  // user-specified region
  R_MIN = NAN;
  R_MAX = NAN;
  D_MIN = NAN;
  D_MAX = NAN;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    if (N > argc - 4) {
      fprintf (stderr, "USAGE: -region requires 4 arguments (Rmin Rmax Dmin Dmax)\n");
      exit (1);
    }
    R_MIN = atof (argv[N]); remove_argument (N, &argc, argv);
    R_MAX = atof (argv[N]); remove_argument (N, &argc, argv);
    D_MIN = atof (argv[N]); remove_argument (N, &argc, argv);
    D_MAX = atof (argv[N]); remove_argument (N, &argc, argv);
    REGION_OPTION = REGION_USER;
  }

  if ((N = get_argument (argc, argv, "-region-min"))) {
    remove_argument (N, &argc, argv);
    REGION_OPTION = REGION_MIN;
  }

  if ((N = get_argument (argc, argv, "-region-max"))) {
    remove_argument (N, &argc, argv);
    REGION_OPTION = REGION_MAX;
  }

  if ((N = get_argument (argc, argv, "-basename"))) {
    remove_argument (N, &argc, argv);
    if (N > argc - 3) {
      fprintf (stderr, "USAGE: -basename (name) (proj_offset) (skycell_offset)\n");
      exit (1);
    }
    BASENAME = strcreate  (argv[N]);
    remove_argument (N, &argc, argv);
    projectIDoff = atoi(argv[N]);
    remove_argument (N, &argc, argv);
    skycellIDoff = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* extra error messages */
  int MODE = TREE_NONE;
  if ((N = get_argument (argc, argv, "-mktree"))) {
    MODE = TREE_MAKE;
    remove_argument (N, &argc, argv);
    treefile = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-mkcfis"))) {
    MODE = TREE_CFIS;
    remove_argument (N, &argc, argv);
    treefile = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-mklocal"))) {
    MODE = TREE_LOCAL;
    remove_argument (N, &argc, argv);
    treefile = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-tree"))) {
    MODE = TREE_USE;
    remove_argument (N, &argc, argv);
    treefile = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // generate the boundary tree
  if (argc < 2) usage();
  if (!MODE) usage();

  if (MODE == TREE_MAKE) {
    mktree (treefile, argv[1]);
    exit (0);
  }

  if (MODE == TREE_CFIS) {
    mkcfis (treefile, argv[1]);
    exit (0);
  }

  if (MODE == TREE_LOCAL) {
    mklocal (treefile, argv[1]);
    exit (0);
  }

  apply_tree (treefile, argv[1]);
  exit (0);
}

int mkcfis (char *treefile, char *catdir) {

  int i, j, zone, band, status;
  FITS_DB db;
  Image *image;
  off_t Nimage;
  double x, y, ra, dec;

  char imagefile[DVO_MAX_PATH];
  snprintf (imagefile, DVO_MAX_PATH, "%s/Images.dat", catdir);

  gfits_db_init (&db);
  status = dvo_image_lock (&db, imagefile, 2.0, LCK_XCLD);
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) Shutdown ("can't read image catalog %s", db.filename);

  if (!dvo_image_load (&db, TRUE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);

  // convert database table to internal structure (binary to Image)
  // 'image' points to the same memory as db->ftable->buffer
  image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }
  
  // generate an empty BoundaryTree
  BoundaryTree tree;
  tree.FixedGridDEC = TRUE;
  tree.FixedGridRA = TRUE;
  
  float CELLSIZE = 0.5;

  // for the moment, I'm going to hardwire the DEC bands to match RINGS.V3
  tree.DEC_origin = -90.0 - 0.5*CELLSIZE;
  tree.DEC_offset = CELLSIZE;

  tree.Nzone = (180 / CELLSIZE) + 1;
  tree.NX_SUB = NX_SUB;
  tree.NY_SUB = NY_SUB;
  tree.dPix = SCALE/3600.0; // user-supplied (0.185768447409 for CFIS)

  ALLOCATE (tree.Nband, int, tree.Nzone);
  ALLOCATE (tree.NBAND, int, tree.Nzone);

  ALLOCATE (tree.RA_origin,   double, tree.Nzone);
  ALLOCATE (tree.RA_offset,   double, tree.Nzone);
  ALLOCATE (tree.DEC_min,     double, tree.Nzone);
  ALLOCATE (tree.DEC_max,     double, tree.Nzone);
  ALLOCATE (tree.DEC_min_raw, double, tree.Nzone);
  ALLOCATE (tree.DEC_max_raw, double, tree.Nzone);

  ALLOCATE (tree.ra,   double *, tree.Nzone);
  ALLOCATE (tree.dec,  double *, tree.Nzone);
  ALLOCATE (tree.Xo,   double *, tree.Nzone);
  ALLOCATE (tree.Yo,   double *, tree.Nzone);
  ALLOCATE (tree.dX,    float *, tree.Nzone);
  ALLOCATE (tree.dY,    float *, tree.Nzone);
  ALLOCATE (tree.cell,    int *, tree.Nzone);
  ALLOCATE (tree.name,  char **, tree.Nzone);

  // NOTE 1: the RA_origin, RA_offsets for RINGS.V3 are defined so that the first projection cell 
  // overlaps the RA = 0,360 boundary.  This make the split a bit of a hack.  We end up
  // with Nbands, but the max boundary of the last band only goes to 360.0 -
  // 0.5*RA_offset.  To get the right band number for the boundary region, if the
  // calculation for the band number lands beyond the Nbands (ie, band >= Nbands), then 
  // we need to loop back to the first band.

  // NOTE 2: when we generate the zone & bands initially, we do not know the number of
  // bands in the end.  we cannot use the test of band >= Nband unless we set an absurdly
  // large default value.  Thus the value of 1000000 below.

  // assign the bands for RINGS.V3
  for (zone = 0; zone < tree.Nzone; zone++) {
    tree.Nband[zone] = 1000000;
    tree.NBAND[zone] = 10;

    float zoneDec = -90.0 + zone * CELLSIZE;
    float dRA = CELLSIZE / cos(RAD_DEG*zoneDec); // dRA is a size in RA degrees == \alpha_n

    tree.RA_origin[zone] = 0.0 - 0.5*dRA;
    tree.RA_offset[zone] = dRA;

    // set the starting values here.  In fact, the MIN and MAX values need to be tweaked 
    // on the basis of the real images...
    tree.DEC_min_raw[zone] = +90.0;
    tree.DEC_max_raw[zone] = -90.0;
    tree.DEC_min[zone] = NAN;
    tree.DEC_max[zone] = NAN;

    ALLOCATE (tree.ra[zone],   double, tree.NBAND[zone]);
    ALLOCATE (tree.dec[zone],  double, tree.NBAND[zone]);
    ALLOCATE (tree.Xo[zone],   double, tree.NBAND[zone]);
    ALLOCATE (tree.Yo[zone],   double, tree.NBAND[zone]);
    ALLOCATE (tree.dX[zone],    float, tree.NBAND[zone]);
    ALLOCATE (tree.dY[zone],    float, tree.NBAND[zone]);
    ALLOCATE (tree.cell[zone], int,    tree.NBAND[zone]);
    ALLOCATE (tree.name[zone], char *, tree.NBAND[zone]);
    for (band = 0; band < tree.NBAND[zone]; band++) {
      tree.ra[zone][band] = NAN;
      tree.dec[zone][band] = NAN;
      tree.Xo[zone][band] = NAN;
      tree.Yo[zone][band] = NAN;
      tree.dX[zone][band] = NAN;
      tree.dY[zone][band] = NAN;
      tree.cell[zone][band] = -1;
      ALLOCATE (tree.name[zone][band], char, BOUNDARY_TREE_NAME_LENGTH);
    }
  }

  // find the RA,DEC of the image centers & assign to cells
  for (i = 0; i < Nimage; i++) {
    x = 0.5*image[i].NX;
    y = 0.5*image[i].NY;
    XY_to_RD (&ra, &dec, x, y, &image[i].coords);

    if (!BoundaryTreeCellCoords (&tree, &zone, &band, ra, dec)) {
      fprintf (stderr, "mismatch!\n");
      continue;
    }
    // fprintf (stderr, "%d  %f %f  %f  %f %f  %d %d\n", i, x, y, tree.RA_offset[zone], ra, dec, zone, band);
    
    if (band >= tree.NBAND[zone]) {
      int start = tree.NBAND[zone];
      tree.NBAND[zone] = band + 10;
      REALLOCATE (tree.ra[zone],   double, tree.NBAND[zone]);
      REALLOCATE (tree.dec[zone],  double, tree.NBAND[zone]);
      REALLOCATE (tree.Xo[zone],   double, tree.NBAND[zone]);
      REALLOCATE (tree.Yo[zone],   double, tree.NBAND[zone]);
      REALLOCATE (tree.dX[zone],    float, tree.NBAND[zone]);
      REALLOCATE (tree.dY[zone],    float, tree.NBAND[zone]);
      REALLOCATE (tree.cell[zone],    int, tree.NBAND[zone]);
      REALLOCATE (tree.name[zone], char *, tree.NBAND[zone]);
      for (j = start; j < tree.NBAND[zone]; j++) {
	tree.ra[zone][j] = NAN;
	tree.dec[zone][j] = NAN;
	tree.Xo[zone][band] = NAN;
	tree.Yo[zone][band] = NAN;
	tree.dX[zone][band] = NAN;
	tree.dY[zone][band] = NAN;
	tree.cell[zone][j] = -1;
	ALLOCATE (tree.name[zone][j], char, BOUNDARY_TREE_NAME_LENGTH);
      }
    }
    tree.ra[zone][band] = ra;
    tree.dec[zone][band] = dec;
    tree.Xo[zone][band] = x;
    tree.Yo[zone][band] = y;
    tree.dX[zone][band] = image[i].NX / NX_SUB;
    tree.dY[zone][band] = image[i].NY / NY_SUB;
    
    tree.cell[zone][band] = i;

    // what are the min and max DEC values for this zone? (test center and corners of top and bottom edge)  

    if (dec > 0.0) {
      // min DEC at bottom of the cell at the center
      x = 0.5*image[i].NX;
      y = 0.0*image[i].NY;
      XY_to_RD (&ra, &dec, x, y, &image[i].coords);
      tree.DEC_min_raw[zone] = MIN(tree.DEC_min_raw[zone], dec);
  
      // max DEC : find the intersection between the RA boundary and the top of the cell 
      double ra_band_min = tree.RA_origin[zone] + tree.RA_offset[zone] * band;
  
      // does the parity matter?
      x = 0.0*image[i].NX;
      y = 1.0*image[i].NY;
  
      int Niter;
      for (Niter = 0; Niter < 3; Niter ++) {
        XY_to_RD (&ra, &dec, x, y, &image[i].coords);
        RD_to_XY (&x, &y, ra_band_min, dec, &image[i].coords);
        y = 1.0*image[i].NY;
      }
      tree.DEC_max_raw[zone] = MAX(tree.DEC_max_raw[zone], dec);
    } else {
      // max DEC at top of the cell at the center
      x = 0.5*image[i].NX;
      y = 1.0*image[i].NY;
      XY_to_RD (&ra, &dec, x, y, &image[i].coords);
      tree.DEC_max_raw[zone] = MAX(tree.DEC_max_raw[zone], dec);
  
      // max DEC : find the intersection between the RA boundary and the bottom of the cell 
      double ra_band_min = tree.RA_origin[zone] + tree.RA_offset[zone] * band;
  
      // does the parity matter? (NO)
      x = 0.0*image[i].NX;
      y = 0.0*image[i].NY;
  
      int Niter;
      for (Niter = 0; Niter < 3; Niter ++) {
        XY_to_RD (&ra, &dec, x, y, &image[i].coords);
        RD_to_XY (&x, &y, ra_band_min, dec, &image[i].coords);
        y = 0.0*image[i].NY;
      }
      tree.DEC_min_raw[zone] = MIN(tree.DEC_min_raw[zone], dec);
    }
    memcpy (tree.name[zone][band], image[i].name, BOUNDARY_TREE_NAME_LENGTH);
  }

  // figure out the max band value for each zone?
  for (zone = 0; zone < tree.Nzone; zone++) {
    int found_last = FALSE;
    int last_band = -1;
    for (band = 0; band < tree.NBAND[zone]; band++) {
      // all cells should be filled
      if (tree.cell[zone][band] < 0) {
	if (!found_last) {
	  found_last = TRUE;
	  last_band = band;
	}
      } else {
	if (found_last) {
	  fprintf (stderr, "error: empty cell (%d,%d) after last band (%d)\n", zone, band, last_band);
	}
      }
    }
    if (last_band == -1) {
      last_band = tree.NBAND[zone];
    }
    tree.Nband[zone] = last_band;
    // fprintf (stderr, "last_band : %d, Nband: %d\n", last_band, tree.Nband[zone]);
  }

  // figure out the max band value for each zone?
  int Nm = 0;
  tree.DEC_min[Nm] = -90.0;
  tree.DEC_max[Nm] = 0.5*(tree.DEC_max_raw[Nm] + tree.DEC_min_raw[Nm + 1]);

  int Np = tree.Nzone - 1;
  tree.DEC_min[Np] = 0.5*(tree.DEC_min_raw[Np] + tree.DEC_max_raw[Np - 1]);
  tree.DEC_max[Np] = +90.0;

  for (zone = 1; zone < tree.Nzone - 1; zone++) {
    tree.DEC_min[zone] = 0.5*(tree.DEC_min_raw[zone] + tree.DEC_max_raw[zone - 1]);
    tree.DEC_max[zone] = 0.5*(tree.DEC_max_raw[zone] + tree.DEC_min_raw[zone + 1]);
  }

  INITTIME;

  int Npts = 10000000;

  // test : find skycell for NN random points on the sky
  long A = time(NULL);
  long B = A + 10000;
  srand48(B);
  for (i = 0; i < Npts; i++) {
    ra  = 360.0 * drand48();
    dec = 180.0 * drand48() - 90.0;
    if (!BoundaryTreeCellCoords (&tree, &zone, &band, ra, dec)) {
      fprintf (stderr, "failure for %f,%f\n", ra, dec);
    }
  }
  MARKTIME("-- test %d pts: %f sec\n", Npts, dtime);

  int Ntess = 0;
  TessellationTable *tess = NULL;

  if (APPEND) {
    tess = TessellationTableLoad (treefile, &Ntess);
    REALLOCATE (tess, TessellationTable, Ntess + 1);
  } else {
    ALLOCATE (tess, TessellationTable, Ntess + 1);
  }    

  TessellationTableInit (&tess[Ntess], 1);
  tess[Ntess].tree = &tree;
  tess[Ntess].type = TESS_RINGS;
  tess[Ntess].Rmin =   0;
  tess[Ntess].Rmax = 360;
  tess[Ntess].Dmin = -90;
  tess[Ntess].Dmax = +90;

  if (BASENAME) {
    tess[Ntess].Nbasename = strlen(BASENAME);
    tess[Ntess].basename = strcreate(BASENAME);
    tess[Ntess].projectIDoff = projectIDoff;
    tess[Ntess].skycellIDoff = skycellIDoff;
  }
      
  Ntess ++;
  TessellationTableSave (treefile, tess, Ntess);

  return TRUE;
}

int mktree (char *treefile, char *catdir) {

  int i, j, zone, band, status;
  FITS_DB db;
  Image *image;
  off_t Nimage;
  double x, y, ra, dec;

  char imagefile[DVO_MAX_PATH];
  snprintf (imagefile, DVO_MAX_PATH, "%s/Images.dat", catdir);

  gfits_db_init (&db);
  status = dvo_image_lock (&db, imagefile, 2.0, LCK_XCLD);
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) Shutdown ("can't read image catalog %s", db.filename);

  if (!dvo_image_load (&db, TRUE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);

  // convert database table to internal structure (binary to Image)
  // 'image' points to the same memory as db->ftable->buffer
  image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
  }
  
  // generate an empty BoundaryTree
  BoundaryTree tree;
  tree.FixedGridDEC = TRUE;
  tree.FixedGridRA = TRUE;
  
  // for the moment, I'm going to hardwire the DEC bands to match RINGS.V3
  tree.DEC_origin = -92.0;
  tree.DEC_offset =   4.0;

  tree.Nzone = 46;
  tree.NX_SUB = NX_SUB;
  tree.NY_SUB = NY_SUB;
  tree.dPix = SCALE/3600.0;

  ALLOCATE (tree.Nband, int, tree.Nzone);
  ALLOCATE (tree.NBAND, int, tree.Nzone);

  ALLOCATE (tree.RA_origin,   double, tree.Nzone);
  ALLOCATE (tree.RA_offset,   double, tree.Nzone);
  ALLOCATE (tree.DEC_min,     double, tree.Nzone);
  ALLOCATE (tree.DEC_max,     double, tree.Nzone);
  ALLOCATE (tree.DEC_min_raw, double, tree.Nzone);
  ALLOCATE (tree.DEC_max_raw, double, tree.Nzone);

  ALLOCATE (tree.ra,   double *, tree.Nzone);
  ALLOCATE (tree.dec,  double *, tree.Nzone);
  ALLOCATE (tree.Xo,   double *, tree.Nzone);
  ALLOCATE (tree.Yo,   double *, tree.Nzone);
  ALLOCATE (tree.dX,    float *, tree.Nzone);
  ALLOCATE (tree.dY,    float *, tree.Nzone);
  ALLOCATE (tree.cell,    int *, tree.Nzone);
  ALLOCATE (tree.name,  char **, tree.Nzone);

  // NOTE 1: the RA_origin, RA_offsets for RINGS.V3 are defined so that the first projection cell 
  // overlaps the RA = 0,360 boundary.  This make the split a bit of a hack.  We end up
  // with Nbands, but the max boundary of the last band only goes to 360.0 -
  // 0.5*RA_offset.  To get the right band number for the boundary region, if the
  // calculation for the band number lands beyond the Nbands (ie, band >= Nbands), then 
  // we need to loop back to the first band.

  // NOTE 2: when we generate the zone & bands initially, we do not know the number of
  // bands in the end.  we cannot use the test of band >= Nband unless we set an absurdly
  // large default value.  Thus the value of 1000000 below.

  // assign the bands for RINGS.V3
  for (zone = 0; zone < tree.Nzone; zone++) {
    tree.Nband[zone] = 1000000;
    tree.NBAND[zone] = 10;
    tree.RA_origin[zone] = -0.5*RA_offset_RINGS_V3[zone];
    tree.RA_offset[zone] = RA_offset_RINGS_V3[zone];

    // set the starting values here.  In fact, the MIN and MAX values need to be tweaked 
    // on the basis of the real images...
    tree.DEC_min_raw[zone] = +90.0;
    tree.DEC_max_raw[zone] = -90.0;
    tree.DEC_min[zone] = NAN;
    tree.DEC_max[zone] = NAN;

    ALLOCATE (tree.ra[zone],   double, tree.NBAND[zone]);
    ALLOCATE (tree.dec[zone],  double, tree.NBAND[zone]);
    ALLOCATE (tree.Xo[zone],   double, tree.NBAND[zone]);
    ALLOCATE (tree.Yo[zone],   double, tree.NBAND[zone]);
    ALLOCATE (tree.dX[zone],    float, tree.NBAND[zone]);
    ALLOCATE (tree.dY[zone],    float, tree.NBAND[zone]);
    ALLOCATE (tree.cell[zone], int,    tree.NBAND[zone]);
    ALLOCATE (tree.name[zone], char *, tree.NBAND[zone]);
    for (band = 0; band < tree.NBAND[zone]; band++) {
      tree.ra[zone][band] = NAN;
      tree.dec[zone][band] = NAN;
      tree.Xo[zone][band] = NAN;
      tree.Yo[zone][band] = NAN;
      tree.dX[zone][band] = NAN;
      tree.dY[zone][band] = NAN;
      tree.cell[zone][band] = -1;
      ALLOCATE (tree.name[zone][band], char, BOUNDARY_TREE_NAME_LENGTH);
    }
  }

  // find the RA,DEC of the image centers & assign to cells
  for (i = 0; i < Nimage; i++) {
    x = 0.5*image[i].NX;
    y = 0.5*image[i].NY;
    XY_to_RD (&ra, &dec, x, y, &image[i].coords);

    if (!BoundaryTreeCellCoords (&tree, &zone, &band, ra, dec)) {
      fprintf (stderr, "mismatch!\n");
      continue;
    }
    // fprintf (stderr, "%d  %f %f  %f  %f %f  %d %d\n", i, x, y, tree.RA_offset[zone], ra, dec, zone, band);
    
    if (band >= tree.NBAND[zone]) {
      int start = tree.NBAND[zone];
      tree.NBAND[zone] = band + 10;
      REALLOCATE (tree.ra[zone],   double, tree.NBAND[zone]);
      REALLOCATE (tree.dec[zone],  double, tree.NBAND[zone]);
      REALLOCATE (tree.Xo[zone],   double, tree.NBAND[zone]);
      REALLOCATE (tree.Yo[zone],   double, tree.NBAND[zone]);
      REALLOCATE (tree.dX[zone],    float, tree.NBAND[zone]);
      REALLOCATE (tree.dY[zone],    float, tree.NBAND[zone]);
      REALLOCATE (tree.cell[zone],    int, tree.NBAND[zone]);
      REALLOCATE (tree.name[zone], char *, tree.NBAND[zone]);
      for (j = start; j < tree.NBAND[zone]; j++) {
	tree.ra[zone][j] = NAN;
	tree.dec[zone][j] = NAN;
	tree.Xo[zone][band] = NAN;
	tree.Yo[zone][band] = NAN;
	tree.dX[zone][band] = NAN;
	tree.dY[zone][band] = NAN;
	tree.cell[zone][j] = -1;
	ALLOCATE (tree.name[zone][j], char, BOUNDARY_TREE_NAME_LENGTH);
      }
    }
    tree.ra[zone][band] = ra;
    tree.dec[zone][band] = dec;
    tree.Xo[zone][band] = x;
    tree.Yo[zone][band] = y;
    tree.dX[zone][band] = image[i].NX / NX_SUB;
    tree.dY[zone][band] = image[i].NY / NY_SUB;
    
    tree.cell[zone][band] = i;

    // what are the min and max DEC values for this zone? (test center and corners of top and bottom edge)  

    if (dec > 0.0) {
      // min DEC at bottom of the cell at the center
      x = 0.5*image[i].NX;
      y = 0.0*image[i].NY;
      XY_to_RD (&ra, &dec, x, y, &image[i].coords);
      tree.DEC_min_raw[zone] = MIN(tree.DEC_min_raw[zone], dec);
  
      // max DEC : find the intersection between the RA boundary and the top of the cell 
      double ra_band_min = tree.RA_origin[zone] + tree.RA_offset[zone] * band;
  
      // does the parity matter?
      x = 0.0*image[i].NX;
      y = 1.0*image[i].NY;
  
      int Niter;
      for (Niter = 0; Niter < 3; Niter ++) {
        XY_to_RD (&ra, &dec, x, y, &image[i].coords);
        RD_to_XY (&x, &y, ra_band_min, dec, &image[i].coords);
        y = 1.0*image[i].NY;
      }
      tree.DEC_max_raw[zone] = MAX(tree.DEC_max_raw[zone], dec);
    } else {
      // max DEC at top of the cell at the center
      x = 0.5*image[i].NX;
      y = 1.0*image[i].NY;
      XY_to_RD (&ra, &dec, x, y, &image[i].coords);
      tree.DEC_max_raw[zone] = MAX(tree.DEC_max_raw[zone], dec);
  
      // max DEC : find the intersection between the RA boundary and the bottom of the cell 
      double ra_band_min = tree.RA_origin[zone] + tree.RA_offset[zone] * band;
  
      // does the parity matter? (NO)
      x = 0.0*image[i].NX;
      y = 0.0*image[i].NY;
  
      int Niter;
      for (Niter = 0; Niter < 3; Niter ++) {
        XY_to_RD (&ra, &dec, x, y, &image[i].coords);
        RD_to_XY (&x, &y, ra_band_min, dec, &image[i].coords);
        y = 0.0*image[i].NY;
      }
      tree.DEC_min_raw[zone] = MIN(tree.DEC_min_raw[zone], dec);
    }
    memcpy (tree.name[zone][band], image[i].name, BOUNDARY_TREE_NAME_LENGTH);
  }

  // figure out the max band value for each zone?
  for (zone = 0; zone < tree.Nzone; zone++) {
    int found_last = FALSE;
    int last_band = -1;
    for (band = 0; band < tree.NBAND[zone]; band++) {
      // all cells should be filled
      if (tree.cell[zone][band] < 0) {
	if (!found_last) {
	  found_last = TRUE;
	  last_band = band;
	}
      } else {
	if (found_last) {
	  fprintf (stderr, "error: empty cell (%d,%d) after last band (%d)\n", zone, band, last_band);
	}
      }
    }
    if (last_band == -1) {
      last_band = tree.NBAND[zone];
    }
    tree.Nband[zone] = last_band;
    // fprintf (stderr, "last_band : %d, Nband: %d\n", last_band, tree.Nband[zone]);
  }

  // figure out the max band value for each zone?
  int Nm = 0;
  tree.DEC_min[Nm] = -90.0;
  tree.DEC_max[Nm] = 0.5*(tree.DEC_max_raw[Nm] + tree.DEC_min_raw[Nm + 1]);

  int Np = tree.Nzone - 1;
  tree.DEC_min[Np] = 0.5*(tree.DEC_min_raw[Np] + tree.DEC_max_raw[Np - 1]);
  tree.DEC_max[Np] = +90.0;

  for (zone = 1; zone < tree.Nzone - 1; zone++) {
    tree.DEC_min[zone] = 0.5*(tree.DEC_min_raw[zone] + tree.DEC_max_raw[zone - 1]);
    tree.DEC_max[zone] = 0.5*(tree.DEC_max_raw[zone] + tree.DEC_min_raw[zone + 1]);
  }

  INITTIME;

  int Npts = 10000000;

  // test : find skycell for NN random points on the sky
  long A = time(NULL);
  long B = A + 10000;
  srand48(B);
  for (i = 0; i < Npts; i++) {
    ra  = 360.0 * drand48();
    dec = 180.0 * drand48() - 90.0;
    if (!BoundaryTreeCellCoords (&tree, &zone, &band, ra, dec)) {
      fprintf (stderr, "failure for %f,%f\n", ra, dec);
    }
  }
  MARKTIME("-- test %d pts: %f sec\n", Npts, dtime);

  if (APPEND) {
    int Ntess = 0;
    TessellationTable *tess = TessellationTableLoad (treefile, &Ntess);
    REALLOCATE (tess, TessellationTable, Ntess + 1);
    TessellationTableInit (&tess[Ntess], 1);
    tess[Ntess].tree = &tree;
    tess[Ntess].type = TESS_RINGS;
    tess[Ntess].Rmin =   0;
    tess[Ntess].Rmax = 360;
    tess[Ntess].Dmin = -90;
    tess[Ntess].Dmax = +90;

    if (BASENAME) {
      tess[Ntess].Nbasename = strlen(BASENAME);
      tess[Ntess].basename = strcreate(BASENAME);
      tess[Ntess].projectIDoff = projectIDoff;
      tess[Ntess].skycellIDoff = skycellIDoff;
    }
      
    // add basename an related here...
    Ntess ++;
    TessellationTableSave (treefile, tess, Ntess);
  } else {
    BoundaryTreeSave (treefile, &tree);
  }

  return TRUE;
}

int apply_tree (char *treefile, char *datafile) {

  int Ntess = 0;
  TessellationTable *tess = TessellationTableLoad (treefile, &Ntess);
  if (!tess) {
    fprintf (stderr, "error loading tessellation table file %s\n", treefile);
    exit (2);
  }

  FILE *f = fopen (datafile, "r");
  if (!f) {
    fprintf (stderr, "error opening data file %s\n", datafile);
    exit (3);
  }

  double ra, dec;
  int Nvalue = 0;
  while ((Nvalue = fscanf (f, "%lf %lf", &ra, &dec)) != EOF) {

    int tessID, projID, skycellID;

    if (!TessellationPrimaryCellIDs (tess, Ntess, &tessID, &projID, &skycellID, ra, dec)) {
      fprintf (stderr, "error finding cell for %f,%f\n", ra, dec);
      continue;
    }

    fprintf (stdout, "%10.6f %10.6f : %2d  %04d %03d : %s\n", ra, dec, tessID, projID, skycellID, tess[tessID].basename);
  }

  exit (0);
}

int apply_tree_old (char *treefile, char *datafile) {

  BoundaryTree *tree = BoundaryTreeLoad (treefile);
  if (!tree) {
    fprintf (stderr, "error loading boundary tree file %s\n", treefile);
    exit (2);
  }

  FILE *f = fopen (datafile, "r");
  if (!f) {
    fprintf (stderr, "error opening data file %s\n", datafile);
    exit (3);
  }

  double ra, dec;
  int Nvalue = 0;
  while ((Nvalue = fscanf (f, "%lf %lf", &ra, &dec)) != EOF) {

    int zone, band;
    if (!BoundaryTreeCellCoords (tree, &zone, &band, ra, dec)) {
      fprintf (stderr, "error finding cell for %f,%f\n", ra, dec);
      continue;
    }

    // I know the projection cell (band,zone), but I need to find the skycell within that projection cell.
    // the proj cell is divided into Nx, Ny bits.
    // (ra,dec) for (Ro,Do) -> (x,y).  given (Xo,Yo),(dX,dY) I can find ix,iy
    // I currently track Ro,Do (tree->ra[zone][band], tree->dec[zone][band])
    // I need to have the skycell center in pixels (Xo,Yo) and the scale dX,dY
    
    // convert R,D to X,Y with hard-wired projection and scale, orientation?
    
    double x = 0.0;
    double y = 0.0;
    BoundaryTreeProjection (&x, &y, ra, dec, tree, zone, band);

    int xi = x / tree->dX[zone][band];
    int yi = y / tree->dY[zone][band];
    
    char format[24], skycellname[128];
    int Ndigit = (int)(log10(tree->NX_SUB*tree->NY_SUB)) + 1 ;
    snprintf (format, 24, "%s.%%0%dd", tree->name[zone][band], Ndigit);

    int N = xi + tree->NX_SUB * yi;
    snprintf (skycellname, 128, format, N);

    fprintf (stdout, "%10.6f %10.6f  %8.3f %8.3f  %3d %3d  %s\n", ra, dec, x, y, zone, band, skycellname);
  }

  exit (0);
}

// a given tess is defined by a catdir (more than one per catdir?)
// this function takes a catdir and generates an extension for a tess file for a local tess.

/*

  LOCAL projections are defined by single projection cells.  should I be supplying the info on
  the cmd line or figure out the bounds from the catdir?  

 */

int mklocal (char *treefile, char *catdir) {

  int i, j, status;
  FITS_DB db;
  Image *image;
  off_t Nimage;
  double x, y, ra, dec;

  if (REGION_OPTION == REGION_NONE) {
    fprintf (stderr, "ERROR: need to define bounding region (-region Rmin Rmax Dmin Dmax | -region-min | -region-max)\n");
    exit (2);
  }

  if (!BASENAME) {
    fprintf (stderr, "ERROR: need to define BASENAME -basename (name) (proj_offset) (skycell_offset)\n");
    exit (2);
  }

  char imagefile[DVO_MAX_PATH];
  snprintf (imagefile, DVO_MAX_PATH, "%s/Images.dat", catdir);

  gfits_db_init (&db);
  status = dvo_image_lock (&db, imagefile, 2.0, LCK_XCLD);
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) Shutdown ("can't read image catalog %s", db.filename);

  if (!dvo_image_load (&db, TRUE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);

  // convert database table to internal structure (binary to Image)
  // 'image' points to the same memory as db->ftable->buffer
  image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }
  
  // generate an empty BoundaryTree
  TessellationTable *tess = NULL;

  int NtessDisk = 0;
  if (APPEND) {
    tess = TessellationTableLoad (treefile, &NtessDisk);
    REALLOCATE (tess, TessellationTable, Nimage + NtessDisk);
  } else {
    ALLOCATE (tess, TessellationTable, Nimage + NtessDisk);
  }

  TessellationTableInit (&tess[NtessDisk], Nimage);

  // find the RA,DEC of the image centers & assign to cells
  int Ntess = NtessDisk;
  for (i = 0; i < Nimage; i++) {
    // user supplied values, do not try to derive from Image.dat
    tess[Ntess].NX_SUB = NX_SUB;
    tess[Ntess].NY_SUB = NY_SUB;
    tess[Ntess].dPix = SCALE/3600.0;

    x = 0.5*image[i].NX;
    y = 0.5*image[i].NY;
    XY_to_RD (&ra, &dec, x, y, &image[i].coords);

    tess[Ntess].Ro  = ra;
    tess[Ntess].Do  = dec;
    tess[Ntess].Xo  = x;
    tess[Ntess].Yo  = y;
    tess[Ntess].dX  = image[i].NX / NX_SUB;
    tess[Ntess].dY  = image[i].NY / NY_SUB;

    // find the minimum or maximum containing region
    if ((REGION_OPTION == REGION_MIN) || (REGION_OPTION == REGION_MAX)) {
      // XXX short cut for now : if the projection cell bounds the equator or 0,360 boundary, this will fail:
      double R[4], D[4];
      XY_to_RD (&R[0], &D[0],           0,           0, &image[i].coords);
      XY_to_RD (&R[1], &D[1], image[i].NX,           0, &image[i].coords);
      XY_to_RD (&R[2], &D[2],           0, image[i].NY, &image[i].coords);
      XY_to_RD (&R[3], &D[3], image[i].NX, image[i].NY, &image[i].coords);

      if (REGION_OPTION == REGION_MIN) { 
	for (j = 0; j < 4; j++) {
	  R_MIN = (R[j] < ra)  ? (isfinite(R_MIN) ? MAX(R_MIN, R[j]) : R[j]) : R_MIN;
	  R_MAX = (R[j] > ra)  ? (isfinite(R_MAX) ? MIN(R_MAX, R[j]) : R[j]) : R_MAX;
	  D_MIN = (D[j] < dec) ? (isfinite(D_MIN) ? MAX(D_MIN, D[j]) : D[j]) : D_MIN;
	  D_MAX = (D[j] > dec) ? (isfinite(D_MAX) ? MIN(D_MAX, D[j]) : D[j]) : D_MAX;
	}
      } else {
	for (j = 0; j < 4; j++) {
	  R_MIN = (R[j] < ra)  ? (isfinite(R_MIN) ? MIN(R_MIN, R[j]) : R[j]) : R_MIN;
	  R_MAX = (R[j] > ra)  ? (isfinite(R_MAX) ? MAX(R_MAX, R[j]) : R[j]) : R_MAX;
	  D_MIN = (D[j] < dec) ? (isfinite(D_MIN) ? MIN(D_MIN, D[j]) : D[j]) : D_MIN;
	  D_MAX = (D[j] > dec) ? (isfinite(D_MAX) ? MAX(D_MAX, D[j]) : D[j]) : D_MAX;
	}
      }
    }
    tess[Ntess].Rmin = R_MIN;
    tess[Ntess].Rmax = R_MAX;
    tess[Ntess].Dmin = D_MIN;
    tess[Ntess].Dmax = D_MAX;

    tess[Ntess].type = TESS_LOCAL;

    // XXX I don't really want to do the work of discovering the rule...
    tess[Ntess].Nbasename = strlen(BASENAME);
    tess[Ntess].basename = strcreate(BASENAME);

    tess[Ntess].projectIDoff = projectIDoff;
    tess[Ntess].skycellIDoff = skycellIDoff;

    Ntess ++;
  }

  TessellationTableSave (treefile, tess, Ntess);

  return TRUE;
}

