# include "setastrom.h"

// Koppenhoefer astrometry correction functions:
// load the correction set from a file

// the correction is saved as a set of splines, one per file extension
// extensions have names of the form XYnn.Dx.Td

// where nn = 00 - 77, the chip number 
// x is X or Y, the correction direction (dir)
// d is 0 or 1, pre or post camera correction (sub)

// these are stored in an N-D array of (Spline *) splineset[sub][dir][chip]

# define N_SUB 2
# define N_DIR 2
# define N_CHIP 78
// note that we are allocating pointers for XY00 - XY77, but only the octal elements are set (and not the 00,07,70,77 ones)

static Spline ****splineset;

int load_kh_correction (char *filename) {

  int Ncol;
  off_t Nrow;

  char type[16];
  char extname[80];

  Header header;
  Header theader;
  FTable ftable;

  /* open file for input */
  FILE *f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for read : %s\n", filename);
    return FALSE;
  }

  // read the PHU header
  if (!gfits_load_header (f, &header)) return (FALSE);
  
  int Nbytes = gfits_data_size (&header);
  fseeko (f, Nbytes, SEEK_CUR);
  // XXX gfits_free_header (&header);

  ftable.header = &theader;

  int sub, dir, chip;

  // we have a N-D array of splines:
  // splineset[sub][dir][chip]

  ALLOCATE (splineset, Spline ***, N_SUB);
  for (sub = 0; sub < N_SUB; sub++) {
    ALLOCATE (splineset[sub], Spline **, N_DIR);
    for (dir = 0; dir < N_DIR; dir++) {
      ALLOCATE (splineset[sub][dir], Spline *, N_CHIP);
      for (chip = 0; chip < N_CHIP; chip ++) {
	splineset[sub][dir][chip] = NULL;
      }
    }
  }

  while (TRUE) {
    // load data for this header : if not found, assume we hit the end of the file
    if (!gfits_load_header (f, &theader)) break;
    
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) return (FALSE);
    
    if (!gfits_scan (&theader, "EXTNAME", "%s", 1, extname)) return (FALSE);

    // figure out the chip ID, diretion, and sequence from the EXTNAME

    // EXTNAME is of the form XY%d%d.D%s.T%s where 
    char chipname[3];
    chipname[0] = extname[2];
    chipname[1] = extname[3];
    chipname[2] = 0;
    int chip = atoi(chipname);
    myAssert (chip >   0, "error in extname %s not(chip >   0)\n", extname);
    myAssert (chip <  77, "error in extname %s not(chip <  77)\n", extname);
    myAssert (chip !=  7, "error in extname %s not(chip !=  7)\n", extname);
    myAssert (chip != 70, "error in extname %s not(chip != 70)\n", extname);

    int dir = -1;
    if (extname[6] == 'X') {
      dir = 0;
    }
    if (extname[6] == 'Y') {
      dir = 1;
    }
    myAssert (dir != -1, "error in extname %s (dir)\n", extname);

    int sub = -1;
    if (extname[9] == '0') {
      sub = 0;
    }
    if (extname[9] == '1') {
      sub = 1;
    }
    myAssert (sub != -1, "error in extname %s (sub)\n", extname);

    // need to create and assign to flat-field correction
    double *xk = gfits_get_bintable_column_data (&theader, &ftable, "X_KNOT", type, &Nrow, &Ncol);
    myAssert (!strcmp(type, "double"), "error reading X_KNOT");
    
    // need to create and assign to flat-field correction
    double *yk = gfits_get_bintable_column_data (&theader, &ftable, "Y_KNOT", type, &Nrow, &Ncol);
    myAssert (!strcmp(type, "double"), "error reading X_KNOT");
    
    // need to create and assign to flat-field correction
    double *y2 = gfits_get_bintable_column_data (&theader, &ftable, "DY2_DX", type, &Nrow, &Ncol);
    myAssert (!strcmp(type, "double"), "error reading X_KNOT");
    
    ALLOCATE (splineset[sub][dir][chip], Spline, 1);
    splineset[sub][dir][chip]->xk = xk;
    splineset[sub][dir][chip]->yk = yk;
    splineset[sub][dir][chip]->y2 = y2;
    splineset[sub][dir][chip]->Nknots = Nrow;
  }
  return (TRUE);
}

int get_kh_correction (int sub, int chip, double *dX, double *dY, float Minst) {

  Spline *spline = NULL;

  *dX = 0.0;
  *dY = 0.0;

  // XXX need to include some limits on Minst?

  spline = splineset[sub][0][chip];
  if (spline) {
    *dX = spline_apply_dbl (spline->xk, spline->yk, spline->y2, spline->Nknots, Minst);
  }

  spline = splineset[sub][1][chip];
  if (spline) {
    *dY = spline_apply_dbl (spline->xk, spline->yk, spline->y2, spline->Nknots, Minst);
  }
  return (TRUE);
}

