# include "data.h"

int vshimage (int argc, char **argv) {
  
  int ix, iy;
  Buffer *Rbim, *Reim, *Dbim, *Deim, *src;

  if (argc != 8) {
    gprint (GP_ERR, "USAGE: vsh image l m dRb dRe dDb dDe\n");
    gprint (GP_ERR, "  set the dRb, dRe, dDb, dDe values for pixels in the given image for (l,m)\n");
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

  if ((Rbim = SelectBuffer (argv[4], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Reim = SelectBuffer (argv[5], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Dbim = SelectBuffer (argv[6], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Deim = SelectBuffer (argv[7], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  int Nx = src[0].header.Naxis[0];
  int Ny = src[0].header.Naxis[1];

  gfits_free_matrix (&Rbim[0].matrix); gfits_free_header (&Rbim[0].header); if (!CreateBuffer (Rbim, Nx, Ny, -32, 1.0, 0.0)) return FALSE;
  gfits_free_matrix (&Reim[0].matrix); gfits_free_header (&Reim[0].header); if (!CreateBuffer (Reim, Nx, Ny, -32, 1.0, 0.0)) return FALSE;
  gfits_free_matrix (&Dbim[0].matrix); gfits_free_header (&Dbim[0].header); if (!CreateBuffer (Dbim, Nx, Ny, -32, 1.0, 0.0)) return FALSE;
  gfits_free_matrix (&Deim[0].matrix); gfits_free_header (&Deim[0].header); if (!CreateBuffer (Deim, Nx, Ny, -32, 1.0, 0.0)) return FALSE;

  // l=0 allocates space for a single value
  VSHterms *terms = VSHtermsInit (0);

  float *Rb = (float *) Rbim[0].matrix.buffer;
  float *Re = (float *) Reim[0].matrix.buffer;
  float *Db = (float *) Dbim[0].matrix.buffer;
  float *De = (float *) Deim[0].matrix.buffer;

  for (ix = 0; ix < Nx; ix++) {
    for (iy = 0; iy < Ny; iy++) {

      int Npix = ix + iy*Nx;

      double R, D;
      int status = XY_to_RD (&R, &D, ix, iy, &coords);

      if (!status) {
	Rb[Npix] = NAN;
	Re[Npix] = NAN;
	Db[Npix] = NAN;
	De[Npix] = NAN;
	continue;
      }
      
      VSHtermsForLM (terms, R, D, l, m); 

      Rb[Npix] = terms->dR_B[0];
      Re[Npix] = terms->dR_E[0];
      Db[Npix] = terms->dD_B[0];
      De[Npix] = terms->dD_E[0];
    }
  }
  VSHtermsFree (terms);

  return (TRUE);
}
