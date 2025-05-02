# include "data.h"

int shimage (int argc, char **argv) {
  
  int ix, iy;
  Buffer *FrIm, *FiIm, *src;

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: shimage (image) l m Fr Fi\n");
    gprint (GP_ERR, "  set the Fr and Fi values for pixels in the given image for (l,m)\n");
    return (FALSE);
  }

  if ((src = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  Coords coords;
  GetCoords (&coords, &src[0].header);
  if (!strcmp(&coords.ctype[4], "-WRP")) {
    fprintf (stderr, "mosaic astrometry not yet supported for vshimage\n");
    return FALSE;
  }

  int l = atoi(argv[2]);
  int m = atoi(argv[3]);

  if ((FrIm = SelectBuffer (argv[4], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((FiIm = SelectBuffer (argv[5], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  int Nx = src[0].header.Naxis[0];
  int Ny = src[0].header.Naxis[1];

  gfits_free_matrix (&FrIm[0].matrix); gfits_free_header (&FrIm[0].header); if (!CreateBuffer (FrIm, Nx, Ny, -32, 1.0, 0.0)) return FALSE;
  gfits_free_matrix (&FiIm[0].matrix); gfits_free_header (&FiIm[0].header); if (!CreateBuffer (FiIm, Nx, Ny, -32, 1.0, 0.0)) return FALSE;

  // l=0 allocates space for a single value
  SHterms *terms = SHtermsInit (0);

  float *Fr = (float *) FrIm[0].matrix.buffer;
  float *Fi = (float *) FiIm[0].matrix.buffer;

  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {

      int Npix = ix + iy*Nx;

      double R, D;
      int status = XY_to_RD (&R, &D, ix, iy, &coords);

      if (!status) {
	Fr[Npix] = NAN;
	Fi[Npix] = NAN;
	continue;
      }
      
      SHtermsForLM (terms, R, D, l, m); 

      Fr[Npix] = terms->Fr[0];
      Fi[Npix] = terms->Fi[0];
    }
  }
  SHtermsFree (terms);

  return (TRUE);
}
