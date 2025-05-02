# include "dvoshell.h"
static float dr[] = {0.0, 1.0, 1.0, 0.0};
static float dd[] = {0.0, 0.0, 1.0, 1.0};

int showtile (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  int kapa, Nd, N, NPTS, status, i, InPic;
  Graphdata graphmode;
  Coords coords;
  Vector Xvec, Yvec;
  float r, d, R, D;
  float Ro[90], Do[90];

  /* show tile pattern in viewed region */
  if (!GetGraph (&graphmode, &kapa, NULL)) return (FALSE);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: showtile [option]\n");
    return (FALSE);
  }
  
  N = 0;
  NPTS = 200;
  SetVector (&Xvec, OPIHI_FLT, NPTS);
  SetVector (&Yvec, OPIHI_FLT, NPTS);

  /* starting position */

  /* reference for coords is this image */
  InitCoords (&coords, "DEC--TAN");
  
  /* fill in top-left region */
  for (r = 0; r < 3; r += 1.0) {
    gprint (GP_ERR, "r: %f\n", r);
    for (Nd = d = 0; d < 90; Nd ++, d += 1.0) {
      if (r == 0) {
	coords.crval1 = r;
	coords.crval2 = d;
      } else {
	coords.crval1 = Ro[Nd];
	coords.crval2 = Do[Nd];
      }
      for (i = 0; i < 4; i++) {
	fXY_to_RD (&R, &D, dr[i], dd[i], &coords);
	status |= RD_to_XY (&Xvec.elements.Flt[N+2*i], &Yvec.elements.Flt[N+2*i], R, D, &graphmode.coords);
	if (i > 0) {
	  Xvec.elements.Flt[N+2*i - 1] = Xvec.elements.Flt[N+2*i];
	  Yvec.elements.Flt[N+2*i - 1] = Yvec.elements.Flt[N+2*i];
	}
	if (i == 1) {
	  Ro[Nd] = R;
	  Do[Nd] = D;
	}
      }
      Xvec.elements.Flt[N+7] = Xvec.elements.Flt[N];
      Yvec.elements.Flt[N+7] = Yvec.elements.Flt[N];

      /* check if any corner is in plotting region */
      InPic = FALSE;
      for (i = 0; i < 8; i+=2) {
	if ((Xvec.elements.Flt[N+i] >= graphmode.xmin) && 
	    (Xvec.elements.Flt[N+i] <= graphmode.xmax) && 
	    (Yvec.elements.Flt[N+i] >= graphmode.ymin) && 
	    (Yvec.elements.Flt[N+i] <= graphmode.ymax))
	  InPic = TRUE;
      }
      if (!InPic) continue;
      N+=8;
      if (N > NPTS - 1) {  /* this is OK because NPTS is made always a multiple of 8 */
	NPTS += 200;
	REALLOCATE (Xvec.elements.Flt, opihi_flt, NPTS);
	REALLOCATE (Yvec.elements.Flt, opihi_flt, NPTS);
      }
    }
  }
  
  Xvec.Nelements = Yvec.Nelements = N;
  if (N > 0) {
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.ptype = KAPA_POINT_PAIR_CONNECT; /* connect pairs of points */
    graphmode.etype = 0;
    PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  }

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);
  return (TRUE);

}
