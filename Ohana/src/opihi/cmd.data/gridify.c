# include "data.h"

int gridify (int argc, char **argv) {

  int i, Nx, Ny, Xb, Yb, Normalize, N;
  float Xmin, Xmax, dX, Ymin, Ymax, dY, initValue;
  float *buf, *val, *cnt;
  int *Nval;
  Vector *vx, *vy, *vz;
  opihi_flt *x, *y, *z;

  Buffer *bf = NULL;
  Buffer *ct = NULL;

  Normalize = TRUE;
  if ((N = get_argument (argc, argv, "-raw"))) {
    remove_argument (N, &argc, argv);
    Normalize = FALSE;
    if ((ct = SelectBuffer (argv[N], ANYBUFFER, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  initValue = 0.0;
  if ((N = get_argument (argc, argv, "-init-value"))) {
    remove_argument (N, &argc, argv);
    initValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  Xmin = Xmax = dX = NAN;
  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    Xmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    Xmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    dX   = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }    

  Ymin = Ymax = dY = NAN;
  if ((N = get_argument (argc, argv, "-y"))) {
    remove_argument (N, &argc, argv);
    Ymin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    Ymax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    dY   = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }    

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: gridify x y z buffer [-x Xmin Xmax dX] [-y Ymin Ymax dY] [-init-value value] [-raw]\n");
    return (FALSE);
  }
  
  if ((vx = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vy = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vz = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((bf = SelectBuffer (argv[4], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  if (vx[0].Nelements != vy[0].Nelements) return (FALSE);
  if (vx[0].Nelements != vz[0].Nelements) return (FALSE);

  REQUIRE_VECTOR_FLT (vx, FALSE); 
  REQUIRE_VECTOR_FLT (vy, FALSE); 
  REQUIRE_VECTOR_FLT (vz, FALSE); 

  if (isnan(dX)) {
    Xmin = 0;
    Xmax = bf[0].matrix.Naxis[0];
    dX = 1;
  }

  if (isnan(dY)) {
    Ymin = 0;
    Ymax = bf[0].matrix.Naxis[1];
    dY = 1;
  }

  Nx = (Xmax - Xmin) / dX;
  Ny = (Ymax - Ymin) / dY;
  
  if ((Nx != bf[0].matrix.Naxis[0]) || (Ny != bf[0].matrix.Naxis[1])) {
    gfits_free_matrix (&bf[0].matrix);
    gfits_free_header (&bf[0].header);
    if (!CreateBuffer (bf, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
    strcpy (bf[0].file, "(empty)");
  }

  ALLOCATE (val, float, Nx*Ny);
  bzero (val, Nx*Ny*sizeof(float));
  ALLOCATE (Nval, int, Nx*Ny);
  bzero (Nval, Nx*Ny*sizeof(int));

  x = vx[0].elements.Flt;
  y = vy[0].elements.Flt;
  z = vz[0].elements.Flt;
  for (i = 0; i < vx[0].Nelements; i++, x++, y++, z++) {
    Xb = (*x - Xmin) / dX;
    Yb = (*y - Ymin) / dY;
    if (Xb < 0) continue;
    if (Yb < 0) continue;
    if (Xb >= Nx) continue;
    if (Yb >= Ny) continue;
    val[Xb + Yb*Nx] += *z;
    Nval[Xb + Yb*Nx]++;
  }

  if (!Normalize) {
    gfits_free_matrix (&ct[0].matrix);
    gfits_free_header (&ct[0].header);
    if (!CreateBuffer (ct, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
    strcpy (ct[0].file, "(empty)");

    buf = (float *) bf[0].matrix.buffer;
    cnt = (float *) ct[0].matrix.buffer;
    for (i = 0; i < Nx*Ny; i++) {
      if (Nval[i] == 0) continue;
      buf[i] = val[i];
      cnt[i] = Nval[i];
    }
    free (val);
    free (Nval);
    return TRUE;
  }

  buf = (float *) bf[0].matrix.buffer;
  for (i = 0; i < Nx*Ny; i++) {
    buf[i] = initValue;
    if (Nval[i] == 0) continue;
    buf[i] = val[i] / Nval[i];
  }

  free (val);
  free (Nval);

  return (TRUE);

}
