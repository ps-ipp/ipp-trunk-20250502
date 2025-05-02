# include "data.h"

int lookup (int argc, char **argv) {
  
  int i, j;
  opihi_flt *ip, *op, *xp, *yp;
  Vector *in, *out, *xv, *yv;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: lookup (input) (output) (x) (y)\n");
    return (FALSE);
  }

  if ((in  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((out = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((xv  = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yv  = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (in, FALSE); 
  REQUIRE_VECTOR_FLT (xv, FALSE); 
  REQUIRE_VECTOR_FLT (yv, FALSE); 

  if (xv[0].Nelements != yv[0].Nelements) {
      gprint (GP_ERR, "unmatched lookup table lengths\n");
      return (FALSE);
  }

  ResetVector (out, OPIHI_FLT, in[0].Nelements);

  ip = in[0].elements.Flt;
  op = out[0].elements.Flt;

  for (i = 0; i < in[0].Nelements; i++, ip++, op++) {
    // re-write this using bisection
    xp = xv[0].elements.Flt;
    yp = yv[0].elements.Flt;

    for (j = 0; (*ip < *xp) && (j < yv[0].Nelements); j++);
    *op = *yp;
  }      

  return (TRUE);
}
