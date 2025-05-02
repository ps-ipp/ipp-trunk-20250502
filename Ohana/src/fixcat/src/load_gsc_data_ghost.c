# include "markstar.h"
# define NBYTES 160000
# define BYTES_STAR 23
# define BLOCK 1000
# define DNSTARS 1000

/* this routine is basically identical to load_gsc_data, but
   it is not limited to a single region file, and it merges the 
   results */

load_gsc_data_ghost (catalog, region, Nregion, image)
Catalog catalog[];
GSCRegion *region;
int Nregion;
Image image[];
{

  /* load data from the GSC files */
  char filename[128];
  char *tbuffer;
  int nstar, NSTARS;
  int i, j, Nbytes, nbytes;
  double R, D, M, X, Y;
  FILE *f;
  Coords *tcoords;
  int MinX, MinY, MaxX, MaxY;
  
  nstar = 0;
  NSTARS = DNSTARS;
  ALLOCATE (tbuffer, char, (BLOCK*BYTES_STAR));
  ALLOCATE (catalog[0].average, Average, NSTARS);
  tcoords = &image[0].coords;
  MinX = 0;
  MinY = 0;
  MaxX = image[0].NX;
  MaxY = image[0].NY;
  
  for (j = 0; j < Nregion; j++) {
    
    f = fopen (region[j].filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "no GSC file for region %s (2)\n", region[j].filename);
      exit (0);
    }
    
    Nbytes = BLOCK*BYTES_STAR;
    while ((nbytes = fread (tbuffer, 1, Nbytes, f)) > 0) {
      for (i = 0; i < nbytes / BYTES_STAR; i++) {
	dparse (&M, 3, &tbuffer[i*BYTES_STAR]);
	if (M > GHOST_MAG) continue;
	dparse (&R, 1, &tbuffer[i*BYTES_STAR]);
	dparse (&D, 2, &tbuffer[i*BYTES_STAR]);
	RD_to_XY (&X, &Y, R, D, tcoords);
	if ((X < MinX) || (X > MaxX) || (Y < MinY) || (Y > MaxY)) continue;
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
  REALLOCATE (catalog[0].average, Average, MAX (nstar, 1));
  catalog[0].Naverage = nstar;

}


