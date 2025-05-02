# include "data.h"

int shift (int argc, char **argv) {
 
  int i, j;
  int nx, ny, dx, dy, DXin, DXot, DYin, DYot;
  float *Vin, *Vot;
  double Dx, Dy, fdx, fdy;
  Buffer *in, *out;

  // int ROLL = FALSE;
  // if ((N = get_argument (argc, argv, "-roll"))) {
  //   remove_argument (N, &argc, argv);
  //   ROLL = TRUE;
  // }

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: shift (input) (output) dx dy\n");
    return (FALSE);
  }

  // define the input buffer and examine the shift
  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  Dx = atof (argv[3]);
  Dy = atof (argv[4]);

  dx = Dx;
  dy = Dy;
  fdx = Dx - dx;
  fdy = Dy - dy;
  if (fdx < -0.000001) {dx -= 1; fdx += 1;}
  if (fdy < -0.000001) {dy -= 1; fdy += 1;}
  // we always specify a positive fractional shift
  // the above choice defines a minimum fractional shift of 1e-5

  nx = in[0].matrix.Naxis[0];
  ny = in[0].matrix.Naxis[1];

  if ((dx > nx) || (dy > ny)) {
    gprint (GP_ERR, "shifting data out of image\n");
    return (FALSE);
  }
  
  // define the output buffer
  if ((out = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  gfits_free_matrix (&out[0].matrix);
  gfits_free_header (&out[0].header);
  if (!CreateBuffer (out, nx, ny, -32, 0.0, 1.0)) return FALSE;

  DXin = (dx < 0) ? -dx : 0;
  DXot = (dx < 0) ?   0 : dx;
  DYin = (dy < 0) ? -dy : 0;
  DYot = (dy < 0) ?   0 : dy;
  
  for (j = 0; j < ny - abs(dy); j++) {
    Vin = (float *)(in[0].matrix.buffer)  + (j + DYin)*nx + DXin;  
    Vot = (float *)(out[0].matrix.buffer) + (j + DYot)*nx + DXot; 
    for (i = 0; i < nx - abs(dx); i++, Vin++, Vot++) {
      *Vot = *Vin;
    }
    // fill in the exposed x-border with 0.0
    Vot = (dx > 0) ? 
      (float *)(out[0].matrix.buffer) + (j + DYot)*nx : 
      (float *)(out[0].matrix.buffer) + (j + DYot)*nx + nx - abs(dx);
    for (i = 0; i < abs(dx); i++, Vot++) {
      *Vot = 0.0;
    }	
  }

  // fill in the exposed y-border with 0.0
  Vot = (dy > 0) ? 
    (float *)(out[0].matrix.buffer) :
    (float *)(out[0].matrix.buffer) + (ny - abs(dy))*nx;

  for (j = 0; j < nx * abs(dy); j++, Vot++) {
    *Vot = 0.0;
  }   

  // apply the fractional shift 
  gprint (GP_ERR, "%f %f\n", fdx, fdy);
  if ((fdx > 0) || (fdy > 0)) {
    double f00, f01, f10, f11;
    float value;

    f00 = (1-fdx)*(1-fdy);
    f01 =    fdx *(1-fdy);
    f10 = (1-fdx)*   fdy;
    f11 =    fdx *   fdy;

    Vin = (float *)out[0].matrix.buffer;
    Vot = (float *)out[0].matrix.buffer;
    for (j = 0; j < ny - 1; j++) {
      for (i = 0; i < nx - 1; i++, Vin++, Vot++) {
	value  = Vin[   0] * f00;
	value += Vin[   1] * f01;
	value += Vin[nx  ] * f10;
	value += Vin[nx+1] * f11;
	*Vot = value;
      }
      Vin ++;
      Vot ++;
    }
  }   

  return (TRUE);

}

