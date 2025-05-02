# include "data.h"

int subraster (int argc, char **argv) {
  
  int i, j;
  float *Vin, *Vout;
  int sx, sy, nx, ny;
  int Sx, Sy, Nx, Ny;
  int NX, NY;
  Buffer *ibuf, *obuf;

  if (argc != 11) {
    gprint (GP_ERR, "USAGE: extract <from> <to> sx sy nx ny Sx Sy Nx Ny\n");
    return (FALSE);
  }

  if ((ibuf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  NX = ibuf[0].matrix.Naxis[0];
  NY = ibuf[0].matrix.Naxis[1];

  sx = atof (argv[3]);
  sy = atof (argv[4]);
  nx = atof (argv[5]);
  ny = atof (argv[6]);

  Sx = atof (argv[7]);
  Sy = atof (argv[8]);
  Nx = atof (argv[9]);
  Ny = atof (argv[10]);

  if ((Sy + ny > Ny) || (Sx + nx > Nx)) {
    gprint (GP_ERR, "mismatch between source and dest regions\n");
    gprint (GP_ERR, "%d + %d > %d or %d + %d > %d\n", Sy, ny, Ny, Sx, nx, Nx);
    return (FALSE);
  }

  /* region is not on first image */
  if ((sx + nx < 0) || (sy + ny < 0) || 
      (sx > ibuf[0].matrix.Naxis[0]) || 
      (sy > ibuf[0].matrix.Naxis[1])) {
    gprint (GP_ERR, "region outside of source image\n");
    return (FALSE);
  }

  if ((Sx + nx > Nx) || (Sy + ny > Ny)) {
    gprint (GP_ERR, "source region larger than dest region\n");
    return (FALSE);
  }
  if ((Sx < 0) || (Sy < 0)) {
    gprint (GP_ERR, "dest region out of range\n");
    return (FALSE);
  }

  if ((obuf = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  gfits_free_matrix (&obuf[0].matrix);
  gfits_free_header (&obuf[0].header);

  obuf[0].bitpix = ibuf[0].bitpix;
  obuf[0].unsign = ibuf[0].unsign;
  obuf[0].bscale = ibuf[0].bscale;
  obuf[0].bzero  = ibuf[0].bzero;
  /* strcpy (obuf[0].name, ibuf[0].name); */
  strcpy (obuf[0].file, ibuf[0].file);
  gfits_copy_header (&ibuf[0].header, &obuf[0].header);
  gfits_modify (&obuf[0].header, "NAXIS1", "%d", 1, Nx);
  gfits_modify (&obuf[0].header, "NAXIS2", "%d", 1, Ny);
  obuf[0].header.Naxis[0] = Nx;
  obuf[0].header.Naxis[1] = Ny;
  gfits_create_matrix (&obuf[0].header, &obuf[0].matrix);

  for (j = 0; j < ny; j++) {
    if (j + sy < 0) continue;
    if (j + sy >= NY) continue;
    Vin = (float *)(ibuf[0].matrix.buffer) + (j + sy)*ibuf[0].matrix.Naxis[0] + sx;  
    Vout = (float *)(obuf[0].matrix.buffer) + (j + Sy)*obuf[0].matrix.Naxis[0] + Sx;   
    for (i = 0; i < nx; i++, Vin++, Vout++) {
      if (i + sx < 0) continue;
      if (i + sx >= NX) continue;
      *Vout = *Vin;
    }
  }

  return (TRUE);

}

