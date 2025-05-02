# include "astro.h"

int medianmap (int argc, char **argv) {
  
  float *temp, *tp;
  int i, j, k, I0, I1, J0, J1, I, J, n, N;
  int nx, ny, Nx, Ny, NX, NY, Ignore;
  float value, min, max, IgnoreValue;
  float *In, *Out, *ip;
  float fx, fy;
  Buffer *in, *out;

  IgnoreValue = 0;
  Ignore = FALSE;
  if ((N = get_argument (argc, argv, "-ignore"))) {
    Ignore = TRUE;
    remove_argument (N, &argc, argv);
    IgnoreValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  min = 0.45;
  max = 0.55;
  if ((N = get_argument (argc, argv, "-range"))) {
    remove_argument (N, &argc, argv);
    min  = atof(argv[N]);
    remove_argument (N, &argc, argv);
    max  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: medianmap (in) (out) Nx Ny [-range min max]\n");
    gprint (GP_ERR, "       Nx, Ny specify dimensions of output image\n");
    gprint (GP_ERR, "       min, max specify fractional range for sorted average\n");
    return (FALSE);
  }

  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  Nx = atof (argv[3]);
  Ny = atof (argv[4]);
  NX = in[0].header.Naxis[0];
  NY = in[0].header.Naxis[1];

  /* duplicate the (in) buffer to the (out), with different size */
  /* this should probably be a function in misc */
  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);
  out[0].bitpix = in[0].bitpix;
  out[0].unsign = in[0].unsign;
  out[0].bscale = in[0].bscale;
  out[0].bzero  = in[0].bzero;
  gfits_copy_header (&in[0].header, &out[0].header);
  gfits_modify (&out[0].header, "NAXIS1", "%d", 1, Nx);
  gfits_modify (&out[0].header, "NAXIS2", "%d", 1, Ny);
  out[0].header.Naxis[0] = Nx;
  out[0].header.Naxis[1] = Ny;
  gfits_create_matrix (&out[0].header, &out[0].matrix);

  In = (float *) in[0].matrix.buffer;
  Out = (float *) out[0].matrix.buffer;

  fx = (float) Nx / NX;
  fy = (float) Ny / NY;

  nx = 1 + 1/fx;
  ny = 1 + 1/fy;

  ALLOCATE (temp, float, 2*nx*ny);

  // float Mv = Mv2 = 0.0;

  for (j = 0; j < Ny; j++) {
    J0 = j / fy;
    J1 = (j + 1) / fy;
    for (i = 0; i < Nx; i++) {
      
      I0 = i / fx;
      I1 = (i + 1) / fx;

      n = 0;
      tp = temp;
      for (J = J0; J < J1; J++) {
	ip = &In[J*NX + I0];
	for (I = I0; I < I1; I++, tp++, ip++) {
	  if (Ignore && (fabs (*ip - IgnoreValue) < 0.01)) continue;
	  if (isnan (*ip)) continue;
	  if (isinf (*ip)) continue;
	  *tp = *ip;
	  n++;
	}
      }

      fsort (temp, n);

      value = 0;
      N = 0;
      for (k = min*n; k < max*n; k++) {
	value += temp[k];
	N ++;
      }
      if (N == 0)
	Out[j*Nx + i] = 0;
      else 
	Out[j*Nx + i] = value / N;
    }
  }

  return (TRUE);

}

