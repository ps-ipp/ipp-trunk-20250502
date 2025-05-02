# include "astro.h"

int czplot (int argc, char **argv) {
  
  char *outname = NULL;
  int i, kapa, valid, size;
  int Npts, status;
  double min, range, Rmin, Rmax;
  opihi_flt *out, *r, *d, *x, *y;
  Vector Xvec, Yvec, Zvec, *xvec, *yvec, *zvec;
  Graphdata graphmode;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  valid  = (argc == 6);
  valid |= (argc > 7) && !strcmp (argv[6], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: czplot <x> <y> <z> min max\n");
    gprint (GP_ERR, "   OR: czplot <x> <y> <z> min max where (condition)\n");
    return (FALSE);
  }

  min = atof(argv[4]);
  range = atof(argv[5]) - min;
  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;

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

  REQUIRE_VECTOR_FLT (xvec, FALSE); 
  REQUIRE_VECTOR_FLT (yvec, FALSE); 
  REQUIRE_VECTOR_FLT (zvec, FALSE); 

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (xvec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[3]);
    return (FALSE);
  }
  if (tvec && (xvec[0].Nelements != tvec[0].Nelements)) {
    gprint (GP_ERR, "logic test vector not the same length as data vectors\n");
    return (FALSE);
  }

  SetVector (&Xvec, OPIHI_FLT, xvec[0].Nelements);
  SetVector (&Yvec, OPIHI_FLT, xvec[0].Nelements);
  SetVector (&Zvec, OPIHI_FLT, xvec[0].Nelements);
  
  if (tvec) {
    ALLOCATE (mask, char, tvec->Nelements);
  }

  r   = xvec[0].elements.Flt;
  d   = yvec[0].elements.Flt;
  x   = Xvec.elements.Flt;
  y   = Yvec.elements.Flt;
  out = Zvec.elements.Flt;

  Npts = 0;
  for (i = 0; i < xvec[0].Nelements; i++, r++, d++) {
    *r = ohana_normalize_angle (*r);
    while (*r < Rmin) *r += 360.0;
    while (*r > Rmax) *r -= 360.0;

    status = RD_to_XY (x, y, *r, *d, &graphmode.coords);
    if (!status) continue;

    if (zvec->type == OPIHI_FLT) {
      opihi_flt *in  = zvec[0].elements.Flt;
      *out = MIN (1.0, MAX (0.01, (in[i] - min) / range));
    } else {
      opihi_int *in  = zvec[0].elements.Int;
      *out = MIN (1.0, MAX (0.01, (in[i] - min) / range));
    }

    if (mask) {
      // NOTE: the X,Y,Z sequence is not (necessarily) the same as the RA,DEC,zvec seq
      // I only set the mask for points sent to kapa...
      mask[Npts] = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0.0);
    }

    x++;
    y++;
    out++;
    Npts++;
  }
  Xvec.Nelements = Npts;
  Yvec.Nelements = Npts;
  Zvec.Nelements = Npts;

  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.size = -1; /* point size determined by Zvec */
  graphmode.etype = 0;
  PlotVectorTriplet (kapa, &Xvec, &Yvec, &Zvec, mask, &graphmode);

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  free (Zvec.elements.Flt);

  if (mask) free (mask);

  return (TRUE);

}

int czcplot (int argc, char **argv) {
  
  char *outname = NULL;
  int i, kapa, valid, size;
  int Npts, status;
  double min, range, Rmin, Rmax;
  opihi_flt *out, *r, *d, *x, *y;
  Vector Xvec, Yvec, Zvec, *xvec, *yvec, *zvec;
  Graphdata graphmode;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  valid  = (argc == 6);
  valid |= (argc > 7) && !strcmp (argv[6], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: czcplot <x> <y> <z> min max\n");
    gprint (GP_ERR, "   OR: czcplot <x> <y> <z> min max where (condition)\n");
    return (FALSE);
  }

  min = atof(argv[4]);
  range = atof(argv[5]) - min;
  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;

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

  REQUIRE_VECTOR_FLT (xvec, FALSE); 
  REQUIRE_VECTOR_FLT (yvec, FALSE); 
  REQUIRE_VECTOR_FLT (zvec, FALSE); 

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[2]);
    return (FALSE);
  }
  if (xvec[0].Nelements != zvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[1], argv[3]);
    return (FALSE);
  }
  if (tvec && (xvec[0].Nelements != tvec[0].Nelements)) {
    gprint (GP_ERR, "logic test vector not the same length as data vectors\n");
    return (FALSE);
  }

  SetVector (&Xvec, OPIHI_FLT, xvec[0].Nelements);
  SetVector (&Yvec, OPIHI_FLT, xvec[0].Nelements);
  SetVector (&Zvec, OPIHI_FLT, xvec[0].Nelements);
  
  if (tvec) {
    ALLOCATE (mask, char, tvec->Nelements);
  }

  r   = xvec[0].elements.Flt;
  d   = yvec[0].elements.Flt;
  x   = Xvec.elements.Flt;
  y   = Yvec.elements.Flt;
  out = Zvec.elements.Flt;

  Npts = 0;
  for (i = 0; i < xvec[0].Nelements; i++, r++, d++) {
    *r = ohana_normalize_angle (*r);
    while (*r < Rmin) *r += 360.0;
    while (*r > Rmax) *r -= 360.0;

    status = RD_to_XY (x, y, *r, *d, &graphmode.coords);
    if (!status) continue;

    if (zvec->type == OPIHI_FLT) {
      opihi_flt *in  = zvec[0].elements.Flt;
      *out = MIN (1.0, MAX (0.01, (in[i] - min) / range));
    } else {
      opihi_int *in  = zvec[0].elements.Int;
      *out = MIN (1.0, MAX (0.01, (in[i] - min) / range));
    }

    if (mask) {
      // NOTE: the X,Y,Z sequence is not (necessarily) the same as the RA,DEC,zvec seq
      // I only set the mask for points sent to kapa...
      mask[Npts] = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0.0);
    }

    x++;
    y++;
    out++;
    Npts++;
  }
  Xvec.Nelements = Npts;
  Yvec.Nelements = Npts;
  Zvec.Nelements = Npts;

  graphmode.style = KAPA_PLOT_POINTS;
  graphmode.color = -1; /* point color determined by Zvec */
  graphmode.etype = 0;
  PlotVectorTriplet (kapa, &Xvec, &Yvec, &Zvec, mask, &graphmode);

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  free (Zvec.elements.Flt);

  if (mask) free (mask);

  return (TRUE);

}
