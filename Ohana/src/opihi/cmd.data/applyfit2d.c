# include "data.h"

int applyfit2d (int argc, char **argv) {
  
  int i, j, n, order;
  char *c, name[64];
  double **C, X, Y;
  opihi_flt *z;
  Vector *xvec, *yvec, *zvec;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: applyfit2d x y z\n");
    return (FALSE);
  }

  c = get_variable ("Cnn");
  if (c == NULL) {
    gprint (GP_ERR, "no fit available\n");
    return (FALSE);
  }
  order = atof (c);
  free (c);

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((zvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);    

  ALLOCATE (C, double *, order+1);
  for (i = 0; i < order + 1; i++) {
    ALLOCATE (C[i], double, order+1);
    for (j = 0; j < order + 1 - i; j++) {
      sprintf (name, "CX%dY%d", i, j);
      c = get_variable (name);
      if (c == NULL) {
	gprint (GP_ERR, "missing fit term %d,%d\n", i, j);
	for (j = 0; j < i; j++) free (C[j]);
	free (C);
	return (FALSE);
      }
      C[i][j] = atof (c);
      free (c);
    }
  }

  ResetVector (zvec, OPIHI_FLT, xvec[0].Nelements);
  bzero (zvec[0].elements.Flt, sizeof(opihi_flt)*zvec[0].Nelements);
  z = zvec[0].elements.Flt;

  // we have four cases (x == flt or int) and (y == flt or int)
  if ((xvec[0].type == OPIHI_FLT) && (yvec[0].type == OPIHI_FLT)) {
    opihi_flt *x = xvec[0].elements.Flt;
    opihi_flt *y = yvec[0].elements.Flt;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j < order + 1; j++) {
	X = Y;
	for (i = 0; i < order + 1 - j; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }
  if ((xvec[0].type == OPIHI_FLT) && (yvec[0].type == OPIHI_INT)) {
    opihi_flt *x = xvec[0].elements.Flt;
    opihi_int *y = yvec[0].elements.Int;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j < order + 1; j++) {
	X = Y;
	for (i = 0; i < order + 1 - j; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }
  if ((xvec[0].type == OPIHI_INT) && (yvec[0].type == OPIHI_FLT)) {
    opihi_int *x = xvec[0].elements.Int;
    opihi_flt *y = yvec[0].elements.Flt;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j < order + 1; j++) {
	X = Y;
	for (i = 0; i < order + 1 - j; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }
  if ((xvec[0].type == OPIHI_INT) && (yvec[0].type == OPIHI_INT)) {
    opihi_int *x = xvec[0].elements.Int;
    opihi_int *y = yvec[0].elements.Int;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j < order + 1; j++) {
	X = Y;
	for (i = 0; i < order + 1 - j; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }

  for (i = 0; i < order + 1; i++) free (C[i]);
  free (C);

  return (TRUE);
}


int applyfit2d_full (int argc, char **argv) {
  
  int i, j, n, order;
  char *c, name[64];
  double **C, X, Y;
  opihi_flt *z;
  Vector *xvec, *yvec, *zvec;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: applyfit2d_full x y z\n");
    return (FALSE);
  }

  c = get_variable ("Cnn");
  if (c == NULL) {
    gprint (GP_ERR, "no fit available\n");
    return (FALSE);
  }
  order = atof (c);
  free (c);

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((zvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);    

  ALLOCATE (C, double *, order+1);
  for (i = 0; i <= order; i++) {
    ALLOCATE (C[i], double, order+1);
    for (j = 0; j <= order; j++) {
      sprintf (name, "CX%dY%d", i, j);
      c = get_variable (name);
      if (c == NULL) {
	gprint (GP_ERR, "missing fit term %d,%d\n", i, j);
	for (j = 0; j < i; j++) free (C[j]);
	free (C);
	return (FALSE);
      }
      C[i][j] = atof (c);
      free (c);
    }
  }

  ResetVector (zvec, OPIHI_FLT, xvec[0].Nelements);
  bzero (zvec[0].elements.Flt, sizeof(opihi_flt)*zvec[0].Nelements);
  z = zvec[0].elements.Flt;

  // we have four cases (x == flt or int) and (y == flt or int)
  if ((xvec[0].type == OPIHI_FLT) && (yvec[0].type == OPIHI_FLT)) {
    opihi_flt *x = xvec[0].elements.Flt;
    opihi_flt *y = yvec[0].elements.Flt;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j <= order; j++) {
	X = Y;
	for (i = 0; i <= order; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }
  if ((xvec[0].type == OPIHI_FLT) && (yvec[0].type == OPIHI_INT)) {
    opihi_flt *x = xvec[0].elements.Flt;
    opihi_int *y = yvec[0].elements.Int;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j <= order; j++) {
	X = Y;
	for (i = 0; i <= order; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }
  if ((xvec[0].type == OPIHI_INT) && (yvec[0].type == OPIHI_FLT)) {
    opihi_int *x = xvec[0].elements.Int;
    opihi_flt *y = yvec[0].elements.Flt;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j <= order; j++) {
	X = Y;
	for (i = 0; i <= order; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }
  if ((xvec[0].type == OPIHI_INT) && (yvec[0].type == OPIHI_INT)) {
    opihi_int *x = xvec[0].elements.Int;
    opihi_int *y = yvec[0].elements.Int;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++) {
      Y = X = 1;
      for (j = 0; j <= order; j++) {
	X = Y;
	for (i = 0; i <= order; i++) {
	  *z += C[i][j]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }
  }

  for (i = 0; i < order + 1; i++) free (C[i]);
  free (C);

  return (TRUE);
}


