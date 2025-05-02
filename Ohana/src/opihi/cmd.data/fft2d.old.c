# include "data.h"

int fft2dold (int argc, char **argv) {
  
  int i, N, Nx, Ny, Naxis[2];
  int Npix, ZeroImaginary, isign;
  float *t1, *t2, *out, *temp;
  Buffer *Ire, *Iim, *Ore, *Oim;;

  isign = 1;
  if ((N = get_argument (argc, argv, "-inverse"))) {
    remove_argument (N, &argc, argv);
    isign = -1;
  }

  if ((argc != 6) || (strcmp (argv[3], "to"))) {
    gprint (GP_ERR, "USAGE: fft2d (real) (imag) to (real) (imag)\n");
    return (FALSE);
  }

  /* select input / output buffers */
  Iim = NULL;
  ZeroImaginary = TRUE; /* Input(imaginary) may be 0, in which case we create a 0 filled image */
  if (!strcmp (argv[2], "0")) { 
  } else {
    ZeroImaginary = FALSE;
    if ((Iim = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  }    
  if ((Ire = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Ore = SelectBuffer (argv[4], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Oim = SelectBuffer (argv[5], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  /* free up output space */
  gfits_free_matrix (&Ore[0].matrix);
  gfits_free_header (&Ore[0].header);
  gfits_free_matrix (&Oim[0].matrix);
  gfits_free_header (&Oim[0].header);

  /* get image dimensions, check value */
  Npix = Ire[0].header.Naxis[0]*Ire[0].header.Naxis[1];
  Nx = Ire[0].header.Naxis[0];
  Ny = Ire[0].header.Naxis[1];
  Naxis[0] = Ny; Naxis[1] = Nx;
  if (!IsBinaryOld (Npix)) {
    gprint (GP_ERR, "dimensions are not binary!\n");
    return (FALSE);
  }
  
  /* create working space */
  t1 = (float *) Ire[0].matrix.buffer;
  ALLOCATE (temp, float, 2*Npix);
  out = temp;

  /* copy input to working space */
  if (ZeroImaginary) {
    for (i = 0; i < Npix; i++, t1++) {
      *out = *t1;
      out++;
      *out = 0;
      out++;
    }
  } else {
    t2 = (float *) Iim[0].matrix.buffer;
    for (i = 0; i < Npix; i++, t1++, t2++) {
      *out = *t1;
      out++;
      *out = *t2;
      out++;
    }
  } 
    
  /* run the fft */
  fftNold (temp, Naxis, 2, isign);

  /* fix up output headers (real) */
  gfits_copy_header (&Ire[0].header, &Ore[0].header);
  gfits_modify (&Ore[0].header, "NAXIS1", "%d", 1, Nx);
  gfits_modify (&Ore[0].header, "NAXIS2", "%d", 1, Ny);
  Ore[0].header.Naxis[0] = Nx;
  Ore[0].header.Naxis[1] = Ny;
  Ore[0].bitpix = Ire[0].bitpix;
  Ore[0].unsign = Ire[0].unsign;
  Ore[0].bscale = Ire[0].bscale;
  Ore[0].bzero  = Ire[0].bzero;
  gfits_create_matrix (&Ore[0].header, &Ore[0].matrix);

  /* fix up output headers (imaginary) */
  gfits_copy_header (&Ire[0].header, &Oim[0].header);
  gfits_modify (&Oim[0].header, "NAXIS1", "%d", 1, Nx);
  gfits_modify (&Oim[0].header, "NAXIS2", "%d", 1, Ny);
  Oim[0].header.Naxis[0] = Nx;
  Oim[0].header.Naxis[1] = Ny;
  Oim[0].bitpix = Ire[0].bitpix;
  Oim[0].unsign = Ire[0].unsign;
  Oim[0].bscale = Ire[0].bscale;
  Oim[0].bzero  = Ire[0].bzero;
  gfits_create_matrix (&Oim[0].header, &Oim[0].matrix);

  /* move data from working space to output buffers */
  out = temp;
  t1 = (float *) Ore[0].matrix.buffer;
  t2 = (float *) Oim[0].matrix.buffer;
  for (i = 0; i < Npix; i++, t1++, t2++) {
    *t1 = *out / Npix;
    out ++;
    *t2 = *out / Npix;
    out ++;
  }    

  free (temp);

  return (TRUE);
}
