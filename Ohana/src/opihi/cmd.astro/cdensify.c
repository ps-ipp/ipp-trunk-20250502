# include "data.h"

# define CHECKVAL(ARG) if (!isfinite(ARG)) { gprint (GP_ERR, "illegal value for %s: %f\n", #ARG, ARG); return (FALSE); }
enum {IS_DOT, IS_SQUARE, IS_CIRCLE, IS_GAUSS};

int cdensify (int argc, char **argv) {

  int i, Nx, Ny, Xb, Yb, N, Xpix, Ypix;
  double Xmin, Xmax, dX, Ymin, Ymax, dY, ix, iy;
  float *val;
  Buffer *bf;
  Vector *vr, *vd;
  opihi_flt x, y;
  int kapa;
  Graphdata graphmode;

  int Normalize = TRUE;
  if ((N = get_argument (argc, argv, "-raw"))) {
    remove_argument (N, &argc, argv);
    Normalize = FALSE;
  }

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;
  double Rmin = graphmode.coords.crval1 - 182.0;
  double Rmax = graphmode.coords.crval1 + 182.0;

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

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: cdensify buffer R D\n");
    gprint (GP_ERR, " option: -psf [dot] (circle) (square) (gauss)\n");
    gprint (GP_ERR, " other options:\n");
    gprint (GP_ERR, "   -raw : do not renormalize PSF\n");
    gprint (GP_ERR, "   -scale : spatial scale factor for PSF (radius, half-width, or sigma)\n");
    gprint (GP_ERR, "   -value (vector) : multiply sum by the given vector\n");
    return (FALSE);
  }
  
  if ((bf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vr = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vd = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (vr[0].Nelements != vd[0].Nelements) return (FALSE);

  if (vv) {
    if (vv[0].Nelements != vr[0].Nelements) {
      gprint (GP_ERR, "mis-match in vector lengths\n");
      return FALSE;
    }
  }

  REQUIRE_VECTOR_FLT (vr, FALSE); 
  REQUIRE_VECTOR_FLT (vd, FALSE); 

  KapaGetImageRange (kapa, &Xmin, &Xmax, &Ymax, &Ymin, &Xpix, &Ypix);
  Xmax = graphmode.xmax;
  Xmin = graphmode.xmin;
  Ymax = graphmode.ymax;
  Ymin = graphmode.ymin;

  // (dX, dY) are the pixel scale of the output image (output pixels / input pixels)

  dX = binning * (Xmax - Xmin) / (Xpix - 1);
  dY = binning * (Ymax - Ymin) / (Ypix - 1);

  CHECKVAL(Xmin);
  CHECKVAL(Xmax);
  CHECKVAL(dX);

  CHECKVAL(Ymin);
  CHECKVAL(Ymax);
  CHECKVAL(dY);

  Nx = abs((Xmax - Xmin) / dX) + 1;
  Ny = abs((Ymax - Ymin) / dY) + 1;
  
  Coords newcoords = graphmode.coords;
  newcoords.cdelt1 *= dX;
  newcoords.cdelt2 *= dY;
  newcoords.crpix1 = (newcoords.crpix1 - Xmin) / dX;
  newcoords.crpix2 = (newcoords.crpix2 - Ymin) / dY;

  gfits_free_matrix (&bf[0].matrix);
  gfits_free_header (&bf[0].header);
  if (!CreateBuffer (bf, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
  strcpy (bf[0].file, "(empty)");
  PutCoords (&newcoords, &bf[0].header);
  
  float scalescale = scale*scale;
  float scale2 = (scale + 1.0) * (scale + 1.0);
  float fSquare = 1.0 / scale2;
  float fCircle = 1.0 / (3.141592 * scale2);
  float fSigma  = 0.5 / scale2;
  float fGauss  = 1.0 / (2.0 * 3.141592 * scale2);

  // generate the PSF in a local tangent plane
  Coords coords;
  InitCoords (&coords, "DEC--TAN");

  opihi_flt *r = vr[0].elements.Flt;
  opihi_flt *d = vd[0].elements.Flt;

  opihi_flt *Fs = vv ? vv[0].elements.Flt : NULL;
  opihi_int *Is = vv ? vv[0].elements.Int : NULL;
  int isFloatScale = (vv && vv[0].type == OPIHI_FLT);

  val = (float *)bf[0].matrix.buffer;

  for (i = 0; i < vr[0].Nelements; i++, r++, d++) {
    double rn = ohana_normalize_angle (*r);
    while (rn < Rmin) rn += 360.0;
    while (rn > Rmax) rn -= 360.0;
    coords.crval1 = rn;
    coords.crval2 = *d;

    float F = 1.0;
    if (vv) { F = isFloatScale ? Fs[i] : Is[i]; }

    switch (PSFTYPE) {
      case IS_DOT:
	RD_to_XY (&x, &y, rn, *d, &newcoords);
	Xb = (int) x;
	Yb = (int) y;
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
	for (ix = -scale; ix <= scale; ix += dX) {
	  for (iy = -scale; iy <= scale; iy += dY) {
	    double rp, dp;
	    XY_to_RD (&rp, &dp, ix, iy, &coords);
	    while (rp < Rmin) rp += 360.0;
	    while (rp > Rmax) rp -= 360.0;
	    RD_to_XY (&x, &y, rp, dp, &graphmode.coords);
	    Xb = (x - Xmin) / dX;
	    Yb = (y - Ymin) / dY;
	    if (Xb >= Nx) continue;
	    if (Yb >= Ny) continue;
	    if (Xb < 0) continue;
	    if (Yb < 0) continue;
	    if (vv) {
	      val[Xb + Yb*Nx] += Normalize ? fSquare*F : F;
	    } else {
	      val[Xb + Yb*Nx] += Normalize ? fSquare : 1.0;
	    }
	  }
	}
	break;
      case IS_CIRCLE:
	for (ix = -scale; ix <= scale; ix += dX) {
	  for (iy = -scale; iy <= scale; iy += dY) {
	    float r2 = ix*ix + iy*iy;
	    double rp, dp;
	    if (r2 > scalescale) continue;
	    XY_to_RD (&rp, &dp, ix, iy, &coords);
	    while (rp < Rmin) rp += 360.0;
	    while (rp > Rmax) rp -= 360.0;
	    RD_to_XY (&x, &y, rp, dp, &graphmode.coords);
	    Xb = (x - Xmin) / dX;
	    Yb = (y - Ymin) / dY;
	    if (Xb >= Nx) continue;
	    if (Yb >= Ny) continue;
	    if (Xb < 0) continue;
	    if (Yb < 0) continue;
	    if (vv) {
	      val[Xb + Yb*Nx] += Normalize ? fCircle*F : F;
	    } else {
	      val[Xb + Yb*Nx] += Normalize ? fCircle : 1.0;
	    }
	  }
	}
	break;
      case IS_GAUSS:
	for (ix = -3.0*scale; ix <= 3.0*scale; ix += dX) {
	  for (iy = -3.0*scale; iy <= 3.0*scale; iy += dY) {
	    float r2 = ix*ix + iy*iy;
	    double rp, dp;
	    XY_to_RD (&rp, &dp, ix, iy, &coords);
	    while (rp < Rmin) rp += 360.0;
	    while (rp > Rmax) rp -= 360.0;
	    RD_to_XY (&x, &y, rp, dp, &graphmode.coords);
	    Xb = (x - Xmin) / dX;
	    Yb = (y - Ymin) / dY;
	    if (Xb >= Nx) continue;
	    if (Yb >= Ny) continue;
	    if (Xb < 0) continue;
	    if (Yb < 0) continue;
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
