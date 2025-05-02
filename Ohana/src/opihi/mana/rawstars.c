# include "mana.h"

int rawstars (int argc, char **argv) {
  
  int i, x, y, N, Nx, Ny, Np;
  float *v;
  double Raper, Rinner, Router;
  Vector *xp, *yp;
  Vector *xc, *yc, *sx, *sy, *sxy, *zs, *zc, *sk;
  Buffer *buff;

  Raper = 5;
  if ((N = get_argument (argc, argv, "-Raper"))) {
    remove_argument (N, &argc, argv);
    Raper = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  Rinner = 10;
  if ((N = get_argument (argc, argv, "-Rinner"))) {
    remove_argument (N, &argc, argv);
    Rinner = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  Router = 15;
  if ((N = get_argument (argc, argv, "-Router"))) {
    remove_argument (N, &argc, argv);
    Router = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc < 4) goto usage;

  if ((buff = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((xp = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yp = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (xp[0].Nelements != yp[0].Nelements) {
    gprint (GP_ERR, "vectors are not the same length\n");
    return (FALSE);
  }

  set_rough_radii (Raper, Rinner, Router);

  /* output vectors */
  if ((xc = SelectVector ("xc", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((yc = SelectVector ("yc", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((zc = SelectVector ("zc", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((zs = SelectVector ("zs", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((sk = SelectVector ("sk", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((sx = SelectVector ("sx", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((sy = SelectVector ("sy", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((sxy = SelectVector ("sxy", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  Nx = buff[0].matrix.Naxis[0];
  Ny = buff[0].matrix.Naxis[1];
  Np = xp[0].Nelements;

  ResetVector (xc, OPIHI_FLT, Np);
  ResetVector (yc, OPIHI_FLT, Np);
  ResetVector (sx, OPIHI_FLT, Np);
  ResetVector (sy, OPIHI_FLT, Np);
  ResetVector (sxy, OPIHI_FLT, Np);
  ResetVector (zs, OPIHI_FLT, Np);
  ResetVector (zc, OPIHI_FLT, Np);
  ResetVector (sk, OPIHI_FLT, Np);

  v = (float *) buff[0].matrix.buffer;
  for (i = 0; i < Np; i++) {
    x = (xp[0].type == OPIHI_FLT) ? xp[0].elements.Flt[i] : xp[0].elements.Int[i];
    y = (yp[0].type == OPIHI_FLT) ? yp[0].elements.Flt[i] : yp[0].elements.Int[i];
    if (x < 0) continue;
    if (x >= Nx) continue;
    if (y < 0) continue;
    if (y >= Ny) continue;

    get_rough_star (v, Nx, Ny, x, y, 
		    &xc[0].elements.Flt[i], 
		    &yc[0].elements.Flt[i], 
		    &sx[0].elements.Flt[i], 
		    &sy[0].elements.Flt[i], 
		    &sxy[0].elements.Flt[i], 
		    &zs[0].elements.Flt[i], 
		    &zc[0].elements.Flt[i],
		    &sk[0].elements.Flt[i]);
  }

  return (TRUE);

 usage:
  gprint (GP_ERR, "rawstars (buffer) (xp) (yp)\n");
  return (FALSE);
}

