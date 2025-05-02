# include "gophot.h"

static float *mediansky;
static int Nx, Ny;
static float fx, fy;

int make_mediansky () {

  /* we have an image of nfast x nslow pix.  
     make a new image of size sqrt(nfast) x sqrt(nslow)
     (2k x 4k -> 45 x 63)
  */

  float *temp;
  int i, j, I0, I1, J0, J1, I, J, n;
  int nx, ny;
  float Mv, Nv, Mv2, value;

  Nx = sqrt (nfast);
  Ny = sqrt (nslow);

  /* nthpix is used in isearch.c to update sky guess */
  nthpix = Nx;

  fx = (float) Nx / nfast;
  fy = (float) Ny / nslow;

  ALLOCATE (mediansky, float, Nx*Ny);

  nx = 1 + 1/fx;
  ny = 1 + 1/fy;

  ALLOCATE (temp, float, 2*nx*ny);

  Nv = Mv = Mv2 = 0.0;

  for (j = 0; j < Ny; j++) {
    for (i = 0; i < Nx; i++) {
      
      I0 = i / fx;
      J0 = j / fy;

      I1 = (i + 1) / fx;
      J1 = (j + 1) / fy;

      n = 0;
      temp[0] = 0;
      
      for (J = J0; J < J1; J++) {
	if (J < nbadbot) continue;
	if (J > nslow - nbadtop - 1) continue;
	for (I = I0; I < I1; I++) {
	  if (I < nbadleft) continue;
	  if (I > nfast - nbadright - 1) continue;
	  temp[n] = big[J*nfast + I];
	  n++;
	}
      }

      fsort (temp, n);
      value = temp[(int)(0.5*n)];

      if (n < 2) {
	mediansky[j*Nx + i] = MAGIC;
      } else {
	mediansky[j*Nx + i] = value;
      }
    }
  }
  return (0);
}

/* take array which is the median sky, copy to a separate vector, 
   sort, find median and sigma in 50% interval around median */

get_skystats (float *sky, float *dsky) {

  float *temp, sky2;
  int i, Npix, Nsky;

  Npix = Nx*Ny;

  ALLOCATE (temp, float, Npix);

  memcpy (temp, mediansky, Npix*sizeof(float));

  fsort (temp, Npix);

  *sky = temp[(int)(0.5*Npix)];

  Nsky = 0;
  sky2 = 0;
  for (i = 0.25*Npix; i < 0.75*Npix; i++) {
    if (!finite(temp[i])) continue;
    sky2 += SQ(temp[i] - *sky);
    Nsky ++;
  }
  
  *dsky = sqrt (sky2/Nsky);

  free (temp);

  mprint (0, "median sky: %f %f\n", *sky, *dsky);

  return (1);

}

fix_mediansky (float sky) {

  int i;

  for (i = 0; i < Nx*Ny; i++) {
    if (!finite (mediansky[i])) {
      mediansky[i] = sky;
    }
  }
}

/* this version is fast and has no edge problems. */

# if (1) 
float get_mediansky (int i, int j) {

  float value;
  int I, J;

  I = i * fx;
  J = j * fy;

  value = mediansky[J*Nx + I];

  return (value);

}
# endif

/* this version is slow, but more accurate.
   on the other hand, it needs to be fixed for
   edge problems. */

# if (0) 
float get_mediansky (int i, int j) {

  float Fx, Fy, Vo, Vm, Vx, Vy;
  float value;
  int I, J, dx, dy;

  I = i * fx;
  J = j * fy;

  Vo = mediansky[J*Nx + I];

  Fx = i * fx - I;
  if (Fx < 0.5) {
    dx = -1;
  } else {
    dx = +1;
    Fx = 1.0 - Fx;
  }
  Vm = mediansky[J*Nx + I + dx];
  Vx = (Vo*(0.5+Fx) + Vm*(0.5-Fx));

  Fy = j * fy - J;
  if (Fy < 0.5) {
    dy = -1;
  } else {
    dy = +1;
    Fy = 1.0 - Fy;
  }
  Vm = mediansky[J*Nx + I + dy*Nx];
  Vy = (Vo*(0.5+Fy) + Vm*(0.5-Fy));

  value = 0.5*(Vx + Vy);

  return (value);

}
# endif

float *copy_mediansky (int *nx, int *ny, float *Fx, float *Fy) {

  float *temp;

  ALLOCATE (temp, float, Nx*Ny);
  memcpy (temp, mediansky, Nx*Ny*sizeof(float));
  
  *nx = Nx;
  *ny = Ny;
  *Fx = fx;
  *Fy = fy;

  return (temp);

}

