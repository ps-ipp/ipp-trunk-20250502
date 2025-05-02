# include "addstar.h"
# include "loadstarpar.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

StarPar_Stars *loadstarpar_readstars (char *filename, int *nstars) {

  // read in the full FITS files ('cause I don't have a partial read option)
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read stellar parameter file: %s", filename);

  int i, Ncol;
  off_t Nrow;

  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  
  // load in PHU segment (ignore)
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return NULL;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return NULL;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    return NULL;
  }
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return (NULL);
  }

  char type[16];

  GET_COLUMN (glon,    "l",    float);
  GET_COLUMN (glat,    "b",    float);
  GET_COLUMN (conv,    "conv", gfbyte);
  GET_COLUMN (lnZ,     "lnZ",  float);
  GET_COLUMN (DistMag, "DM",   float);
  GET_COLUMN (Ebv,     "EBV",  float);
  GET_COLUMN (M_r,     "Mr",   float);
  GET_COLUMN (FeH,     "FeH",  float);

  // free the memory associated with the FITS files
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  // the next FITS extension contains the error information
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return NULL;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return NULL;
  }
  fclose (f);

  double *errImage = (double *) matrix.buffer;

  myAssert (Nrow == matrix.Naxis[2], "size mismatch?");

  int NstarsIn = matrix.Naxis[2];

  double Rmin = +360.0;
  double Rmax = -360.0;
  double Dmin = +360.0;
  double Dmax = -360.0;

  int Nstars = 0;
  int NSTARS = 0.1*NstarsIn;

  StarPar_Stars *stars = NULL;
  ALLOCATE (stars, StarPar_Stars, NSTARS);

  // libdvo uses Liu et al 2011 (A&A 526, A16) for default galactic coords,
  // but Greg Green / LSD use the older Reid et al 2004 (ApJ 616, 872) definition
  CoordTransform *transform = InitTransform (COORD_GALACTIC_REID_2004, COORD_CELESTIAL);

  for (i = 0; i < NstarsIn; i++) {

    // skip stars based on conv and lnZ
    if (!conv[i]) continue;
    if (lnZ[i] < -15.0) continue;

    double R, D;
    ApplyTransform (&R, &D, glon[i], glat[i], transform);

    Rmin = MIN (Rmin, R);
    Rmax = MAX (Rmax, R);
    Dmin = MIN (Dmin, D);
    Dmax = MAX (Dmax, D);

    float dEbv     = 0.5*(errImage[i*20 + 5*0 + 3] - errImage[i*20 + 5*0 + 1]);
    float dDistMag = 0.5*(errImage[i*20 + 5*1 + 3] - errImage[i*20 + 5*1 + 1]);
    float dM_r     = 0.5*(errImage[i*20 + 5*2 + 3] - errImage[i*20 + 5*2 + 1]);
    float dFeH     = 0.5*(errImage[i*20 + 5*3 + 3] - errImage[i*20 + 5*3 + 1]);

    stars[Nstars].R = R;
    stars[Nstars].D = D;
    stars[Nstars].flag  = FALSE;
    stars[Nstars].found = FALSE;

    // NOTE that we have both stars.R,D and stars.starpar.R,D.  stars.R,D are used 
    // here (in parallel with addstar) to locate the objects.  BUT, in the database,
    // starpar.R,D is the authoratative position for the object (either that coming 
    // from Greg / Eddie's table or that coming from the simulation)
    stars[Nstars].starpar.R      = R;
    stars[Nstars].starpar.D      = D;
    stars[Nstars].starpar.galLon = glon[i];
    stars[Nstars].starpar.galLat = glat[i];

    stars[Nstars].starpar.Ebv      = Ebv[i]    ;
    stars[Nstars].starpar.dEbv     = dEbv      ;
    stars[Nstars].starpar.DistMag  = DistMag[i];
    stars[Nstars].starpar.dDistMag = dDistMag  ;
    stars[Nstars].starpar.M_r      = M_r[i]    ;
    stars[Nstars].starpar.dM_r     = dM_r      ;
    stars[Nstars].starpar.FeH      = FeH[i]    ;
    stars[Nstars].starpar.dFeH     = dFeH      ;

    Nstars ++;

    CHECK_REALLOCATE (stars, StarPar_Stars, NSTARS, Nstars, 10000);
  }
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  free (glon);
  free (glat);
  free (conv);
  free (lnZ);
  free (DistMag);
  free (Ebv);
  free (M_r);
  free (FeH);

  free (transform);

  *nstars = Nstars;
  return (stars);
}

int loadstarpar_sortStars (StarPar_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ StarPar_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].R < stars[B].R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

