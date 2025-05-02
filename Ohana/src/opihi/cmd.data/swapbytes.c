# include "data.h"

int swapbytes (int argc, char **argv) {
  
  int i, nx, ny;
  char *V, tmp;
  Buffer *buf;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: swapbytes <buffer>\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  nx = buf[0].matrix.Naxis[0];
  ny = buf[0].matrix.Naxis[1];

  gprint (GP_ERR, "npix: %d\n", nx*ny);

  V = buf[0].matrix.buffer;
  for (i = 0; i < nx*ny; i++, V+=4) {
    tmp = V[0]; V[0] = V[3]; V[3] = tmp;
    tmp = V[1]; V[1] = V[2]; V[2] = tmp;
  }

  return (TRUE);
}

