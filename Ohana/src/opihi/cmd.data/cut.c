# include "data.h"

enum {SUM, MEAN, MEDIAN, INNER, NGOOD};

double cutstat (float *value, int Nvalue, int mode) {

  double output = 0.0;

  if (Nvalue == 0) return NAN;

  if (mode == MEDIAN) {
    fsort (value, Nvalue);
    if (Nvalue % 2) {
      int Ncenter = Nvalue / 2;
      myAssert ((Ncenter >= 0) && (Ncenter < Nvalue), "cutstat");
      output = value[Ncenter];
    } else {
      int Ncenter = Nvalue / 2 - 1;
      myAssert ((Ncenter >= 0) && (Ncenter < Nvalue - 1), "cutstat");
      output = 0.5*(value[Ncenter] + value[Ncenter + 1]);
    }
    return output;
  } 
  if (mode == INNER) {
    fsort (value, Nvalue);

    int Ns = 0, Ne = 0;
    if (Nvalue % 2) {
      // for an odd number of points take the same number below
      // and above the center value
      int Ncenter = Nvalue / 2;
      int Nquarter = 0.25*Nvalue;
      Ns = Ncenter - Nquarter;
      Ne = Ncenter + Nquarter;
    } else {
      // for an even number, the middle lies between two points
      // take the same number below as above
      int Ncenter = (int)(Nvalue / 2) - 1;
      int Nquarter = 0.25*Nvalue;
      Ns = Ncenter     - Nquarter;
      Ne = Ncenter + 1 + Nquarter;
    }
    int Nv = 0;
    for (int i = Ns; i <= Ne; i++) {
      myAssert ((i >= 0) && (i < Nvalue), "cutstat");
      output += value[i];
      Nv ++;
    }
    output /= Nv;
    return output;
  }
  if (mode == NGOOD) {
    return Nvalue;
  }
  if ((mode == MEAN) || (mode == SUM)) {
    for (int i = 0; i < Nvalue; i++) {
      myAssert ((i >= 0) && (i < Nvalue), "cutstat");
      output += value[i];
    }
    if (mode == MEAN) { output /= Nvalue; }
    return output;
  }
  return NAN;
}

int cut (int argc, char **argv) {
  
  int i, j, N, Nx, Ny, Mode;
  float *Vin, *Vbuf;
  int sx, sy, nx, ny;
  Vector *xvec, *yvec;
  Buffer *buf;

  Mode = SUM;
  if ((N = get_argument (argc, argv, "-sum"))) {
    remove_argument (N, &argc, argv);
    Mode = SUM;
  }
  if ((N = get_argument (argc, argv, "-median"))) {
    remove_argument (N, &argc, argv);
    Mode = MEDIAN;
  }
  if ((N = get_argument (argc, argv, "-mean"))) {
    remove_argument (N, &argc, argv);
    Mode = MEAN;
  }
  if ((N = get_argument (argc, argv, "-inner"))) {
    remove_argument (N, &argc, argv);
    Mode = INNER;
  }
  if ((N = get_argument (argc, argv, "-ngood"))) {
    remove_argument (N, &argc, argv);
    Mode = NGOOD;
  }

  if (argc != 9) {
    gprint (GP_ERR, "USAGE: cut <buffer> <X vector> <Y vector> <X|Y> sx sy nx ny\n");
    gprint (GP_ERR, "  extract a vector in the X or Y direction based on the pixel values in window\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
 
  sx = atof (argv[5]);
  sy = atof (argv[6]);
  nx = atof (argv[7]);
  ny = atof (argv[8]);

  Nx = buf[0].matrix.Naxis[0];
  Ny = buf[0].matrix.Naxis[1];

  // ny & nx do not need to be constrained by the buffer, but they do need to be sensible
  if ((nx < 0) || (ny < 0) || (nx > 1e8) || (ny > 1e8)) {
    gprint (GP_ERR, "warning : extraction size is crazy: %d,%d\n", nx, ny);
    return (FALSE);
  }

  if ((xvec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[3], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  switch (argv[4][0]) {
  case 'x':
  case 'X':
    /* create output vectors */
    ResetVector (xvec, OPIHI_FLT, nx);
    ResetVector (yvec, OPIHI_FLT, nx);
    bzero (yvec[0].elements.Flt, nx*sizeof(opihi_flt));
    for (i = 0; i < nx; i++) {
      xvec[0].elements.Flt[i] = i + sx; 
    }
    ALLOCATE (Vbuf, float, MAX (ny, 1));

    for (i = 0; i < nx; i++) {
      // for out-of-range areas, set the output pixels to NAN
      if (i + sx < 0) {
	yvec[0].elements.Flt[i] = NAN;
	continue;
      }
      if (i + sx >= Nx) {
	yvec[0].elements.Flt[i] = NAN;
	continue;
      }

      /* accumulate values */
      // skip out-of-range areas in y
      if (sy < 0) {
	ny += sy;
	sy = 0;
      }
      if (sy > Ny) sy = Ny;
      if (sy + ny >= Ny) ny = Ny - sy;

      Vin = (float *)(buf[0].matrix.buffer) + sy*Nx + sx + i; 
      int Npix = 0;
      for (j = 0; j < ny; j++, Vin += Nx) {
	if (!isfinite(*Vin)) continue;
	Vbuf[Npix] = *Vin;
	Npix ++;
      }
      yvec[0].elements.Flt[i] = cutstat (Vbuf, Npix, Mode);
    }
    free (Vbuf);
    break;
    
  case 'y':
  case 'Y':
    ResetVector (xvec, OPIHI_FLT, ny);
    ResetVector (yvec, OPIHI_FLT, ny);
    bzero (yvec[0].elements.Flt, ny*sizeof(opihi_flt));
    for (i = 0; i < ny; i++) {
      xvec[0].elements.Flt[i] = i + sy; 
    }
    ALLOCATE (Vbuf, float, MAX (nx, 1));

    for (i = 0; i < ny; i++) {
      // for out-of-range areas, set the output pixels to NAN
      if (i + sy < 0) {
	yvec[0].elements.Flt[i] = NAN;
	continue;
      }
      if (i + sy >= Ny) {
	yvec[0].elements.Flt[i] = NAN;
	continue;
      }

      /* accumulate values */
      // skip out-of-range areas in x
      if (sx < 0) {
	nx += sx;
	sx = 0;
      }
      if (sx > Nx) sx = Nx;
      if (sx + nx >= Nx) nx = Nx - sx;

      Vin = (float *)(buf[0].matrix.buffer) + (sy + i)*Nx + sx; 
      int Npix = 0;
      for (j = 0; j < nx; j++, Vin ++) {
	if (!isfinite(*Vin)) continue;
	Vbuf[Npix] = *Vin;
	Npix ++;
      }
      yvec[0].elements.Flt[i] = cutstat (Vbuf, Npix, Mode);
    }
    free (Vbuf);
    break;

  default:
    gprint (GP_ERR, "<dir> can be X or Y\n");
    return (FALSE);
    break;
  }

  return (TRUE);
}

