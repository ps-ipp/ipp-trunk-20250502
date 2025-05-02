# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>
# include <gfits_compress.h>

# define ESCAPE(RET) { fprintf (stderr, "gzip error in %s @ %s:%d\n", __func__, __FILE__, __LINE__); return (RET); }

/* This function uncompresses the data in 'zdata' into the pre-allocated output buffer 'outdata'.
   The compression algorithm is chosen based on 'cmptype', with options specified by 'optname, optvalue, Nopt'
   zdata : input compressed data, Nzdata : number of compressed BYTES
   outdata : output buffer, Nout : number of actual output PIXELS, Nout_alloc : number of allocated bytes in the buffer, out_pixsize : bytes per output pixel
 */

int gfits_uncompress_data (char *zdata, unsigned long Nzdata, char *cmptype, char **optname, char **optvalue, int Nopt, char *outdata, unsigned long int *Nout, unsigned long int Nout_alloc, int out_pixsize) {

  int status;

  // do not actually uncompress : this is used for testing
  if (!strcasecmp(cmptype, "NONE") || !strcasecmp(cmptype, "NONE_1") || !strcasecmp(cmptype, "NONE_2")) {
    memcpy (outdata, zdata, Nzdata);
    myAssert (*Nout == Nzdata, "invalid size");
    *Nout = Nzdata / out_pixsize;
    return (TRUE);
  }

  if (!strcasecmp(cmptype, "GZIP_1") || !strcasecmp(cmptype, "GZIP_2")) {
    // unsigned long tNout = *Nout * out_pixsize;
    // input value of *Nout is number of BYTES
    unsigned long tNout = Nout_alloc;

    // for GZIP data, I need to check for and remove the header on the first block:
    // XXX maybe not anymore
    // gfits_gz_stripheader ((unsigned char *)zdata, &Nzdata);

    // uncompress does not require us to know the expected number of pixel; it tells us the number
    // XXX shouldn't we validate the result : we think we know the size
    status = gfits_uncompress ((Bytef *) outdata, &tNout, (Bytef *) zdata, Nzdata);
    if (status != Z_OK) ESCAPE(FALSE);
    myAssert (*Nout == tNout, "uncompressed size mismatch");
    *Nout = tNout / out_pixsize;
    // output value of *Nout is number of PIXELS

    // the resulting uncompressed data is byteswapped
    // if (!gfits_byteswap_zdata (outdata, *Nout, out_pixsize)) return (FALSE);

    return (TRUE);
  }

  if (!strcasecmp(cmptype, "RICE_1") || !strcasecmp(cmptype, "RICE_ONE")) {
    int i, blocksize;
    // look for the BLOCKSIZE
    blocksize = 32;
    for (i = 0; i < Nopt; i++) {
      if (!strcmp(optname[i], "BLOCKSIZE")) {
	blocksize = atoi (optvalue[i]);
	if ((blocksize != 16) && (blocksize != 32)) {
	  fprintf (stderr, "RICE blocksize is not valid: %d (%s = %s)\n", blocksize, optname[i], optvalue[i]);
	  return (FALSE);
	}
      }
    }

    int status = FALSE;
    unsigned long int Npix = *Nout / out_pixsize;

    switch (out_pixsize) {
      case 4:
	// rice decompression from the CFITSIO source tree : we need to tell it the expected number of pixels
	status = fits_rdecomp ((unsigned char *) zdata, Nzdata, (unsigned int *) outdata, Npix, blocksize);
	break;
      case 2:
	if (0) {
	  unsigned long int k;
	  fprintf (stderr, "Nout: %lu, Nrawpix: %lu\n", Nzdata, Npix);
	  fprintf (stderr, "cmp out: "); 
	  for (k = 0; k < Nzdata; k++) { fprintf (stderr, "0x%02hhx ", zdata[k]); } 
	  fprintf (stderr, "\n");
	}
	status = fits_rdecomp_short ((unsigned char *) zdata, Nzdata, (unsigned short *) outdata, Npix, blocksize);
	break;
      case 1:
	status = fits_rdecomp_byte ((unsigned char *) zdata, Nzdata, (unsigned char *) outdata, Npix, blocksize);
	break;
      default:
	fprintf (stderr, "invalid output pixel size %d\n", out_pixsize);
	status = FALSE;
	break;
    }
    if (status) {
      *Nout = 0;
      return FALSE;
    }
    *Nout = Npix;
    return TRUE;
  }
  
  if (!strcasecmp(cmptype, "PLIO_1")) {
    int Ntru = *Nout;
    int Npix;
    Npix = pl_l2pi ((short *) zdata, 1, (int *) outdata, Ntru);
    if (Npix != Ntru) {
      fprintf (stderr, "error in plio decompression\n");
      return (FALSE);
    }
    return (TRUE);
  }

  if (!strcasecmp(cmptype, "HCOMPRESS_1")) {
    unsigned long int Ntru = *Nout;
    int Nx, Ny, scale;
    status = 0;
    // call hdecompress without smoothing
    fits_hdecompress ((unsigned char *) zdata, FALSE, (int *) outdata, &Nx, &Ny, &scale, &status);
    if (status) {
      fprintf (stderr, "error in hdecompress\n");
      return (FALSE);
    }
    // fprintf (stderr, "decompression yields image %d x %d (scale: %d)\n", Nx, Ny, scale);
    
    unsigned long Npix = Nx * Ny;
    if (Npix != Ntru) {
      fprintf (stderr, "error in hdecompress: mismatched output size\n");
      return (FALSE);
    }
    return (TRUE);
  }

  fprintf (stderr, "unknown compression %s\n", cmptype);
  return (FALSE);
}

