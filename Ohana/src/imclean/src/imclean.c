# include "imclean.h"

int main (int argc, char **argv) {

  Header header;
  SMPData *stars;
  int Nstars;

  ConfigInit (&argc, argv);

  args (argc, argv);

  /* load in FITS header from image */
  if (!gfits_read_header (argv[1], &header)) {
    fprintf (stderr, "ERROR: can't find image file %s\n", argv[1]);
    exit (1);
  }

  AdjustHeader (&header);
  
  switch (MODE) {
  case DOPHOT:
    stars = LoadStarsDophot (argv[2], &Nstars, &header);
    break;
  case CHAD:
    stars = LoadStarsChad (argv[2], &Nstars, &header);
    break;
  case SEXTRACT:
    stars = LoadStarsSex (argv[2], &Nstars, &header);
    break;
  default: 
    fprintf (stderr, "unknown mode: %d\n", MODE);
    exit (1);
  }

  sort_stars (stars, Nstars);
  find_trails (stars, Nstars);  
  fix_total (stars, Nstars, &header);

  if (FITS_OUTPUT) {
    wfits (argv[3], stars, Nstars, &header); 
  } else {
    wstars (argv[3], stars, Nstars, &header); 
  }

  fprintf (stderr, "SUCCESS\n");
  exit (0);

}

/* based on fstat and markstar:

   0) load config data, global parameters 
   1) load header
   2) load data from *.obj file
   3) eliminate bad star types: 6, 8
   4) identify trails
   5) get statistics on remaining stars

*/

/* 
imclean (file.fits) (file.obj) (file.cmp) 

 [-p photcode]
 [-chad]
 [-sex]
 [-coords RA DEC] 
 [-astrom file]
 [-v]
 [-key name %f value]
 [-key name %s value]
 (maximum of 64 keywords can be changed)

*/
