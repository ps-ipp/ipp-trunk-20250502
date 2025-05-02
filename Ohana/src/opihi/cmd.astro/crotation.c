# include "astro.h"

int crotation (int argc, char **argv) {

  int i;
  double X, Y, x, y;
  opihi_flt *xptr, *yptr;
  Vector *xvec, *yvec;
  CoordTransform *transform;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: crotation (phi) (Xo) (xo) X Y\n");
    return (FALSE);
  }

  double phi = atof(argv[1]);
  double Xo  = atof(argv[2]);
  double xo  = atof(argv[3]);
  char *Xname = argv[4];
  char *Yname = argv[5];

  transform = AllocTransform (phi, Xo, xo);
  if (SelectScalar (Xname, &X)) {
    if (!SelectScalar (Yname, &Y)) return (FALSE);
      
    ApplyTransform (&x, &y, X, Y, transform);

    gprint (GP_LOG, "%10.6f %10.6f\n", x, y);
    return (TRUE);
  }

  /* find vectors */
  if ((xvec = SelectVector (Xname, OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (Yname, OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", Xname, Yname);
    return (FALSE);
  }
  
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);

  xptr = xvec[0].elements.Flt;
  yptr = yvec[0].elements.Flt;

  for (i = 0; i < xvec[0].Nelements; i++, xptr++, yptr++) {
    ApplyTransform (xptr, yptr, *xptr, *yptr, transform);
  }

  return (TRUE);
}
