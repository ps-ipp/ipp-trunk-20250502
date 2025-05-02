# include "dimm.h"

int fillrow (char *buffer, int Nx, int offset, int sx, int *xs, int *xe);
int addpix (int pix);
int clearpix ();
int freepix ();
int statpix (float *x, float *y, float *buffer, int Nx);
int addstar (double x, double y);

Vector *vecx, *vecy, *vecf, *vecn;

int findstars (int argc, char **argv) {

  int i, j, I, J, Npix, Nbuf, status;
  int xs, xe, Nx, Ny;
  char *binary, *bp;
  float *ap, threshold, x, y;
  Buffer *buf;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: findstars (buffer) (threshold)\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  threshold = atof (argv[2]);

  if ((vecx = SelectVector ("star_x", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector ("star_y", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecf = SelectVector ("star_f", ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecn = SelectVector ("star_n", ANYVECTOR, TRUE)) == NULL) return (FALSE);

  vecx[0].Nelements = vecy[0].Nelements = 0;
  vecf[0].Nelements = vecn[0].Nelements = 0;

  REALLOCATE (vecx[0].elements, float, 1);
  REALLOCATE (vecy[0].elements, float, 1);
  REALLOCATE (vecf[0].elements, float, 1);
  REALLOCATE (vecn[0].elements, float, 1);

  /* binarize @ threshold */
  Nx = buf[0].header.Naxis[0];
  Ny = buf[0].header.Naxis[1];
  Npix = Nx*Ny;
  ALLOCATE (binary, char, Npix);

  ap = (float *) buf[0].matrix.buffer;
  bp = binary;
  bzero (bp, Npix);

  for (i = 0; i < Npix; i++, ap++, bp++) {
    if (*ap > threshold) *bp = 1;
  }

  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++) {
      if (binary[j*Nx + i]) {
	clearpix ();
	status = fillrow (binary, Nx, j*Nx, i, &xs, &xe);
	for (J = j + 1; (J < Ny) && status; J++) {
	  for (I = xs; !binary[J*Nx + I] && (I <= xe); I++);
	  status = fillrow (binary, Nx, J*Nx, I, &xs, &xe);
	}  
	/* we now have a stack of pixels, find geometric center */
	statpix (&x, &y, (float *)buf[0].matrix.buffer, Nx);
      }
    }
  }
  freepix ();
  return (TRUE);
}

/* find contiguous trigger pixels in row from starting point */
int fillrow (char *buffer, int Nx, int offset, int sx, int *xs, int *xe) {

  int i, pix, trigger;

  *xe = *xs = sx;
  trigger = FALSE;
  for (i = sx, pix = offset + i; buffer[pix] && (i < Nx); i++, pix++) {
    addpix (pix);
    buffer[pix] = 0;
    trigger = TRUE;
    *xe = i;
  }
  for (i = sx - 1, pix = offset + i; (i >= 0) && buffer[pix]; i--, pix--) {
    addpix (pix);
    buffer[pix] = 0;
    trigger = TRUE;
    *xs = i;
  }
  return (trigger);
}

static int Npixlist = 0;
static int *pixlist = (int *) NULL;

addpix (int pix) {
  Npixlist ++;
  if (pixlist == (int *) NULL) {
    ALLOCATE (pixlist, int, MAX (1, Npixlist));
  } else {
    REALLOCATE (pixlist, int, MAX (1, Npixlist));
  }    
  pixlist[Npixlist - 1] = pix;
}

clearpix () {
  Npixlist = 0;
  REALLOCATE (pixlist, int, 1);
}

freepix () {
  Npixlist = 0;
  free (pixlist);
  pixlist = (int *) NULL;
}

statpix (float *x, float *y, float *buffer, int Nx) {

  int i, X, Y, pix, Nv, No;
  double Sx, Sy, So;

  So = Sx = Sy = 0;
  for (i = 0; i < Npixlist; i++) {
    pix = pixlist[i];
    Y = pix / Nx;
    X = pix % Nx;
    So += buffer[pix];
    Sx += X * buffer[pix];
    Sy += Y * buffer[pix];
  }
  *x = Sx / So;
  *y = Sy / So;
  gprint (GP_ERR, "%f %f  %f %d\n", *x, *y, So, Npixlist);

  No = vecx[0].Nelements;
  Nv = No + 1;
  vecx[0].Nelements = vecy[0].Nelements = Nv;
  vecf[0].Nelements = vecn[0].Nelements = Nv;

  REALLOCATE (vecx[0].elements, float, MAX (Nv, 1));
  REALLOCATE (vecy[0].elements, float, MAX (Nv, 1));
  REALLOCATE (vecf[0].elements, float, MAX (Nv, 1));
  REALLOCATE (vecn[0].elements, float, MAX (Nv, 1));

  vecx[0].elements[No] = *x;
  vecy[0].elements[No] = *y;
  vecf[0].elements[No] = So;
  vecn[0].elements[No] = Npixlist;

}
