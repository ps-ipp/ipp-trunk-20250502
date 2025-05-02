# include "ohana.h"
# include "dvo.h"

main (int argc, char **argv) {

  FILE *f;
  char line[256], name[256];
  Header header;
  Coords coords;
  double R, D, dR, dD;
  double X, Y, dX, dY;
  double angle;
  int Nfield;

  if (argc != 3) {
    fprintf (stderr, "USAGE: %s (image.fits) (input)\n", argv[0]);
    exit (1);
  }

  if (!gfits_read_header (argv[1], &header)) {
    fprintf (stderr, "can't read header from file %s\n", argv[1]);
    exit (1);
  }

  if (!GetCoords (&coords, &header)) {
    fprintf (stderr, "can't find appropriate astrometry in image header\n");
    exit (1);
  }

  /* load in the object description list */

  /* fields for each line should be:
     name (string)
     Ra (dec. degrees)
     Dec (dec. degrees)
     dRa (arcsec)
     dDec (arcsec)
     angle (degrees)
  */

  if (argv[2][0] == '-') {
    f = stdin;
  } else {
    f = fopen (argv[2], "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "can't open input data file\n");
      exit (1);
    }
  }

  while (scan_line (f, line) != EOF) {
    Nfield = sscanf (line, "%s %lf %lf %lf %lf %lf", name, &R, &D, &dR, &dD, &angle);
    
    if (Nfield < 4) {
      fprintf (stderr, "*");
      continue;
    }
    
    RD_to_XY (&X, &Y, R, D, &coords);

    /* we won't worry about the case of cdelt1 != cdelt2,
       in which case we don't have to worry about rotation 
       between RA,DEC and X,Y for the calculation of the
       slit lenghts.  Note that this assumption does not 
       cause errors with the varying projected slitlength in
       RA units, that is taken care of by the projection.
       But, we are not taking into account a rotated slit */

    dX = dR / 3600.0 / coords.cdelt1;
    dY = dD / 3600.0 / coords.cdelt2;

    switch (Nfield) {
    case 4:
      fprintf (stdout, "%s(%f,%f,%f)\n", name, X, Y, dX);
      break;
    case 5:
      fprintf (stdout, "%s(%f,%f,%f,%f)\n", name, X, Y, dX, dY);
      break;
    case 6:
      fprintf (stdout, "%s(%f,%f,%f,%f,%f)\n", name, X, Y, dX, dY, angle);
      break;
    }
  }

  if (f != stdin) fclose (f);

}
