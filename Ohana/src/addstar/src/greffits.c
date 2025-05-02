# include "addstar.h"

# define GET_COLUMN(VER,OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&header_##VER, &ftable_##VER, NAME, type, &NrowNew, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type"); \
  if (firstCol) { Nrow = NrowNew; firstCol = FALSE; } \
  else { myAssert (Nrow == NrowNew, "table length mismatch"); }

/* read FITS file with ref star data (ra, dec, ??) */
Catalog *greffits (char *filename, int photcode) {

  int Ncol;

  char type[16];
  off_t NrowNew;
  off_t Nrow = 0;

  // read in the full FITS files ('cause I don't have a partial read option)
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read stellar parameter file: %s", filename);

  // load in PHU for astrometry
  Header PHU;
  if (!gfits_fread_header (f, &PHU)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    fclose (f);
    return NULL;
  }

  // EXTNAME = votable
  Header header_tbl;
  FTable ftable_tbl;
  if (!gfits_find_Xheader (f, &header_tbl, "votable")) {
    if (VERBOSE) fprintf (stderr, "can't read table header\n");
    gfits_free_header (&header_tbl);
    gfits_free_header (&PHU);
    fclose (f);
    return NULL;
  }
  ftable_tbl.header = &header_tbl;

  if (header_tbl.Naxis[1] == 0) {
    if (VERBOSE) fprintf (stderr, "no data in file %s, skipping\n", filename);
    gfits_free_header (&header_tbl);
    gfits_free_header (&PHU);
    fclose (f);
    return NULL;
  }

  if (!gfits_fread_ftable_data (f, &ftable_tbl, FALSE)) {
    if (VERBOSE) fprintf (stderr, "can't read FITS data\n");
    gfits_free_header (&header_tbl);
    gfits_free_header (&PHU);
    fclose (f);
    return (NULL);
  }

  int firstCol = TRUE;
  GET_COLUMN (tbl, ra,         "ra",              double);
  GET_COLUMN (tbl, dec,        "dec",             double);
  GET_COLUMN (tbl, Gmag,       "phot_g_mean_mag", float);
  GET_COLUMN (tbl, GmagSN,     "phot_g_mean_flux_over_error", float);

  int Nstars = Nrow;

  // free the memory associated with the FITS files
  gfits_free_header (&header_tbl);
  gfits_free_table  (&ftable_tbl);

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, Nstars);
  ALLOCATE (catalog->measure, Measure, Nstars);

  for (int i = 0; i < Nstars; i++) {

    dvo_average_init (&catalog->average[i]);
    dvo_measure_init (&catalog->measure[i]);

    catalog->average[i].R = ra[i];
    catalog->average[i].D = dec[i];
    catalog->measure[i].M = Gmag[i];
    catalog->measure[i].McalPSF = 0.0;
    catalog->measure[i].dM = 1.08 / GmagSN[i];

    catalog->measure[i].R = catalog->average[i].R;
    catalog->measure[i].D = catalog->average[i].D;

    catalog->measure[i].photcode = photcode;

    catalog->average[i].Nmeasure = 1;
    catalog->average[i].measureOffset = i;
  }
  catalog->Naverage = Nstars;
  catalog->Nmeasure = Nstars;

  free (ra);
  free (dec);
  free (Gmag);
  free (GmagSN);

  return (catalog);
}
