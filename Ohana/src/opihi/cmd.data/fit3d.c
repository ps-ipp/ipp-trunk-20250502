# include "data.h"

int fit3d (int argc, char **argv) {
  
  double **c, **b, ***s, X, Y, Z, dF1, dF2;
  double ClipNSigma, mean, sigma, maxsigma;
  int ix, iy, i, j, k, n, Npt, Nmask, nx, ny, nz, nterm, mterm, wterm, order;
  int N, Weight, Quiet, ClipNiter, VERBOSE;
  opihi_flt *x, *y, *z, *F, *dFv, *Ffit, *Ff; 
  char name[64], *mask;
  Vector *xvec, *yvec, *zvec, *Fvec, *dFvec;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  ClipNSigma = 0;
  ClipNiter  = 1;
  if ((N = get_argument (argc, argv, "-clip"))) {
    remove_argument (N, &argc, argv);
    ClipNSigma = atof(argv[N]);
    remove_argument (N, &argc, argv);
    ClipNiter  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  dFv = NULL;
  dFvec = NULL;
  Weight = FALSE;
  if ((N = get_argument (argc, argv, "-dF"))) {
    remove_argument (N, &argc, argv);
    if ((dFvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
    Weight = TRUE;
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: fit x y z F order [-dF wt]\n");
    return (FALSE);
  }

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((Fvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  if (xvec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  if (xvec[0].Nelements != Fvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);
  CastVector (zvec, OPIHI_FLT);
  CastVector (Fvec, OPIHI_FLT);

  if (Weight) {
    if (xvec[0].Nelements != dFvec[0].Nelements) {
      gprint (GP_ERR, "vectors must have same length\n");
      return (FALSE);
    }
    CastVector (dFvec, OPIHI_FLT);
  }
  
  order = atof (argv[5]);
  nterm = order + 1;
  wterm = (order + 3)*(order + 2)*(order + 1)/6;
  mterm = (order + 2)*(order + 1)/2;

  ALLOCATE (Ffit, opihi_flt, xvec[0].Nelements);
  ALLOCATE (mask, char, xvec[0].Nelements);
  memset (mask, 0, xvec[0].Nelements);

  /* allocate the summation matrices */
  ALLOCATE (b, double *, wterm);
  ALLOCATE (c, double *, wterm);
  for (i = 0; i < wterm; i++) {
    ALLOCATE (c[i], double, wterm);
    ALLOCATE (b[i], double, 1);
  }

  /* we initially sum the values in a cube */
  ALLOCATE (s, double **, mterm);
  for (i = 0; i < mterm; i++) {
    ALLOCATE (s[i], double *, mterm);
    for (j = 0; j < mterm; j++) {
      ALLOCATE (s[i][j], double, mterm);
    }
  }

  for (N = 0; N < ClipNiter; N++) {

    /* init registers for current pass */
    for (i = 0; i < wterm; i++) {
      memset (c[i], 0, wterm*sizeof(double));
      memset (b[i], 0, sizeof(double));
    }
    for (i = 0; i < mterm; i++) {
      for (j = 0; j < mterm; j++) {
	memset (s[i][j], 0, mterm*sizeof(double));
      }
    }

    x = xvec[0].elements.Flt;
    y = yvec[0].elements.Flt;
    z = zvec[0].elements.Flt;
    F = Fvec[0].elements.Flt;
    if (Weight) dFv = dFvec[0].elements.Flt;

    /* add up the x,y,z values */
    for (i = 0; i < xvec[0].Nelements; i++, x++, y++, z++, F++) {
      if (mask[i]) continue;
      if (!finite(*x)) continue;
      if (!finite(*y)) continue;
      if (!finite(*z)) continue;
      if (!finite(*F)) continue;
      dF1 = 1.0;
      if (Weight) { 
	dF1 = 1.0 / SQ(*dFv);
	dFv ++;
      }
      X = Y = Z = dF1;
      for (nz = 0; nz < mterm; nz++) {
	Y = Z;
	for (ny = 0; ny < mterm - nz; ny++) {
	  X = Y;
	  for (nx = 0; nx < mterm - nz - ny; nx++) {
	    s[nx][ny][nz] += X;
	    X = X * (*x);
	  }
	  Y = Y * (*y);
	}
	Z = Z * (*z);
      }
    }

    /* add up the F values */
    x = xvec[0].elements.Flt;
    y = yvec[0].elements.Flt;
    z = zvec[0].elements.Flt;
    F = Fvec[0].elements.Flt;
    for (i = 0; i < xvec[0].Nelements; i++, x++, y++, z++, F++) {
      if (mask[i]) continue;
      if (!finite(*x)) continue;
      if (!finite(*y)) continue;
      if (!finite(*z)) continue;
      if (!finite(*F)) continue;
      dF1 = 1.0;
      if (Weight) { 
	dF1 = 1.0 / SQ(*dFv);
	dFv ++;
      }
      X = Y = Z = *F*dF1;
      j = 0;
      for (nz = 0; nz < nterm; nz++) {
	Y = Z;
	for (ny = 0; ny < nterm - nz; ny++) {
	  X = Y;
	  for (nx = 0; nx < nterm - ny - nz; nx++, j++) {
	    b[j][0] += X;
	    X = X * (*x);
	  }
	  Y = Y * (*y);
	}
	Z = Z * (*z);
      }
    }

    /* re-sort mterm x mterm matrix to wterm matrix */
    ix = 0;
    for (k = 0; k < nterm; k++) {
      for (j = 0; j < nterm - k; j++) {
	for (i = 0; i < nterm - k - j; i++, ix++) {
	  iy = 0;
	  for (nz = 0; nz < nterm; nz++) {
	    for (ny = 0; ny < nterm - nz; ny++) {
	      for (nx = 0; nx < nterm - ny - nz; nx++, iy++) {
		c[ix][iy] = s[nx+i][ny+j][nz+k];
	      }
	    }
	  }
	}
      }
    }

    /** test print **/
    if (VERBOSE) {
      for (i = 0; i < wterm; i++) {
	for (j = 0; j < wterm; j++) {
	  gprint (GP_ERR, "%g  ", c[i][j]);
	}
	gprint (GP_ERR, "\n");
      }
      gprint (GP_ERR, "-----\n");
    }

    dgaussjordan (c, b, wterm, 1);

    /** test print **/
    if (VERBOSE) {
      i = 0;
      for (nz = 0; nz < nterm; nz++) {
	for (ny = 0; ny < nterm - nz; ny++) {
	  for (nx = 0; nx < nterm - nz - ny; nx++, i++) {
	    gprint (GP_ERR, "x^%d y^%d z^%d: %g\n", nx, ny, nz, b[i][0]);
	  }
	}
      }
    }

    /** test print **/
    if (VERBOSE) {
      for (i = 0; i < wterm; i++) {
	for (j = 0; j < wterm; j++) {
	  gprint (GP_ERR, "%g  ", c[i][j]);
	}
	gprint (GP_ERR, "\n");
      }
    }

    /* the b[][0] terms are in the following order:
       y^0 x^0, y^0 x^1, ... y^0 x^N
       y^1 x^0, y^1 x^1, ... y^1 x^N
       ...
       y^N x^0, y^N x^1, ... y^N x^N
    */
    /* generate fitted values */
    x = xvec[0].elements.Flt;
    y = yvec[0].elements.Flt;
    z = zvec[0].elements.Flt;
    Ff = Ffit;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, z++, Ff++) {
      if (!finite(*x)) continue;
      if (!finite(*y)) continue;
      if (!finite(*z)) continue;
      if (!finite(*F)) continue;
      *Ff = 0;
      Z = Y = X = 1;
      i = 0;
      for (nz = 0; nz < nterm; nz++) {
	Y = Z;
	for (ny = 0; ny < nterm - nz; ny++) {
	  X = Y;
	  for (nx = 0; nx < nterm - nz - ny; nx++, i++) {
	    *Ff += b[i][0]*X;
	    X = X * (*x);
	  }
	  Y = Y * (*y);
	}
	Z = Z * (*z);
      }
    }

    /* measure fit residual scatter */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    z  = zvec[0].elements.Flt;
    F  = Fvec[0].elements.Flt;
    Ff = Ffit;
    dF1 = dF2 = 0;
    for (i = Npt = 0; i < xvec[0].Nelements; i++, x++, y++, z++, F++, Ff++) {
      if (mask[i]) continue;
      if (!finite(*x)) continue;
      if (!finite(*y)) continue;
      if (!finite(*z)) continue;
      if (!finite(*F)) continue;
      dF1 += (*F - *Ff);
      dF2 += SQ(*F - *Ff);
      Npt ++;
    }
    mean  = dF1 / Npt;
    sigma = sqrt (fabs(dF2/Npt - SQ(mean)));
    maxsigma = ClipNSigma * sigma;

    if (VERBOSE) gprint (GP_ERR, "mean: %g, sigma: %g, maxsigma: %g\n", mean, sigma, maxsigma);

    /* mask outlier points */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    z  = zvec[0].elements.Flt;
    F  = Fvec[0].elements.Flt;
    Ff = Ffit;
    Nmask = 0;
    for (i = 0; ClipNSigma && (i < xvec[0].Nelements); i++, x++, y++, z++, Ff++) {
      dF1 = (*F - *Ff);
      if (fabs(dF1) > maxsigma) {
	mask[i] = TRUE;
	Nmask ++;
      } else {
	mask[i] = FALSE;
      }	
    }
    if (VERBOSE) gprint (GP_ERR, "pass: %d, Nmask: %d\n", N, Nmask);
  }

  if (!Quiet) gprint (GP_ERR, "F = ");

  N = 0;
  for (nz = 0; nz < nterm; nz++) {
    for (ny = 0; ny < nterm - nz; ny++) {
      for (nx = 0; nx < nterm - nz - ny; nx++, N++) {
	sprintf (name, "CX%dY%dZ%d", nx, ny, nz);
	set_variable (name, b[N][0]);
	if (!Quiet) gprint (GP_ERR, "%f x^%d y^%d z^%d ", b[N][0], nx, ny, nz);
      }
    }
  }
  sprintf (name, "Cnnn");
  set_variable (name, (double) order);
  
  if (!Quiet) gprint (GP_ERR, "\n");

  /* free internal data */
  free (Ffit);
  free (mask);

  for (i = 0; i < wterm; i++) {
    free (c[i]);
    free (b[i]);
  }
  free (b);
  free (c);

  for (i = 0; i < mterm; i++) {
    for (j = 0; j < mterm; j++) {
      free (s[i][j]);
    }
    free (s[i]);
  }
  free (s);

  return (TRUE);
}
