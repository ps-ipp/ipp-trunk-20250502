# include "data.h"

int mlayer (int argc, char **argv) {
  
  int N;
  Buffer *src, *tgt;

  int Dir = 0;
  if ((N = get_argument (argc, argv, "-x"))) {
    remove_argument (N, &argc, argv);
    Dir = 1;    
  }
  if ((N = get_argument (argc, argv, "-y"))) {
    remove_argument (N, &argc, argv);
    if (Dir) {
      gprint (GP_ERR, "ERROR: multiple -x,-y,-z options are not allowed\n");
      return FALSE;
    }
    Dir = 2;    
  }
  if ((N = get_argument (argc, argv, "-z"))) {
    remove_argument (N, &argc, argv);
    if (Dir) {
      gprint (GP_ERR, "ERROR: multiple -x,-y,-z options are not allowed\n");
      return FALSE;
    }
    Dir = 3;    
  }
  if (!Dir) Dir = 3;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: mlayer <output> <input> plane [-x,-y,-z]\n");
    gprint (GP_ERR, "  insert 2D image into 3D cube (opposite of mslice)\n");
    gprint (GP_ERR, "  -z is default\n");
    return (FALSE);
  }

  int plane = atoi(argv[3]);

  if ((tgt = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if (tgt[0].matrix.Naxes < 3) {
    gprint (GP_ERR, "buffer is not 3D\n");
    return FALSE;
  }
  if ((src = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  int Nx = tgt[0].matrix.Naxis[0];
  int Ny = tgt[0].matrix.Naxis[1];
  int Nz = tgt[0].matrix.Naxis[2];

  int NplaneMax = 0;
  int Nsrc1 = 0;
  int Nsrc2 = 0;

  switch (Dir) {
    case 1: NplaneMax = Nx; Nsrc1 = Ny; Nsrc2 = Nz; break;
    case 2: NplaneMax = Ny; Nsrc1 = Nx; Nsrc2 = Nz; break;
    case 3: NplaneMax = Nz; Nsrc1 = Nx; Nsrc2 = Ny; break;
    default: myAbort ("impossible");
  }

  int invalid = FALSE;
  invalid = invalid || (plane < 0);
  invalid = invalid || (plane >= NplaneMax);
  if (invalid) {
    gprint (GP_ERR, "plane %d out of range (max = %d)\n", plane, NplaneMax);
    return (FALSE);
  }
  if (Nsrc1 != src->matrix.Naxis[0]) {
    gprint (GP_ERR, "source image does not match output cube dimensions (%d vs %ld)\n", Nsrc1, src->matrix.Naxis[0]);
    return (FALSE);
  }
  if (Nsrc2 != src->matrix.Naxis[1]) {
    gprint (GP_ERR, "source image does not match output cube dimensions (%d vs %ld)\n", Nsrc2, src->matrix.Naxis[1]);
    return (FALSE);
  }

  // pixel (ix, iy, iz) : tgt[0].matrix.buffer + ix + iy*Nx + iz*Nx*Ny

  switch (Dir) {
    case 1: {
      float *srcF = (float *) src[0].matrix.buffer;
      
      // pixel (ix, iy, iz) : tgt[0].matrix.buffer + ix + iy*Nx + iz*Nx*Ny

      for (int iz = 0; iz < Nz; iz++) {
	float *tgtF  = (float *) tgt[0].matrix.buffer + plane + iz*Nx*Ny;
	for (int iy = 0; iy < Ny; iy++, tgtF += Nx, srcF++) {
	  *tgtF = *srcF;
	}
      }
      break;
    }
    case 2: {

      float *srcF = (float *) src[0].matrix.buffer;

      for (int iz = 0; iz < Nz; iz++) {
	float *tgtF  = (float *) tgt[0].matrix.buffer + plane*Nx + iz*Nx*Ny;
	for (int ix = 0; ix < Nx; ix++, tgtF++, srcF++) {
	   *tgtF = *srcF;
	}
      }
      break;
    }
    case 3: {
      float *tgtF  = (float *) tgt[0].matrix.buffer + plane*Nx*Ny;
      float *srcF = (float *) src[0].matrix.buffer;
      
      for (int i = 0; i < Nx*Ny; i++, tgtF ++, srcF++) {
	*tgtF = *srcF;
      }
      break;
    }
    default: myAbort ("impossible");
  }
  return (TRUE);
}
