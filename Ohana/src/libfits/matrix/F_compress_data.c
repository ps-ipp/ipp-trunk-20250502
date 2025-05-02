# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>
# include <gfits_compress.h>

# define ESCAPE(RET) { fprintf (stderr, "gzip error in %s @ %s:%d\n", __func__, __FILE__, __LINE__); return (RET); }

// XXX I'm putting Nx and Ny on the argument list -- only used by hcompress
// with a little work, these could go on the option list
int gfits_compress_data (char *zdata, unsigned long int *Nzdata, char *cmptype, char **optname, char **optvalue, int Nopt, char *rawdata, unsigned long int Nrawpix, int rawpix_size, int Nx, int Ny) {

  int status;

  // do not actually compress : this is used for testing
  if (!strcasecmp(cmptype, "NONE") || !strcasecmp(cmptype, "NONE_1") || !strcasecmp(cmptype, "NONE_2")) {
    unsigned long Nbytes = Nrawpix * rawpix_size;

    memcpy (zdata, rawdata, Nbytes);
    *Nzdata = Nbytes;
    return (TRUE);
  }

  // GZIP_1 uses inflate / deflate, GZIP_2 does as well, but operates on a buffer with
  // a different byte organization (gfits_distribute_gzp2)
  if (!strcasecmp(cmptype, "GZIP_1") || !strcasecmp(cmptype, "GZIP_2")) {
    unsigned long Nbytes = Nrawpix * rawpix_size;

    // the data must be byteswapped before compression begins
    // if (!gfits_byteswap_zdata (rawdata, Nrawpix, rawpix_size)) return (FALSE);

    // Nzdata is the size of the allocated buffer before the function is called, returns the actual size
    // Nbytes is the number of bytes in the raw data buffer
    uLongf destLen = *Nzdata;
    status = gfits_compress ((Bytef *) zdata, &destLen, (Bytef *) rawdata, Nbytes);
    *Nzdata = destLen;
    if (status != Z_OK) ESCAPE(FALSE);
    return (TRUE);
  }

  if (!strcasecmp(cmptype, "RICE_1") || !strcasecmp(cmptype, "RICE_ONE")) {
    long int Nout;
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

    // Ninsum += Nzdata;
    // Noutsum += *Nout;
    // fprintf (stderr, "%d comp bytes; %d uncomp 'pixels', totals: %d %d\n", Nzdata, Npix, Ninsum, Noutsum);

    switch (rawpix_size) {
      case 4:
	// rice compression from the CFITSIO source tree
	Nout = fits_rcomp ((int *) rawdata, Nrawpix, (unsigned char *) zdata, *Nzdata, blocksize);
	break;

      case 2:
	// Nout is the number of bytes in the compressed buffer
	Nout = fits_rcomp_short ((short *) rawdata, Nrawpix, (unsigned char *) zdata, *Nzdata, blocksize);

	if (0) {
	  long int k;
	  fprintf (stderr, "Nout: %lu, Nrawpix: %lu\n", Nout, Nrawpix);
	  fprintf (stderr, "cmp inp: "); 
	  for (k = 0; k < Nout; k++) { fprintf (stderr, "%02hhx", zdata[k]); if (k % 2) fprintf (stderr, " "); } 
	  fprintf (stderr, "\n");
	}
	break;

      case 1:
	Nout = fits_rcomp_byte ((char *) rawdata, Nrawpix, (unsigned char *) zdata, *Nzdata, blocksize);
	break;
	
      default:
	fprintf (stderr, "invalid output pixel size %d\n", rawpix_size);
	*Nzdata = 0;
	return (FALSE);
    }
    if (Nout < 0) {
      fprintf (stderr, "error in rice decompression\n");
      *Nzdata = 0;
      return (FALSE);
    }
    if (Nout > (long int) *Nzdata) {
      fprintf (stderr, "buffer overrun! %ld out, %lu available\n", Nout, *Nzdata);
      return FALSE;
    }
    *Nzdata = (unsigned long int) Nout;
    return TRUE;
  }
  
  if (!strcasecmp(cmptype, "PLIO_1")) {
    // note the fortan starting point: zdata is decremented at start
    int Nout = pl_p2li ((int *) rawdata, 1, (short *) zdata, Nrawpix);
    *Nzdata = Nout;
    if (!Nout) return (FALSE);
    return (TRUE);
  }

  if (!strcasecmp(cmptype, "HCOMPRESS_1")) {
    long Nbytes = Nrawpix * rawpix_size;
    int scale = 0;
    status = 0;
    // call hdecompress without smoothing

    if (rawpix_size == 4) {
      fits_hcompress ((int *) rawdata, Nx, Ny, scale, (char *) zdata, &Nbytes, &status);
    } 
    if (rawpix_size == 8) {
      fits_hcompress64 ((long long *) rawdata, Nx, Ny, scale, (char *) zdata, &Nbytes, &status);
    }

    if (status) {
      fprintf (stderr, "error in hcompress\n");
      return (FALSE);
    }
    // fprintf (stderr, "decompression yields image %d x %d (scale: %d)\n", Nx, Ny, scale);
    
    *Nzdata = Nbytes / rawpix_size;
    return (TRUE);
  }

  fprintf (stderr, "unknown compression %s\n", cmptype);
  return (FALSE);
}

