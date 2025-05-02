# include "astro.h"

float sersic_norm (float index);

// the sersic model has an integral of 2 \pi Rmin Rmax Io norm * Nsubpix^2

int sersic (int argc, char **argv) {
  
  int i, j;
  Buffer *buf;

  // generate a sersic profile in the given buffer using N

  if (argc != 10) {
    gprint (GP_ERR, "USAGE: sersic (buffer) (Io) (Xo) (Yo) (Rmajor) (Rminor) (theta) (index) (Nsubpix)\n");
    gprint (GP_ERR, "  generate a sersic profile for the given parameters\n");
    return (FALSE);
  }

  /* select input / output buffers */
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  int Nx = buf[0].header.Naxis[0];
  int Ny = buf[0].header.Naxis[1];
  
  // Io is the central pixel normalization
  float Io = atof(argv[2]);

  // Xo, Yo are the central pixel coordinates in real pixels
  float Xo = atof(argv[3]);
  float Yo = atof(argv[4]);

  float Rmaj = atof(argv[5]);
  float Rmin = atof(argv[6]);

  float theta = atof(argv[7]);
  float index = atof(argv[8]);
  float rindex = 0.5 / index;

  float Nsubpix = atof(argv[9]);

  float f1 = 1.0 / SQ(Rmin) + 1.0 / SQ(Rmaj);
  float f2 = 1.0 / SQ(Rmin) - 1.0 / SQ(Rmaj);
  
  float sxr = 0.5*f1 - 0.5*f2*cos(2.0*theta*RAD_DEG);
  float syr = 0.5*f1 + 0.5*f2*cos(2.0*theta*RAD_DEG);
  
  // sxr, syr cannot be < 0 (f1 >= f2)
  
  float Rxx  = +1.0 / sqrt(sxr);
  float Ryy  = +1.0 / sqrt(syr);
  float Rxy = -f2*sin(2.0*theta*RAD_DEG);

  float kappa = -0.275552 + 1.972625*index + 0.003487 * SQ(index);

  // dereference pointer
  float *in = (float *) buf[0].matrix.buffer;

  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++) {

      float x = ((i / Nsubpix) - Xo);
      float y = ((j / Nsubpix) - Yo);

      // z = r^2
      float z = SQ(x/Rxx) + SQ(y/Ryy) + Rxy*x*y;
	  
      float q = pow (z, rindex);
      float f = Io * exp(-kappa*q);

      in[i + j*Nx] = f;
    }
  }

  float norm = sersic_norm(index);
  set_variable ("sersic_norm", norm);
  set_variable ("sersic_kappa", kappa);
  
  return (TRUE);
}

float sersic_norm (float index) {

  float lnorm, norm;

  if ((index >= 0.0) && (index < 1.0)) {
    lnorm = 0.201545  - 0.950965 * index - 0.315248 * SQ(index);
    norm = exp(lnorm);
    return norm;
  }

  if ((index >= 1.0) && (index < 2.0)) {
    lnorm = 0.402084  - 1.357775 * index - 0.105102 * SQ(index);
    norm = exp(lnorm);
    return norm;
  }

  if ((index >= 2.0) && (index < 3.0)) {
    lnorm = 0.619093 - 1.591674 * index - 0.041576 * SQ(index);
    norm = exp(lnorm);
    return norm;
  }

  if ((index >= 3.0) && (index < 4.0)) {
    lnorm = 0.770263 - 1.696421 * index - 0.023363 * SQ(index);
    norm = exp(lnorm);
    return norm;
  }

  if ((index >= 4.0) && (index < 5.5))  {
    lnorm = 0.885891 - 1.755684 * index - 0.015753 * SQ(index);
    norm = exp(lnorm);
    return norm;
  }
  return NAN;
}
