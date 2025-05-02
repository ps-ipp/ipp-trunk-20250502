# include "astro.h"

int fiximage (int argc, char **argv) {

  int ix, iy;
  Buffer *in, *ct, *mask;

  // int VERBOSE = FALSE;
  // if ((N = get_argument (argc, argv, "-v"))) {
  //   VERBOSE = TRUE;
  //   remove_argument (N, &argc, argv);
  // }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: fiximage <data> <count> (mask)\n");
    return (FALSE);
  }

  if ((in   = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((ct   = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  if ((mask = SelectBuffer (argv[3], ANYBUFFER, TRUE)) == NULL) return (FALSE);

  int Nx = in[0].matrix.Naxis[0];
  int Ny = in[0].matrix.Naxis[1];

  gfits_free_matrix (&mask[0].matrix);
  gfits_free_header (&mask[0].header);
  if (!CreateBuffer (mask, Nx, Ny, -32, 0.0, 1.0)) return FALSE;
  strcpy (mask[0].file, "(empty)");
  memset (mask[0].matrix.buffer, 0, Nx*Ny*sizeof(float));
  // 0 -- init, cannot be repaired
  // 1 -- does not need to be repaired
  // 2 -- repair x only
  // 4 -- repair y only
  // 6 -- repair x & y only

  float *inB = (float *) in[0].matrix.buffer;
  float *ctB = (float *) ct[0].matrix.buffer;
  float *mkB = (float *) mask[0].matrix.buffer;

  // loop over input ct and find pixels that can be repaired.
  for (iy = 1; iy < Ny - 1; iy++) {
    for (ix = 1; ix < Nx - 1; ix++) {

      int Npix = ix + Nx*iy;

      // does not need to be repaired
      if (ctB[Npix]) { mkB[Npix] = 1.0; continue; }

      // if we have all 4 valid neighbors, we can fix
      if (ctB[Npix - 1 ] && ctB[Npix + 1 ]) mkB[Npix] += 2.0;
      if (ctB[Npix - Nx] && ctB[Npix + Nx]) mkB[Npix] += 4.0;
    }
  }

  // loop over input ct and repair the reparable pixels
  for (iy = 1; iy < Ny - 1; iy++) {
    for (ix = 1; ix < Nx - 1; ix++) {

      int Npix = ix + Nx*iy;

      if (mkB[Npix] == 0.0) continue; // cannot be repaired
      if (mkB[Npix] == 1.0) continue; // does not need to be repaired

      float Vxm = 0.0;
      float Vxp = 0.0;
      float Vym = 0.0;
      float Vyp = 0.0;
      float Value = 0.0;
      if ((mkB[Npix] == 2) || (mkB[Npix] == 6)) {
        Vxm = inB[Npix - 1 ] / ctB[Npix - 1 ];
        Vxp = inB[Npix + 1 ] / ctB[Npix + 1 ];
      }
      if ((mkB[Npix] == 4) || (mkB[Npix] == 6)) {
        Vym = inB[Npix - Nx] / ctB[Npix - Nx];
        Vyp = inB[Npix + Nx] / ctB[Npix + Nx];
      }

      if (mkB[Npix] == 2) {
        Value = 0.5*(Vxm + Vxp);
      }
      if (mkB[Npix] == 4) {
        Value = 0.5*(Vym + Vyp);
      }
      if (mkB[Npix] == 6) {
        Value = 0.25*(Vxm + Vxp + Vym + Vyp);
      }

      ctB[Npix] = 1;
      inB[Npix] = Value;
    }
  }

  return TRUE;
}
