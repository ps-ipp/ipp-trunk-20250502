# include "astro.h"

enum {NONE, SKY, PIXEL, VECTOR, SCALAR};

int getcoords (int argc, char **argv) {

  int ix, iy, N;
  double c1, c2, t1, t2;
  Coords coords, moscoords;
  Buffer *input, *mosbuffer, *outC1, *outC2;
  CoordTransformSystem inTrans, outTrans;

  CoordTransform *transform = NULL;
  if ((N = get_argument (argc, argv, "-transform"))) {
    if (argc < N + 3) goto syntax;
    remove_argument (N, &argc, argv);

    switch (argv[N][0]) {
      case 'C': inTrans = COORD_CELESTIAL; break;
      case 'G': inTrans = COORD_GALACTIC; break;
      case 'E': inTrans = COORD_ECLIPTIC; break;
      default: goto syntax;
    }
    remove_argument (N, &argc, argv);

    switch (argv[N][0]) {
      case 'C': outTrans = COORD_CELESTIAL; break;
      case 'G': outTrans = COORD_GALACTIC; break;
      case 'E': outTrans = COORD_ECLIPTIC; break;
      default: goto syntax;
    }
    remove_argument (N, &argc, argv);
    
    transform = InitTransform (inTrans, outTrans);
    if (transform == NULL) {
      gprint (GP_ERR, "transform %c to %c is not yet defined\n", argv[1][0], argv[2][0]);
      return (FALSE);
    }
  }

  char *MOSAIC = NULL;
  if ((N = get_argument (argc, argv, "-mosaic"))) {
    remove_argument (N, &argc, argv);
    MOSAIC = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) goto syntax;
  if ((input = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto syntax;
  if ((outC1 = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) goto syntax;
  if ((outC2 = SelectBuffer (argv[3], ANYBUFFER, TRUE)) == NULL) goto syntax;

  GetCoords (&coords, &input[0].header);
  if (!strcmp(&coords.ctype[4], "-WRP")) {
    if (MOSAIC == NULL) {
      gprint (GP_ERR, "must supply mosaic for WRP coords with -mosaic [buffer]\n");
      return (FALSE);
    }
    if ((mosbuffer = SelectBuffer (MOSAIC, OLDBUFFER, TRUE)) == NULL) {
        // Using goto_escape triggers a compiler bug with our build flags when using gcc 4.3.2
        // It thinks MOSAIC may be uninitialzed even when it can't be. Just free 
        // goto escape;
        free (MOSAIC);
        return (FALSE);
    }
    GetCoords (&moscoords, &mosbuffer[0].header);
    coords.mosaic = &moscoords;
  }
  
  int Nx = input[0].matrix.Naxis[0];
  int Ny = input[0].matrix.Naxis[1];

  gfits_free_matrix (&outC1[0].matrix);
  gfits_free_header (&outC1[0].header);
  if (!CreateBuffer (outC1, Nx, Ny, -32, 1.0, 0.0)) return FALSE;

  gfits_free_matrix (&outC2[0].matrix);
  gfits_free_header (&outC2[0].header);
  if (!CreateBuffer (outC2, Nx, Ny, -32, 1.0, 0.0)) return FALSE;

  float *valC1 = (float *) outC1[0].matrix.buffer;
  float *valC2 = (float *) outC2[0].matrix.buffer;

  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {
      int status = XY_to_RD (&c1, &c2, (double) ix, (double) iy, &coords);
      if (!status) {
	valC1[ix + Nx*iy] = NAN;
	valC2[ix + Nx*iy] = NAN;
	continue;
      }
      if (transform) {
	ApplyTransform (&t1, &t2, c1, c2, transform);
	c1 = t1;
	c2 = t2;
      }
      valC1[ix + Nx*iy] = c1;
      valC2[ix + Nx*iy] = c2;
    }
  }
  if (transform) free (transform);

  return (TRUE);

 syntax:
  gprint (GP_ERR, "USAGE: getcoords [buffer] (crval1) (crval2) [-transform C/E/G C/E/G]\n");
  gprint (GP_ERR, "generate images of the two coordinate dimensions for the image WCS\n");
  // escape:
  // Jumping here and trying to free this here causes the compiler to fail on optimized build to fail See comments
  // above.
  // if (MOSAIC != NULL) free (MOSAIC);
  return (FALSE);
}
