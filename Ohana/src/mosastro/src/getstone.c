# include "mosastro.h"

StarData *getstone (CatStats *input, int *nstars) {

  FILE *f;
  int i, Nregion, NREGION, Nstars, NSTARS;
  double r, d, R, D, M, Tr, Td, dRdT, dDdT;
  char **regname, filename[1024], line[1024];
  StarData *stars;
  CatStats *regions;

  sprintf (filename, "%s/Regions.dat", StoneRegions);
  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't open stone %s\n", filename);
    exit (1);
  }

  Nregion = 0;
  NREGION = 20;
  ALLOCATE (regions, CatStats, NREGION);
  ALLOCATE (regname, char *, NREGION);

  /* strip off first commented line */
  scan_line (f, line);
  while (scan_line (f, line) != EOF) {
    sscanf (line, "%s %lf %lf %lf %lf", filename, 
	    &regions[Nregion].RA[0], &regions[Nregion].RA[1], 
	    &regions[Nregion].DEC[0], &regions[Nregion].DEC[1]);
    snprintf_nowarn (line, 1024, "%s/%s", StoneRegions, filename);
    regname[Nregion] = strcreate (line);
    Nregion ++;
    if (Nregion == NREGION) {
      NREGION += 50;
      REALLOCATE (regions, CatStats, NREGION);
      REALLOCATE (regname, char *, NREGION);
    }	  
  }
  fclose (f);

  Nstars = 0;
  NSTARS = 1000;
  ALLOCATE (stars, StarData, NSTARS);

  /* find the region(s) that overlap the input region */
  for (i = 0; i < Nregion; i++) {
    if (input[0].RA[0] > regions[i].RA[1]) continue;
    if (input[0].RA[1] < regions[i].RA[0]) continue;
    if (input[0].DEC[0] > regions[i].DEC[1]) continue;
    if (input[0].DEC[1] < regions[i].DEC[0]) continue;
    /* this will fail on 0,360 boundary */
    fprintf (stderr, "loading data from %s\n", regname[i]);

    f = fopen (regname[i], "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "ERROR: can't open stone %s\n", regname[i]);
      exit (1);
    }

    while (scan_line (f, line) != EOF) {
    
      dparse (&R, 3, line);
      dparse (&D, 4, line);
      dparse (&M, 5, line);
      
      dparse (&Tr, 12, line);
      dparse (&Td, 13, line);
      dparse (&dRdT, 14, line);
      dparse (&dDdT, 15, line);
      
      r = R + dRdT*(Year - Tr)/(100.0*3600.0);
      d = D + dDdT*(Year - Td)/(100.0*3600.0);
      
      stars[Nstars].M = M;
      stars[Nstars].R = r*15.0;
      stars[Nstars].D = d;
      Nstars ++;
      if (Nstars == NSTARS) {
	NSTARS += 5000;
	REALLOCATE (stars, StarData, NSTARS);
      }	  
    }
    fclose (f);
  }

  *nstars = Nstars;
  return (stars);
}

