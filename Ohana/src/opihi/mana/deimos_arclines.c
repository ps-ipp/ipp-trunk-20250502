# include "data.h"

// global parameters of the slit kernel:
static int   Nkernel_pix = 0; // total number of pixels
static int   Nkernel_y2  = 0;  // center of the interpolation 
static float *kernel     = NULL; 

void deimos_make_LSF_kernel (Vector *LSF, float stilt, int Nx);

int deimos_arclines (int argc, char **argv) {

  // arclines (buffer) (LSF) (STILT) (coord) (flux)

  int N;
  
  Vector *LSF     = NULL;
  Vector *flux    = NULL;
  Vector *coord   = NULL;
  Buffer *buff    = NULL;

  Buffer *kern    = NULL;
  if ((N = get_argument (argc, argv, "-save-kern"))) {
    remove_argument (N, &argc, argv);
    if ((kern = SelectBuffer (argv[N], ANYBUFFER, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: deimos arclines (buffer) (LSF) (STILT) (coord) (flux)\n");
    gprint (GP_ERR, "  inputs:  buffer (observed 2D flux), LSF (line spread function vector), STILT (degrees)\n");
    gprint (GP_ERR, "  outputs: coord (dispersion coordinate vector), flux (intensity at coord)\n");
    gprint (GP_ERR, "  options: [-save-kern (buffer)]\n");
    return FALSE;
  }

  // XXX I probably should rename FindSpline as SelectSpline and give it the same behavior
  if ((buff  = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((LSF   = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  float stilt = atof (argv[3]);
  if ((coord = SelectVector (argv[4], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  if ((flux  = SelectVector (argv[5], ANYVECTOR, TRUE)) == NULL) return (FALSE);

  // define the output window
  int Nx = buff[0].matrix.Naxis[0];
  int Ny = buff[0].matrix.Naxis[1];
  
  ResetVector (coord, OPIHI_FLT, Ny);
  ResetVector (flux,  OPIHI_FLT, Ny);

  float *Fin = (float *) buff[0].matrix.buffer;

  // generate a convolution kernel 
  deimos_make_LSF_kernel (LSF, stilt, Nx);

  // FILE *Fout = NULL;

  // loop over the rows
  for (int iy = 0; iy < Ny; iy++) {
    float Fsum = 0;
    float Ksum = 0;

    // if (iy == 5802) Fout = fopen ("test.5802.dat", "w");
    // if (iy == 5815) Fout = fopen ("test.5815.dat", "w");

    // cross-correlation of buffer values and kernel values
    for (int ix = 0; ix < Nx; ix++) {
      for (int ky = -Nkernel_y2; ky <= Nkernel_y2; ky++) {
	int ko = ky + Nkernel_y2;
	if ((iy + ky) < 0) continue;
	if ((iy + ky) >= Ny) continue;
	if (!isfinite(Fin[ix + (iy + ky)*Nx])) continue;
	if (!isfinite(kernel[ix + ko*Nx])) continue;
	Fsum += Fin[ix + (iy + ky)*Nx] * kernel[ix + ko*Nx];
	Ksum += kernel[ix + ko*Nx];
	// if ((iy == 5815) || (iy == 5802)) {
	//   fprintf (Fout, "%d %d %d : %f %f\n", iy, ix, ky, Fin[ix + (iy + ky)*Nx], kernel[ix + ko*Nx]);
	// }
      }
    }

    // if ((iy == 5815) || (iy == 5802)) fclose (Fout);

    coord->elements.Flt[iy] = iy;
    flux->elements.Flt[iy] = Fsum;
  }

  if (kern) {
    ResetBuffer (kern, Nx, 2*Nkernel_y2+1, -32, 0.0, 1.0);
    float *kB = (float *) kern[0].matrix.buffer;
    for (int iy = 0; iy < 2*Nkernel_y2 + 1; iy++) {
      for (int ix = 0; ix < 2*Nx; ix++) {
	kB[ix + iy*Nx] = kernel[ix + iy*Nx];
      }
    }
  } 
  free (kernel);

  return TRUE;
}

// kernel for slit tilt; no LSF for now
void deimos_make_LSF_kernel (Vector *LSF, float stilt, int Nx) {

  // first, generate the interpolation kernels.  For a slit tilt of theta, there is a
  // displacement in the y-direction of dy = dx sin(theta) where dx is the x-coord
  // relative to the slit window center (Nx / 2).  we thus need an image of size Nx,
  // Nx*sin(theta)

  opihi_flt *LSFv = LSF->elements.Flt;

  int Ns = LSF->Nelements;
  int Ns2 = Ns/2;
  
  float sin_stilt = sin(stilt*RAD_DEG);
  int Ny = Ns + ceil(Nx * fabs(sin_stilt)) + 1; // one extra pixel buffer
  if (Ny < 3) Ny = 3; // minimum of 3 pixels (or kernel assumption below fails)

  if (Ny % 2 == 0) Ny ++; // force Nyk to be odd

  Nkernel_y2 = Ny/2; // Nkernel_y2 is the center pixel in the interpolation direction

  ALLOCATE (kernel, float, Nx*Ny);
  Nkernel_pix = Nx*Ny;
  for (int ix = 0; ix < Nkernel_pix; ix++) { kernel[ix] = 0.0; }

  // fprintf (stderr, "allocating %d pixels (%d,%d)\n", Nkernel_pix, Nx, Ny);

  // XXX use Xref not Nx/2?
  int Nx2 = floor(Nx/2);

  for (int ix = 0; ix < Nx; ix++) {
    // displacement in y-dir due to slit tilt:
    float dy = (ix - Nx2) * sin_stilt;
    int dyi = floor(dy);
    float dyf = dy - dyi;

    // offset of the LSF relative to the kernel window
    int Sy = Nkernel_y2 - Ns2 + dyi;

    // if fractional offset is small, do not interpolate
    int doInterp = fabs(dyf) < 1e-5 ? FALSE : TRUE;

    // loop over the LSF direction and insert into kernel
    for (int iy = 0; iy < Ny; iy++) {

      // equivalent coord in the LSF:
      int py = iy - Sy;
      if (py < 0) continue;
      if (py >= Ns) continue;

      // a default value:
      float vout = NAN;

      if (doInterp) {
	if ((py > 0) && (py < Ns - 1)) {
	  vout = LSFv[py]*(1 - dyf) + LSFv[py-1]*dyf;
	}
	if (py == 0) {
	  vout = LSFv[py]*dyf;
	}
	if (py == Ns - 1) {
	  vout = LSFv[Ns - 1]*(1.0 - dyf);
	}
      } else {
	vout = LSFv[py];
      }

      kernel[ix + iy*Nx] = vout;
    }
  }
}
