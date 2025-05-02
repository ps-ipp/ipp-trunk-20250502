# include "astro.h"

/* local private functions */
float fgaussOD (float, float *, int, float *);

int imfit (int argc, char **argv) {

  float par[4], *v1, *v2, *dy, chisq, **covar;
  int i, Npts, Npar;
  Vector *xvec, *yvec, *svec;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: imfit <x> <y> <dy>\n");
    return (FALSE);
  }
  
  if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((svec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  Npts = xvec[0].Nelements;
  ALLOCATE (dy, float, Npts);
  v1 = svec[0].elements;
  v2 = dy;
  
  for (i = 0; i < Npts; i++, v1++, v2++) *v2 = 1.0 / (*v1 * *v1);
  
  par[0] = 7;
  par[1] = 2;
  par[2] = 6;
  par[3] = 1;
  Npar = 4;

  mrqinit (xvec[0].elements, yvec[0].elements, dy, Npts, par, Npar, fgaussOD);

  for (i = 0; i < 10; i++) {

    chisq = mrqmin (xvec[0].elements, yvec[0].elements, dy, Npts, par, Npar, fgaussOD);
    gprint (GP_ERR, "chisq: %f, %f %f %f %f\n", chisq, par[0], par[1], par[2], par[3]);

  }  

  covar = mrqcovar (Npar);

  for (i = 0; i < Npar; i++) {
    gprint (GP_ERR, "%d  %f  %f\n", i, par[i], covar[i][i]);
  }

  mrqfree (Npar);
  return (TRUE);
}


/* pars: x, y, sx, sy, sxy, sky I, */
float fgaussOD (float x, float *par, int Npar, float *dpar) {

  float X, S, Z, R, f;

  X = x - par[0];
  S = 1.0 / (par[1]*par[1]);
  Z = -0.5*X*X*S;
  R = exp (Z);
  f = par[2]*R + par[3];

  dpar[0] = par[2]*R*X*S;
  dpar[1] = dpar[0]*X/par[1];
  dpar[2] = R;
  dpar[3] = 1;
  
  return (f);

}

# if (0)

/* pars: x, y, sx, sy, sxy, sky I, */
float testF (float x, float *par, int Npar, float *dpar) {

  float f;

  f = par[0]*x + par[1];

  dpar[0] = x;
  dpar[1] = 1;
  
  return (f);

}


/* pars: x, y, sx, sy, sxy, sky I, */
float fgaussTD (float x, float y, float *par, int Npar) {

  X = x - par[0];
  Y = y - par[1];
  
  t1 = X / par[2];
  t2 = Y * Y / par[3];
  t3 = Y * par[4] * 2.0;

  r = 0.5 * ((t1 + t3)*X + t2);
  f = par[5] + par[6] / (1.0 + r*(1.0 + 0.5*r*(1.0 + 0.33333333*r)));
  
  return (f);

}

float chisq (float *buf, float *sig, int Nx, int Ny, float (func)(), float *par, int Npar) {

  float *ptr;

  X = 0;
  ptr = buf;
  for (i = 0; i < Nx; i++) {
    for (j = 0; j < Ny; j++, ptr++, sig++) {
      f = *ptr - func ((float) i, (float) j, par, Npar);
      X += (f * f) / *sig;
    }
  }    
  return (X);
}

  int i, j, Nbuf, status;
  char *string;
  double Npix, N1, N2, max, min, range, median;
  float *V;
  int sx, sy, nx, ny, *hist, Nhist, bin;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: imfit <buffer> sx sy nx ny\n");
    return (FALSE);
  }

  if (!SelectBuffer (&Nbuf, argv[1], OLDBUFFER)) return (FALSE);

  sx = atof (argv[2]);
  sy = atof (argv[3]);
  nx = atof (argv[4]);
  ny = atof (argv[5]);

  Npix = N1 = N2 = 0;
  if ((sx < 0) || (sy < 0) || 
      (sx+nx > buffers[Nbuf].matrix.Naxis[0]) || 
      (sy+ny > buffers[Nbuf].matrix.Naxis[1])) {
    gprint (GP_ERR, "region out of range\n");
    return (FALSE);
  }

  Npix = nx*ny;

  ALLOCATE (tempbuf, float, Npix);

  buf = tempbuf;
  for (j = 0; j < ny; j++) {
    V = (float *)(buffers[Nbuf].matrix.buffer) + (j+sy)*buffers[Nbuf].matrix.Naxis[0] + sx; 
    for (i = 0; i < nx; i++, V++) {
      *buf = *V;
    }
  }

# endif
