# include "data.h"

int fit2d (int argc, char **argv) {
  
  double **c, **b, **s, X, Y, dZ, dZ2;
  double ClipNSigma, mean, sigma, maxsigma;
  int k, K, i, j, n, Npt, Nmask, nx, ny, nterm, mterm, wterm, order;
  int N, Weight, Quiet, ClipNiter, VERBOSE;
  opihi_flt *x, *y, *z, *dz, *zfit, *zf; 
  char name[64], *mask;
  Vector *xvec, *yvec, *zvec, *dzvec;

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

  dz = NULL;
  dzvec = NULL;
  Weight = FALSE;
  if ((N = get_argument (argc, argv, "-dz"))) {
    remove_argument (N, &argc, argv);
    if ((dzvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
    Weight = TRUE;
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: fit2d x y z order [-dz wt] [-quiet/-q] [-clip Nsigma Niter]\n");
    return (FALSE);
  }

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  if (xvec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);
  CastVector (zvec, OPIHI_FLT);

  if (Weight) {
    CastVector (dzvec, OPIHI_FLT);
    if (xvec[0].Nelements != dzvec[0].Nelements) {
      gprint (GP_ERR, "vectors must have same length\n");
      return (FALSE);
    }
  }
  
  order = atof (argv[4]);
  nterm = order + 1;
  wterm = nterm*(nterm + 1)/2;
  mterm = 2*order + 1;

  ALLOCATE (zfit, opihi_flt, xvec[0].Nelements);
  ALLOCATE (mask, char, xvec[0].Nelements);
  memset (mask, 0, xvec[0].Nelements);

  /* allocate the summation matrices */
  ALLOCATE (s, double *, mterm);
  ALLOCATE (b, double *, wterm);
  ALLOCATE (c, double *, wterm);
  for (i = 0; i < wterm; i++) {
    ALLOCATE (c[i], double, wterm);
    ALLOCATE (b[i], double, 1);
  }
  for (i = 0; i < mterm; i++) {
    ALLOCATE (s[i], double, mterm);
  }

  for (N = 0; N < ClipNiter; N++) {

    /* init registers for current pass */
    // XXX this was incorrectly using nterm (missing 1 row)
    for (i = 0; i < wterm; i++) {
      memset (c[i], 0, wterm*sizeof(double));
      memset (b[i], 0, sizeof(double));
    }
    for (i = 0; i < mterm; i++) {
      memset (s[i], 0, mterm*sizeof(double));
    }

    x = xvec[0].elements.Flt;
    y = yvec[0].elements.Flt;
    z = zvec[0].elements.Flt;
    if (Weight) dz = dzvec[0].elements.Flt;

    /* add up the x,y values */
    for (i = 0; i < xvec[0].Nelements; i++, x++, y++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y)) continue;
      dZ = 1.0;
      if (Weight) { 
	dZ = 1.0 / SQ(*dz);
	dz ++;
      }
      Y = X = 1*dZ;
      for (ny = 0; ny < mterm; ny++) {
	X = Y;
	for (nx = 0; nx < mterm - ny; nx++) {
	  s[nx][ny] += X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }

    /* add up the z values */
    x = xvec[0].elements.Flt;
    y = yvec[0].elements.Flt;
    z = zvec[0].elements.Flt;
    if (Weight) dz = dzvec[0].elements.Flt;

    for (i = 0; i < xvec[0].Nelements; i++, x++, y++, z++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y)) continue;
      dZ = 1.0;
      if (Weight) { 
	dZ = 1.0 / SQ(*dz);
	dz ++;
      }
      Y = X = *z*dZ;
      for (j = 0, ny = 0; ny < nterm; ny++) {
	X = Y;
	for (nx = 0; nx < nterm - ny; nx++, j++) {
	  b[j][0] += X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }

    /* re-sort mterm x mterm matrix to wterm matrix */
    for (k = j = 0; j < nterm; j++) {
      for (i = 0; i < nterm - j; i++, k++) {
	for (K = ny = 0; ny < nterm; ny++) {
	  for (nx = 0; nx < nterm - ny; nx++, K++) {
	    c[K][k] = s[nx+i][ny+j];
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
      for (i = ny = 0; ny < nterm; ny++) {
	for (nx = 0; nx < nterm - ny; nx++, i++) {
	  gprint (GP_ERR, "x^%d y^%d: %g\n", nx, ny, b[i][0]);
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
    zf = zfit;
    for (n = 0; n < xvec[0].Nelements; n++, x++, y++, zf++) {
      if (!finite(*x) || !finite(*y)) continue;
      *zf = 0;
      Y = X = 1;
      for (i = ny = 0; ny < nterm; ny++) {
	X = Y;
	for (nx = 0; nx < nterm - ny; nx++, i++) {
	  *zf += b[i][0]*X;
	  X = X * (*x);
	}
	Y = Y * (*y);
      }
    }

    /* measure fit residual scatter */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    z  = zvec[0].elements.Flt;
    zf = zfit;
    dZ = dZ2 = 0;
    for (i = Npt = 0; i < xvec[0].Nelements; i++, x++, y++, z++, zf++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y)) continue;
      dZ  += (*z - *zf);
      dZ2 += SQ(*z - *zf);
      Npt ++;
    }
    mean  = dZ / Npt;
    sigma = sqrt (fabs(dZ2/Npt - SQ(mean)));
    maxsigma = ClipNSigma * sigma;

    if (VERBOSE) gprint (GP_ERR, "mean: %g, sigma: %g, maxsigma: %g\n", mean, sigma, maxsigma);

    /* mask outlier points */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    z  = zvec[0].elements.Flt;
    zf = zfit;
    Nmask = 0;
    for (i = 0; ClipNSigma && (i < xvec[0].Nelements); i++, x++, y++, z++, zf++) {
      dZ = (*z - *zf);
      if (fabs(dZ) > maxsigma) {
	mask[i] = TRUE;
	Nmask ++;
      } else {
	mask[i] = FALSE;
      }	
    }
    if (VERBOSE) gprint (GP_ERR, "pass: %d, Nmask: %d\n", N, Nmask);
  }

  if (!Quiet) gprint (GP_ERR, "z = ");
  for (N = ny = 0; ny < nterm; ny++) {
    for (nx = 0; nx < nterm - ny; nx++, N++) {
      sprintf (name, "CX%dY%d", nx, ny);
      set_variable (name, b[N][0]);
      if (!Quiet) gprint (GP_ERR, "%f x^%d y^%d  ", b[N][0], nx, ny);
    }
  }
  sprintf (name, "Cnn");
  set_variable (name, (double) order);
  
  if (!Quiet) gprint (GP_ERR, "\n");

  /* free internal data */
  free (zfit);
  free (mask);

  for (i = 0; i < wterm; i++) {
    free (c[i]);
    free (b[i]);
  }
  free (b);
  free (c);

  for (i = 0; i < mterm; i++) {
    free (s[i]);
  }
  free (s);

  return (TRUE);
}

int fit2d_full (int argc, char **argv) {
  
  int N;
  char name[64];

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  int Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  float ClipNSigma = 0;
  int ClipNiter  = 1;
  if ((N = get_argument (argc, argv, "-clip"))) {
    remove_argument (N, &argc, argv);
    ClipNSigma = atof(argv[N]);
    remove_argument (N, &argc, argv);
    ClipNiter  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  opihi_flt *dz = NULL;
  Vector *dzvec = NULL;
  int Weight = FALSE;
  if ((N = get_argument (argc, argv, "-dz"))) {
    remove_argument (N, &argc, argv);
    if ((dzvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
    Weight = TRUE;
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: fit2d_full x y z order [-dz wt] [-quiet/-q] [-clip Nsigma Niter]\n");
    return (FALSE);
  }

  Vector *xvec, *yvec, *zvec;
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  if (xvec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);
  CastVector (zvec, OPIHI_FLT);

  if (Weight) {
    CastVector (dzvec, OPIHI_FLT);
    if (xvec[0].Nelements != dzvec[0].Nelements) {
      gprint (GP_ERR, "vectors must have same length\n");
      return (FALSE);
    }
  }
  
  int order = atof (argv[4]);
  int nterm = SQ(order + 1);

  ALLOCATE_PTR (zfit, opihi_flt, xvec[0].Nelements);
  ALLOCATE_PTR (mask, char, xvec[0].Nelements);
  memset (mask, 0, xvec[0].Nelements);

  /* allocate the summation matrices */
  ALLOCATE_PTR (b, double *, nterm);
  ALLOCATE_PTR (c, double *, nterm);
  for (int i = 0; i < nterm; i++) {
    ALLOCATE (c[i], double, nterm);
    ALLOCATE (b[i], double, 1);
  }

  for (N = 0; N < ClipNiter; N++) {

    /* init registers for current pass */
    for (int i = 0; i < nterm; i++) {
      memset (c[i], 0, nterm*sizeof(double));
      memset (b[i], 0, sizeof(double));
    }

    opihi_flt *x = xvec[0].elements.Flt;
    opihi_flt *y = yvec[0].elements.Flt;
    opihi_flt *z = zvec[0].elements.Flt;
    if (Weight) dz = dzvec[0].elements.Flt;

    /* add up the x,y values */
    for (int i = 0; i < xvec[0].Nelements; i++, x++, y++, z++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y)) continue;
      double dZ = 1.0;
      if (Weight) { 
	dZ = 1.0 / SQ(*dz);
	dz ++;
      }

      int n = 0;
      double iY = 1*dZ;
      double iX = iY;

      for (int iy = 0; iy <= order; iy++) {
	iX = iY;
	for (int ix = 0; ix <= order; ix++) {
	  b[n][0] += iX * (*z);

	  int m = 0;
	  double jY = 1;
	  double jX = jY;

	  for (int jy = 0; jy <= order; jy++) {
	    jX = jY;
	    for (int jx = 0; jx <= order; jx++) {
	      c[n][m] += iX*jX;
	      jX = jX * (*x);
	      m ++;
	    }
	    jY = jY * (*y);
	  }
	  iX = iX * (*x);
	  n ++;
	}
	iY = iY * (*y);
      }
    }

    /** test print **/
    if (VERBOSE) {
      for (int i = 0; i < nterm; i++) {
	for (int j = 0; j < nterm; j++) {
	  gprint (GP_ERR, "%g  ", c[i][j]);
	}
	gprint (GP_ERR, "\n");
      }
      gprint (GP_ERR, "-----\n");
    }

    dgaussjordan (c, b, nterm, 1);

    /** test print **/
    if (VERBOSE) {
      int n = 0;
      for (int ny = 0; ny <= order; ny++) {
	for (int nx = 0; nx <= order; nx++) {
	  gprint (GP_ERR, "x^%d y^%d: %g\n", nx, ny, b[n][0]);
	  n ++;
	}
      }
    }

    /** test print **/
    if (VERBOSE) {
      for (int i = 0; i < nterm; i++) {
	for (int j = 0; j < nterm; j++) {
	  gprint (GP_ERR, "%g  ", c[i][j]);
	}
	gprint (GP_ERR, "\n");
      }
      gprint (GP_ERR, "-----\n");
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
    opihi_flt *zf = zfit;
    for (int n = 0; n < xvec[0].Nelements; n++, x++, y++, zf++) {
      if (!finite(*x) || !finite(*y)) continue;
      *zf = 0;
      double Y = 1;
      double X = Y;

      int n = 0;
      for (int ny = 0; ny <= order; ny++) {
	X = Y;
	for (int nx = 0; nx <= order; nx++) {
	  *zf += b[n][0]*X;
	  X = X * (*x);
	  n ++;
	}
	Y = Y * (*y);
      }
    }

    /* measure fit residual scatter */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    z  = zvec[0].elements.Flt;
    zf = zfit;
    double dZ = 0.0;
    double dZ2 = 0.0;
    int Npt = 0;
    for (int i = 0, Npt = 0; i < xvec[0].Nelements; i++, x++, y++, z++, zf++) {
      if (mask[i]) continue;
      if (!finite(*x) || !finite(*y)) continue;
      dZ  += (*z - *zf);
      dZ2 += SQ(*z - *zf);
      Npt ++;
    }
    double mean  = dZ / Npt;
    double sigma = sqrt (fabs(dZ2/Npt - SQ(mean)));
    double maxsigma = ClipNSigma * sigma;

    if (VERBOSE) gprint (GP_ERR, "mean: %g, sigma: %g, maxsigma: %g\n", mean, sigma, maxsigma);

    /* mask outlier points */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    z  = zvec[0].elements.Flt;
    zf = zfit;
    int Nmask = 0;
    for (int i = 0; ClipNSigma && (i < xvec[0].Nelements); i++, x++, y++, z++, zf++) {
      dZ = (*z - *zf);
      if (fabs(dZ) > maxsigma) {
	mask[i] = TRUE;
	Nmask ++;
      } else {
	mask[i] = FALSE;
      }	
    }
    if (VERBOSE) gprint (GP_ERR, "pass: %d, Nmask: %d\n", N, Nmask);
  }

  if (!Quiet) gprint (GP_ERR, "z = ");
  int n = 0;
  for (int ny = 0; ny <= order; ny++) {
    for (int nx = 0; nx <= order; nx++) {
      sprintf (name, "CX%dY%d", nx, ny);
      set_variable (name, b[n][0]);
      if (!Quiet) gprint (GP_ERR, "%f x^%d y^%d  ", b[n][0], nx, ny);
      n++;
    }
  }
  sprintf (name, "Cnn");
  set_variable (name, (double) order);
  
  if (!Quiet) gprint (GP_ERR, "\n");

  /* free internal data */
  free (zfit);
  free (mask);

  for (int i = 0; i < nterm; i++) {
    free (c[i]);
    free (b[i]);
  }
  free (b);
  free (c);

  return (TRUE);
}
