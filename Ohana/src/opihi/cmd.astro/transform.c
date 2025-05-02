# include "astro.h"

int transform (int argc, char **argv) {

  int i, j, Nx, Ny;
  Coords coords_in, coords_out;
  int X, Y;
  double x, y, r, d, dx, dy;
  double frac;
  char *Sout, *S;
  float *Vin, *Vout;
  Buffer *in, *out;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: transform <from> <to>\n");
    return (FALSE);
  }

  if ((in  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((out = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  GetCoords (&coords_in, &in[0].header);
  GetCoords (&coords_out, &out[0].header);

  /* for the moment, disable WRP / DIS */
  if (!strcmp(&coords_in.ctype[4], "-WRP") || !strcmp(&coords_out.ctype[4], "-WRP")) {
    gprint (GP_ERR, "WRP mode not implemented for astrom\n");
    return (FALSE);
  }
  
  // double scale_in = sqrt(fabs(coords_in.cdelt1*coords_in.cdelt2*(coords_in.pc1_1*coords_in.pc2_2 - coords_in.pc1_2*coords_in.pc2_1)));
  // double scale_out = sqrt(fabs(coords_out.cdelt1*coords_out.cdelt2*(coords_out.pc1_1*coords_out.pc2_2 - coords_out.pc1_2*coords_out.pc2_1)));

  Vin  = (float *) in[0].matrix.buffer;
  Vout = (float *) out[0].matrix.buffer;
  Nx = out[0].header.Naxis[0];
  Ny = out[0].header.Naxis[1];
  bzero (Vout, Nx*Ny*sizeof(float));
  ALLOCATE (S, char, Nx*Ny);
  Sout = S;
  bzero (Sout, Nx*Ny*sizeof(char));
  frac = 0.333;

  /* if (scale_in < scale_out) { */

  for (j = 0; j < in[0].header.Naxis[1]; j++) {
    gprint (GP_ERR, ".");
    for (i = 0; i < in[0].header.Naxis[0]; i++, Vin++) {
      for (dx = 0.0 + 0.5*frac; dx < 1.0 - 0.5*frac; dx += frac) {
	for (dy = 0.0 + 0.5*frac; dy < 1.0 - 0.5*frac; dy += frac) {
	  XY_to_RD (&r, &d, i + dx, j + dy, &coords_in);
	  RD_to_XY (&x, &y, r, d, &coords_out);
	  X = x; Y = y;
	  if ((X > -1) && (X < Nx) && (Y > -1) && (Y < Ny)) {
	    if (!isfinite(*Vin)) continue;
	    Vout[X + Y*Nx] += *Vin;
	    Sout[X + Y*Nx] ++;
	  }
	}
      }
    }
  }

  Sout = S;
  Vout = (float *) out[0].matrix.buffer;
  for (i = 0; i < Nx*Ny; i++, Vout++, Sout++) {
    *Vout = *Vout / *Sout;
  }

  free (S);
    
  return (TRUE);
}

