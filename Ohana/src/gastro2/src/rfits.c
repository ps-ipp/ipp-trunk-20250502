# include "gastro2.h"

StarData *rfits (FILE *f, int *nstars) {

  int N;
  off_t i, Nstars;
  Header theader;
  FTable table;
  SMPData  *stars;
  StarData *stardata;

  /* init & load in table data */
  table.header   = &theader;
  if (!gfits_fread_ftable (f, &table, "SMPFILE")) goto escape;

  stars = gfits_table_get_SMPData (&table, &Nstars, NULL, NULL);
  if (!stars) {
    fprintf (stderr, "ERROR: failed to read stars\n");
    exit (2);
  }

  ALLOCATE (stardata, StarData, Nstars);
  for (i = N = 0; i < Nstars; i++) {
    /* hardwired dophot exclusions should eventually be encapsulated elsewhere */
    if (stars[i].dophot == 4) continue;
    if (stars[i].dophot == 5) continue;
    if (stars[i].dophot == 6) continue;
    if (stars[i].dophot == 9) continue;
    if ((MAX_ERROR > 0) && (stars[i].dM > MAX_ERROR)) continue;
    stardata[N].X    = stars[i].X;
    stardata[N].Y    = stars[i].Y;
    stardata[N].M    = stars[i].M;
    stardata[N].dM   = 1000*stars[i].dM;
    stardata[N].type = stars[i].dophot;
    stardata[N].R    = 0;
    stardata[N].D    = 0;
    N++;
  }    
  if (N < 5) { 
    fprintf (stderr, "ERROR: too few stars for reliable solution, only %d\n", N);
    exit (1);
  }

  REALLOCATE (stardata, StarData, MAX(N,1));
  *nstars = N;
  return (stardata);

escape:
  fprintf (stderr, "error reading file\n");
  exit (1);
}
