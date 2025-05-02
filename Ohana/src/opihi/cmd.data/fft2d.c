# include "data.h"

int fft2d (int argc, char **argv) {
  
  int ZeroImaginary, forward;
  int N, Nx, Ny;
  Buffer *Ire, *Iim, *Ore, *Oim;;

  forward = TRUE;
  if ((N = get_argument (argc, argv, "-inverse"))) {
    remove_argument (N, &argc, argv);
    forward = FALSE;
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

  Nx = Ire[0].header.Naxis[0];
  Ny = Ire[0].header.Naxis[1];

  // check input image dimensions (match lengths? binary lengths?)
  if (!ZeroImaginary) {
    if ((Nx != Iim[0].header.Naxis[0]) || 
	(Ny != Iim[0].header.Naxis[1])) {
      gprint (GP_ERR, "image size mismatch\n");
      return (FALSE);
    }
  }
  if (!IsBinary (Nx, NULL)) {
    gprint (GP_ERR, "Nx is not a binary number!\n");
    return (FALSE);
  }
  if (!IsBinary (Ny, NULL)) {
    gprint (GP_ERR, "Ny is not a binary number!\n");
    return (FALSE);
  }

  if ((Ore = SelectBuffer (argv[4], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  if ((Oim = SelectBuffer (argv[5], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  /* free up output space */
  gfits_free_matrix (&Ore[0].matrix);
  gfits_free_header (&Ore[0].header);
  gfits_free_matrix (&Oim[0].matrix);
  gfits_free_header (&Oim[0].header);
  
  /* fix up output headers (real) & allocate data buffer */
  if (!CreateBuffer (Ore, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
  if (!CreateBuffer (Oim, Nx, Ny, -32, 0.0, 1.0)) return FALSE;

  gfits_copy_header (&Ire[0].header, &Ore[0].header);
  gfits_copy_header (&Ire[0].header, &Oim[0].header);

  // copy data to output buffers (fft is done in place)
  memcpy (Ore[0].matrix.buffer, Ire[0].matrix.buffer, Nx*Ny*sizeof(float));

  if (ZeroImaginary) {
    memset (Oim[0].matrix.buffer, 0, Nx*Ny*sizeof(float));
  } else {
    memcpy (Oim[0].matrix.buffer, Iim[0].matrix.buffer, Nx*Ny*sizeof(float));
  }

  /* run the fft */
  fftND ((float *)Ore[0].matrix.buffer, (float *)Oim[0].matrix.buffer, 2, Ire[0].header.Naxis, forward);

  return (TRUE);
}
