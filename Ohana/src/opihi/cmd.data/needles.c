# include "data.h"

// generate needle plots for (x,y) (dx,dy)
int needles (int argc, char **argv) {
  
  int kapa, N, Npts, valid, size, i;
  Graphdata graphmode;
  Vector *xvec, *yvec, *dxvec, *dyvec;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  float scale = 1.0;
  if ((N = get_argument (argc, argv, "-scale"))) {
    remove_argument (N, &argc, argv);
    scale = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }    

  valid  = (argc == 5);
  valid |= (argc > 6) && !strcmp (argv[5], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: needles <x> <y> <dx> <dy> [-scale scale] [style]\n");
    gprint (GP_ERR, "   OR: needles <x> <y> <dx> <dy> [-scale scale] [style] where (condition)\n");
    return (FALSE);
  }

  // tvec is used for logical test (truth vector)
  Vector *tvec = NULL;
  char *mask = NULL;
  if (argc > 6) {
    char *out = dvomath (argc - 6, &argv[6], &size, 1);
    if (out == NULL) {
      print_error ();
      return FALSE;
    }
    if ((tvec = SelectVector (out, OLDVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, " invalid logic result\n");
      DeleteNamedVector (out);
      free (out);
      return (FALSE);
    }
  }

  graphmode.etype = 0;
  graphmode.ptype = 100;
  
  /* find vectors */
  if ((xvec  = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec  = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dxvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dyvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (tvec && tvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "logic test vector not the same length as data vectors\n");
    return (FALSE);
  }
  if (dxvec->Nelements != xvec->Nelements) goto mismatch;
  if (dyvec->Nelements != xvec->Nelements) goto mismatch;

  Npts = xvec[0].Nelements;
  if (Npts == 0) {
    if (tvec) DeleteVector (tvec);
    return (TRUE);
  }

  if (tvec) {
    Npts = 0;
    ALLOCATE (mask, char, tvec->Nelements);
    for (i = 0; i < tvec->Nelements; i++) {
      mask[i] = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0.0);
      if (!mask[i]) Npts ++;
    }
    if (Npts == 0) {
      DeleteVector (tvec);
      free (mask);
      return TRUE;
    }
  }
  Npts = 2*Npts;

  // we need to generate the 2x length vectors
  Vector *xfull = InitVector();
  Vector *yfull = InitVector();
  ResetVector (xfull, OPIHI_FLT, Npts);
  ResetVector (yfull, OPIHI_FLT, Npts);

  N = 0;
  for (i = 0; i < xvec->Nelements; i++) {
    if (mask && mask[i]) continue;
    xfull->elements.Flt[N] = xvec->elements.Flt[i];
    yfull->elements.Flt[N] = yvec->elements.Flt[i];
    N ++;
    myAssert (N <= Npts, "oops");
    xfull->elements.Flt[N] = xvec->elements.Flt[i] + scale*dxvec->elements.Flt[i];
    yfull->elements.Flt[N] = yvec->elements.Flt[i] + scale*dyvec->elements.Flt[i];
    N ++;
    myAssert (N <= Npts, "oops");
  }    
  myAssert (N == Npts, "oops");

  if (!KapaPrepPlot (kapa, Npts, &graphmode)) return (FALSE);
  PlotVectorSingle (kapa, xfull, NULL, "x");
  PlotVectorSingle (kapa, yfull, NULL, "y");

  if (tvec) {
    free (mask);
    DeleteVector (tvec);
  }

  FreeVector (xfull);
  FreeVector (yfull);

  return (TRUE);

mismatch:
  gprint (GP_ERR, "x,y and dx,dy lengths are mismatched\n");
  return (FALSE);
}
