# include "data.h"

// XXX use 'threshold' to interpolate to a value
int interpolate_presort (int argc, char **argv) {

  int  i, j;
  double x0, dx, dy, y0;
  Vector *xout, *yout, *xin, *yin;

  int N;
  int FillEnds = FALSE;
  if ((N = get_argument (argc, argv, "-fill-ends"))) {
    FillEnds = TRUE;
    remove_argument (N, &argc, argv);
  }

  /** check basic syntax **/
  if (argc != 5) {
    gprint (GP_ERR, "USAGE: interpolate Xi Yi Xo Yo\n");
    gprint (GP_ERR, "  Xi Yi - sorted reference vectors\n");
    gprint (GP_ERR, "  Xo    - output positions (vector)\n");
    gprint (GP_ERR, "  Yo    - output values (vector)\n");
    gprint (GP_ERR, "  (vectors must be pre-sorted)\n");
    gprint (GP_ERR, "  (use 'threshold' to interpolate to a value)\n");
    return (FALSE);
  }

  if ((xin  = SelectVector (argv[1],  OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yin  = SelectVector (argv[2],  OLDVECTOR, TRUE)) == NULL) return (FALSE);

  // target positions are a vector
  if ((xout = SelectVector (argv[3],  OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yout = SelectVector (argv[4],  ANYVECTOR, TRUE)) == NULL) return (FALSE);

  REQUIRE_VECTOR_FLT (xin, FALSE); 
  REQUIRE_VECTOR_FLT (yin, FALSE); 
  REQUIRE_VECTOR_FLT (xout, FALSE); 
  ResetVector (yout, OPIHI_FLT, xout[0].Nelements);

  dx = xin[0].elements.Flt[1] - xin[0].elements.Flt[0];
  dy = yin[0].elements.Flt[1] - yin[0].elements.Flt[0];
  x0 = xin[0].elements.Flt[0];
  y0 = yin[0].elements.Flt[0];
  
  // fill in the start with NANs
  for (i = 0; (i < xout[0].Nelements) && (xout[0].elements.Flt[i] < xin[0].elements.Flt[0]); i++) {
    yout[0].elements.Flt[i] = FillEnds ? yin[0].elements.Flt[0] : NAN;
  }

  // every output pixel should get a value unless 
  // we are below the lowest in or above the highest in
  for (j = 0; (i < xout[0].Nelements) && (j < xin[0].Nelements - 1); ) {

    yout[0].elements.Flt[i] = NAN;

    // are we with range of the current input pixel? (xin[j] <= xout[i] < xin[j+1]) ?
    if (xout[0].elements.Flt[i] >= xin[0].elements.Flt[j+1]) { 
      j ++; 
      continue;
    }
    if (xout[0].elements.Flt[i] < xin[0].elements.Flt[j]) { 
      i ++; 
      continue;
    }

    dx = xin[0].elements.Flt[j+1] - xin[0].elements.Flt[j];
    dy = yin[0].elements.Flt[j+1] - yin[0].elements.Flt[j];
    x0 = xin[0].elements.Flt[j];
    y0 = yin[0].elements.Flt[j];
    // x1 = xin[0].elements.Flt[j+1];

    yout[0].elements.Flt[i] = (dy/dx)*(xout[0].elements.Flt[i] - x0) + y0;
    i ++;
  }

  // fill in the rest with NANs
  int NinLast = xin[0].Nelements - 1;
  while (i < yout[0].Nelements) {
    yout[0].elements.Flt[i] = FillEnds ? yin[0].elements.Flt[NinLast] : NAN;
    i++;
  }

  return (TRUE);
}
