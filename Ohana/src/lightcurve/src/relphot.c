# include "lightcurve.h"

void main (argc, argv)
int argc;
char **argv;
{
  
  Image   *images;
  Star    *stars;
  Unique  *unique;
  int     *radec;
  int      Nimages, Nstars, Nunique;
  int      i, j;

  /*** this still needs to be fixed / updated */
  args (argc, argv);

  get_names  (&images, &Nimages);
  get_stars  (&stars, &Nstars, images, Nimages);
  sort_stars (&radec, stars, Nstars);

  get_unique (&unique, &Nunique, stars, radec, Nstars);
  count_unique (images, Nimages, Nstars);

  for (i = 0; i < NLOOP; i++) {
    set_Mcal (images, Nimages);
    get_Mrel (unique, images, Nunique);
    get_Mcal (images, unique, Nimages);
    ChiSquare (unique, images, Nunique, Nimages);
  }  
 

  if (PRINT) {
    alter_headers (images, Nimages);
    make_table (unique, images, Nunique);
  }
  
}


/* 
   list of system wide variables that are (should be) defined in relphot.h:

   RADIUS  (radius of search in pixels or arcsec)
   A_LAMBDA (airmass extinction coefficient)
   C_LAMBDA (zero point C_\lambda)
   NLOOP   (desired number of interations)
   SIG     (number of sigma for cloudiness criterion)
   OUTFILE (output file)
   PRINT   (print processing info?)
   PIXELS  (RADIUS in pixels (rastro) or degrees (astro))
   COS     (RA scaling factor = cos(DEC(center of field)) if astro, 
                              = 1 if rastro )

*/

   
