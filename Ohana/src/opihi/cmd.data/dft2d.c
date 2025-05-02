# include "data.h"

// perform a 2D fourier transform.  

static int NxLast = 0;
static int NyLast = 0;
static float *Fterms = NULL;

int dft2d (int argc, char **argv) {
  
  int ix, iy, wx, wy;
  Buffer *src = NULL;
  Buffer *tgt = NULL;

  if (argc != 4) goto usage;
  
  if (strcasecmp(argv[2], "to")) goto usage;
  if ((src = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto usage;
  if ((tgt = SelectBuffer (argv[3], ANYBUFFER, TRUE)) == NULL) goto usage;

  int Nx = src[0].matrix.Naxis[0];
  int Ny = src[0].matrix.Naxis[1];
  ResetBuffer (tgt, Nx, Ny, -32, 0.0, 1.0);

  if (Nx % 2) goto usage;
  if (Ny % 2) goto usage;

  // generate and save the fourier terms, if needed
  if (Fterms && (NxLast == Nx) && (NyLast == Ny)) goto reuse;

  if (!Fterms) {
    ALLOCATE (Fterms, float, SQ(Nx*Ny));
    NxLast = Nx;
    NyLast = Ny;
  } else {
    REALLOCATE (Fterms, float, SQ(Nx*Ny));
    NxLast = Nx;
    NyLast = Ny;
  }

  // x-dir cosine terms
  float Wx = 2*M_PI/(float)Nx;
  float Wy = 2*M_PI/(float)Ny;
  for (wx = 0; wx <= Nx / 2; wx++) {
    for (wy = 0; wy <= Ny / 2; wy++) {
      float rx = ((wx == 0) || (wx == Nx/2)) ? (1.0/Nx) : (2.0/Nx);
      float ry = ((wy == 0) || (wy == Ny/2)) ? (1.0/Ny) : (2.0/Ny);
      float rn = rx*ry;
      for (ix = 0; ix < Nx; ix++) {
	float fxc = cos(wx*Wx*ix);
	for (iy = 0; iy < Ny; iy++) {
	  float fyc = cos(wy*Wy*iy);
	  int Nf = wx + wy*Ny;
	  int out = ix + iy*Nx + Nf*Nx*Ny;
	  Fterms[out] = fxc*fyc*rn;
	  if (wy == 0) continue;
	  if (wy == Ny/2) continue;
	  float fys = sin(wy*Wy*iy);
	  Nf = wx + (Ny - wy)*Ny;
	  out = ix + iy*Nx + Nf*Nx*Ny;
	  Fterms[out] = fxc*fys*rn;
	}
	if (wx == 0) continue;
	if (wx == Nx/2) continue;
	float fxs = sin(wx*Wx*ix);
	for (iy = 0; iy < Ny; iy++) {
	  float fyc = cos(wy*Wy*iy);
	  int Nf = (Nx - wx) + wy*Ny;
	  int out = ix + iy*Nx + Nf*Nx*Ny;
	  Fterms[out] = fxs*fyc*rn;
	  if (wy == 0) continue;
	  if (wy == Ny/2) continue;
	  float fys = sin(wy*Wy*iy);
	  Nf = (Nx - wx) + (Ny - wy)*Ny;
	  out = ix + iy*Nx + Nf*Nx*Ny;
	  Fterms[out] = fxs*fys*rn;
	}
      }
    }
  }

 reuse:
  {
    // generate and save the dot products
    float *sv = (float *)src->matrix.buffer;
    float *tv = (float *)tgt->matrix.buffer;
    for (wx = 0; wx <= Nx/2; wx++) {
      for (wy = 0; wy <= Ny/2; wy++) {
	float fsum = 0.0;
	int Nf = (wx + wy*Nx)*Nx*Ny;
	for (ix = 0; ix < Nx; ix++) {
	  for (iy = 0; iy < Ny; iy++) {
	    float Ft = Fterms[Nf + ix + iy*Nx];
	    float Fi = sv[ix + iy*Nx];
	    fsum += Ft*Fi;
	    // fprintf (stderr, "%d,%d : %d,%d : %f %f %f\n", ix, iy, wx, wy, Ft, Fi, fsum);
	  }
	}
	tv[wx + wy*Nx] = fsum;
      }
    }
  }
  return (TRUE);

 usage:
  gprint (GP_ERR, "USAGE: dft2d (input) to (output)\n");
  gprint (GP_ERR, "  NOTE: Nx & Ny must both be even\n");
  return (FALSE);
}


/*** 

     2D direct fourier transform:

     we have an image src of size [Nx,Ny].  in each direction,
     we can generate the following fourier components:

     cos(0*2pi*x/Nx), cos(1*2pi*x/Nx),... cos((Nx/2)  *2pi*x/Nx), 
     sin((Nx/2-1)*2pi*x/Nx), sin((Nx/2-2)*2pi*x/Nx),.. sin(1*2pi*x/Nx)

     Note that there are 2 fewer sin elements than there are cos
     elements:
       cos(0) = 1 -> DC term
       sin(0) = 0
       sin(pi*x) = 0 for integer values of x
		     
     we represent the output with the highest frequency in the middle and the DC element
     in the 0,0 corner

     to generate the output elements, we need to take the dot product 
     of the input image with an image for the given x,y frequency
 
     thus, for an Nx*Ny input image we need a cube of fourier terms of dimension
     Nx*Ny*(Nx*Ny) (only one of which is trivial)

     In the assumption that we will likely re-do the same size image multiple times, I
     store the fourier factor array once it is generated.

     NOTE: do I need to require Nx & Ny even?

 ***/
