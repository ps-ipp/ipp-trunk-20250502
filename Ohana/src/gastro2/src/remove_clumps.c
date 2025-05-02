# include "gastro2.h"

StarData *remove_clumps (StarData *instars, int *nstars, int NX, int NY) {

  int i, j, nx, ny, Npix, nn, x, y, pix, Nstars;
  double s1, s2, mean, sigma, cutoff;
  int *hist, *xcld;
  int nxcld, Nxcld, Nout;
  
  StarData *outstars;

  /* create histogram of pixels using coords */

  Nstars = *nstars;

  Npix = 100;
  nx = (int) (NX / Npix) + 1;
  ny = (int) (NY / Npix) + 1;
  nn = nx * ny;

  ALLOCATE (hist, int, nn);
  ALLOCATE (xcld, int, nn);
  bzero (hist, nn*sizeof(int));

  for (i = 0; i < Nstars; i++) {
    
    x = (int) (instars[i].X / Npix);
    y = (int) (instars[i].Y / Npix);
    pix = x + nx * y;

    if ((pix < 0) || (pix >= nn)) { 
      fprintf (stderr, "! %f %f  %d %d  %d %d\n", instars[i].X, instars[i].Y, x, y, pix, nn);
      continue;
    }
    
    hist[pix] ++;

  }

  /* find stats on histogram */
  s1 = s2 = 0;
  for (i = 0; i < nn; i++) {
    s1 += hist[i];
    s2 += hist[i]*hist[i];
  }
  mean  = s1 / nn;
  sigma = 2 + sqrt (s2 / nn - mean*mean);
  cutoff = mean + 5*sigma;

  /* identify clumps to exclude */
  Nxcld = 0;
  for (i = 0; i < nn; i++) {
    if (hist[i] > cutoff) {
      y = (int) (i / nx);
      x = i - y*nx;
      fprintf (stderr, "cut: %d  %d %d  %d\n", i, x, y, hist[i]);
      xcld[Nxcld] = i;
      Nxcld ++;
    }
  }

  /* identify stars to exclude (type = -1) */
  for (i = 0; i < Nxcld; i++) {

    nxcld = 0;
    for (j = 0; j < Nstars; j++) {
      
      x = (int) (instars[j].X / Npix);
      y = (int) (instars[j].Y / Npix);
      pix = x + nx * y;
      if (pix != xcld[i]) continue;
      nxcld ++;      
      instars[j].type = -1;
    }
    fprintf (stderr, "exclude %d in clump %d\n", nxcld, i);
  }

  ALLOCATE (outstars, StarData, Nstars);
  Nout = 0;

  for (i = 0; i < Nstars; i++) {
    if (instars[i].type == -1) continue;
    outstars[Nout] = instars[i];
    Nout ++;
  } 

  REALLOCATE (outstars, StarData, Nout);
  *nstars = Nout;
  fprintf (stderr, "keeping %d of %d stars\n", Nout, Nstars);
  return (outstars);

}
  
     
