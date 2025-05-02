# include "data.h"

int mset (int argc, char **argv) {
  
  int i, Nx, Ny, Npix, xdir, Nset;
  float *out;
  Buffer *buf;
  Vector *vec;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: mset <buffer> <vector> <-x/-y> <N>\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((vec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (strcasecmp (argv[3], "-x") && strcasecmp (argv[3], "-y")) {
    gprint (GP_ERR, "USAGE: mset <buffer> <vector> <-x/-y> <N>\n");
    return (FALSE);
  }
  xdir = TRUE;
  if (!strcasecmp (argv[3], "-y")) xdir = FALSE;

  Npix = vec[0].Nelements;
  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];
  Nset = atof (argv[4]);
  if (Nset < 0) {
    gprint (GP_ERR, "selection out of range\n");
    return (FALSE);
  }

  if (xdir) {
    if (Nx != Npix) {
      gprint (GP_ERR, "dimensions don't match\n");
      return (FALSE);
    }
    if (Nset >= Ny) {
      gprint (GP_ERR, "row out of range\n");
      return (FALSE);
    }
    out = (float *) buf[0].matrix.buffer + Nx*Nset;

    if (vec[0].type == OPIHI_FLT) {
      opihi_flt *in = vec[0].elements.Flt;
      for (i = 0; i < Npix; i++, in++, out++) {
	*out = *in;
      }
    } else {
      opihi_int *in = vec[0].elements.Int;
      for (i = 0; i < Npix; i++, in++, out++) {
	*out = *in;
      }
    }
    return (TRUE);

  } else {
    if (Ny != Npix) {
      gprint (GP_ERR, "dimensions don't match\n");
      return (FALSE);
    }
    if (Nset >= Nx) {
      gprint (GP_ERR, "column out of range\n");
      return (FALSE);
    }
    out = (float *) buf[0].matrix.buffer + Nset;

    if (vec[0].type == OPIHI_FLT) {
      opihi_flt *in = vec[0].elements.Flt;
      for (i = 0; i < Npix; i++, in++, out+=Nx) {
	*out = *in;
      }
    } else {
      opihi_int *in = vec[0].elements.Int;
      for (i = 0; i < Npix; i++, in++, out+=Nx) {
	*out = *in;
      }
    }
    return (TRUE);
  }    
}

