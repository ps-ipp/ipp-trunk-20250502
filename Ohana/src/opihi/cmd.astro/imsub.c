# include "imfit.h"

int imsub (int argc, char **argv) {

  int i, j, N;
  int sx, sy, nx, ny, Nx, Ny;
  float value;
  float *V;
  Buffer *buf;

  // int VERBOSE = FALSE;
  // if ((N = get_argument (argc, argv, "-v"))) {
  //   remove_argument (N, &argc, argv);
  //   VERBOSE = TRUE;
  // }

  /* set fitting function */
  fgauss_setup ("fgauss");
  if ((N = get_argument (argc, argv, "-func"))) {
    fitfunc = NULL;
    remove_argument (N, &argc, argv);
    fgauss_setup (argv[N]);
    pgauss_setup (argv[N]);
    pgauss_psf_setup (argv[N]);
    sgauss_setup (argv[N]);
    sgauss_psf_setup (argv[N]);
    qgauss_setup (argv[N]);
    qgauss_psf_setup (argv[N]);
    qfgauss_setup (argv[N]);
    qrgauss_setup (argv[N]);
    if (fitfunc == NULL) {
      gprint (GP_ERR, "unknown function %s\n", argv[N]);
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: imfit <buffer> sx sy nx ny\n");
    return (FALSE);
  }

  /* non-optional arguments */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  sx = atof (argv[2]);
  sy = atof (argv[3]);
  nx = atof (argv[4]);
  ny = atof (argv[5]);
  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];

  /* check if region is valid */
  if (sx >= Nx) goto range;
  if (sy >= Ny) goto range;
  if (sx + nx < 0) goto range;
  if (sy + ny < 0) goto range;

  /* subtract model fit, but not local sky */
  for (j = 0; j < ny; j++) {
    if (j + sy < 0) continue;
    if (j + sy >= Ny) continue;
    V = (float *)(buf[0].matrix.buffer) + (j+sy)*buf[0].matrix.Naxis[0] + sx; 
    for (i = 0; i < nx; i++, V++) {
      if (i + sx < 0) continue;
      if (i + sx >= Nx) continue;
      value = fitfunc ((float)(i+sx), (float)(j+sy), par, Npar, NULL);
      *V -= value;
    }
  }

  free (par);
  free (fpar);
  return (TRUE);

range:
  gprint (GP_ERR, "region out of range\n");
  return (FALSE);
}

