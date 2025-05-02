# include "data.h"

int applyfit3d (int argc, char **argv) {
  
  int i, j, k, io, jo, n, order, nterm;
  char *c, name[64];
  double ***C, X, Y, Z;
  opihi_flt *F;
  Vector *xvec, *yvec, *zvec, *Fvec;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: applyfit x y z F\n");
    return (FALSE);
  }

  c = get_variable ("Cnnn");
  if (c == NULL) {
    gprint (GP_ERR, "no fit available\n");
    return (FALSE);
  }
  order = atof (c);
  free (c);

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((Fvec = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);    

  // this is a bit crude, but perhaps needed
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);
  CastVector (zvec, OPIHI_FLT);
  CastVector (Fvec, OPIHI_FLT);

  nterm = order + 1;
  ALLOCATE (C, double **, nterm);
  for (i = 0; i < order + 1; i++) {
    ALLOCATE (C[i], double *, nterm);
    for (j = 0; j < nterm - i; j++) {
      ALLOCATE (C[i][j], double, nterm);
      for (k = 0; k < nterm - i - j; k++) {
	sprintf (name, "CX%dY%dZ%d", i, j, k);
	c = get_variable (name);
	if (c == NULL) {
	  gprint (GP_ERR, "missing fit term %d,%d,%d\n", i, j, k);
	  for (io = 0; io < i; io++) {
	    for (jo = 0; jo < j; jo++) {
	      free (C[io][jo]);
	    }
	    free (C[io]);
	  }
	  free (C);
	  return (FALSE);
	}
	C[i][j][k] = atof (c);
	free (c);
      }
    }
  }

  ResetVector (Fvec, OPIHI_FLT, xvec[0].Nelements);
  bzero (Fvec[0].elements.Flt, sizeof(opihi_flt)*zvec[0].Nelements);
  F = Fvec[0].elements.Flt;

  opihi_flt *x = xvec[0].elements.Flt;
  opihi_flt *y = yvec[0].elements.Flt;
  opihi_flt *z = zvec[0].elements.Flt;

  for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++, F++) {
    Z = Y = X = 1;
    for (k = 0; k < nterm; k++) {
      Y = Z;
      for (j = 0; j < nterm - k; j++) {
	X = Y;
	for (i = 0; i < nterm - k - j; i++) {
	  *F += C[i][j][k]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
      Z = Z * (*z);
    }
  }

  for (i = 0; i < nterm; i++) {
    for (j = 0; j < nterm - i; j++) {
      free (C[i][j]);
    }
    free (C[i]);
  }
  free (C);

  return (TRUE);
}
