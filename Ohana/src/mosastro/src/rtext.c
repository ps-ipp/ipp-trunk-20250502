# include "mosastro.h"
# define BYTES_STAR 66

int rtext (Chip *mychip) {

  int i, Nbytes, nbytes;
  FILE *f;

  f = fopen (mychip[0].file, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't read data from %s\n", mychip[0].file);
    return (FALSE);
  }
  fseeko (f, mychip[0].header.datasize, SEEK_SET); 

  /* find expected number of stars */
  gfits_scan (&mychip[0].header, "NSTARS", "%d", 1, &mychip[0].Nstars);
  if (mychip[0].Nstars == 0) {
    fprintf (stderr, "ERROR: can't get NSTARS from header for %s\n", mychip[0].file);
    return (FALSE);
  }
  ALLOCATE (mychip[0].stars, StarData, mychip[0].Nstars);
    
  Nbytes = BYTES_STAR * mychip[0].Nstars;
  ALLOCATE (mychip[0].buffer, char, Nbytes + 1);
  nbytes = fread (mychip[0].buffer, 1, Nbytes, f);
  if (nbytes != Nbytes) { exit (1); }
  mychip[0].Nbuffer = Nbytes;
  mychip[0].FITS = FALSE;

  for (i = 0; i < mychip[0].Nstars; i++) {
    bzero (&mychip[0].stars[i], sizeof(StarData));
    dparse (&mychip[0].stars[i].X,    1, &mychip[0].buffer[i*BYTES_STAR]);
    dparse (&mychip[0].stars[i].Y,    2, &mychip[0].buffer[i*BYTES_STAR]);
    dparse (&mychip[0].stars[i].Mag,  3, &mychip[0].buffer[i*BYTES_STAR]);
    dparse (&mychip[0].stars[i].dMag, 4, &mychip[0].buffer[i*BYTES_STAR]);
    mychip[0].stars[i].dMag *= 0.001;  /* millimag errors stored in file */
  }
  return (TRUE);
}
