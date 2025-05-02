# include "data.h"

enum {SQUASH_NONE, SQUASH_X, SQUASH_Y, SQUASH_Z};

int squash3d (int argc, char **argv) {
  
  int ox, oy, oz;
  Buffer *src;
  Buffer *tgt;

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: squash3d <3d> <2d> (dir)\n");
    gprint (GP_ERR, " dir: x -> squash in x-dir, y -> squash in y-dir, z -> squash in z-dir\n");
    return (FALSE);
  }

  int dir = SQUASH_NONE;
  if (!strcasecmp (argv[3], "x")) dir = SQUASH_X;
  if (!strcasecmp (argv[3], "y")) dir = SQUASH_Y;
  if (!strcasecmp (argv[3], "z")) dir = SQUASH_Z;
  if (!dir) {
    gprint (GP_ERR, "invalid direction %s\n", argv[3]);
    return FALSE;
  }

  if ((src = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if (src[0].matrix.Naxes < 3) {
    gprint (GP_ERR, "buffer is not 3D\n");
    return FALSE;
  }

  float *iBuf  = (float *) src[0].matrix.buffer;

  if ((tgt = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  gfits_free_matrix (&tgt[0].matrix);
  gfits_free_header (&tgt[0].header);

  switch (dir) {
    case SQUASH_X:
      {     
	int iNx = src[0].matrix.Naxis[0];
	int iNy = src[0].matrix.Naxis[1];
	int iNz = src[0].matrix.Naxis[2];

	// output is Nz,Ny
	int oNx = iNz;
	int oNy = iNy;
	if (!CreateBuffer (tgt, oNx, oNy, -32, 0.0, 1.0)) return FALSE;
	float *oBuf  = (float *) tgt[0].matrix.buffer;

	for (ox = 0; ox < oNx; ox ++) {
	  for (oy = 0; oy < oNy; oy ++) {
	    float val = 0.0;
	    for (oz = 0; oz < iNx; oz ++) { // src x is tgt z
	      val += iBuf[oz + oy*iNx + ox*iNx*iNy];
	    }
	    oBuf[ox + oy*oNx] = val;
	  }
	}
      }
      break;
      
    case SQUASH_Y:
      {     
	int iNx = src[0].matrix.Naxis[0];
	int iNy = src[0].matrix.Naxis[1];
	int iNz = src[0].matrix.Naxis[2];

	// output is Nx,Nz
	int oNx = iNx;
	int oNy = iNz;
	if (!CreateBuffer (tgt, oNx, oNy, -32, 0.0, 1.0)) return FALSE;
	float *oBuf  = (float *) tgt[0].matrix.buffer;

	for (ox = 0; ox < oNx; ox ++) {
	  for (oy = 0; oy < oNy; oy ++) { // src z is tgt y
	    float val = 0.0;
	    for (oz = 0; oz < iNy; oz ++) {
	      val += iBuf[ox + oz*iNx + oy*iNx*iNy];
	    }
	    oBuf[ox + oy*oNx] = val;
	  }
	}
      }
      break;
      
    case SQUASH_Z:
      {     
	int iNx = src[0].matrix.Naxis[0];
	int iNy = src[0].matrix.Naxis[1];
	int iNz = src[0].matrix.Naxis[2];

	// output is Nx,Ny
	int oNx = iNx;
	int oNy = iNy;
	if (!CreateBuffer (tgt, oNx, oNy, -32, 0.0, 1.0)) return FALSE;
	float *oBuf  = (float *) tgt[0].matrix.buffer;

	for (ox = 0; ox < oNx; ox ++) {
	  for (oy = 0; oy < oNy; oy ++) {
	    float val = 0.0;
	    for (oz = 0; oz < iNz; oz ++) {
	      val += iBuf[ox + oy*iNx + oz*iNx*iNy];
	    }
	    oBuf[ox + oy*oNx] = val;
	  }
	}
      }
      break;
  }

  return (TRUE);
}
