# include "setastrom.h"

// DCR correction functions:
// load the correction set from a file

// the correction is saved as a set of splines, one per filter and direction
// extensions have names of the form [grizy].[xy].v0

// [grizy] is the filter
// [xy] is the correction direction (dir)

// these are stored in an N-D array of (Spline *) splineset[dir][filter]

# define N_DIR 2
# define N_FILTER 5

static Spline ***splineset;

static float MaxDCR[] = {+0.030, +0.005, +0.005, +0.010, +0.015};
static float MinDCR[] = {-0.030, -0.005, -0.010, -0.015, -0.010};

int load_dcr_correction (char *filename) {

  int Ncol;
  off_t Nrow;

  char type[16];

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
  
  // skip the PHU matrix
  int Nbytes = gfits_data_size (&header);
  fseeko (f, Nbytes, SEEK_CUR);

  ftable.header = &theader;

  int dir, filter;

  // we have a N-D array of splines:
  // splineset[dir][filter]

  ALLOCATE (splineset, Spline **, N_DIR);
  for (dir = 0; dir < N_DIR; dir++) {
    ALLOCATE (splineset[dir], Spline *, N_FILTER);
    for (filter = 0; filter < N_FILTER; filter ++) {
      splineset[dir][filter] = NULL;
    }
  }

  while (TRUE) {
    // load data for this header : if not found, assume we hit the end of the file
    if (!gfits_load_header (f, &theader)) break;
    
    if (!gfits_fread_ftable_data (f, &ftable, FALSE)) return (FALSE);
    
    char word[80];
    if (!gfits_scan (&theader, "EXTNAME", "%s", 1, word)) return (FALSE);
    char *extname = strcreate(word);

    // figure out the filter and direction from the EXTNAME
    // EXTNAME is of the form DCR.[grizy].dP[xy]

    char *ptr = NULL;
    char *w0 = strtok_r (extname, ".", &ptr); myAssert (w0, "invalid dcr file");
    char *w1 = strtok_r (NULL, ".", &ptr); myAssert (w1, "invalid dcr file");
    char *w2 = strtok_r (NULL, ".", &ptr); myAssert (w2, "invalid dcr file");
    
    myAssert (!strcmp(w0, "DCR"), "invalid dcr file");

    char filtname = w1[0];
    int filtnum = -1;
    switch (filtname) {
      case 'g':	filtnum = 0; break;
      case 'r':	filtnum = 1; break;
      case 'i':	filtnum = 2; break;
      case 'z':	filtnum = 3; break;
      case 'y':	filtnum = 4; break;
      default:
	fprintf (stderr, "invalid filter in ext: %s\n", extname);
	abort();
    }

    int dir = -1;
    if (w2[2] == 'x') {
      dir = 0;
    }
    if (w2[2] == 'y') {
      dir = 1;
    }
    myAssert (dir != -1, "error in extname %s (dir)\n", extname);

    // load the spline elements:
    double *xk = gfits_get_bintable_column_data (&theader, &ftable, "X_KNOT", type, &Nrow, &Ncol);
    myAssert (!strcmp(type, "double"), "error reading X_KNOT");
    
    double *yk = gfits_get_bintable_column_data (&theader, &ftable, "Y_KNOT", type, &Nrow, &Ncol);
    myAssert (!strcmp(type, "double"), "error reading Y_KNOT");
    
    double *y2 = gfits_get_bintable_column_data (&theader, &ftable, "DY2_DX", type, &Nrow, &Ncol);
    myAssert (!strcmp(type, "double"), "error reading DY2_DX");
    
    ALLOCATE (splineset[dir][filtnum], Spline, 1);
    splineset[dir][filtnum]->xk = xk;
    splineset[dir][filtnum]->yk = yk;
    splineset[dir][filtnum]->y2 = y2;
    splineset[dir][filtnum]->Nknots = Nrow;
  }
  return (TRUE);
}

int get_dcr_correction (int filter, double *dX, double *dY, float Color) {

  Spline *spline = NULL;

  *dX = 0.0;
  *dY = 0.0;

  // XXX need to include some limits on Minst?

  if (0) {
    spline = splineset[0][filter];
    if (spline) {
      *dX = spline_apply_dbl (spline->xk, spline->yk, spline->y2, spline->Nknots, Color);
    }
  }

  spline = splineset[1][filter];
  if (spline) {
    *dY = spline_apply_dbl (spline->xk, spline->yk, spline->y2, spline->Nknots, Color);
    *dY = MIN(MaxDCR[filter], *dY);
    *dY = MAX(MinDCR[filter], *dY);
  }
  return (TRUE);
}
