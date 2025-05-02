# include "data.h"

int fit1d (int argc, char **argv) {
  
  double **c, **b, *s, X, Y, dY, dY2;
  double ClipNSigma, mean, sigma, maxsigma;
  int i, j, Npt, Nmask;
  int N, Weight, Quiet, ClipNiter;
  Vector *xvec, *yvec, *dyvec;
  opihi_flt *x, *y, *dy, *yf, *yfit;
  char name[64], *mask;

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

  dy = NULL;
  dyvec = NULL;
  Weight = FALSE;
  if ((N = get_argument (argc, argv, "-dy"))) {
    remove_argument (N, &argc, argv);
    if ((dyvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
    Weight = TRUE;
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: fit1d x y order [-dy wt] [-quiet/-q] [-clip Nsigma Niter]\n");
    return (FALSE);
  }

  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);    

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors must have same length\n");
    return (FALSE);
  }
  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);

  if (Weight) {
    CastVector (dyvec, OPIHI_FLT);
    if (xvec[0].Nelements != dyvec[0].Nelements) {
      gprint (GP_ERR, "vectors must have same length\n");
      return (FALSE);
    }
  }
 

  /* nterm is number of polynomial terms, starting at x^0 */
  int order = atof (argv[3]);
  int nterm = order + 1;
  int mterm = 2*order + 1;

  ALLOCATE (yfit, opihi_flt, xvec[0].Nelements);
  ALLOCATE (mask, char, xvec[0].Nelements);
  memset (mask, 0, xvec[0].Nelements);

  ALLOCATE (s, double, mterm);
  ALLOCATE (b, double *, nterm);
  ALLOCATE (c, double *, nterm);
  for (i = 0; i < nterm; i++) {
    ALLOCATE (c[i], double, nterm);
    ALLOCATE (b[i], double, 1);
  }

  Nmask = 0;
  sigma = 0.0;

  for (N = 0; N < ClipNiter; N++) {

    /* init registers for current pass */
    memset (s, 0, mterm*sizeof(double));
    for (i = 0; i < nterm; i++) {
      memset (c[i], 0, nterm*sizeof(double));
      memset (b[i], 0, sizeof(double));
    }

    /* perform linear fit */
    x = xvec[0].elements.Flt;
    y = yvec[0].elements.Flt;
    if (Weight) dy = dyvec[0].elements.Flt;

    for (i = 0; i < xvec[0].Nelements; i++, x++, y++) {
      if (mask[i]) continue;
      if (!(finite(*x) && finite(*y))) continue;
      dY = 1.0;
      if (Weight) { 
	dY = 1.0 / SQ(*dy);
	dy ++;
      }
      X = 1*dY;
      Y = *y*dY;
      for (j = 0; j < nterm; j++) {
	s[j] += X;
	b[j][0] += Y;
	X = X * (*x);
	Y = Y * (*x);
      }
      for (j = nterm; j < mterm; j++) {
	s[j] += X;
	X = X * (*x);
      }
    }
    for (i = 0; i < nterm; i++) {
      for (j = 0; j < nterm; j++) {
	c[i][j] = s[i + j];
      }
    }
    if (!dgaussjordan (c, b, nterm, 1)) {
	gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
	goto escape;
    }

    /* generate fitted values */
    x = xvec[0].elements.Flt;
    yf = yfit;
    for (i = 0; i < xvec[0].Nelements; i++, x++, yf++) {
      if (!finite(*x)) continue;
      *yf = 0;
      X = 1;
      for (j = 0; j < order + 1; j++) {
	*yf += b[j][0]*X;
	X = X * (*x);
      }
    }

    /* measure fit residual scatter */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    yf = yfit;
    dY = dY2 = 0;
    for (i = Npt = 0; i < xvec[0].Nelements; i++, x++, y++, yf++) {
      if (mask[i]) continue;
      if (!finite(*x)) continue;
      dY  += (*y - *yf);
      dY2 += SQ(*y - *yf);
      Npt ++;
    }
    mean  = dY / Npt;
    sigma = sqrt (fabs(dY2/Npt - SQ(mean)));
    maxsigma = ClipNSigma * sigma;

    /* mask outlier points */
    x  = xvec[0].elements.Flt;
    y  = yvec[0].elements.Flt;
    yf = yfit;
    Nmask = 0;
    for (i = 0; ClipNSigma && (i < xvec[0].Nelements); i++, x++, y++, yf++) {
      dY = (*y - *yf);
      if (fabs(dY) > maxsigma) {
	mask[i] = TRUE;
	Nmask ++;
      } else {
	mask[i] = FALSE;
      }	
    }
  }
      
  /* print & save basic fit parameters */
  if (!Quiet) gprint (GP_ERR, "y = ");
  for (i = 0; i < nterm; i++) {
    sprintf (name, "C%d", i);
    set_variable (name, b[i][0]);
    if (!Quiet) gprint (GP_ERR, "%f x^%d ", b[i][0], i);
  }
  sprintf (name, "Cn");
  set_variable (name, (double) order);
  
  /* print & save basic fit parameters */
  if (!Quiet) gprint (GP_ERR, "\n");
  if (!Quiet) gprint (GP_ERR, "    ");
  for (i = 0; i < nterm; i++) {
    sprintf (name, "dC%d", i);
    set_variable (name, sqrt(c[i][i]));
    if (!Quiet) gprint (GP_ERR, "%f     ", sqrt(c[i][i]));
  }
  if (!Quiet) gprint (GP_ERR, "\n");

  set_variable ("dC", sigma);
  set_variable ("Cnv", (xvec[0].Nelements - Nmask));

  /* save mask and yfit for testing? */
  if (1) { 
    Vector *fvec, *mvec;
    if ((fvec = SelectVector ("yfit", ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    if ((mvec = SelectVector ("mask", ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    free (fvec[0].elements.Flt);
    fvec[0].elements.Flt = yfit;
    fvec[0].Nelements = xvec[0].Nelements;
    mvec[0].Nelements = xvec[0].Nelements;

    REALLOCATE (mvec[0].elements.Flt, opihi_flt, xvec[0].Nelements);
    for (i = 0; i < xvec[0].Nelements; i++) {
      mvec[0].elements.Flt[i] = mask[i];
    }
  } else {
    free (yfit);
  }
  free (mask);

  for (i = 0; i < nterm; i++) {
    free (b[i]);
    free (c[i]);
  }
  free (b);
  free (c);
  free (s);
  return (TRUE);

escape:
  for (i = 0; i < nterm; i++) {
    free (b[i]);
    free (c[i]);
  }
  free (b);
  free (c);
  free (s);
  return (FALSE);
}
