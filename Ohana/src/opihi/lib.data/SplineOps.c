# include "data.h"

/* this file contains functions to manage the collection of Splines.
   A spline in opihi is a native data structure which defines a 1D spline.  
 */

Spline **splines;   /* book to store the list of all splines */
int     Nsplines;   /* number of currently defined splines */
int     NSPLINES;   /* number of currently allocated splines */

void InitSplines () {
  Nsplines = 0;
  NSPLINES = 16;
  ALLOCATE (splines, Spline *, NSPLINES); 
}

void FreeSplines () {

  int i;

  for (i = 0; i < Nsplines; i++) {
    FreeSpline (splines[i]);
  }
  free (splines);
}

void InitSpline (Spline *spline, char *name, int Nknots) {

  if (!spline[0].name) {
    spline[0].name = strcreate (name);
  }
  spline[0].Nknots = Nknots;
  
  if (spline[0].xk) {
    REALLOCATE (spline[0].xk, opihi_flt, spline[0].Nknots);
    REALLOCATE (spline[0].yk, opihi_flt, spline[0].Nknots);
    REALLOCATE (spline[0].y2, opihi_flt, spline[0].Nknots);
  } else {
    ALLOCATE (spline[0].xk, opihi_flt, spline[0].Nknots);
    ALLOCATE (spline[0].yk, opihi_flt, spline[0].Nknots);
    ALLOCATE (spline[0].y2, opihi_flt, spline[0].Nknots);
  }
  if (spline[0].Nknots) {
    memset (spline[0].xk, 0, spline[0].Nknots * sizeof(opihi_flt));
    memset (spline[0].yk, 0, spline[0].Nknots * sizeof(opihi_flt));
    memset (spline[0].y2, 0, spline[0].Nknots * sizeof(opihi_flt));
  }
}

void FreeSpline (Spline *spline) {

    free (spline[0].name);
    free (spline[0].xk);
    free (spline[0].yk);
    free (spline[0].y2);
    free (spline);
}

/* return the given spline */
Spline *GetSpline (int where) {

  if (where < 0) where += Nsplines;
  if (where < 0) return NULL;
  if (where >= Nsplines) return NULL;
  return (splines[where]);
}

/* return the given spline */
Spline *FindSpline (char *name) {

  int i;

  for (i = 0; i < Nsplines; i++) {
    if (!strcmp (splines[i][0].name, name)) {
      return (splines[i]);
    }
  }
  return (NULL);
}

/* make a new named spline */
Spline *CreateSpline (char *name, int Nknots) {

  int N;
  Spline *spline;

  spline = FindSpline (name);
  if (spline != NULL) {
    InitSpline (spline, name, Nknots);
    return (spline);
  }

  N = Nsplines;
  Nsplines ++;
  CHECK_REALLOCATE (splines, Spline *, NSPLINES, Nsplines, 16);
  ALLOCATE (spline, Spline, 1);
  spline->name = NULL;
  spline->xk = NULL;
  spline->yk = NULL;
  spline->y2 = NULL;
  InitSpline (spline, name, Nknots);
  splines[N] = spline;
  return (spline);
}

/* delete a spline */
int DeleteSpline (Spline *spline) {

  int i, N, NSPLINES_2;

  /* find spline in spline list */
  N = -1;
  for (i = 0; i < Nsplines; i++) {
    if (splines[i] == spline) {
      N = i;
      break;
    }
  }
  if (N == -1) return (FALSE);

  for (i = N; i < Nsplines - 1; i++) {
    splines[i] = splines[i + 1];
  }
  Nsplines --;
  NSPLINES_2 = MAX (16, NSPLINES / 2);
  if (Nsplines < NSPLINES_2) {
    NSPLINES = NSPLINES_2;
    REALLOCATE (splines, Spline *, NSPLINES);
  }

  FreeSpline (spline);
  return (TRUE);
}

/* list known books */
void ListSplines () {

  int i;

  for (i = 0; i < Nsplines; i++) {
    gprint (GP_ERR, "%-15s %3d\n", splines[i][0].name, splines[i][0].Nknots);
  }
  return;
}

// write the spline data to a FITS file.
int SaveSpline (char *filename, char *name, int append) {
  
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;

  Spline *myspline = FindSpline (name);
  if (!myspline) {
    gprint (GP_ERR, "can't find spline for write : %s\n", name);
    return (FALSE);
  }

  FILE *f = NULL;

  /* open file for output */
  if (append) {
    f = fopen (filename, "a");
  } else {
    f = fopen (filename, "w");
  }
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for write : %s\n", filename);
    return (FALSE);
  }

  gfits_create_table_header (&theader, "BINTABLE", name);

  gfits_define_bintable_column (&theader, "D", "X_KNOT", NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "Y_KNOT", NULL, NULL, 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DY2_DX", NULL, NULL, 1.0, 0.0);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // NOTE: if we want to compress the output table, use native byte order here (last element)
  gfits_set_bintable_column_reformat (&theader, &ftable, "X_KNOT", "double", myspline->xk, myspline->Nknots, 0, FALSE);
  gfits_set_bintable_column_reformat (&theader, &ftable, "Y_KNOT", "double", myspline->yk, myspline->Nknots, 0, FALSE);
  gfits_set_bintable_column_reformat (&theader, &ftable, "DY2_DX", "double", myspline->y2, myspline->Nknots, 0, FALSE);

  if (!append) {
    gfits_init_header (&header);
    header.extend = TRUE;

    gfits_create_header (&header);
    gfits_create_matrix (&header, &matrix);

    gfits_fwrite_header  (f, &header);
    gfits_fwrite_matrix  (f, &matrix);

    gfits_free_header (&header);
    gfits_free_matrix (&matrix);
  }
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table  (f, &ftable);

  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  fclose (f);
  fflush (f);
  return (TRUE);
}

// read the spline data from a FITS file.
// these functions (LoadSpline, SaveSpline) should probably take spline pointers not names
int LoadSpline (char *filename, char *name) {
  
  int i, Ncol;
  off_t Nrow;
  char type[16];

  Header theader;
  FTable ftable;

  FILE *f = NULL;

  /* open file for input */
  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "can't open file for read : %s\n", filename);
    return FALSE;
  }

  ftable.header = &theader;

  // read the full table data into a buffer
  if (!gfits_fread_ftable (f, &ftable, name)) {
    fclose (f);
    gprint (GP_ERR, "can't read FITS file : %s\n", filename);
    return FALSE;
  }

  // XXX: need to handle case of spline data not existing...

  // need to create and assign to flat-field correction
  double *xk = gfits_get_bintable_column_data (&theader, &ftable, "X_KNOT", type, &Nrow, &Ncol);
  assert (!strcmp(type, "double"));
  
  // need to create and assign to flat-field correction
  double *yk = gfits_get_bintable_column_data (&theader, &ftable, "Y_KNOT", type, &Nrow, &Ncol);
  assert (!strcmp(type, "double"));

  // need to create and assign to flat-field correction
  double *y2 = gfits_get_bintable_column_data (&theader, &ftable, "DY2_DX", type, &Nrow, &Ncol);
  assert (!strcmp(type, "double"));

  Spline *myspline = CreateSpline (name, Nrow);
  for (i = 0; i < Nrow; i++) {
    myspline->xk[i] = xk[i];
    myspline->yk[i] = yk[i];
    myspline->y2[i] = y2[i];
  }
  free (xk);
  free (yk);
  free (y2);

  fclose (f);
  return (TRUE);
}
