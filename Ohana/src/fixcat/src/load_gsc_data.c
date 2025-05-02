# include "markstar.h"
# define NBYTES 160000
# define BYTES_STAR 23
# define BLOCK 1000
# define DNSTARS 1000

load_gsc_data (catalog, catstats)
Catalog catalog[];
CatStats catstats[];
{

  /* load data from the GSC files */
  char filename[128];
  char *tbuffer;
  int nstar, NSTARS;
  int i, j, Nbytes, nbytes, Nregions;
  double R, D, M, MagLimit;
  FILE *f;
  GSCRegion *region, *gregions1(), *gregions2();
  
  region = gregions1 (catstats, &Nregions);

  MagLimit = MAX (MAX (BRIGHT_HALO_MAG, BRIGHT_XTRAIL_MAG), BRIGHT_YTRAIL_MAG); 
  nstar = 0;
  NSTARS = DNSTARS;
  ALLOCATE (tbuffer, char, (BLOCK*BYTES_STAR));
  ALLOCATE (catalog[0].average, Average, NSTARS);
  Nbytes = BLOCK*BYTES_STAR;
  
  for (j = 0; j < Nregions; j++) {
    f = fopen (region[j].filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "GSC file for region %s missing\n", region[j].filename);
      exit (0);
    }
    
    while ((nbytes = fread (tbuffer, 1, Nbytes, f)) > 0) {
      for (i = 0; i < nbytes / BYTES_STAR; i++) {
	dparse (&M, 3, &tbuffer[i*BYTES_STAR]);
	if (M > MagLimit) continue;
	dparse (&R, 1, &tbuffer[i*BYTES_STAR]);
	dparse (&D, 2, &tbuffer[i*BYTES_STAR]);
	catalog[0].average[nstar].R = R;
	catalog[0].average[nstar].D = D;
	catalog[0].average[nstar].M = M * 1000.0;
	nstar++;
	if (nstar == NSTARS - 1) {
	  NSTARS += DNSTARS;
	  REALLOCATE (catalog[0].average, Average, NSTARS);
	}
      }
    }
    fclose (f);
  }

  free (tbuffer);
  REALLOCATE (catalog[0].average, Average, nstar);
  catalog[0].Naverage = nstar;
  
}

