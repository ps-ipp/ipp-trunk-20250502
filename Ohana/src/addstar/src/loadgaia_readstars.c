# include "addstar.h"
# include "gaia.h"

# define GET_COLUMN(OUT,NAME,TYPE) \
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

Gaia_Stars *loadgaia_readstars (char *filename, Gaia_Stars *stars, int *nstars, AddstarClientOptions *options) {

  // read in the full FITS files
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read gaia file: %s", filename);

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
    return stars;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    gfits_free_header (&header);
    fclose (f);
    return stars;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) {
    fclose (f);
    return stars;
  }
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) {
    fclose (f);
    return stars;
  }

  char type[16];

  GET_COLUMN (RA,      "RA",              	 double);
  GET_COLUMN (DEC,     "DEC",             	 double);
  GET_COLUMN (dRA,     "RA_ERROR",        	 float);
  GET_COLUMN (dDEC,    "DEC_ERROR",       	 float);
  GET_COLUMN (gMag,    "PHOT_G_MEAN_MAG", 	 float);
  GET_COLUMN (dgFlux,  "PHOT_G_MEAN_FLUX_ERROR", float);
  GET_COLUMN (Nobs,    "PHOT_G_N_OBS",           short);

  int NstarsIn = Nrow;

  // free the memory associated with the FITS files
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  fclose (f);

  double Rmin = +360.0;
  double Rmax = -360.0;
  double Dmin = +360.0;
  double Dmax = -360.0;

  // start off where we finished on a previous read
  int Nstars = *nstars;
  int NSTARS = Nstars + 0.1*NstarsIn;

  if (!stars) {
    ALLOCATE (stars, Gaia_Stars, NSTARS);
  } else {
    REALLOCATE (stars, Gaia_Stars, NSTARS);
  }

  for (i = 0; i < NstarsIn; i++) {

    Rmin = MIN (Rmin, RA[i]);
    Rmax = MAX (Rmax, RA[i]);
    Dmin = MIN (Dmin, DEC[i]);
    Dmax = MAX (Dmax, DEC[i]);

    float flux = pow(10.0, -0.4*(gMag[i] - 25.524770));

    stars[Nstars].R = RA[i];
    stars[Nstars].D = DEC[i];
    stars[Nstars].flag  = FALSE;
    stars[Nstars].found = FALSE;

    dvo_measure_init (&stars[Nstars].measure);

    stars[Nstars].measure.R = RA[i];
    stars[Nstars].measure.D = DEC[i];
    stars[Nstars].measure.dXccd = (int)(0x7fff*MAX(0.000,MIN(0.999, ((log10(dRA[i]) + 3.0)/6.0)))); 
    stars[Nstars].measure.dYccd = (int)(0x7fff*MAX(0.000,MIN(0.999, ((log10(dDEC[i]) + 3.0)/6.0)))); 
    // dXccd,dYccd range from 0 (10^-3) mas to 0x7fff (10^3 mas)
    stars[Nstars].measure.M = gMag[i];
    stars[Nstars].measure.dM = dgFlux[i] / flux;

    stars[Nstars].measure.FluxPSF = flux;
    stars[Nstars].measure.dFluxPSF = dgFlux[i];

    stars[Nstars].measure.photcode = options->photcode;
    stars[Nstars].measure.t = options->timeref;

    Nstars ++;

    CHECK_REALLOCATE (stars, Gaia_Stars, NSTARS, Nstars, 10000);
  }
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);

  free (RA);
  free (DEC);
  free (dRA);
  free (dDEC);
  free (gMag);
  free (dgFlux);
  free (Nobs);

  *nstars = Nstars;
  return (stars);
}

int loadgaia_sortStars (Gaia_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ Gaia_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].R < stars[B].R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

