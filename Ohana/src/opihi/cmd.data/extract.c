# include "data.h"

/* <from> : source image, must exist
   <to>   : target image -- if it does not exist, it is created of size (Nx,Ny)
   sx, sy : source starting coordinate -- need not be on a valid image pixel
   nx, ny : number of source pixels -- currently must not go out of bounds -> allow out-of-bounds
   Sx, Sy : target starting coordinate -- need not be on a valid image pixel
   Nx, Ny : redundant information UNLESS <to> does exist (in which case it is required information) -> make Nx,Ny optional if <to> exists?
 */

int extract (int argc, char **argv) {
  
  int N;
  Buffer *in, *out;

  float initValue = 0.0;
  int initOutput = FALSE;
  if ((N = get_argument (argc, argv, "-init"))) {
    remove_argument (N, &argc, argv);
    initValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
    initOutput = TRUE;
  }

  if (argc != 11) {
    gprint (GP_ERR, "USAGE: extract <from> <to> sx sy nx ny Sx Sy Nx Ny\n");
    return (FALSE);
  }

  if ((in = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  int NX = in[0].matrix.Naxis[0];
  int NY = in[0].matrix.Naxis[1];

  int sx = atof (argv[3]);
  int sy = atof (argv[4]);
  int nx = atof (argv[5]);
  int ny = atof (argv[6]);

  int Sx = atof (argv[7]);
  int Sy = atof (argv[8]);
  int Nx = atof (argv[9]);
  int Ny = atof (argv[10]);

  if ((Sy + ny > Ny) || (Sx + nx > Nx)) {
    gprint (GP_ERR, "source pixels extend beyond target pixels\n");
    gprint (GP_ERR, "%d + %d > %d or %d + %d > %d\n", Sy, ny, Ny, Sx, nx, Nx);
    // return (FALSE);
  }

  // XXX : allow source region to fall outside source image

  /* region is not on first image */
  if ((sx + nx < 0) || (sy + ny < 0) || 
      (sx > in[0].matrix.Naxis[0]) || 
      (sy > in[0].matrix.Naxis[1])) {
    gprint (GP_ERR, "region outside of source image\n");
    // return (FALSE);
  }

  if ((Sx < 0) || (Sy < 0)) {
    gprint (GP_ERR, "dest region out of range\n");
    // return (FALSE);
  }

  if ((out = SelectBuffer (argv[2], OLDBUFFER, FALSE)) == NULL) {
    if ((out = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);
    gfits_free_matrix (&out[0].matrix);
    gfits_free_header (&out[0].header);

    out[0].bitpix = in[0].bitpix;
    out[0].unsign = in[0].unsign;
    out[0].bscale = in[0].bscale;
    out[0].bzero  = in[0].bzero;
    strcpy (out[0].file, in[0].file);
    gfits_copy_header (&in[0].header, &out[0].header);
    gfits_modify (&out[0].header, "NAXIS1", "%d", 1, Nx);
    gfits_modify (&out[0].header, "NAXIS2", "%d", 1, Ny);
    out[0].header.Naxis[0] = Nx;
    out[0].header.Naxis[1] = Ny;
    gfits_create_matrix (&out[0].header, &out[0].matrix);
  } else {
    if ((out[0].header.Naxis[1] != Ny) || (out[0].header.Naxis[0] != Nx)) {
      gprint (GP_ERR, "matrix sizes mis-matched\n");
      gprint (GP_ERR, "%d x %d  vs  "OFF_T_FMT" x "OFF_T_FMT"\n", Nx, Ny, 
	       out[0].header.Naxis[0],  out[0].header.Naxis[1]);
      return (FALSE);
    }
  }

  // (NX,NY) : source image dimensions
  // (Nx,Ny) : target image dimensions

  // allow (sx, sy), (Sx, Sy) to be off image
  // allow (sx+nx, sy+ny) to be off image

  // if (sx < 0) {
  //   nx += sx;
  //   sx = 0;
  // }
  // if (sx > NX) sx = NX;
  // 
  // if (Sx < 0) {
  //   Nx += sx;
  //   sx = 0;
  // }
  // if (sx > NX) sx = NX;
  
  float *Vin = (float *)(in[0].matrix.buffer);
  float *Vout = (float *)(out[0].matrix.buffer);

  if (initOutput) {
    for (int j = 0; j < Ny; j++) {
      for (int i = 0; i < Nx; i++) {
	Vout[i + Nx*j] = initValue;
      }
    }
  }

  int Nps = NX*NY;
  int Npt = Nx*Ny;

  for (int j = 0; j < ny; j++) {
    if (j + sy < 0) continue; // not yet on source image
    if (j + Sy < 0) continue; // not yet on target image
    if (j + sy >= NY) continue; // past edge of source image
    if (j + Sy >= Ny) continue; // past edge of target image

    // Vin = (float *)(in[0].matrix.buffer) + (j + sy)*in[0].matrix.Naxis[0] + sx;  
    // Vout = (float *)(out[0].matrix.buffer) + (j + Sy)*out[0].matrix.Naxis[0] + Sx;   

    for (int i = 0; i < nx; i++) {
      if (i + sx < 0) continue;
      if (i + sx >= NX) continue;
      if (i + Sx < 0) continue;
      if (i + Sx >= Nx) continue;

      int ps = i + sx + (j + sy)*NX;
      int pt = i + Sx + (j + Sy)*Nx;

      myAssert (pt >= 0, "oops 1");
      myAssert (pt < Npt, "oops 2");

      myAssert (ps >= 0, "oops 3");
      myAssert (ps < Nps, "oops 4");

      Vout[pt] = Vin[ps];
    }
  }

  return (TRUE);

}
