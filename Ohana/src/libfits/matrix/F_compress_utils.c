# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>

// gfits_varlength returns a pointer to a chunk of Nzdata of data elements starting at zdata.

// XXX need to have an API to send the user data

// XXX need to byte-swap the table column; this needs to be worked out more clearly
// in the APIs: do the table read / column extract functions swap or not?.  we have
// put the byte-swapping in gifts_varlength_column_pointer, but this is fairly weak.

// XXX inconsistency between data element size between compressed data, uncompressed data, 
// and output pixel data.....

// need a structure to describe tiles, current tile?

typedef struct {
  unsigned long int *ztile; // max size of a tile (edge tiles may be smaller)
  int *otile; // counter for current tile (eg, for (2,3,0) otile[0] = 2, otile[1] = 3, otile[2] = 0)
  int *ntile; // number of tiles in dimension [i]
} ImageTile;

// advance the otile counter. 
int gfits_imtile_next (Matrix *matrix, unsigned long int *ztile, int *ntile, int *otile) {
  OHANA_UNUSED_PARAM(ztile);

  int i;

  // update the tile counters, carrying to the next dimension if needed
  for (i = 0; i < matrix->Naxes; i++) {
    otile[i] ++;
    if (otile[i] == ntile[i]) {
      if (i == matrix->Naxes - 1) return FALSE;
      otile[i] = 0;
    } else {
      return TRUE;
    }
  }
  return TRUE;
}

// how many tiles needed for this image (given ztile[])
int gfits_imtile_start (Matrix *matrix, int *otile) {

  int i;

  // update the tile counters, carrying to the next dimension if needed
  for (i = 0; i < matrix->Naxes; i++) {
    otile[i] = 0;
  }
  return TRUE;
}

// how many tiles needed for this image (given ztile[])
int gfits_imtile_count (Matrix *matrix, unsigned long int *ztile, int *ntile) {

  int i;

  int Ntiles = 1;
  for (i = 0; i < matrix->Naxes; i++) {
    ntile[i] = (matrix->Naxis[i] % ztile[i]) ? (matrix->Naxis[i] / ztile[i] + 1) : (matrix->Naxis[i] / ztile[i]);
    Ntiles *= ntile[i];
  }
  
  return (Ntiles);
}

// how many tiles needed for this image (given ztile[])
int gfits_imtile_maxsize (Matrix *matrix, unsigned long int *ztile) {

  int i;

  unsigned long int max_tile_size = 1;
  for (i = 0; i < matrix->Naxes; i++) {
    max_tile_size *= ztile[i];
  }
  
  return (max_tile_size);
}

// true sizes of this tile (in pixels)
off_t gfits_tile_size (Matrix *matrix, int *otile, unsigned long int *ztile) {

  off_t i, Npixels, Ndimen;

  Npixels = 1;
  for (i = 0; i < matrix->Naxes; i++) {
    Ndimen = MIN ((matrix->Naxis[i] - otile[i]*ztile[i]), ztile[i]);
    Npixels *= Ndimen;
  }
  
  return (Npixels);
}

int gfits_byteswap_zdata (char *zdata, off_t Nzdata, int pixsize) {

# define DOSWAP(A,B) { char tmp = A; A = B; B = tmp; }

# ifdef BYTE_SWAP

  off_t i;

  // fprintf (stderr, "swapping %d bytes in pix of size %d bytes...\n", Nzdata, pixsize);
  switch (pixsize) {
    case 0:
    case 1:
      break;

    case 2:
      for (i = 0; i < Nzdata; i+=2) {
	DOSWAP (zdata[i+0], zdata[i+1]);
      }
      break;

    case 4:
      for (i = 0; i < Nzdata; i+=4) {
	DOSWAP (zdata[i+0], zdata[i+3]);
	DOSWAP (zdata[i+1], zdata[i+2]);
      }
      break;

    case 8:
      for (i = 0; i < Nzdata; i+=8) {
	DOSWAP (zdata[i+0], zdata[i+7]);
	DOSWAP (zdata[i+1], zdata[i+6]);
	DOSWAP (zdata[i+2], zdata[i+5]);
	DOSWAP (zdata[i+3], zdata[i+4]);
      }
      break;
  }
# endif
  return (TRUE);
}

int gfits_extension_is_compressed_image (Header *header) {

    int has_extension, has_extname, has_zimage, zimage;
    char extname[80], extension[80];

    has_extension = gfits_scan (header, "XTENSION", "%s", 1, extension);
    has_extname = gfits_scan (header, "EXTNAME", "%s", 1, extname);
    has_zimage  = gfits_scan_alt (header, "ZIMAGE", "%t", 1, &zimage);

    if (has_extension && !strcmp (extension, "IMAGE")) return (FALSE);
    if (has_zimage && zimage) return (TRUE);
    if (has_extname) {
	if (!strcmp (extname, "COMPRESSED_IMAGE")) return (TRUE);
    }

    return (FALSE);
}

int gfits_extension_is_compressed_table (Header *header) {

    int has_ztable, ztable;
    has_ztable  = gfits_scan_alt (header, "ZTABLE", "%t", 1, &ztable);
    if (has_ztable && ztable) return (TRUE);
    return (FALSE);
}

int gfits_compressed_is_primary (Header *header) {

  int zimage = FALSE;
  // int ztension = FALSE;

    int has_zimage   = gfits_scan_alt (header, "ZIMAGE",   "%t", 1, &zimage);
    // int has_ztension = gfits_scan_alt (header, "ZTENSION", "%t", 1, &ztension);

    if (has_zimage && zimage) return (TRUE);

    return (FALSE);
}

int gfits_cmptype_valid (char *cmptype) {

  if (!strcasecmp(cmptype, "NONE"))   return TRUE;
  if (!strcasecmp(cmptype, "NONE_1")) return TRUE; // do not compress, but shuffle a la GZIP_1
  if (!strcasecmp(cmptype, "NONE_2")) return TRUE; // do not compress, but shuffle a la GZIP_2
  if (!strcasecmp(cmptype, "GZIP_1")) return TRUE;
  if (!strcasecmp(cmptype, "GZIP_2")) return TRUE;
  if (!strcasecmp(cmptype, "PLIO_1")) return TRUE;
  if (!strcasecmp(cmptype, "RICE_1")) return TRUE;
  if (!strcasecmp(cmptype, "RICE_ONE")) return TRUE;
  if (!strcasecmp(cmptype, "HCOMPRESS_1")) return TRUE;
  return FALSE;
}

// the size of the compressed data pixels, needed to perform swaps
int gfits_compressed_data_pixsize (char *cmptype, int out_bitpix, char **optname, char **optvalue, int Noptions) {

  // NONE should be swapped to mimic swapping types
  if (!strcasecmp(cmptype, "NONE") || !strcasecmp(cmptype, "NONE_1")) {
    if (out_bitpix ==   8) return (1);
    if (out_bitpix ==  16) return (2);
    if (out_bitpix ==  32) return (4);
    if (out_bitpix == -32) return (4);
    if (out_bitpix == -64) return (8);
    return (1);
  }

  // GZIP_1, GZIP_2 should not be swapped after compression / before decompression
  if (!strcasecmp(cmptype, "GZIP_1") || 
      !strcasecmp(cmptype, "GZIP_2") || 
      !strcasecmp(cmptype, "NONE_2")) {
    if (out_bitpix ==   8) return (1);
    if (out_bitpix ==  16) return (1);
    if (out_bitpix ==  32) return (1);
    if (out_bitpix == -32) return (1);
    if (out_bitpix == -64) return (1);
    return (1);
  }

  // RICE_1 should not be swapped after compression / before decompression
  if (!strcasecmp(cmptype, "RICE_1") ||
      !strcasecmp(cmptype, "RICE_ONE")) {
    return (1);
  }

  // PLIO_1 always results in 4-byte ints (not 2 byte ints?)
  if (!strcasecmp(cmptype, "PLIO_1")) {
    return (4);
  }

  if (!strcasecmp(cmptype, "HCOMPRESS_1")) {
    if (out_bitpix == 8)  return (4);
    if (out_bitpix == 16) return (4);
    return (8);
  }
  return (0);
}

int gfits_uncompressed_data_pixsize (char *cmptype, int out_bitpix, char **optname, char **optvalue, int Noptions, int rice_use_bitpix) {

  int i, Nbyte;

  // GZIP_1, GZIP_2 use matched input & output pixels
  if (!strcasecmp(cmptype, "GZIP_1") || 
      !strcasecmp(cmptype, "GZIP_2") || 
      !strcasecmp(cmptype, "NONE_2") || 
      !strcasecmp(cmptype, "NONE_1") || 
      !strcasecmp(cmptype, "NONE")) {
    if (out_bitpix ==   8) return (1);
    if (out_bitpix ==  16) return (2);
    if (out_bitpix ==  32) return (4);
    if (out_bitpix == -32) return (4);
    if (out_bitpix == -64) return (8);
    return (1);
  }

  // RICE_1 must operate on integer pixels
  if (!strcasecmp(cmptype, "RICE_1") ||
      !strcasecmp(cmptype, "RICE_ONE")) {
    // on compression, user may set BYTEPIX based on header.bitpix:
    if (rice_use_bitpix) {
      if (out_bitpix ==   8) return (1);
      if (out_bitpix ==  16) return (2);
      if (out_bitpix ==  32) return (4);
    }
    // if BYTEPIX option is specified, use that for Nbyte
    for (i = 0; i < Noptions; i++) {
      if (!strcmp(optname[i], "BYTEPIX")) {
	Nbyte = atoi (optvalue[i]);
	return (Nbyte);
      }
    }
    return (4); // RICE_1 default is 4 byte int if BYTEPIX undefined (White et al 6.2)
  }

  // PLIO_1 always results in 4-byte ints (not 2 byte ints?)
  if (!strcasecmp(cmptype, "PLIO_1")) {
    return (4);
  }

  if (!strcasecmp(cmptype, "HCOMPRESS_1")) {
    if (out_bitpix == 8)  return (4);
    if (out_bitpix == 16) return (4);
    return (8);
  }
  return (0);
}

int gfits_uncompressed_data_bitpix (char *cmptype, int out_bitpix, char **optname, char **optvalue, int Noptions, int rice_use_bitpix) {

  int i, Nbyte;

  // GZIP_1, GZIP_2 use matched input & output pixels
  if (!strcasecmp(cmptype, "GZIP_1") || 
      !strcasecmp(cmptype, "GZIP_2") || 
      !strcasecmp(cmptype, "NONE_1") || 
      !strcasecmp(cmptype, "NONE_2") || 
      !strcasecmp(cmptype, "NONE")) {
    return (out_bitpix);
  }

  // RICE_1 must operate on integer pixels
  if (!strcasecmp(cmptype, "RICE_1") ||
      !strcasecmp(cmptype, "RICE_ONE")) {
    if (rice_use_bitpix) {
      if (out_bitpix ==   8) return (out_bitpix);
      if (out_bitpix ==  16) return (out_bitpix);
      if (out_bitpix ==  32) return (out_bitpix);
    }
    // if BYTEPIX option is specified, use that for Nbyte
    for (i = 0; i < Noptions; i++) {
      if (!strcmp(optname[i], "BYTEPIX")) {
	Nbyte = atoi (optvalue[i]);
	return (Nbyte*8);
      }
    }
    return (32);
  }

  // PLIO_1 always results in 4-byte ints (not 2 byte ints?)
  if (!strcasecmp(cmptype, "PLIO_1")) {
    return (32);
  }

  if (!strcasecmp(cmptype, "HCOMPRESS_1")) {
    if (out_bitpix == 8)  return (32);
    if (out_bitpix == 16) return (32);
    return (64);
  }
  return (0);
}

int gfits_vartable_heap_pixsize (char format) {

  if (format == 'B') {
      return (1);
  }
  if (format == 'I') {
      return (2);
  }
  if (format == 'J') {
      return (4);
  }
  fprintf (stderr, "invalid size for compressed data: %c\n", format);  
  abort ();
}
