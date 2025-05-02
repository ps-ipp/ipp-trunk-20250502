# include "gophot.h"

/* these don't need to be 'static' unless we intend to call these 
   functions outside of this file... */
   
static int *x;
static int *y;
static int Npts = 0;
static int NPTS = 0;

static int dx[] = {-1, 1, 0, 0};
static int dy[] = {0, 0, -1, 1};
static int Ntry = 4;

static float *mediansky;
static int Nx, Ny;
static float fx, fy;

float *copy_mediansky (int *, int *, float *, float *);
int set_value (int, int, float);
void large_features (float, float);
void get_neighbors (int, int, float);
float get_value (int, int);
int outline (float, float, float, float, float, float, float *);
float delete_ellipse (float *, float);

void large_features (float sky, float dsky) {

  int i, j, k;
  float min, flux, dSky;
  float Xo, Yo, dX, dY;
  float Xmin, Xmax, Ymin, Ymax;
  float fitpars[5];

  nregion = 0;

  /* use a copy to protect original */
  mediansky = copy_mediansky (&Nx, &Ny, &fx, &fy);

  dSky = sqrt(sky + rnoise);
  dsky = MAX (dsky, dSky);

  min = sky + 3*dsky;
  fprintf (stderr, "sky %f, %f, %f\n", sky, dsky, min);
  /* find pixels which stand above threshold */

  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++) {
      if (mediansky [i + j*Nx] > min) {
	/* this is always the first point in the group */
	/* init the storage arrays */
	Npts = 0;
	NPTS = 100;
	ALLOCATE (x, int, NPTS);
	ALLOCATE (y, int, NPTS);
	x[Npts] = i;
	y[Npts] = j;
	Npts ++;
	get_neighbors (i, j, min);
	/* we now have a list x, y, Npts */
	Xmin = Xmax = x[0];
	Ymin = Ymax = y[0];
	for (k = 0; k < Npts; k++) {
	  Xmin = MIN (Xmin, x[k]);
	  Ymin = MIN (Ymin, y[k]);
	  Xmax = MAX (Xmax, x[k]);
	  Ymax = MAX (Ymax, y[k]);
	}
	Xo = 0.5*(Xmax + 1 + Xmin);
	Yo = 0.5*(Ymax + 1 + Ymin);
	dX = (Xmax + 1 - Xmin);
	dY = (Ymax + 1 - Ymin);
	convert_coords (&Xo, &Yo, &dX, &dY);
	fprintf (stderr, "large feature: %f %f  %f %f\n", Xo, Yo, dX, dY);
	outline (Xo, Yo, 0.4*dX, 0.4*dY, sky + 7*dsky, 2*dsky, fitpars);
	flux = delete_ellipse (fitpars, sky);
	fprintf (stderr, " flux: %f  %f\n", flux, sky);
	region[nregion][0] = sky;
	region[nregion][1] = flux;
	region[nregion][2] = fitpars[0];
	region[nregion][3] = fitpars[1];
	region[nregion][4] = fitpars[2];
	region[nregion][5] = fitpars[3];
	region[nregion][6] = fitpars[4];
	region[nregion][7] = flux;
	nregion ++;
	if (nregion == 100) {
	  fprintf (stderr, "too many regions!\n");
	  exit (0);
	}
      }
    }
  }
}

void get_neighbors (int ix, int iy, float min) {

  int i, Ix, Iy;

  for (i = 0; i < Ntry; i++) {
    
    Ix = ix + dx[i];
    Iy = iy + dy[i];

    if (get_value(Ix, Iy) > min) {
      
      x[Npts] = Ix;
      y[Npts] = Iy;
      Npts ++;
      if (Npts == NPTS) {
	NPTS += 100;
	REALLOCATE (x, int, NPTS);
	REALLOCATE (y, int, NPTS);
      } 
      
      set_value (Ix, Iy, 0);
      
      get_neighbors (Ix, Iy, min);
      
    }
  }
}

float get_value (int i, int j) {

  float value;

  if (i < 0) return (0);
  if (i >= Nx) return (0);

  if (j < 0) return (0);
  if (j >= Ny) return (0);

  value = mediansky[i + Nx*j];

  return (value);

}

int set_value (int i, int j, float value) {

  if (i < 0) return (0);
  if (i >= Nx) return (0);

  if (j < 0) return (0);
  if (j >= Ny) return (0);

  mediansky[i + Nx*j] = value;;

  return (1);

}

convert_coords (float *X, float *Y, float *dX, float *dY) {

  *X /= fx;
  *dX /= fx;
  *Y /= fy;
  *dY /= fy;

}
