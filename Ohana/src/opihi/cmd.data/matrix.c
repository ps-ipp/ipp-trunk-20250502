# include "data.h"

int matrix (int argc, char **argv) {
  
  int ix, iy, ik, Nx, Ny, Nk;
  float *Va, *Vb, *Vc, sum;
  Buffer *bufA, *bufB, *bufC;

  if ((argc != 5) && (argc != 7)) {
    gprint (GP_ERR, "USAGE: matrix set A = B * C : matrix multiplication\n");
    gprint (GP_ERR, "USAGE: matrix transpose A to B\n");
    return (FALSE);
  }

  if (!strcasecmp (argv[1], "set")) {

    if ((argc != 7) || strcasecmp (argv[3], "=") || strcasecmp (argv[5], "*")) {
      gprint (GP_ERR, "USAGE: matrix set A = B * C : matrix multiplication\n");
      return (FALSE);
    }
    if ((bufB = SelectBuffer (argv[4], OLDBUFFER, TRUE)) == NULL) return (FALSE);
    if ((bufC = SelectBuffer (argv[6], OLDBUFFER, TRUE)) == NULL) return (FALSE);

    if (bufB[0].matrix.Naxis[0] != bufC[0].matrix.Naxis[1]) {
      gprint (GP_ERR, "size mis-match in matrices: ("OFF_T_FMT" x "OFF_T_FMT") * ("OFF_T_FMT" x "OFF_T_FMT")\n", 
	       bufB[0].matrix.Naxis[0],  bufB[0].matrix.Naxis[1], 
	       bufC[0].matrix.Naxis[0],  bufC[0].matrix.Naxis[1]);
      return (FALSE);
    }
    if ((bufA = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

    Nx = bufC[0].matrix.Naxis[0];
    Ny = bufB[0].matrix.Naxis[1];
    Nk = bufB[0].matrix.Naxis[0];

    gfits_free_matrix (&bufA[0].matrix);
    gfits_free_header (&bufA[0].header);

    bufA[0].bitpix = bufB[0].bitpix;
    bufA[0].unsign = bufB[0].unsign;
    bufA[0].bscale = bufB[0].bscale;
    bufA[0].bzero  = bufB[0].bzero;
    gfits_copy_header (&bufB[0].header, &bufA[0].header);
    gfits_modify (&bufA[0].header, "NAXIS1", "%d", 1, Nx);
    gfits_modify (&bufA[0].header, "NAXIS2", "%d", 1, Ny);
    bufA[0].header.Naxis[0] = Nx;
    bufA[0].header.Naxis[1] = Ny;
    gfits_create_matrix (&bufA[0].header, &bufA[0].matrix);

    Va = (float *)bufA[0].matrix.buffer;
    Vb = (float *)bufB[0].matrix.buffer;
    Vc = (float *)bufC[0].matrix.buffer;

    for (iy = 0; iy < Ny; iy++) {
      for (ix = 0; ix < Nx; ix++) {
	sum = 0.0;
	for (ik = 0; ik < Nk; ik++) {
	  sum += Vb[iy*Nk + ik] * Vc[ik*Nx + ix];
	}
	Va[iy*Nx + ix] = sum;
      }
    }
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "transpose")) {
    if ((argc != 5) || strcasecmp (argv[3], "to")) {
      gprint (GP_ERR, "USAGE: matrix transpose A to B\n");
      return (FALSE);
    }
    if ((bufA = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
    if ((bufB = SelectBuffer (argv[4], ANYBUFFER, TRUE)) == NULL) return (FALSE);

    Nx = bufA[0].matrix.Naxis[0];
    Ny = bufA[0].matrix.Naxis[1];

    gfits_free_matrix (&bufB[0].matrix);
    gfits_free_header (&bufB[0].header);

    bufB[0].bitpix = bufA[0].bitpix;
    bufB[0].unsign = bufA[0].unsign;
    bufB[0].bscale = bufA[0].bscale;
    bufB[0].bzero  = bufA[0].bzero;
    gfits_copy_header (&bufA[0].header, &bufB[0].header);
    gfits_modify (&bufB[0].header, "NAXIS1", "%d", 1, Ny);
    gfits_modify (&bufB[0].header, "NAXIS2", "%d", 1, Nx);
    bufB[0].header.Naxis[0] = Ny;
    bufB[0].header.Naxis[1] = Nx;
    gfits_create_matrix (&bufB[0].header, &bufB[0].matrix);

    Va = (float *)bufA[0].matrix.buffer;
    Vb = (float *)bufB[0].matrix.buffer;

    for (iy = 0; iy < Ny; iy++) {
      for (ix = 0; ix < Nx; ix++) {
	Vb[ix*Ny + iy] = Va[iy*Nx + ix];
      }
    }

    return (TRUE);
  }

    gprint (GP_ERR, "USAGE: matrix set A = B * C : matrix and/or vector multiplication\n");
    gprint (GP_ERR, "USAGE: matrix transpose A to B\n");
    return (FALSE);
}
