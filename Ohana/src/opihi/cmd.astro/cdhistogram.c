# include "data.h"

# define CHECKVAL(ARG) if (!isfinite(ARG)) { gprint (GP_ERR, "illegal value for %s: %f\n", #ARG, ARG); return (FALSE); }

int cdhistogram (int argc, char **argv) {

  int i, Nz, N, Xpix, Ypix;
  double Xmin, Xmax, dX, Ymin, Ymax, dY;
  float *val;
  Buffer *bf;
  Vector *vr, *vd, *vz, *range;
  opihi_flt *r, *d, *z, x, y;
  int kapa;
  Graphdata graphmode;

  int binning = 1;
  if ((N = get_argument (argc, argv, "-binning"))) {
    remove_argument (N, &argc, argv);
    binning = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  range = NULL;
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    if ((range = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;
  double Rmin = graphmode.coords.crval1 - 182.0;
  double Rmax = graphmode.coords.crval1 + 182.0;

  float dZ = NAN;
  if ((N = get_argument (argc, argv, "-delta"))) {
    remove_argument (N, &argc, argv);
    dZ = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: cdhistogram buffer R D value (min) (max) [-delta dval]\n");
    gprint (GP_ERR, " output buffer is 3D\n");
    return (FALSE);
  }
  
  if ((bf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vr = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vd = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vz = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (vr[0].Nelements != vd[0].Nelements) return (FALSE);
  if (vr[0].Nelements != vz[0].Nelements) return (FALSE);

  float Zmin = atof(argv[5]);
  float Zmax = atof(argv[6]);
  if (!isfinite(dZ)) {
    Nz = 100;
    dZ = (Zmax - Zmin) / (Nz - 1);
  } else {
    Nz = (Zmax - Zmin) / dZ + 1;
  }
  if (dZ < 0) {
    gprint (GP_ERR, "invalid value for delta: %f\n", dZ);
    return (FALSE);
  }
  if (Nz > 1000) {
    gprint (GP_ERR, "warning: delta of %f will result in %d histogram bins\n", dZ, Nz);
    return (FALSE);
  }

  if (range) {
    ResetVector (range, OPIHI_FLT, Nz);
    for (i = 0; i < range[0].Nelements; i++) {
      range[0].elements.Flt[i] = Zmin + i*dZ;
    }
  }

  REQUIRE_VECTOR_FLT (vr, FALSE); 
  REQUIRE_VECTOR_FLT (vd, FALSE); 
  REQUIRE_VECTOR_FLT (vz, FALSE); 

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

  int Nx = abs((Xmax - Xmin) / dX) + 1;
  int Ny = abs((Ymax - Ymin) / dY) + 1;
  
  Coords newcoords = graphmode.coords;
  newcoords.cdelt1 *= dX;
  newcoords.cdelt2 *= dY;
  newcoords.crpix1 = (newcoords.crpix1 - Xmin) / dX;
  newcoords.crpix2 = (newcoords.crpix2 - Ymin) / dY;

  gfits_free_matrix (&bf[0].matrix);
  gfits_free_header (&bf[0].header);
  if (!CreateBuffer3D (bf, Nx, Ny, Nz, -32, 0.0, 1.0)) return FALSE;
  strcpy (bf[0].file, "(empty)");
  PutCoords (&newcoords, &bf[0].header);
  
  // generate the PSF in a local tangent plane
  Coords coords;
  InitCoords (&coords, "DEC--TAN");

  r = vr[0].elements.Flt;
  d = vd[0].elements.Flt;
  z = vz[0].elements.Flt;

  val = (float *)bf[0].matrix.buffer;

  for (i = 0; i < vr[0].Nelements; i++, r++, d++, z++) {
    double rn = ohana_normalize_angle (*r);
    while (rn < Rmin) rn += 360.0;
    while (rn > Rmax) rn -= 360.0;
    RD_to_XY (&x, &y, rn, *d, &newcoords);
    int Xb = x;
    int Yb = y;
    int Zb = (*z - Zmin) / dZ;

    if (Xb >= Nx) continue;
    if (Yb >= Ny) continue;
    if (Zb >= Nz) continue;
    if (Xb < 0) continue;
    if (Yb < 0) continue;
    if (Zb < 0) continue;
    val[Xb + Yb*Nx + Zb*Nx*Ny] ++;
  }
  return (TRUE);
}
