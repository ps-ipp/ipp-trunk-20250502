# include "mosastro.h"

int rfits (Chip *mychip) {

  int i, Nx;
  FILE *f;
  FTable table;
  SMPData *stars;

  /* open file for stars */
  f = fopen (mychip[0].file, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't open file to load stars\n");
    exit (1);
  }
  fseeko (f, mychip[0].header.datasize, SEEK_SET); 

  /* init & load in table data */
  table.header   = &mychip[0].theader;
  if (!gfits_fread_matrix (f, &mychip[0].matrix, &mychip[0].header)) {
    fprintf (stderr, "error reading file\n");
    fclose (f);
    return (FALSE);
  }
  if (!gfits_fread_ftable (f, &table, "SMPFILE")) {
    fprintf (stderr, "error reading file\n");
    gfits_free_matrix (&mychip[0].matrix);
    fclose (f);
    return (FALSE);
  }

  off_t Nstars;
  stars = gfits_table_get_SMPData (&table, &Nstars, NULL, NULL);
  if (!stars) {
    fprintf (stderr, "ERROR: failed to read stars\n");
    exit (2);
  }

  mychip[0].Nstars = Nstars;
  gfits_scan (table.header, "NAXIS1", "%d", 1, &Nx);

  /* save raw data for output */
  mychip[0].FITS = TRUE;
  mychip[0].buffer = (char *) stars;
  mychip[0].Nbuffer = Nx*mychip[0].Nstars;

  if (mychip[0].Nstars < 5) { 
    fprintf (stderr, "Too few stars for reliable solution, only %d\n", mychip[0].Nstars);
    gfits_free_matrix (&mychip[0].matrix);
    gfits_free_table (&table);
    fclose (f);
    return (FALSE);
  }

  /* note the different structures for mychip[0].stars and stars */
  ALLOCATE (mychip[0].stars, StarData, mychip[0].Nstars);
  for (i = 0; i < mychip[0].Nstars; i++) {
    mychip[0].stars[i].X    = stars[i].X;
    mychip[0].stars[i].Y    = stars[i].Y;
    mychip[0].stars[i].Mag  = stars[i].M;
    mychip[0].stars[i].dMag = stars[i].dM;
  }    
  return (TRUE);
}

