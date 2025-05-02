# include "data.h"

int mgaussdev (int argc, char **argv) {
  
  Buffer *buf;

  if (argc != 6) goto usage;

  if ((buf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  int Nx = atof (argv[2]);
  int Ny = atof (argv[3]);

  double mean = atof (argv[4]);
  double sigma = atof (argv[5]);

  /* I should encapsulate this in a create_default_buffer */
  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);

  // 3D CUBE OPTION: if (!CreateBuffer3D (buf, Nx, Ny, Nz, -32, 1.0, 0.0)) return FALSE;
  if (!CreateBuffer (buf, Nx, Ny, -32, 1.0, 0.0)) return FALSE;

  ohana_gaussdev_init ();
  
  float *v = (float *) buf[0].matrix.buffer;
  for (int i = 0; i < Nx*Ny; i++, v++) {
    *v = ohana_gaussdev_rnd (mean, sigma);
  }
  return (TRUE);

 usage:
  gprint (GP_ERR, "USAGE: mgaussdev (buff) Nx Ny mean sigma\n");
  return (FALSE);
}
