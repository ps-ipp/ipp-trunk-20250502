# include "astro.h"

# define FLUX(BUF,X,Y) {						\
	char valid = TRUE;						\
	valid &= (X >= 0);						\
	valid &= (Y >= 0);						\
	valid &= (X < Nx);						\
	valid &= (Y < Ny);						\
	if (valid) {							\
	    out[Nvec] = BUF[(int)(X) + ((int)(Y))*Nx];			\
	    Nvec++;							\
	    if (Nvec >= vec[0].Nelements) abort();			\
	}								\
    }

# define C1 0

int ringflux (int argc, char **argv) {
  
  int Nx, Ny, x, y, d, radius, Radius, dRadius, Nvec;
  double *out;
  float *in;
  float Xo, Yo;
  Buffer *buf;
  Vector *vec;

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: ringflux (buffer) (vector) (Xo) (Yo) (Radius) (dRadius)\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];

  Xo = atof(argv[3]);
  Yo = atof(argv[4]);

  Radius = atoi (argv[5]);
  dRadius = atoi (argv[6]);

  // just a rough guess at the number needed
  ResetVector (vec, OPIHI_FLT, MAX (6*Radius*dRadius, 8));

  Nvec = 0;
  for (radius = Radius; radius < Radius + dRadius; radius ++) {

    x = 0;
    y = radius;

# if (C1)
    d = 3 - 2*radius;
# else
    d = 5 - 4*radius;
# endif

    in = (float *) buf[0].matrix.buffer;
    out = vec[0].elements.Flt;

    while (x <= y) {
      FLUX(in, (Xo + x), (Yo + y));
      FLUX(in, (Xo + x), (Yo - y));
      FLUX(in, (Xo + y), (Yo + x));
      FLUX(in, (Xo - y), (Yo + x));

      if (x > 0) {
	FLUX(in, (Xo - x), (Yo + y));
	FLUX(in, (Xo - x), (Yo - y));
	FLUX(in, (Xo - y), (Yo - x));
	FLUX(in, (Xo + y), (Yo - x));
      }
      if (Nvec >= vec[0].Nelements - 8) {
	  // counting error here
	ResetVector (vec, OPIHI_FLT, vec[0].Nelements + 64);
	out = vec[0].elements.Flt;
      }

      if (d < 0) {
# if (C1)      
	d = d + 4*x + 6;
# else
	d = d + 8*x + 4;
# endif
      } else {
# if (C1)      
	d = d + 4*(x-y) + 10;
# else
	d = d + 8*(x-y) + 8;
# endif
	y--;
      }
      x++;
    }
  }
  vec[0].Nelements = Nvec;
  return (TRUE);
}
