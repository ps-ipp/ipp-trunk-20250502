# include "gastro.h"
# define BYTES_STAR 23
# define BLOCK 1000

SStars *rd_gsc (char *filename, int *Nstars);

SStars *getgsc (CatStats *catstats, int *NSTARS) {
  
  int i, j, k, Ns, Ngsc, Nstars; 
  SStars *gsc;
  SStars *stars;
  SkyList *skylist;
  SkyTable *sky;
  SkyRegion patch;

  patch.Rmin = catstats[0].RA[0];
  patch.Rmax = catstats[0].RA[1];
  patch.Dmin = catstats[0].DEC[0];
  patch.Dmax = catstats[0].DEC[1];

  /* load regions from GSC table, restrict to patch */
  sky = SkyTableFromGSC (GSCFILE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (sky, GSCDIR, "cpt");
  skylist = SkyListByPatch (sky, -1, &patch);
  
  Nstars = 0;
  ALLOCATE (stars, SStars, 1);

  for (i = 0; i < skylist[0].Nregions; i++) {
    gsc = rd_gsc (skylist[0].filename[i], &Ngsc);

    Ns = Nstars;
    Nstars += Ngsc;

    REALLOCATE (stars, SStars, MAX (1, Nstars));
    for (k = Ns, j = 0; j < Ngsc; k++, j++) {
      stars[k].X   = gsc[j].X;
      stars[k].Y   = gsc[j].Y;
      stars[k].mag = gsc[j].mag;
    }      
    free (gsc);
  }
  SkyTableFree (sky);

  *NSTARS = Nstars;
  return (stars);
}  

SStars *rd_gsc (char *filename, int *Nstars) {
  
  SStars *stars;
  int i, NSTAR, nstar, Nbytes, nbytes;
  char *buffer;
  FILE *f;

  f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't find catalog file %s\n", filename);
    exit (1);
  }
  
  nstar = 0;
  NSTAR = 1000;
  ALLOCATE (stars, SStars, NSTAR);

  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR));
  Nbytes = BLOCK*BYTES_STAR;

  while ((nbytes = fread (buffer, 1, Nbytes, f)) > 0) {
    for (i = 0; i < nbytes / BYTES_STAR; i++) {
      dparse (&stars[nstar].X, 1, &buffer[i*BYTES_STAR]);
      dparse (&stars[nstar].Y, 2, &buffer[i*BYTES_STAR]);
      dparse (&stars[nstar].mag, 3, &buffer[i*BYTES_STAR]);
      nstar++;
      if (nstar == NSTAR) {
	NSTAR += 1000;
	REALLOCATE (stars, SStars, NSTAR);
      }
    }
  }

  free (buffer);

  *Nstars = nstar;
  return (stars);
}
