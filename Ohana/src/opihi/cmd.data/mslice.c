# include "data.h"

int mslice (int argc, char **argv) {
  
  int N;
  Buffer *in, *out;

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
    gprint (GP_ERR, "USAGE: mslice <input> <output> plane [-x,-y,-z]\n");
    gprint (GP_ERR, "  extract 2D image from 3D cube (opposite of mlayer)\n");
    gprint (GP_ERR, "  -z is default\n");
    return (FALSE);
  }

  int plane = atoi(argv[3]);

  if ((in = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if (in[0].matrix.Naxes < 3) {
    gprint (GP_ERR, "buffer is not 3D\n");
    return FALSE;
  }
  if ((out = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  int Nx = in[0].matrix.Naxis[0];
  int Ny = in[0].matrix.Naxis[1];
  int Nz = in[0].matrix.Naxis[2];

  int NplaneMax = 0;
  int Nout1 = 0;
  int Nout2 = 0;
  switch (Dir) {
    case 1: NplaneMax = Nx; Nout1 = Ny; Nout2 = Nz; break;
    case 2: NplaneMax = Ny; Nout1 = Nx; Nout2 = Nz; break;
    case 3: NplaneMax = Nz; Nout1 = Nx; Nout2 = Ny; break;
    default: myAbort ("impossible");
  }

  int invalid = FALSE;
  invalid = invalid || (plane < 0);
  invalid = invalid || (plane >= NplaneMax);
  if (invalid) {
    gprint (GP_ERR, "plane %d out of range (max = %d)\n", plane, NplaneMax);
    return (FALSE);
  }

  /* I should encapsulate this in a create_default_buffer */
  
  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);
  if (!CreateBuffer (out, Nout1, Nout2, -32, 1.0, 0.0)) return FALSE;

  // pixel (ix, iy, iz) : in[0].matrix.buffer + ix + iy*Nx + iz*Nx*Ny

  switch (Dir) {
    case 1: {
      float *outF = (float *) out[0].matrix.buffer;
      
      // pixel (ix, iy, iz) : in[0].matrix.buffer + ix + iy*Nx + iz*Nx*Ny

      for (int iz = 0; iz < Nz; iz++) {
	float *inF  = (float *) in[0].matrix.buffer + plane + iz*Nx*Ny;
	for (int iy = 0; iy < Ny; iy++, inF += Nx, outF++) {
	  *outF = *inF;
	}
      }
      break;
    }
    case 2: {

      float *outF = (float *) out[0].matrix.buffer;

      for (int iz = 0; iz < Nz; iz++) {
	float *inF  = (float *) in[0].matrix.buffer + plane*Nx + iz*Nx*Ny;
	for (int ix = 0; ix < Nx; ix++, inF++, outF++) {
	  *outF = *inF;
	}
      }
      break;
    }
    case 3: {
      float *inF  = (float *) in[0].matrix.buffer + plane*Nx*Ny;
      float *outF = (float *) out[0].matrix.buffer;
      
      for (int i = 0; i < Nx*Ny; i++, inF ++, outF++) {
	*outF = *inF;
      }
      break;
    }
    default: myAbort ("impossible");
  }
  return (TRUE);
}
