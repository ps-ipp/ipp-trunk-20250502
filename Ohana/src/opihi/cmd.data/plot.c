# include "data.h"

int plot (int argc, char **argv) {
  
  char *out;
  int kapa, N, Npts, valid, size, i;
  Graphdata graphmode;
  Vector *xvec, *yvec, *dxmvec, *dxpvec, *dymvec, *dypvec;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  /* decide on error bars */
  dxmvec = dxpvec = dymvec = dypvec = NULL;
  if ((N = get_argument (argc, argv, "-dx"))) {
    remove_argument (N, &argc, argv);
    if ((dxmvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+dx"))) {
    remove_argument (N, &argc, argv);
    if ((dxpvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dy"))) {
    remove_argument (N, &argc, argv);
    if ((dymvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+dy"))) {
    remove_argument (N, &argc, argv);
    if ((dypvec = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }

  valid  = (argc == 3);
  valid |= (argc > 4) && !strcmp (argv[3], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: plot <x> <y> [style]\n");
    gprint (GP_ERR, "   OR: plot <x> <y> [style] where (condition)\n");
    return (FALSE);
  }

  // tvec is used for logical test (truth vector)
  Vector *tvec = NULL;
  char *mask = NULL;
  if (argc > 4) {
    out = dvomath (argc - 4, &argv[4], &size, 1);
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

  /* set errorbar mode (these are NOT sticky) */
  graphmode.etype = 0;
  if ((dymvec != NULL) && (dypvec == NULL)) dypvec = dymvec;
  if ((dypvec != NULL) && (dymvec == NULL)) dymvec = dypvec;
  if ((dypvec != NULL) || (dymvec != NULL)) graphmode.etype |= 0x01;
  if ((dxmvec != NULL) && (dxpvec == NULL)) dxpvec = dxmvec;
  if ((dxpvec != NULL) && (dxmvec == NULL)) dxmvec = dxpvec;
  if ((dxpvec != NULL) || (dxmvec != NULL)) graphmode.etype |= 0x02;
  
  /* find vectors */
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (tvec && tvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "logic test vector not the same length as data vectors\n");
    return (FALSE);
  }
  if (dypvec && (dypvec->Nelements != xvec->Nelements)) goto mismatch;
  if (dymvec && (dymvec->Nelements != xvec->Nelements)) goto mismatch;
  if (dxpvec && (dxpvec->Nelements != xvec->Nelements)) goto mismatch;
  if (dxmvec && (dxmvec->Nelements != xvec->Nelements)) goto mismatch;

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

  if (!KapaPrepPlot (kapa, Npts, &graphmode)) return (FALSE);
  
  PlotVectorSingle (kapa, xvec, mask, "x");
  PlotVectorSingle (kapa, yvec, mask, "y");
  if (graphmode.etype & 0x01) {
    PlotVectorSingle (kapa, dymvec, mask, "dym");
    PlotVectorSingle (kapa, dypvec, mask, "dyp");
  }
  if (graphmode.etype & 0x02) {
    PlotVectorSingle (kapa, dxmvec, mask, "dxm");
    PlotVectorSingle (kapa, dxpvec, mask, "dxp");
  }

  if (tvec) {
    free (mask);
    DeleteVector (tvec);
  }
  return (TRUE);

mismatch:
  gprint (GP_ERR, "error and data vector lengths are mismatched\n");
  return (FALSE);
}
