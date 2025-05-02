# include "gastro2.h"

int main (int argc, char **argv) {

  int i;
  RefCatalog Ref;
  CmpCatalog Target;

  /* start_timer (); */

  ConfigInit (&argc, argv);
  args (&argc, argv, &Target.coords); 

  gstars (argv[1], &Target);
  greference (&Target, &Ref);
  gcenter (&Target, &Ref);

  for (i = 0; i < 3; i++) {
    gfit (&Target, &Ref, 1);
    fprintf (stderr, "precision: %f\n", Target.answer.dR / sqrt(Target.answer.N));
    if ((Target.answer.N < 2) || isnan(Target.answer.dR)) {
	fprintf (stderr, "ERROR: bad fit\n");
	exit (1);
    }
    if (VERBOSE) {
      fprintf (stderr, "%s\n", Target.coords.ctype);
      fprintf (stderr, "%f %f\n", Target.coords.crval1, Target.coords.crval2);
      fprintf (stderr, "%f %f\n", Target.coords.crpix1, Target.coords.crpix2);
      fprintf (stderr, "%f %f\n", Target.coords.pc1_1,  Target.coords.pc1_2);
      fprintf (stderr, "%f %f\n", Target.coords.pc2_1,  Target.coords.pc2_2);
      fprintf (stderr, "%f %f\n", Target.coords.cdelt1, Target.coords.cdelt2);
    }

  }

  for (i = 0; i < 5; i++) {
    gfit (&Target, &Ref, MIN (MAX (1, NPOLYTERMS), 3));
    fprintf (stderr, "precision: %f\n", Target.answer.dR / sqrt(Target.answer.N));
    if ((Target.answer.N < 2) || isnan(Target.answer.dR)) {
	fprintf (stderr, "ERROR: bad fit\n");
	exit (1);
    }
  }

  if (VERBOSE) {
    fprintf (stderr, "%s\n", Target.coords.ctype);
    fprintf (stderr, "%f %f\n", Target.coords.crval1, Target.coords.crval2);
    fprintf (stderr, "%f %f\n", Target.coords.crpix1, Target.coords.crpix2);
    fprintf (stderr, "%f %f\n", Target.coords.pc1_1,  Target.coords.pc1_2);
    fprintf (stderr, "%f %f\n", Target.coords.pc2_1,  Target.coords.pc2_2);
    fprintf (stderr, "%f %f\n", Target.coords.cdelt1, Target.coords.cdelt2);
  }

  gheader (argv[1], &Target);

  /* print_timer (); */

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}

  /* 
     load config & args 

     load stars & header from cmp file
     (this should include filtering based on CONFIG data:
     limit number of stars, limit types, etc) 

     identify and load reference catalog(s)
     - load all catalogs available?
     - keep ra, dec, mag

     project catalog to initial guess

     find simple x, y offset in limited number of rotations
   
     reload reference catalog if dx, dy large 

     project to new guess

     fit on star-by-star basis
     - include weighting by dmag
     - downweight by Nmatch to each star
     - iterate a few times?

     project to new guess (why is this not part of the routines?)

     try a higher order fit

     adjust header, re-write file

     TODO:

     1) downweight points by Nmatch
     2) better criterion for success
     3) better criterion for failure
     4) polyterms are broken

  */
