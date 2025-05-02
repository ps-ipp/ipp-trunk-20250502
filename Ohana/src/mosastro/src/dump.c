# include "mosastro.h"

int dump_stars (FILE *f, StarData *stars, int Nstars) {

  int i;

  for (i = 0; i < Nstars; i++) {
    fprintf (f, "%4d  %12.8f %12.8f  %8.2f %8.2f  %8.2f %8.2f  %7.2f %7.2f  %7.2f %7.2f  %d\n", 
	     i, 
	     stars[i].R, stars[i].D,
	     stars[i].P, stars[i].Q,
	     stars[i].L, stars[i].M,
	     stars[i].X, stars[i].Y,
	     stars[i].Mag, stars[i].dMag, stars[i].mask);
  }
  return (1);
}

int dump_grads (Gradients *grad, char *filename) {

  int i;
  FILE *f;

  fprintf (stderr, "printing to file %s\n", filename);

  f = fopen (filename, "w");

  for (i = 0; i < grad[0].Npts; i++) {
    fprintf (f, "%4d  %10.6f %10.6f  %10.6f %10.6f   %10.6f %10.6f\n", 
	     i, 
	     grad[0].dPdL[i], grad[0].dPdM[i], 
	     grad[0].dQdL[i], grad[0].dQdM[i], 
	     grad[0].Lo[i], grad[0].Mo[i]);
  }
  fclose (f);
  return (1);
}

int dump_match () {

  int i;
  FILE *f, *g;
  f = fopen ("raw.dat", "w");
  g = fopen ("ref.dat", "w");
  for (i = 0; i < Nchip; i++) {
    dump_stars (f, chip[i].raw, chip[i].Nmatch);
    dump_stars (g, chip[i].ref, chip[i].Nmatch);
  }
  fclose (f);
  fclose (g);
  exit (1);
}

int dump_rawstars () {

  int i;
  FILE *f;
  f = fopen ("stars.dat", "w");
  for (i = 0; i < Nchip; i++) {
    dump_stars (f, chip[i].stars, chip[i].Nstars);
  }
  fclose (f);
  exit (1);
}

int dump_refcat (StarData *refcat, int Nrefcat) {

  FILE *f;
  f = fopen ("refcat.dat", "w");
  dump_stars (f, refcat, Nrefcat);
  fclose (f);
  exit (1);
}
