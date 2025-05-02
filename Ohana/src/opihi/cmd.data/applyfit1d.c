# include "data.h"

int applyfit1d (int argc, char **argv) {
  
  int i, j, order;
  char *c, name[64];
  double *C, X;
  opihi_flt *y;
  Vector *xvec, *yvec;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: applyfit x y\n");
    return (FALSE);
  }

  c = get_variable ("Cn");
  if (c == NULL) {
    gprint (GP_ERR, "no fit available\n");
    return (FALSE);
  }
  order = atof (c);
  free (c);

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);    

  ALLOCATE (C, double, order+1);
  for (i = 0; i < order + 1; i++) {
    sprintf (name, "C%d", i);
    c = get_variable (name);
    if (c == NULL) {
      gprint (GP_ERR, "missing fit term %d\n", i);
      return (FALSE);
    }
    C[i] = atof (c);
    free (c);
  }

  ResetVector (yvec, OPIHI_FLT, xvec[0].Nelements);
  bzero (yvec[0].elements.Flt, sizeof(opihi_flt)*yvec[0].Nelements);
  y = yvec[0].elements.Flt;

  if (xvec[0].type == OPIHI_FLT) {
    opihi_flt *x = xvec[0].elements.Flt;
    for (j = 0; j < xvec[0].Nelements; j++, x++, y++) {
      X = 1;
      for (i = 0; i < order + 1; i++) {
	*y += C[i]*X;
	X = X * (*x);
      }
    }
  } else {
    opihi_int *x = xvec[0].elements.Int;
    for (j = 0; j < xvec[0].Nelements; j++, x++, y++) {
      X = 1;
      for (i = 0; i < order + 1; i++) {
	*y += C[i]*X;
	X = X * (*x);
      }
    }
  }

  free (C);
  return (TRUE);

}
