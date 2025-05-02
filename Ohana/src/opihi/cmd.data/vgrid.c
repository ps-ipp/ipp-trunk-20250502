# include "data.h"

int vgrid (int argc, char **argv) {

  int i, Nx, Ny, Xb, Yb;
  float Xmin, Xmax, dX, Ymin, Ymax, dY;
  float *buf;
  opihi_flt *val, *x, *y, *z;
  int *Nval;
  Buffer *bf;
  Vector *vx, *vy, *vz;

  if (argc != 11) {
    gprint (GP_ERR, "USAGE: vgrid x y z buffer Xmin Xmax dX Ymin Ymax dY\n");
    gprint (GP_ERR, "  re-grid values from a triplet of vectors (x,y,z) into an image\n");
    gprint (GP_ERR, "  the vectors must be floating-point type\n");
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

  Xmin = atof (argv[5]);
  Xmax = atof (argv[6]);
  dX   = atof (argv[7]);

  Ymin = atof (argv[8]);
  Ymax = atof (argv[9]);
  dY   = atof (argv[10]);

  Nx = (Xmax - Xmin) / dX + 1;
  Ny = (Ymax - Ymin) / dY + 1;
  
  gfits_free_matrix (&bf[0].matrix);
  gfits_free_header (&bf[0].header);
  if (!CreateBuffer (bf, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
  strcpy (bf[0].file, "(empty)");

  ALLOCATE (val, opihi_flt, Nx*Ny);
  bzero (val, Nx*Ny*sizeof(opihi_flt));
  ALLOCATE (Nval, int, Nx*Ny);
  bzero (Nval, Nx*Ny*sizeof(int));

  x = vx[0].elements.Flt;
  y = vy[0].elements.Flt;
  z = vz[0].elements.Flt;
  for (i = 0; i < vx[0].Nelements; i++, x++, y++, z++) {
    Xb = (*x - Xmin) / dX;
    Yb = (*y - Ymin) / dY;
    if (Xb >= Nx) continue;
    if (Yb >= Ny) continue;
    val[Xb + Yb*Nx] = *z;
    Nval[Xb + Yb*Nx]++;
  }

  buf = (float *) bf[0].matrix.buffer;
  for (i = 0; i < Nx*Ny; i++) {
    if (Nval[i] == 0) {
      buf[i] = 0;
      continue;
    }
    buf[i] = val[i] / Nval[i];
  }

  free (val);
  free (Nval);

  return (TRUE);

}
