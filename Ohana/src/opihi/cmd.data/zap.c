# include "data.h"

int zap (int argc, char **argv) {

  int i, j, N;
  int sx, sy, nx, ny;
  float *V, value;
  Buffer *buf;

  value = 0;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    value  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: zap <buffer> sx sy nx ny [-v value]\n");
    return (FALSE);
  }
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  sx = atof (argv[2]);
  sy = atof (argv[3]);
  nx = atof (argv[4]);
  ny = atof (argv[5]);

  if (sx < 0) goto error;
  if (sy < 0) goto error;
  if (sx + nx > buf[0].matrix.Naxis[0]) goto error;
  if (sy + ny > buf[0].matrix.Naxis[1]) goto error;

  for (j = sy; j < sy + ny; j++) {
    V = (float *)(buf[0].matrix.buffer) + j*buf[0].matrix.Naxis[0] + sx; 
    for (i = 0; i < nx; i++, V++) *V = value;
  }
  return (TRUE);

 error:
  gprint (GP_ERR, "region out of range\n");
  return (FALSE);
}

