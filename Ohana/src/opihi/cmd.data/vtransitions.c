# include "data.h"

static int isFltX = FALSE;
static int isFltY = FALSE;
static double interpolate_position (Vector *vecx, Vector *vecy, int bin, float value);

int vtransitions (int argc, char **argv) {
  
  int N;
  Vector *vecx, *vecy, *outbin, *outval, *outpos;

  int QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  int UP_TRANSITIONS = TRUE;
  if ((N = get_argument (argc, argv, "-dn"))) {
    UP_TRANSITIONS = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-down"))) {
    UP_TRANSITIONS = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-up"))) {
    UP_TRANSITIONS = TRUE;
    remove_argument (N, &argc, argv);
  }

  int BinMin = -1;
  int BinMax = -1;
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    BinMin = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    BinMax = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: vtransitions <x> <y> (value) (-up|-down,-dn) [-q] [-range BinMin BinMax]\n");
    gprint (GP_ERR, "  find the x coordinates at which we pass the specified value going up\n");
    gprint (GP_ERR, "  -q : quiet mode\n");
    gprint (GP_ERR, "  -r : reverse (find downward transition)\n");
    return (FALSE);
  }
  
  if ((vecx = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (vecx[0].Nelements != vecy[0].Nelements) return FALSE;
  int Nelements = vecx[0].Nelements;
  
  if ((outbin = SelectVector ("vtr_bin", ANYVECTOR, TRUE)) == NULL) return (FALSE); // bins containing transitions
  if ((outval = SelectVector ("vtr_val", ANYVECTOR, TRUE)) == NULL) return (FALSE); // value of bins containing transitions
  if ((outpos = SelectVector ("vtr_pos", ANYVECTOR, TRUE)) == NULL) return (FALSE); // interpolated coordinate of transitions

  double value = atof (argv[3]);
  
  isFltX = (vecx[0].type == OPIHI_FLT);
  isFltY = (vecy[0].type == OPIHI_FLT);

  // BinMin = -1 -> range not provide
  if (BinMin == -1) {
    BinMin = 0;
    BinMax = Nelements - 1;
  } else {
    if ((BinMin >= vecx[0].Nelements) || (BinMin < -1*vecx[0].Nelements)) {
      gprint (GP_ERR, "vector subscript out of range\n"); 
      return FALSE;
    }
    if (BinMin < 0) BinMin += vecx[0].Nelements;
    if ((BinMax >= vecx[0].Nelements) || (BinMax < -1*vecx[0].Nelements)) {
      gprint (GP_ERR, "vector subscript out of range\n"); 
      return FALSE;
    }
    if (BinMax < 0) BinMax += vecx[0].Nelements;
  }

  int Nout = 0;
  int NOUT = 100;
  ResetVector (outbin, OPIHI_INT,    NOUT);
  ResetVector (outval, vecy[0].type, NOUT);
  ResetVector (outpos, OPIHI_FLT,    NOUT);
  
  // starting disposition
  int isUp = isFltY ? (vecy[0].elements.Flt[BinMin] > value) : (vecy[0].elements.Int[BinMin] > value);

  for (int i = BinMin; i <= BinMax; i++) {
    double testval = isFltY ? vecy[0].elements.Flt[i] : vecy[0].elements.Int[i];
    if (isUp && (testval < value)) {
      isUp = FALSE;
      if (!UP_TRANSITIONS) {
	// save this value
	outbin->elements.Int[Nout] = i;
	if (isFltY) {
	  outval->elements.Flt[Nout] = testval;
	} else {
	  outval->elements.Int[Nout] = testval;
	}
	outpos->elements.Flt[Nout] = interpolate_position (vecx, vecy, i, value);
	Nout ++;
	if (Nout == NOUT) {
	  NOUT += 100;
	  ResetVector (outbin, OPIHI_INT,    NOUT);
	  ResetVector (outval, vecy[0].type, NOUT);
	  ResetVector (outpos, OPIHI_FLT,    NOUT);
	}
      }
      continue;
    }
    if (!isUp && (testval > value)) {
      isUp = TRUE;
      if (UP_TRANSITIONS) {
	// save this value
	outbin->elements.Int[Nout] = i;
	if (isFltY) {
	  outval->elements.Flt[Nout] = testval;
	} else {
	  outval->elements.Int[Nout] = testval;
	}
	outpos->elements.Flt[Nout] = interpolate_position (vecx, vecy, i, value);
	Nout ++;
	if (Nout == NOUT) {
	  NOUT += 100;
	  ResetVector (outbin, OPIHI_INT,    NOUT);
	  ResetVector (outval, vecy[0].type, NOUT);
	  ResetVector (outpos, OPIHI_FLT,    NOUT);
	}
      }    
    }
  }

  ResetVector (outbin, OPIHI_INT,    Nout);
  ResetVector (outval, vecy[0].type, Nout);
  ResetVector (outpos, OPIHI_FLT,    Nout);

  if (!QUIET) gprint (GP_LOG, "found %d transitions\n", Nout);

  return (TRUE);
}

static double interpolate_position (Vector *vecx, Vector *vecy, int bin, float value) {

  double x0, x1, y0, y1, Xvalue;

  if (vecy[0].Nelements == 0) return NAN;
  if (vecy[0].Nelements == 1) return isFltY ? vecy[0].elements.Flt[0] : vecy[0].elements.Int[0];

  if (bin == 0) {
    // interpolate to value:
    y0 = isFltY ? vecy[0].elements.Flt[bin] : vecy[0].elements.Int[bin];
    y1 = isFltY ? vecy[0].elements.Flt[bin+1]   : vecy[0].elements.Int[bin+1];
    x0 = isFltX ? vecx[0].elements.Flt[bin] : vecy[0].elements.Int[bin];
    x1 = isFltX ? vecx[0].elements.Flt[bin+1]   : vecy[0].elements.Int[bin+1];
    if (y0 == y1) {
      Xvalue = 0.5*(x0 + x1);
    } else {
      Xvalue = (value - y0) * (x1 - x0) / (y1 - y0) + x0;
    }    
  } else {
    // interpolate to value:
    y0 = isFltY ? vecy[0].elements.Flt[bin-1] : vecy[0].elements.Int[bin-1];
    y1 = isFltY ? vecy[0].elements.Flt[bin]   : vecy[0].elements.Int[bin];
    x0 = isFltX ? vecx[0].elements.Flt[bin-1] : vecy[0].elements.Int[bin-1];
    x1 = isFltX ? vecx[0].elements.Flt[bin]   : vecy[0].elements.Int[bin];
    if (y0 == y1) {
      Xvalue = 0.5*(x0 + x1);
    } else {
      Xvalue = (value - y0) * (x1 - x0) / (y1 - y0) + x0;
    }
  }
  return Xvalue;
}
