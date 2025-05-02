# include "gastro2.h"

StarData *rd_gsc (char *filename, int *Nstars);

int getgsc (CatStats *catstats, RefCatalog *Ref) {
  
  int i, j, k, Ns, Ngsc; 
  StarData *gsc;
  SkyList *skylist;
  SkyTable *sky;
  SkyRegion patch;

  Ref[0].N = 0;
  Ref[0].Area = 0;
  ALLOCATE (Ref[0].stars, StarData, 1);

  patch.Rmin = catstats[0].RA[0];
  patch.Rmax = catstats[0].RA[1];
  patch.Dmin = catstats[0].DEC[0];
  patch.Dmax = catstats[0].DEC[1];

  /* load regions from GSC table, restrict to patch */
  sky = SkyTableFromGSC (GSCFILE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (sky, GSCDIR, "cpt");
  skylist = SkyListByPatch (sky, -1, &patch);
  
  for (i = 0; i < skylist[0].Nregions; i++) {
    gsc = rd_gsc (skylist[0].filename[i], &Ngsc);

    Ns = Ref[0].N;
    Ref[0].N += Ngsc;
    Ref[0].Area += area_of_skyregion (skylist[0].regions[i]);

    REALLOCATE (Ref[0].stars, StarData, MAX (1, Ref[0].N));
    for (k = Ns, j = 0; j < Ngsc; k++, j++) {
      Ref[0].stars[k].R = gsc[j].R;
      Ref[0].stars[k].D = gsc[j].D;
      Ref[0].stars[k].M = gsc[j].M;
    }      
    free (gsc);
  }
  SkyTableFree (sky);
  
  Ref[0].R0    = catstats[0].RA[0];
  Ref[0].R1    = catstats[0].RA[1];
  Ref[0].D0    = catstats[0].DEC[0];
  Ref[0].D1    = catstats[0].DEC[1];

  /* calculate luminosity function of stars */
  get_luminosity_func (Ref[0].stars, Ref[0].N, &Ref[0].lum);

  if (VERBOSE) fprintf (stderr, "%d stars from HST GSC\n", Ref[0].N);
  return (TRUE);
}  

# define BYTES_STAR 23
# define BLOCK 1000
StarData *rd_gsc (char *filename, int *Nstars) {
  
  StarData *stars;
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
  ALLOCATE (stars, StarData, NSTAR);

  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR));
  Nbytes = BLOCK*BYTES_STAR;

  while ((nbytes = fread (buffer, 1, Nbytes, f)) > 0) {
    for (i = 0; i < nbytes / BYTES_STAR; i++) {
      dparse (&stars[nstar].R, 1, &buffer[i*BYTES_STAR]);
      dparse (&stars[nstar].D, 2, &buffer[i*BYTES_STAR]);
      dparse (&stars[nstar].M, 3, &buffer[i*BYTES_STAR]);
      nstar++;
      if (nstar == NSTAR) {
	NSTAR += 1000;
	REALLOCATE (stars, StarData, NSTAR);
      }
    }
  }

  free (buffer);

  *Nstars = nstar;
  return (stars);
}
