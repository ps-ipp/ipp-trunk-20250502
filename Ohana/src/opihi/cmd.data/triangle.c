# include "data.h"

# ifndef iSWAP
# define iSWAP(X,Y) {int tmp=(X); (X) = (Y); (Y) = tmp;}
# endif

int FillLineHorizontal (Buffer *buf, int Xs, int Xe, int Y, float value);

// algorithm adapted from http://www-users.mat.uni.torun.pl/~wrona/3d_tutor/tri_fillers.html
int triangle (int argc, char **argv) {

  int N;
  Buffer *buf;

  float value = 0;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    value  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 8) {
    gprint (GP_ERR, "USAGE: triangle <buffer> Xa Ya Xb Yb Xc Yc [-v value]\n");
    return (FALSE);
  }
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  float Xa = atof (argv[2]);
  float Ya = atof (argv[3]);
  float Xb = atof (argv[4]);
  float Yb = atof (argv[5]);
  float Xc = atof (argv[6]);
  float Yc = atof (argv[7]);

  // XXX perhaps allow points out of bounds and avoid them in the horizline function?
  // if (Xa < 0) goto error;
  // if (Xb < 0) goto error;
  // if (Xc < 0) goto error;
  // if (Ya < 0) goto error;
  // if (Yb < 0) goto error;
  // if (Yc < 0) goto error;
  // 
  // if (Xa >= buf[0].matrix.Naxis[0]) goto error;
  // if (Xb >= buf[0].matrix.Naxis[0]) goto error;
  // if (Xc >= buf[0].matrix.Naxis[0]) goto error;
  // if (Ya >= buf[0].matrix.Naxis[1]) goto error;
  // if (Yb >= buf[0].matrix.Naxis[1]) goto error;
  // if (Yc >= buf[0].matrix.Naxis[1]) goto error;

  // sort the points so Ya <= Yb <= Yc
  if (Yc < Yb) { SWAP (Xc, Xb); SWAP (Yc, Yb); }
  if (Yc < Ya) { SWAP (Xc, Xa); SWAP (Yc, Ya); }
  if (Yb < Ya) { SWAP (Xb, Xa); SWAP (Yb, Ya); }

  float dy1 = Yb - Ya;
  float dy2 = Yc - Ya;
  float dy3 = Yc - Yb;

  float dx1 = (dy1 > 0) ? (Xb - Xa) / dy1 : 0;
  float dx2 = (dy2 > 0) ? (Xc - Xa) / dy2 : 0;
  float dx3 = (dy3 > 0) ? (Xc - Xb) / dy3 : 0;

  float Xs = Xa, Xe = Xa;

  if (dx1 > dx2) {
    for (int y = Ya; y <= Yb; y++, Xs += dx2, Xe += dx1) {
      FillLineHorizontal (buf, (int) Xs, (int) Xe, y, value);
    }
    Xe = Xb; // is this needed?
    for (int y = Yb; y <= Yc; y++, Xs += dx2, Xe += dx3) {
      FillLineHorizontal (buf, (int) Xs, (int) Xe, y, value);
    }
  } else {
    for (int y = Ya; y <= Yb; y++, Xs += dx1, Xe += dx2) {
      FillLineHorizontal (buf, (int) Xs, (int) Xe, y, value);
    }
    Xs = Xb; // is this needed?
    for (int y = Yb; y <= Yc; y++, Xs += dx3, Xe += dx2) {
      FillLineHorizontal (buf, (int) Xs, (int) Xe, y, value);
    }
  }

  return (TRUE);

 // error:
 //  gprint (GP_ERR, "region out of range\n");
 //  return (FALSE);
}

// we fill in the pixels from Xs to Xe INCLUSIVE
int FillLineHorizontal (Buffer *buf, int Xs, int Xe, int Y, float value) {

  if (Y < 0) return FALSE;
  if (Y >= buf[0].matrix.Naxis[1]) return FALSE;

  int Xstart = MIN (MAX (Xs, 0), buf[0].matrix.Naxis[0] - 1);
  int Xend   = MIN (MAX (Xe, 0), buf[0].matrix.Naxis[0] - 1);

  if (Xend < Xstart) iSWAP (Xstart, Xend);

  int Yoff = Y * buf[0].matrix.Naxis[0];

  float *V = (float *) buf[0].matrix.buffer;
  for (int i = Xstart; i <= Xend; i++) {
    V[i + Yoff] = value;
  }
  return TRUE;
}
