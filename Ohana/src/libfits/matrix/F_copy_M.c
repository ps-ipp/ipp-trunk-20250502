# include <ohana.h>
# include <gfitsio.h>

/*********************** fits copy header ***********************************/
int gfits_copy_matrix (Matrix *in, Matrix *out) {

  int i;

  out[0].unsign   = in[0].unsign;
  out[0].bitpix   = in[0].bitpix;
  out[0].datasize = in[0].datasize;
  out[0].bzero    = in[0].bzero;
  out[0].bscale   = in[0].bscale;
  out[0].Naxes    = in[0].Naxes;
  for (i = 0; i < FT_MAX_NAXES; i++) 
    out[0].Naxis[i] = in[0].Naxis[i];

  if (out[0].buffer != NULL) free (out[0].buffer);
  ALLOCATE (out[0].buffer, char, out[0].datasize);
  
  memcpy (out[0].buffer, in[0].buffer, out[0].datasize);

  return (TRUE);
}	


