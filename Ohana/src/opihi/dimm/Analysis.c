# include "dimm.h"

/* should this all be wrapped within an opihi implementation? */

int subtractImage (Image *a, Image *b) {

  if (a[0].Nx != b[0].Nx) return (FALSE);
  if (a[0].Ny != b[0].Ny) return (FALSE);

  Npix = a[0].Nx*a[0].Ny;
  ap = (float *) a[0].buffer;
  bp = (float *) b[0].buffer;
  for (i = 0; i < Npix; i++, ap++, bp++) {
    *ap -= *bp;
  }
  return (FALSE);
}

void statsImage (Image *image, Stats *stats) {

  val = (float *)image[0].buffer;
  max = min = val[0];
  Npix = image[0].Nx*image[0].Ny;
  for (i = 0; i < Npix; i++, val++) {
    N1 += *val;
    N2 += (*val)*(*val);
    max = MAX (max, *val);
    min = MIN (min, *val);
  }
  stats[0].mean  = N1 / Npix;
  stats[0].sigma = sqrt (N2 / Npix - SQ(stats[0].mean));
  stats[0].min = min;
  stats[0].max = max;

  stats[0].median = stats[0].mean;
  range = MAX (0.5, 0xffff / (max - min));
  if (range == 0) return;

  ALLOCATE (hist, int, 0x10000);
  bzero (hist, 0x10000*sizeof(int));

  val = (float *)image[0].buffer;
  for (i = 0; i < Npix; i++) {
    bin = MIN (MAX (0, (*val - min) * range), 0xffff);
    hist[bin] ++;
  }

  Nhist = 0;
  for (i = 0; (i < 0xffff) && (Nhist < 0.5*Npix); i++) 
    Nhist += hist[i];
  stats[0].median = i / range + min;
  free (hist);

  return;
}

# if (0)
void findStars (Image *image, Stars **stars, int *Nstars, double threshold) {

  /* binarize @ threshold */

  binimage = createImage (image[0].Nx, image[0].Ny);

  Npix = image[0].Nx*image[0].Ny;
  ap = image[0].buffer;
  bp = binimage[0].buffer;
  bzero (bp, Npix*sizeof (short));

  for (i = 0; i < Npix; i++, ap++, bp++) {
    if (*ap > threshold) * bp = 1;
  }

  clearpix ();

  for (i = 0; i < Ny; i++) {
    for (j = 0; j < Nx; j++) {
      pix = j + i*Ny;
      if (binimage.buffer[pix]) {
	addpix (pix);
	binimage.buffer[pix] = 0;
	/* continue in row to end */
	for (k = j + 1; k < Nx; k++) {
	  pix = k + i*Nx;
	  if (!binimage.buffer[pix]) { 
	  }
	}

/* find contiguous trigger pixels in row from starting point */

int fillrow (float *buffer, int Nx, int offset, int sx, int *xs, int *xe) {

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

static int Npix = 0;
static int *Pix = (int *) NULL;

addpix (int pix) {
  Npix ++;
  if (Pix == (int *) NULL) {
    ALLOCATE (Pix, int, MAX (1, Npix));
  } else {
    REALLOCATE (Pix, int, MAX (1, Npix));
  }    
  Pix[Npix - 1] = pix;
}

clearpix () {
  Npix = 0;
  REALLOCATE (Pix, int, 1);
}

statpix (double *x, double *y, float *buffer, int Nx) {

  int X, Y;
  double Sx, Sy, So;

  So = Sx = Sy = 0;
  for (i = 0; i < Npix; i++) {
    Y = pix / Nx;
    X = pix % Nx;
    So += buffer[pix];
    Sx += X * buffer[pix];
    Sy += Y * buffer[pix];
  }
  *x = Sx / So;
  *y = Sy / So;
}

/* find stars: 
   - binarize @ threshold
   - find all contiguous blobs
   - find geom center of each blob
*/



/*

.....................
....x................
...xxx...............
..........xxx..........
.........xx..........
.....................
.....................
.....................
.....................
.....................


 */

# endif

/* find stars: 
   - binarize @ threshold
   - find all contiguous blobs
   - find geom center of each blob
*/



/*

.....................
....x................
...xxx...............
....x...xxx..........
.........xx..........
.....................
.....................
.....................
.....................
.....................


 */
