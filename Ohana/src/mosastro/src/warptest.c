# include "mosastro.h"
int XY_to_RD (double *ra, double *dec, double x, double y, Coords *coords);
int RD_to_XY (double *x, double *y, double ra, double dec, Coords *coords);

main (int argc, char **argv) {

  int i, status;
  double X, Y, x, y, p, q;
  Coords coords;

  if (argc != 2) {
    fprintf (stderr, "USAGE: warptest (system)\n");
    exit (2);
  }

  /* bore site center guess */
  sprintf (coords.ctype, "DEC--%s", argv[1]);
  coords.crval1 = 0.0;
  coords.crval2 = 0.0;
  coords.crpix1 = 0.0;
  coords.crpix2 = 0.0;
  coords.cdelt1 = 1.0;
  coords.cdelt2 = 1.0;

  /* 
  coords.cdelt1 = 1.0/3600.0;
  coords.cdelt2 = 1.0/3600.0;
  */
  
  /** allow guess at field rotation?? **/
  coords.pc1_1  = 1;
  coords.pc2_2  = 1;
  coords.pc1_2  = 0;
  coords.pc2_1  = 0;

  /* allow 2nd and 3rd order? */
  /* how do we handle renormalization? (fixed at 1000 pixels??) */
  coords.Npolyterms = 3;
  for (i = 0; i < 7; i++) {
    coords.polyterms[i][0] = 0;
    coords.polyterms[i][1] = 0;
  }
  coords.polyterms[3][0] = 1e-10;
  coords.polyterms[6][1] = 1e-10;

  for (x = -10000; x <= 10000; x+= 500) {
    for (y = -10000; y <= 10000; y+= 500) {
      status = XY_to_RD (&p, &q, x, y, &coords);
      status = RD_to_XY (&X, &Y, p, q, &coords);
      fprintf (stdout, "%f %f   %f %f   %f %f\n", x, y, p, q, X, Y);
      if (!status) exit (1);
    }
  }
  exit (0);
}
