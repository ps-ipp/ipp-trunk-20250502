# include "astro.h"

int cplot (int argc, char **argv) {
  
  double ra_prev = 0;
  int i, kapa, Npts, status, leftside, valid, size;
  opihi_flt *x, *y, *r, *d, Rmin, Rmax, Rmid;
  Vector Xvec, Yvec, *xvec, *yvec;
  Graphdata graphmode;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  valid  = (argc == 3);
  valid |= (argc > 4) && !strcmp (argv[3], "where");
  if (!valid) {
    gprint (GP_ERR, "USAGE: cplot <ra> <dec> [style]\n");
    gprint (GP_ERR, "   OR: cplot <ra> <dec> [style] where (condition)\n");
    return (FALSE);
  }

  Rmin = graphmode.coords.crval1 - 182.0;
  Rmax = graphmode.coords.crval1 + 182.0;
  Rmid = 0.5*(Rmin + Rmax);

  /* find vectors */
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (xvec, FALSE); 
  REQUIRE_VECTOR_FLT (yvec, FALSE); 

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors are not the same length\n");
    return (FALSE);
  }

  // tvec is used for logical test (truth vector)
  Vector *tvec = NULL;
  if (argc > 4) {
    char *out = dvomath (argc - 4, &argv[4], &size, 1);
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

  if (tvec && tvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "logic test vector not the same length as data vectors\n");
    DeleteVector (tvec);
    return (FALSE);
  }

  SetVector (&Xvec, OPIHI_FLT, xvec[0].Nelements);
  SetVector (&Yvec, OPIHI_FLT, xvec[0].Nelements);
  
  r = xvec[0].elements.Flt;
  d = yvec[0].elements.Flt;
  x = Xvec.elements.Flt;
  y = Yvec.elements.Flt;
  
  Npts = 0;
  for (i = 0; i < Xvec.Nelements; i++, r++, d++) {
    if (tvec) {
      int skip = (tvec->type == OPIHI_FLT) ? (tvec->elements.Flt[i] == 0.0) : (tvec->elements.Int[i] == 0.0);
      if (skip) continue;
    }

    double ra = ohana_normalize_angle (*r);
    while (ra < Rmin) ra += 360.0;
    while (ra > Rmax) ra -= 360.0;

    // for pair-by-pair connections, check on second point if we straddle the back midline
    if (graphmode.ptype == 100) {
      if (i % 2) {
	leftside = (ra_prev < Rmid); // if first of the pair is left, second must be as well
	if ( leftside && (ra > Rmid + 90)) { ra -= 360.0; }
	if (!leftside && (ra < Rmid - 90)) { ra += 360.0; }
      } else {
	ra_prev = ra;
      }
    }
    status = RD_to_XY (x, y, ra, *d, &graphmode.coords);

    // if we fail on one of the points, drop the corresponding pair
    if (!status) {
      if (graphmode.ptype == 100) {
	if (i % 2) {
	  // for odd points, skip previous also
	  x--;
	  y--;
	  Npts--;
	} else {
	  // for even points, skip previous also
	  i++;
	  r++;
	  d++;
	}
      }
      continue;
    }
    x++;
    y++;
    Npts++;
  }
  Xvec.Nelements = Npts;
  Yvec.Nelements = Npts;

  graphmode.etype = 0;
  PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  
  free (Xvec.elements.Ptr);
  free (Yvec.elements.Ptr);
    
  if (tvec) DeleteVector (tvec);

  return (TRUE);
}

