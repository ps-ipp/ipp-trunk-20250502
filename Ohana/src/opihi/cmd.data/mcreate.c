# include "data.h"

int mcreate (int argc, char **argv) {
  
  int N;
  Buffer *buf;

  int Nz = 0;
  if ((N = get_argument (argc, argv, "-nz"))) {
    remove_argument (N, &argc, argv);
    Nz = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: mcreate <buffer> Nx Ny [-nz Nz]\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  int Nx = atof (argv[2]);
  int Ny = atof (argv[3]);

  /* I should encapsulate this in a create_default_buffer */
  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);

  if (Nz) {
    if (!CreateBuffer3D (buf, Nx, Ny, Nz, -32, 1.0, 0.0)) return FALSE;
  } else {
    if (!CreateBuffer (buf, Nx, Ny, -32, 1.0, 0.0)) return FALSE;
  }
  return (TRUE);
}
