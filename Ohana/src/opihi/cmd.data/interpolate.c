# include "data.h"

// XXX use 'threshold' to interpolate to a value
int interpolate (int argc, char **argv) {

  int  i, j;
  double x0, x1, dx, dy, y0;
  Vector *xout, *yout, *xin, *yin;

  /** check basic syntax **/
  if (argc != 5) {
    gprint (GP_ERR, "USAGE: interpolate Xi Yi Xo Yo\n");
    gprint (GP_ERR, "  Xi Yi - sorted reference vectors\n");
    gprint (GP_ERR, "  Xo    - output positions (vector)\n");
    gprint (GP_ERR, "  Yo    - output values (vector)\n");
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
  
  /* in vectors are sorted, out vectors are not */
  for (j = 0; j < xin[0].Nelements - 1; j++) {
    dx = xin[0].elements.Flt[j+1] - xin[0].elements.Flt[j];
    dy = yin[0].elements.Flt[j+1] - yin[0].elements.Flt[j];
    x0 = xin[0].elements.Flt[j];
    y0 = yin[0].elements.Flt[j];
    x1 = xin[0].elements.Flt[j+1];
    for (i = 0; i < xout[0].Nelements; i++) {
      if ((xout[0].elements.Flt[i] >= x0) && (xout[0].elements.Flt[i] < x1)) {
	yout[0].elements.Flt[i] = (dy/dx)*(xout[0].elements.Flt[i] - x0) + y0;
      }
      if ((j == 0) && (xout[0].elements.Flt[i] < x0)) {
	yout[0].elements.Flt[i] = (dy/dx)*(xout[0].elements.Flt[i] - x0) + y0;
      }
      if ((j == xin[0].Nelements - 2) && (xout[0].elements.Flt[i] >= x1)) {
	yout[0].elements.Flt[i] = (dy/dx)*(xout[0].elements.Flt[i] - x0) + y0;
      }
    }    
  }

  return (TRUE);
    
}
