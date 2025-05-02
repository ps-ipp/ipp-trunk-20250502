# include "addstar.h"

/* read ASCII file with ref star data */
Catalog *grefstars (char *file, int photcode) {

  FILE *f;
  int NSTARS;
  char line[256];

  /* open file */
  f = fopen (file, "r");
  if (f == NULL) Shutdown ("can't read data from %s", file);

  NSTARS = 10000000;

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, NSTARS);
  ALLOCATE (catalog->measure, Measure, NSTARS);

  /* read in stars line-by-line */
  int N = 0;
  while (scan_line (f, line) != EOF) {
    stripwhite (line);
    if (line[0] == 0) continue;
    if (line[0] == '#') continue;

    dvo_average_init (&catalog->average[N]);
    dvo_measure_init (&catalog->measure[N]);

    int status = sscanf (line, "%lf %lf %f %f %x", 
			 &catalog->average[N].R, &catalog->average[N].D,  
			 &catalog->measure[N].M, &catalog->measure[N].dM, 
			 &catalog->measure[N].photFlags);

    if ((status != 4) && (status != 5)) {
      fprintf (stderr, "error reading line: %s\n", line);
      continue;
    }
    catalog->average[N].R = ohana_normalize_angle (catalog->average[N].R);

    catalog->measure[N].McalPSF  = 0.0;
    catalog->measure[N].McalAPER = 0.0;
    catalog->measure[N].dMcal    = 0.0;
       
    catalog->measure[N].R = catalog->average[N].R;
    catalog->measure[N].D = catalog->average[N].D;

    catalog->measure[N].photcode = photcode;

    catalog->average[N].Nmeasure = 1;
    catalog->average[N].measureOffset = N;

    N++;
    CHECK_REALLOCATE (catalog->average, Average, NSTARS, N, 1000);
    CHECK_REALLOCATE (catalog->measure, Measure, NSTARS, N, 1000);
  }
  catalog->Naverage = N;
  catalog->Nmeasure = N;
  return (catalog);
}
