# include "data.h"

int zplot (int argc, char **argv) {
  
  char *outname = NULL;
  int i, N, kapa, valid, size;
  opihi_flt *out;
  double min, range;
  Graphdata graphmode;
  Vector *xvec, *yvec, *zvec, *dxmvec, *dxpvec, *dymvec, *dypvec, Zvec;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return (FALSE);

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

  valid  = (argc == 6);
  valid |= (argc > 7) && !strcmp (argv[6], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: zplot <x> <y> <z> min max\n");
    gprint (GP_ERR, "   OR: zplot <x> <y> <z> min max where (condition)\n");
    return (FALSE);
  }

  min = atof(argv[4]);
  range = atof(argv[5]) - min;

  // tvec is used for logical test (truth vector)
  Vector *tvec = NULL;
  char *mask = NULL;
  if (argc > 7) {
    outname = dvomath (argc - 7, &argv[7], &size, 1);
    if (outname == NULL) {
      print_error ();
      return FALSE;
    }
    if ((tvec = SelectVector (outname, OLDVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, " invalid logic result\n");
      DeleteNamedVector (outname);
      free (outname);
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
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (xvec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[3]);
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

  SetVector (&Zvec, OPIHI_FLT, zvec[0].Nelements);
  out = Zvec.elements.Flt;
 
  // note actual size is 3.3x plot -sz sizes. (DrawObjects.c:399)
  if (zvec[0].type == OPIHI_FLT) {
    opihi_flt *in = zvec[0].elements.Flt;
    for (i = 0; i < Zvec.Nelements; i++, in++, out++) {
      *out = MIN (1.0, MAX (0.01, (*in - min) / range));
    }
  } else {
    opihi_int *in = zvec[0].elements.Int;
    for (i = 0; i < Zvec.Nelements; i++, in++, out++) {
      *out = MIN (1.0, MAX (0.01, (*in - min) / range));
    }
  }

  if (tvec) {
    ALLOCATE (mask, char, tvec->Nelements);
    for (i = 0; i < tvec->Nelements; i++) {
      mask[i] = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0.0);
    }
  }

  /* point size determined by Zvec */
  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.size = -1; /* point size determined by Zvec */
  PlotVectorTriplet (kapa, xvec, yvec, &Zvec, mask, &graphmode);
  if (graphmode.etype & 0x01) {
    PlotVectorSingle (kapa, dymvec, mask, "dym");
    PlotVectorSingle (kapa, dypvec, mask, "dyp");
  }
  if (graphmode.etype & 0x02) {
    PlotVectorSingle (kapa, dxmvec, mask, "dxm");
    PlotVectorSingle (kapa, dxpvec, mask, "dxp");
  }

  free (Zvec.elements.Ptr);
  if (mask) free (mask);

  if (outname) {
    DeleteNamedVector (outname);
    free (outname);
  }

  return (TRUE);

mismatch:
  gprint (GP_ERR, "error and data vector lengths are mismatched\n");
  if (outname) {
    DeleteNamedVector (outname);
    free (outname);
  }
  return (FALSE);
}

int zcplot (int argc, char **argv) {
  
  char *outname = NULL;
  int i, kapa, valid, size;
  opihi_flt *out;
  double min, range;
  Graphdata graphmode;
  Vector *xvec, *yvec, *zvec, Zvec;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return (FALSE);

  valid  = (argc == 6);
  valid |= (argc > 7) && !strcmp (argv[6], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: zcplot <x> <y> <z> min max\n");
    gprint (GP_ERR, "   OR: zcplot <x> <y> <z> min max where (condition)\n");
    return (FALSE);
  }

  min = atof(argv[4]);
  range = atof(argv[5]) - min;

  // tvec is used for logical test (truth vector)
  Vector *tvec = NULL;
  char *mask = NULL;
  if (argc > 7) {
    outname = dvomath (argc - 7, &argv[7], &size, 1);
    if (outname == NULL) {
      print_error ();
      return FALSE;
    }
    if ((tvec = SelectVector (outname, OLDVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, " invalid logic result\n");
      DeleteNamedVector (outname);
      free (outname);
      return (FALSE);
    }
  }

  /* find vectors */
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((zvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (xvec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[3]);
    return (FALSE);
  }
  if (tvec && tvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "logic test vector not the same length as data vectors\n");
    return (FALSE);
  }
  SetVector (&Zvec, OPIHI_FLT, zvec[0].Nelements);
  out = Zvec.elements.Flt;
 
  if (zvec[0].type == OPIHI_FLT) {
    opihi_flt *in = zvec[0].elements.Flt;
    for (i = 0; i < Zvec.Nelements; i++, in++, out++) {
      *out = MIN (1.0, MAX (0.01, (*in - min) / range));
    }
  } else {
    opihi_int *in = zvec[0].elements.Int;
    for (i = 0; i < Zvec.Nelements; i++, in++, out++) {
      *out = MIN (1.0, MAX (0.01, (*in - min) / range));
    }
  }

  if (tvec) {
    ALLOCATE (mask, char, tvec->Nelements);
    for (i = 0; i < tvec->Nelements; i++) {
      mask[i] = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0.0);
    }
  }

  /* point size determined by Zvec */
  graphmode.style = KAPA_PLOT_POINTS; /* plot points */
  graphmode.color = -1; /* point color determined by Zvec */
  graphmode.etype = 0; /* no errorbars */
  PlotVectorTriplet (kapa, xvec, yvec, &Zvec, mask, &graphmode);

  free (Zvec.elements.Ptr);
  if (mask) free (mask);
  DeleteNamedVector (outname);

  return (TRUE);

}


