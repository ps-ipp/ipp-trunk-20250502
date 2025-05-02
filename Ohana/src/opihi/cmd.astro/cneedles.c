# include "data.h"

// generate needle plots for (x,y) (dx,dy)
int cneedles (int argc, char **argv) {
  
  int i, N, kapa, Npts, valid, size;
  opihi_flt *x, *y, *r, *d, *dR, *dD, Rmin, Rmax;
  Vector Xvec, Yvec, *xvec, *yvec, *dxvec, *dyvec;
  Graphdata graphmode;

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
    gprint (GP_ERR, "USAGE: needles <ra> <dec> <dR> <dD> [-scale scale] [style]\n");
    gprint (GP_ERR, "   OR: needles <ra> <dec> <dR> <dD> [-scale scale] [style] where (condition)\n");
    return (FALSE);
  }

  // tvec is used for logical test (truth vector)
  Vector *tvec = NULL;
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

  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;

  /* find vectors */
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dxvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((dyvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (xvec, FALSE); 
  REQUIRE_VECTOR_FLT (yvec, FALSE); 
  REQUIRE_VECTOR_FLT (dxvec, FALSE); 
  REQUIRE_VECTOR_FLT (dyvec, FALSE); 

  if (xvec->Nelements != yvec->Nelements) { gprint (GP_ERR, "vectors are not the same length\n"); return (FALSE); }
  if (dxvec->Nelements != xvec->Nelements) { gprint (GP_ERR, "vectors are not the same length\n"); return (FALSE); }
  if (dyvec->Nelements != xvec->Nelements) { gprint (GP_ERR, "vectors are not the same length\n"); return (FALSE); }

  if (tvec && tvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "logic test vector not the same length as data vectors\n");
    DeleteVector (tvec);
    return (FALSE);
  }

  SetVector (&Xvec, OPIHI_FLT, 2*xvec[0].Nelements);
  SetVector (&Yvec, OPIHI_FLT, 2*xvec[0].Nelements);
  
  // input vectors in r,d space
  r  =  xvec[0].elements.Flt;
  d  =  yvec[0].elements.Flt;
  dR = dxvec[0].elements.Flt;
  dD = dyvec[0].elements.Flt;

  // output vectors after projection & offsets
  x = Xvec.elements.Flt;
  y = Yvec.elements.Flt;
  
  Npts = 0;
  for (i = 0; i < xvec->Nelements; i++, r++, d++, dR++, dD++) {
    if (tvec) {
      int skip = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0.0);
      if (skip) continue;
    }

    double dec = *d;
    double ra = ohana_normalize_angle (*r);
    while (ra < Rmin) ra += 360.0;
    while (ra > Rmax) ra -= 360.0;

    double X1, Y1;
    int status1 = RD_to_XY (&X1, &Y1, ra, dec, &graphmode.coords);

    float rescale = cos(dec*RAD_DEG);
    if (fabs(rescale) < 0.01) {
      rescale = 0.01;
    }
    dec = dec + *dD * scale;
    ra  = ra  + *dR * scale / rescale;

    double X2, Y2;
    int status2 = RD_to_XY (&X2, &Y2, ra, dec, &graphmode.coords);

    if (!status1 || !status2) continue;

    *x = X1;
    *y = Y1;
    x++;
    y++;
    Npts++;

    *x = X2;
    *y = Y2;
    x++;
    y++;
    Npts++;
  }
  Xvec.Nelements = Npts;
  Yvec.Nelements = Npts;

  graphmode.etype = 0;
  graphmode.ptype = 100;
  PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  
  free (Xvec.elements.Ptr);
  free (Yvec.elements.Ptr);
    
  if (tvec) DeleteVector (tvec);

  return (TRUE);
}
