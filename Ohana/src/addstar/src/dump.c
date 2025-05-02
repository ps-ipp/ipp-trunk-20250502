# include "addstar.h"

int dump_rawstars (Stars *stars, unsigned int Nstars) {

  unsigned int i;
  FILE *f;

  f = fopen ("stars.dat", "w");

  for (i = 0; i < Nstars; i++) {
    fprintf (f, "%4d  %10.6f %10.6f  %8.2f %8.2f  %7.2f %7.2f\n", 
	     i, 
	     stars[i].average.R, stars[i].average.D,
	     stars[i].measure.Xccd, stars[i].measure.Yccd,
	     stars[i].measure.M, stars[i].measure.dM);
  }

  fclose (f);
  exit (1);
}

