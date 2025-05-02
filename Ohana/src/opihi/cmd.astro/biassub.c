# include "astro.h"

int biassub (int argc, char **argv) {
  
  int i, j, k, N, dir, nlong, nwide, start;
  int sx, sy, nx, ny, NX, NY, NoVector, Nval;
  float *V, dV, *segment, val;
  opihi_flt *DV, *vect;
  Vector *xvec, *yvec;
  Buffer *buf;

  xvec = yvec = NULL;
  NoVector = TRUE;
  if ((N = get_argument (argc, argv, "-v"))) {
    NoVector = FALSE;
    remove_argument (N, &argc, argv);
    if ((xvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
    if ((yvec = SelectVector (argv[N], ANYVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: biassub <buffer> sx sy nx ny dir [-v N V]\n");
    gprint (GP_ERR, "  optional storage of vector and sequence in N and V\n");
    return (FALSE);
  }
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  sx = atof (argv[2]);
  sy = atof (argv[3]);
  nx = atof (argv[4]);
  ny = atof (argv[5]);
  dir = atof (argv[6]);
  if ((dir != 0) && (dir != 1)) {
    gprint (GP_ERR, " dir must be either 0 (x) or 1 (y)\n");
    return (FALSE);
  }
  if (dir) {
    start = sy;
    nwide = nx;
    nlong = ny;
  } else {
    start = sx;
    nwide = ny;
    nlong = nx;
  }    
  gprint (GP_LOG, "start: %d %d  size: %d %d\n", sx, sy, nx, ny);

    if ((sx < 0) || (sy < 0) || 
      (sx+nx > buf[0].matrix.Naxis[0]) || 
      (sy+ny > buf[0].matrix.Naxis[1])) {
    gprint (GP_ERR, "region out of range\n");
    return (FALSE);
  }

  ALLOCATE (vect, opihi_flt, nlong);
  ALLOCATE (segment, float, nwide);

  NX = buf[0].matrix.Naxis[0];
  NY = buf[0].matrix.Naxis[1];
  if (dir) {
    for (j = sy; j < sy + ny; j++) {
      V = (float *)(buf[0].matrix.buffer) + j*NX + sx; 
      for (i = 0; i < nx; i++, V++) {
	segment[i] = *V;
      }
      fsort (segment, nwide);
      val = Nval = 0;
      for (k = 0.25*nwide; k <=0.75*nwide; k++) {
	val += segment[k];
	Nval ++;
      }
      vect[j-sy] = val / Nval;
    }
  } else {
    for (i = 0; i < nx; i++) {
      V = (float *)(buf[0].matrix.buffer) + sy*NX + sx + i; 
      for (j = 0; j < ny; j++, V+=NX) {
	segment[j] = *V;
      }
      fsort (segment, nwide);
      val = Nval = 0;
      for (k = 0.25*nwide; k <=0.75*nwide; k++) {
	val += segment[k];
	Nval ++;
      }
      vect[i] = val / Nval;
    }
  }

  if (!NoVector) {
    ResetVector (xvec, OPIHI_FLT, nlong);
    ResetVector (yvec, OPIHI_FLT, nlong);
    for (i = 0; i < nlong; i++) {
      xvec[0].elements.Flt[i] = i + start;
      yvec[0].elements.Flt[i] = vect[i];
    }
  }

  if (dir) {
    /* here we run all the way across in X for the defined Y range */
    for (j = sy; j < sy + ny; j++) {
      V = (float *)(buf[0].matrix.buffer) + j*NX; 
      dV = vect[j];
      for (i = 0; i < NX; i++, V++) {
	*V -= dV;
      }
    }
  } else {
    /* here we run all the way across in Y for the defined X range */
    for (j = 0; j < NY; j++) {
      V = (float *)(buf[0].matrix.buffer) + j*NX + sx; 
      DV = vect;
      for (i = 0; i < nx; i++, V++, DV++) {
	*V -= *DV;
      }
    }
  }

  free (segment);
  free (vect);

  return (TRUE);
}

