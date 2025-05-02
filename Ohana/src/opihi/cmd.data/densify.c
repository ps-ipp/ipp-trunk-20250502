# include "data.h"

# define CHECKVAL(ARG) if (!isfinite(ARG)) { gprint (GP_ERR, "illegal value for %s: %f\n", #ARG, ARG); return (FALSE); }
enum {IS_DOT, IS_SQUARE, IS_CIRCLE, IS_GAUSS};

int densify (int argc, char **argv) {

  int i, Nx, Ny, Xb, Yb, ix, iy, N, Xpix, Ypix, good, UseGraph;
  double Xmin, Xmax, dX, Ymin, Ymax, dY;
  float *val;
  Buffer *bf;
  Vector *vx, *vy;
  opihi_flt *x, *y;

  int Normalize = TRUE;
  if ((N = get_argument (argc, argv, "-raw"))) {
    remove_argument (N, &argc, argv);
    Normalize = FALSE;
  }

  UseGraph = FALSE;
  if ((N = get_argument (argc, argv, "-graph"))) {
    remove_argument (N, &argc, argv);
    UseGraph = TRUE;
  }

  Vector *vv = NULL;
  if ((N = get_argument (argc, argv, "-value"))) {
    remove_argument (N, &argc, argv);
    if ((vv = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE); 
    remove_argument (N, &argc, argv);
  }

  float scale = 0.0;
  if ((N = get_argument (argc, argv, "-scale"))) {
    remove_argument (N, &argc, argv);
    scale = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int binning = 1;
  if ((N = get_argument (argc, argv, "-binning"))) {
    if (!UseGraph) {
      gprint (GP_ERR, "-binning only valid for -graph option\n");
      return FALSE;
    }
    remove_argument (N, &argc, argv);
    binning = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int PSFTYPE = IS_DOT;
  if ((N = get_argument (argc, argv, "-psf"))) {
    remove_argument (N, &argc, argv);
    if (!strcasecmp(argv[N], "dot"))    PSFTYPE = IS_DOT;
    if (!strcasecmp(argv[N], "square")) PSFTYPE = IS_SQUARE;
    if (!strcasecmp(argv[N], "circle")) PSFTYPE = IS_CIRCLE;
    if (!strcasecmp(argv[N], "gauss"))  PSFTYPE = IS_GAUSS;
    remove_argument (N, &argc, argv);
  }

  good = UseGraph ? (argc == 4) : (argc == 10);
  if (!good) {
    gprint (GP_ERR, "USAGE: densify buffer x y Xmin Xmax dX Ymin Ymax dY\n");
    gprint (GP_ERR, "   OR: densify buffer x y -graph\n");
    gprint (GP_ERR, " option: -psf [dot] (circle) (square) (gauss)\n");
    gprint (GP_ERR, " other options:\n");
    gprint (GP_ERR, "   -raw : do not renormalize PSF\n");
    gprint (GP_ERR, "   -scale : spatial scale factor for PSF (radius, half-width, or sigma)\n");
    gprint (GP_ERR, "   -value (vector) : multiply sum by the given vector\n");
    return (FALSE);
  }
  
  if ((bf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vx = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vy = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (vx[0].Nelements != vy[0].Nelements) return (FALSE);

  if (vv) {
    if (vv[0].Nelements != vx[0].Nelements) {
      gprint (GP_ERR, "mis-match in vector lengths\n");
      return FALSE;
    }
  }

  REQUIRE_VECTOR_FLT (vx, FALSE); 
  REQUIRE_VECTOR_FLT (vy, FALSE); 

  if (UseGraph) {
    int kapa;
    Graphdata graphmode;
    if (!GetGraph (&graphmode, &kapa, NULL)) return (FALSE);
    KapaGetImageRange (kapa, &Xmin, &Xmax, &Ymax, &Ymin, &Xpix, &Ypix);
    Xmax = graphmode.xmax;
    Xmin = graphmode.xmin;
    Ymax = graphmode.ymax;
    Ymin = graphmode.ymin;
    dX = binning * (Xmax - Xmin) / (Xpix - 1);
    dY = binning * (Ymax - Ymin) / (Ypix - 1);
  } else {
    Xmin = atof (argv[4]);
    Xmax = atof (argv[5]);
    dX   = atof (argv[6]);

    Ymin = atof (argv[7]);
    Ymax = atof (argv[8]);
    dY   = atof (argv[9]);
  }

  CHECKVAL(Xmin);
  CHECKVAL(Xmax);
  CHECKVAL(dX);

  CHECKVAL(Ymin);
  CHECKVAL(Ymax);
  CHECKVAL(dY);

  float scaleX = (scale > 0.0) ? scale / dX : 3.0;
  float scaleY = (scale > 0.0) ? scale / dY : 3.0;

  Nx = abs((Xmax - Xmin) / dX) + 1;
  Ny = abs((Ymax - Ymin) / dY) + 1;
  
  gfits_free_matrix (&bf[0].matrix);
  gfits_free_header (&bf[0].header);
  if (!CreateBuffer (bf, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
  strcpy (bf[0].file, "(empty)");
  
  float scale2 = (scaleX + 1.0) * (scaleY + 1.0);
  float fSquare = 1.0 / scale2;
  float fCircle = 1.0 / (3.141592 * scale2);
  float fSigma  = 0.5 / scale2;
  float fGauss  = 1.0 / (2.0 * 3.141592 * scale2);

  x = vx[0].elements.Flt;
  y = vy[0].elements.Flt;

  opihi_flt *Fs = vv ? vv[0].elements.Flt : NULL;
  opihi_int *Is = vv ? vv[0].elements.Int : NULL;
  int isFloatScale = (vv && vv[0].type == OPIHI_FLT);

  val = (float *)bf[0].matrix.buffer;
  for (i = 0; i < vx[0].Nelements; i++, x++, y++) {
    Xb = (*x - Xmin) / dX;
    Yb = (*y - Ymin) / dY;

    float F = 1.0;
    if (vv) { F = isFloatScale ? Fs[i] : Is[i]; }

    switch (PSFTYPE) {
      case IS_DOT:
	if (Xb >= Nx) continue;
	if (Yb >= Ny) continue;
	if (Xb < 0) continue;
	if (Yb < 0) continue;
	if (vv) {
	  val[Xb + Yb*Nx] += F;
	} else {
	  val[Xb + Yb*Nx] ++;
	}
	break;
      case IS_SQUARE:
	for (ix = Xb - scaleX; ix <= Xb + scaleX; ix++) {
	  for (iy = Yb - scaleY; iy <= Yb + scaleY; iy++) {
	    if (ix >= Nx) continue;
	    if (iy >= Ny) continue;
	    if (ix < 0) continue;
	    if (iy < 0) continue;
	    if (vv) {
	      val[ix + iy*Nx] += Normalize ? fSquare*F : F;
	    } else {
	      val[ix + iy*Nx] += Normalize ? fSquare : 1.0;
	    }
	  }
	}
	break;
      case IS_CIRCLE:
	for (ix = Xb - scaleX; ix <= Xb + scaleX; ix++) {
	  float dX = ix - Xb;
	  for (iy = Yb - scaleY; iy <= Yb + scaleY; iy++) {
	    float dY = iy - Yb;
	    float r2 = dX*dX + dY*dY;
	    if (r2 > 9) continue;
	    if (ix >= Nx) continue;
	    if (iy >= Ny) continue;
	    if (ix < 0) continue;
	    if (iy < 0) continue;
	    if (vv) {
	      val[Xb + Yb*Nx] += Normalize ? fCircle*F : F;
	    } else {
	      val[Xb + Yb*Nx] += Normalize ? fCircle : 1.0;
	    }
	  }
	}
	break;
      case IS_GAUSS:
	for (ix = Xb - scaleX; ix <= Xb + scaleX; ix++) {
	  float dX = ix - Xb;
	  for (iy = Yb - scaleY; iy <= Yb + scaleY; iy++) {
	    float dY = iy - Yb;
	    float r2 = dX*dX + dY*dY;
	    if (ix >= Nx) continue;
	    if (iy >= Ny) continue;
	    if (ix < 0) continue;
	    if (iy < 0) continue;
	    if (vv) {
	      val[Xb + Yb*Nx] += F*fGauss*exp(-fSigma*r2);
	    } else {
	      val[Xb + Yb*Nx] += fGauss*exp(-fSigma*r2);
	    }
	  }
	}
	break;
    }
  }
  return (TRUE);
}
