# include "data.h"

# define CHECKVAL(ARG) if (!isfinite(ARG)) { gprint (GP_ERR, "illegal value for %s: %f\n", #ARG, ARG); return (FALSE); }

int tdhistogram (int argc, char **argv) {

  int i, Nx, Ny, Nz, N;
  float *val;
  Buffer *bf;
  Vector *vx, *vy, *vz, *range;
  opihi_flt *x, *y, *z;

  int reuse = FALSE;
  if ((N = get_argument (argc, argv, "-reuse"))) {
    remove_argument (N, &argc, argv);
    reuse = TRUE;
  }

  range = NULL;
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    if ((range = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);    
    remove_argument (N, &argc, argv);
  }

  double dx = NAN;
  if ((N = get_argument (argc, argv, "-dx"))) {
    remove_argument (N, &argc, argv);
    dx = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  double dy = NAN;
  if ((N = get_argument (argc, argv, "-dy"))) {
    remove_argument (N, &argc, argv);
    dy = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  double dz = NAN;
  if ((N = get_argument (argc, argv, "-dz"))) {
    remove_argument (N, &argc, argv);
    dz = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int valid = (!reuse && (argc == 11)) || (reuse && (argc == 5));
  if (!valid) {
    gprint (GP_ERR, "USAGE: tdhistogram buffer x y z (Xmin) (Xmax) (Ymin) (Ymax) (Zmin) (Zmax) [-dx dx] [-dy dy] [-dz dz] [-range range]\n");
    gprint (GP_ERR, "   OR: tdhistogram buffer x y z -reuse\n");
    gprint (GP_ERR, "   -dx, -dy, -dy specify the bin size in these directions\n");
    gprint (GP_ERR, "   -range : generate an output vector corresponding to the elements in the z-direction\n");
    gprint (GP_ERR, " output buffer is 3D\n");
    return (FALSE);
  }
  
  if ((bf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vx = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vy = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vz = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (vx[0].Nelements != vy[0].Nelements) return (FALSE);
  if (vx[0].Nelements != vz[0].Nelements) return (FALSE);

  double Xmin, Xmax, Ymin, Ymax, Zmin, Zmax;

  if (!reuse) {
    Xmin = atof(argv[5]);
    Xmax = atof(argv[6]);
    if (!isfinite(dx)) {
      Nx = 100;
      dx = (Xmax - Xmin) / (Nx - 1);
    } else {
      Nx = (Xmax - Xmin) / dx + 1;
    }
    if (dx < 0) {
      gprint (GP_ERR, "invalid value for delta: %f\n", dx);
      return (FALSE);
    }
    Ymin = atof(argv[7]);
    Ymax = atof(argv[8]);
    if (!isfinite(dy)) {
      Ny = 100;
      dy = (Ymax - Ymin) / (Ny - 1);
    } else {
      Ny = (Ymax - Ymin) / dy + 1;
    }
    if (dy < 0) {
      gprint (GP_ERR, "invalid value for delta: %f\n", dy);
      return (FALSE);
    }
    Zmin = atof(argv[9]);
    Zmax = atof(argv[10]);
    if (!isfinite(dz)) {
      Nz = 100;
      dz = (Zmax - Zmin) / (Nz - 1);
    } else {
      Nz = (Zmax - Zmin) / dz + 1;
    }
    if (dz < 0) {
      gprint (GP_ERR, "invalid value for delta: %f\n", dz);
      return (FALSE);
    }

    if (Nz > 1000) {
      gprint (GP_ERR, "warning: delta of %f will result in %d histogram bins\n", dz, Nz);
      return (FALSE);
    }
  } else {
    gfits_scan (&bf[0].header, "XMIN", "%lf", 1, &Xmin);
    gfits_scan (&bf[0].header, "XMAX", "%lf", 1, &Xmax);
    gfits_scan (&bf[0].header, "XDEL", "%lf", 1, &dx);
    gfits_scan (&bf[0].header, "YMIN", "%lf", 1, &Ymin);
    gfits_scan (&bf[0].header, "YMAX", "%lf", 1, &Ymax);
    gfits_scan (&bf[0].header, "YDEL", "%lf", 1, &dy);
    gfits_scan (&bf[0].header, "ZMIN", "%lf", 1, &Zmin);
    gfits_scan (&bf[0].header, "ZMAX", "%lf", 1, &Zmax);
    gfits_scan (&bf[0].header, "ZDEL", "%lf", 1, &dz);
    Nx = bf[0].header.Naxis[0];
    Ny = bf[0].header.Naxis[1];
    Nz = bf[0].header.Naxis[2];
  }

  REQUIRE_VECTOR_FLT (vx, FALSE); 
  REQUIRE_VECTOR_FLT (vy, FALSE); 
  REQUIRE_VECTOR_FLT (vz, FALSE); 

  CHECKVAL(Xmin);
  CHECKVAL(Xmax);
  CHECKVAL(dx);

  CHECKVAL(Ymin);
  CHECKVAL(Ymax);
  CHECKVAL(dy);

  if (range) {
    ResetVector (range, OPIHI_FLT, Nz);
    for (i = 0; i < range[0].Nelements; i++) {
      range[0].elements.Flt[i] = Zmin + i*dz;
    }
  }

  if (!reuse) {
    gfits_free_matrix (&bf[0].matrix);
    gfits_free_header (&bf[0].header);
    if (!CreateBuffer3D (bf, Nx, Ny, Nz, -32, 0.0, 1.0)) return FALSE;
    strcpy (bf[0].file, "(empty)");

    gfits_modify (&bf[0].header, "XMIN", "%lf", 1, Xmin);
    gfits_modify (&bf[0].header, "XMAX", "%lf", 1, Xmax);
    gfits_modify (&bf[0].header, "XDEL", "%lf", 1, dx);
    gfits_modify (&bf[0].header, "YMIN", "%lf", 1, Ymin);
    gfits_modify (&bf[0].header, "YMAX", "%lf", 1, Ymax);
    gfits_modify (&bf[0].header, "YDEL", "%lf", 1, dy);
    gfits_modify (&bf[0].header, "ZMIN", "%lf", 1, Zmin);
    gfits_modify (&bf[0].header, "ZMAX", "%lf", 1, Zmax);
    gfits_modify (&bf[0].header, "ZDEL", "%lf", 1, dz);
  }
  
  x = vx[0].elements.Flt;
  y = vy[0].elements.Flt;
  z = vz[0].elements.Flt;

  val = (float *) bf[0].matrix.buffer;

  for (i = 0; i < vx[0].Nelements; i++, x++, y++, z++) {
    int Xb = (*x - Xmin) / dx;
    int Yb = (*y - Ymin) / dy;
    int Zb = (*z - Zmin) / dz;

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
