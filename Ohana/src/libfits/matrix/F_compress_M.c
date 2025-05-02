# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>
# define VERBOSE_DUMP 0

int gfits_collect_gzp2 (Matrix *matrix, char *raw, unsigned long int Nraw, int raw_bitpix, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero);

// the user needs to specify the various compression options:
// ztile[i] (may be NULL)
// zcmptype 

// steps to convert an image to a compressed table:
// * determine the number of tiles (from ztile[] and image size)
// * construct an empty table with the right dimensions (1 column, Ntile rows)
// * loop over tiles
// * extract tile data to a buffer
// * compress the buffer data
// * insert in table heap
// * update header keywords

// we do not try to support compression of an image with an associated table (not valid)

# define ESCAPE { fprintf (stderr, "error in %s @ line %d\n", __func__, __LINE__); goto escape; }

int gfits_compress_image (Header *header, Matrix *matrix, FTable *ftable, unsigned long int *Ztile, char *zcmptype) {

  int i;

  // pointers to track the tile data
  int *otile = NULL;
  int *ntile = NULL;
  char *raw = NULL;
  char *zdata = NULL;

  unsigned long int *ztile = Ztile;

  Header *theader = ftable->header;

  if (!gfits_cmptype_valid(zcmptype)) ESCAPE;

  // determine the number of tiles (from ztile[] and image size)
  if (!ztile) {
    ALLOCATE (ztile, unsigned long int, matrix->Naxes);
    ztile[0] = matrix->Naxis[0];
    for (i = 1; i < matrix->Naxes; i++) ztile[i] = 1;
  }
  ALLOCATE (ntile, int, matrix->Naxes);
  ALLOCATE (otile, int, matrix->Naxes);

  int Ntile = gfits_imtile_count (matrix, ztile, ntile);

  // creates an empty (Naxes = 2, Naxis[i] = 0) table header with XTENSION = BINTABLE, EXTNAME = COMPRESSED_IMAGE
  if (!gfits_create_table_header (theader, "BINTABLE", "COMPRESSED_IMAGE")) ESCAPE;

  // by using "P" format, we are limited to 32bit pointers (2GB heap)
  // XXX how is the "B" format used?
  if (!gfits_define_bintable_column (theader, "1PB(0)", "COMPRESSED_DATA", "compressed image data", "none", 1.0, 0.0)) ESCAPE;
  if (!gfits_delete (theader, "TUNIT1", 1)) ESCAPE;

  // allocates the default data array (ftable->buffer)
  if (!gfits_create_table (theader, ftable)) ESCAPE;

  // copy original header to output header (XXX this needs to be finished)
  if (!gfits_copy_keywords_compress (header, theader)) ESCAPE;
  
  // this allocates the pointer data array for the full set of tiles (NOT the heap)
  char *tmpbuffer = NULL;
  ALLOCATE_ZERO (tmpbuffer, char, Ntile);
  if (!gfits_set_bintable_column (theader, ftable, "COMPRESSED_DATA", tmpbuffer, Ntile)) ESCAPE;
  free (tmpbuffer);

  // add ZIMAGE from output header
  if (!gfits_modify_alt (theader, "ZIMAGE", "%t", 1, TRUE)) ESCAPE;

  // define compression-specific keywords, update header as needed.
  if (!gfits_modify (theader, "ZCMPTYPE", "%s", 1, zcmptype)) ESCAPE;

  if (!gfits_modify_alt (theader, "ZSIMPLE", "%t", 1, TRUE)) ESCAPE;
  // gfits_modify (header, "ZTENSION", "%s", 1, exttype);

  // supply the Z* header values from the source image header
  if (!gfits_modify (theader, "ZBITPIX", "%d", 1, header[0].bitpix)) ESCAPE;

  if (!gfits_modify (theader, "ZNAXIS",  "%d", 1, header[0].Naxes)) ESCAPE;

  char keyword[11];
  for (i = 0; i < header[0].Naxes; i++) {
    snprintf_nowarn (keyword, 10, "ZNAXIS%d", i + 1);
    if (!gfits_modify (theader, keyword, OFF_T_FMT, 1,  header[0].Naxis[i])) ESCAPE;
  }

  for (i = 0; i < header->Naxes; i++) {
    snprintf_nowarn (keyword, 10, "ZTILE%d", i + 1);
    if (!gfits_modify (theader, keyword, "%lu", 1, ztile[i])) ESCAPE;
  }

  if (!gfits_modify (theader, "BSCALE", "%lf", 1, header[0].bscale)) ESCAPE;
  if (!gfits_modify (theader, "BZERO",  "%lf", 1, header[0].bzero)) ESCAPE;

  // other keywords to define:
  // ZEXTEND
  // ZBLOCKED
  // ZHECKSUM
  // ZDATASUM
  // PCOUNT -> ZPCOUNT
  // GCOUNT -> ZGCOUNT

  float zscale = 1.0;
  // gfits_modify (header, "ZSCALE", "%f", 1, zscale);

  float zzero = 0.0;
  // gfits_modify (header, "ZZERO", "%f", 1, zzero);

  int zblank = 32767;
  // gfits_modify (header, "ZBLANK", "%d", 1, zblank);

  int oblank = 32767;
  // gfits_modify (header, "BLANK", "%d", 1, oblank);

  // define compression-specific keywords, update header as needed.
  if (!strcasecmp(zcmptype, "GZIP_1") || !strcasecmp(zcmptype, "GZIP_2")) {
    if ((header[0].bitpix == -32) || (header[0].bitpix == -64)) {
      if (!gfits_modify (theader, "ZQUANTIZ", "%s", 1, "NONE")) ESCAPE;
    }
  }

# if (0)
  // need to set ZNAMEnn and ZVALnn if they are defined and used

  // search for algorithm-specific keywords. these are used to control compression options
  // note the difference between the keywords (1 indexed) and the variables (0 indexed)
  for (i = 0; i < Noptions; i++) {
    snprintf_nowarn (key, 10, "ZNAME%d", i + 1);
    if (!gfits_modify (header, key, "%s", 1, optname[i])) break;

    snprintf_nowarn (key, 10, "ZVAL%d", i + 1);
    if (!gfits_modify (header, key, "%s", 1, optvalue[i])) break;
  }
# endif

  VarLengthColumn zdef;
  if (!gfits_varlength_column_define (ftable, &zdef, 1)) ESCAPE;

  // pixel sizes and tile sizes:
  // * We have Ntile tiles, each of Nx * Ny * Nz ... tile pixels
  // -> A pixel in the tile (in the image) has size tile_pixsize = f(BITPIX)
  // * We copy the pixels in the tile to a continuous buffer to be compressed.
  //   During this copy, we can scale the data (eg, floats -> ints), in which 
  //   case the raw buffer may have a different pixel size (and type) than the
  //   image tile
  // -> A pixel in the raw buffer is f(zcmptype,BITPIX):
  // -- GZIP_1 : raw_pixsize = tile_pixsize (no scaling is performed)
  // -- GZIP_2 : raw_pixsize = tile_pixsize (no scaling is performed)
  // -- RICE_1 : raw pixels must be integers. raw_pixsize is user specified? (ZNAMEn,ZVALn : BYTEPIX = 1,2,4,8)
  // -- PLIO_1 : raw_pixsize = 2 bytes
  // -- HCOMPRESS_1 : raw pixels must be integers.  

  // size (in pixels) of the largest tile
  unsigned long int max_tile_size = gfits_imtile_maxsize (matrix, ztile);

  // size of a pixel in the compressed data (cmp_pixsize), used for byteswap below
  int cmp_pixsize = gfits_compressed_data_pixsize (zcmptype, header[0].bitpix, NULL, NULL, 0);

  // size of a pixel in the raw pixel buffer (before compression) (last option says RICE uses header.bitpix)
  int raw_pixsize = gfits_uncompressed_data_pixsize (zcmptype, header[0].bitpix, NULL, NULL, 0, TRUE);

  // we also need the bitpix value for the raw uncompressed buffer to distinguish int and floats
  int raw_bitpix  = gfits_uncompressed_data_bitpix (zcmptype, header[0].bitpix, NULL, NULL, 0, TRUE);

  // define compression-specific keywords, update header as needed.
  if (!strcasecmp(zcmptype, "RICE_1") || !strcasecmp(zcmptype, "RICE_ONE")) {
    if (!gfits_modify (theader, "ZNAME1", "%s", 1, "BLOCKSIZE")) ESCAPE;
    if (!gfits_modify (theader, "ZVAL1", "%d", 1, 32)) ESCAPE;
    if (!gfits_modify (theader, "ZNAME2", "%s", 1, "BYTEPIX")) ESCAPE;
    if (!gfits_modify (theader, "ZVAL2", "%d", 1, raw_pixsize)) ESCAPE;
  }

  // size of a pixel in the image tile -- this is probably not needed
# if VERBOSE_DUMP
  int tile_pixsize = abs(header[0].bitpix) / 8;
  fprintf (stderr, "raw_pixsize: %d, cmp_pixsize: %d, tile_pixsize: %d, raw_bitpix: %d\n", raw_pixsize, cmp_pixsize, tile_pixsize, raw_bitpix);
# endif

  // allocate the buffer for compression work
  unsigned long int Nzdata_alloc = raw_pixsize*max_tile_size + 100;
  ALLOCATE (raw,   char, raw_pixsize*max_tile_size);
  ALLOCATE (zdata, char, Nzdata_alloc);

  // init the otile[] counters
  if (!gfits_imtile_start (matrix, otile)) ESCAPE;

  // compress the data : copy into a tile, compress the tile, then add to the output table
  // each tile -> 1 row of the output table
  for (i = 0; i < Ntile; i++) {

    // size of the current tile in pixels
    unsigned long int Nraw = gfits_tile_size (matrix, otile, ztile);

    // copy the raw pixels from their native matrix locations to the temporary output buffer
    // for float -> int scaling by zscale, zzero may be applied
    if (!strcasecmp (zcmptype, "GZIP_2") || !strcasecmp (zcmptype, "NONE_2")) {
      if (!gfits_collect_gzp2 (matrix, raw, Nraw, raw_bitpix, otile, oblank, ztile, zblank, zscale, zzero)) ESCAPE;
    } else {
      if (!gfits_collect_data (matrix, raw, Nraw, raw_bitpix, otile, oblank, ztile, zblank, zscale, zzero)) ESCAPE;
    }

    if (VERBOSE_DUMP && (i == 0)) {
      int k;
      fprintf (stderr, "cmp mat: "); 
      for (k = 0; k < 64; k++) { fprintf (stderr, "%02hhx", matrix->buffer[k]); if (k % 4 == 3) fprintf (stderr, " "); } 
      fprintf (stderr, "\n");
      fprintf (stderr, "cmp raw: "); 
      for (k = 0; k < 64; k++) { fprintf (stderr, "%02hhx", raw[k]); if (k % 4 == 3) fprintf (stderr, " "); } 
      fprintf (stderr, "\n");
    }

    // gzip compresses bytes which are in BIG-ENDIAN order
    // Nraw is number of pixels
    if (!strcasecmp(zcmptype, "GZIP_1")) {
      if (!gfits_byteswap_zdata (raw, Nraw * raw_pixsize, raw_pixsize)) ESCAPE;
    }
    unsigned long int Nzdata = Nzdata_alloc; // available space, replaced with actual output size on compression

    if (VERBOSE_DUMP && (i == 0)) {
      int k;
      fprintf (stderr, "cmp swp: "); 
      for (k = 0; k < 64; k++) { fprintf (stderr, "%02hhx", raw[k]); if (k % 4 == 3) fprintf (stderr, " "); } 
      fprintf (stderr, "\n");
      fprintf (stderr, "Nzdata: %lu -> ", Nzdata);
    }

    // optname, optvalue = NULL, Noptions = 0 : defaults for RICE_1, HCOMPRESS_1

    if (!gfits_compress_data (zdata, &Nzdata, zcmptype, NULL, NULL, 0, raw, Nraw, raw_pixsize, ztile[1], ztile[0])) ESCAPE;
    if (VERBOSE_DUMP && (i == 0)) {
      int k;
      fprintf (stderr, "%lu\n", Nzdata);
      fprintf (stderr, "cmp SWP: "); 
      for (k = 0; k < 64; k++) { fprintf (stderr, "%02hhx", zdata[k]); if (k % 4 == 3) fprintf (stderr, " "); } 
      fprintf (stderr, "\n");
    }

    // BYTESWAP the compressed data after compressing.  Compression algorithms which do
    // not need to byteswap here return 1 for cmp_pixsize.  GZIP_1 is swapped before
    // compression; GZIP_2 implies swapping.  All other compression modes require swapping
    // after compression (the compresssion & decompression functions operate on native
    // ENDIAN data values).
    if (!gfits_byteswap_zdata (zdata, Nzdata, cmp_pixsize)) ESCAPE;

    if (VERBOSE_DUMP && (i == 0)) {
      int k;
      fprintf (stderr, "cmp dat: "); 
      for (k = 0; k < 64; k++) { fprintf (stderr, "%02hhx", zdata[k]); if (k % 4 == 3) fprintf (stderr, " "); } 
      fprintf (stderr, "\n");
    }
    if (!gfits_varlength_column_add_data (ftable, zdata, Nzdata, i, &zdef)) ESCAPE;

    // update the otile[] counters, carrying to the next dimension if needed
    gfits_imtile_next (matrix, ztile, ntile, otile);
  }

  if (!gfits_varlength_column_finish (ftable, &zdef)) ESCAPE;
  if (VERBOSE_DUMP) { 
    int k;
    fprintf (stderr, "cmp tbl: "); 
    for (k = 0; k < 64; k++) { fprintf (stderr, "%02hhx", ftable->buffer[k]);  if (k % 4 == 3) fprintf (stderr, " "); }
    fprintf (stderr, "\n");
  }

  free (raw);
  free (zdata);

  free (otile);
  free (ntile);

  if (!Ztile) free (ztile);
  return (TRUE);

escape:
  FREE (raw);
  FREE (zdata);

  FREE (otile);
  FREE (ntile);

  if (!Ztile) FREE (ztile);
  return FALSE;
}

// copy everything except the fields which get translated:
// SIMPLE, NAXIS, NAXIS0 - 9, BSCALE, BZERO, BLANK, 
int gfits_copy_keywords_compress (Header *srchead, Header *tgthead) {
  OHANA_UNUSED_PARAM(tgthead);
  OHANA_UNUSED_PARAM(srchead);

# if (0)
  int i;
  char endline[82];

  // find the END of the header
  char *p = gfits_header_field (tgthead, "END", 1);
  if (p == NULL) return (FALSE); 

  // save the END line
  memcpy (endline, p, FT_LINE_LENGTH);

  // p tracks the current last (non-END) header line
  for (i = 0; i < srchead->datasize; i += FT_LINE_LENGTH) {

    NEED TO FIX THIS ---

    // is there enough space for 2 more lines (keyword + END)?
    if (tgthead->datasize - (p - (tgthead->buffer)) < 2*FT_LINE_LENGTH) {
      tgthead->datasize += FT_RECORD_SIZE;
      REALLOCATE (tgthead->buffer, char, tgthead->datasize);
      p = gfits_header_field (tgthead, "END", 1);
      if (p == NULL) return (FALSE); 
    }

    // copy the line to the target header
    memcpy (p, &srchead->buffer[i], FT_LINE_LENGTH);
    
    // advance p:
    p += FT_LINE_LENGTH;
  }

  // is there enough space for 1 more line?
  if (tgthead->datasize - (p - (tgthead->buffer)) < FT_LINE_LENGTH) {
    tgthead->datasize += FT_RECORD_SIZE;
    REALLOCATE (tgthead->buffer, char, tgthead->datasize);
    p = gfits_header_field (tgthead, "END", 1);
    if (p == NULL) return (FALSE); 
  }
# endif  
  return TRUE;
}

# if (0)
int gfits_add_keyword_raw (Header *header) {

    /* new entry, find the END of the header */
    p = gfits_header_field (header, "END", 1);
    if (p == NULL) return (FALSE); 
    
    /* is there enough space for 1 more line? */
    if (header[0].datasize - (p - (header[0].buffer)) < 2*FT_LINE_LENGTH) {
      header[0].datasize += FT_RECORD_SIZE;
      REALLOCATE (header[0].buffer, char, header[0].datasize);
      p = gfits_header_field (header, "END", 1);
      if (p == NULL) return (FALSE); 
      memset (p + FT_LINE_LENGTH, ' ', FT_RECORD_SIZE);
    }
    
    /* push END line back 1 */
    memmove ((p + FT_LINE_LENGTH), p, FT_LINE_LENGTH);
    memset (p, ' ', FT_LINE_LENGTH);
# endif


// raw_pixsize is the bytes / pixel for the input matrix
// place the raw image bytes for the current tile into the tile buffer
// * otile is the counter for the current output tile
// * ztile gives the size of the i-th dimension of the current tile
// * raw_bitpix defines the size and type of the raw buffer
int gfits_collect_data (Matrix *matrix, char *raw, unsigned long int Nraw, int raw_bitpix, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero) {

  int i, k, start, offset, coord, Nline;
  unsigned int *counter = NULL;
  unsigned long int *Ztile = NULL;

  ALLOCATE (counter, unsigned int, matrix->Naxes); // counter for current row in tile to copy
  ALLOCATE (Ztile, unsigned long int, matrix->Naxes); // true sizes of this tile (in pixels)
  
  for (i = 0; i < matrix->Naxes; i++) {
    counter[i] = 0;
    Ztile[i] = MIN ((matrix->Naxis[i] - otile[i]*ztile[i]), ztile[i]);
  }

  // number of lines in the tile (in pixels)
  Nline = 1;
  for (i = 1; i < matrix->Naxes; i++) {
    Nline *= Ztile[i];
  }

  // confirm the tile size:
  assert (Nraw == Ztile[0]*Nline);

  // set the starting point of the tile in the matrix buffer
  // start = otile[0]*ztile[0] + otile[1]*ztile[1]*Naxis[0] + otile[2]*ztile[2]*Naxis[0]*Naxis[1] + ...;
  // start = otile[0]*ztile[0] + Naxis[0]*(otile[1]*ztile[1] + Naxis[1]*(otile[2]*ztile[2] + ...));
  start = otile[matrix->Naxes-1]*ztile[matrix->Naxes-1];
  for (i = matrix->Naxes - 2; i >= 0; i--) {
    coord = otile[i]*ztile[i];
    start = start*matrix->Naxis[i] + coord;
  }
  
  // pixel offset in output array relative to tile start
  offset = 0;

  int directCopy = (zzero == 0.0) && (zscale == 1.0);

  // we need to set up switches for all of the possible combinations:
  // this macro is used at the inner switch to run the actual loop
# define SCALE_AND_DIST_INT_PRINT(TYPE, SIZE, INTYPE) {			\
    unsigned long int j;						\
    TYPE *TILEptr = (TYPE *) &matrix->buffer[SIZE*(offset + start)];	\
    for (j = 0; j < Ztile[0]; j++, TILEptr++, RAWptr++) {		\
      if (*TILEptr == oblank) {						\
	*RAWptr = zblank;						\
      } else {								\
	/* *RAWptr = directCopy ? *TILEptr : (*TILEptr - zzero) / zscale; */ \
	*RAWptr = *TILEptr;	\
	fprintf (stderr, "col: %s to %s : %08x : %08x\n", #TYPE, #INTYPE, *TILEptr, *RAWptr); \
      } } }

  // we need to set up switches for all of the possible combinations:
  // this macro is used at the inner switch to run the actual loop
# define SCALE_AND_DIST_INT(OTYPE, OSIZE) {				\
    unsigned long int j;						\
    OTYPE *TILEptr = (OTYPE *) &matrix->buffer[OSIZE*(offset + start)];	\
    for (j = 0; j < Ztile[0]; j++, TILEptr++, RAWptr++) {		\
      if (*TILEptr == oblank) {						\
	*RAWptr = zblank;						\
      } else {								\
	if (directCopy) {						\
	  *RAWptr = *TILEptr;						\
	} else {							\
	  *RAWptr = (*TILEptr - zzero) / zscale; 			\
	} } } }

  // we need to set up switches for all of the possible combinations:
  // this macro is used at the inner switch to run the actual loop
# define SCALE_AND_DIST_FLOAT(OTYPE, OSIZE) {				\
    unsigned long int j;						\
    OTYPE *TILEptr = (OTYPE *) &matrix->buffer[OSIZE*(offset + start)];	\
    for (j = 0; j < Ztile[0]; j++, TILEptr++, RAWptr++) {		\
      if (!isfinite(*TILEptr)) {					\
	*RAWptr = zblank;						\
      } else {								\
	if (directCopy) {						\
	  *RAWptr = *TILEptr;						\
	} else {							\
	  *RAWptr = (*TILEptr - zzero) / zscale;			\
	} } } }

  // this macro sets up the outer switch and calls above the macro with all RAW buffer bitpix options
  // 
# define SETUP_RAWSIZE_PRINT(TYPE, SIZE) {			\
    TYPE *RAWptr = (TYPE *) &raw[i*SIZE*Ztile[0]];	\
    switch (matrix->bitpix) {				\
      case   8: SCALE_AND_DIST_INT   (char,   1); break;	\
      case  16: SCALE_AND_DIST_INT   (short,  2); break;	\
      case  32: SCALE_AND_DIST_INT_PRINT   (int,    4, TYPE); break;	\
      case -32: SCALE_AND_DIST_FLOAT (float,  4); break;	\
      case -64: SCALE_AND_DIST_FLOAT (double, 8); break;						\
      default: abort();					\
    } }

  // this macro sets up the outer switch and calls above the macro with all RAW buffer bitpix options
  // 
# define SETUP_RAWSIZE(ITYPE, ISIZE) {			\
    ITYPE *RAWptr = (ITYPE *) &raw[i*ISIZE*Ztile[0]];	\
    switch (matrix->bitpix) {				\
      case   8: SCALE_AND_DIST_INT   (char,   1); break;	\
      case  16: SCALE_AND_DIST_INT   (short,  2); break;	\
      case  32: SCALE_AND_DIST_INT   (int,    4); break;	\
      case -32: SCALE_AND_DIST_FLOAT (float,  4); break;	\
      case -64: SCALE_AND_DIST_FLOAT (double, 8); break;						\
      default: abort();					\
    } }

  // loop over lines in the tile
  for (i = 0; i < Nline; i++) {
    switch (raw_bitpix) {
      case   8: SETUP_RAWSIZE (char,   1); break;
      case  16: SETUP_RAWSIZE (short,  2); break;
      case  32: SETUP_RAWSIZE (int,    4); break;
      case -32: SETUP_RAWSIZE (float,  4); break;
      case -64: SETUP_RAWSIZE (double, 8); break;
      default: abort();
    }

    // update the counters, carrying to the next dimension if needed
    for (k = 1; k < matrix->Naxes; k++) {
      counter[k] ++;
      if (counter[k] == Ztile[k]) {
	counter[k] = 0;
      } else {
	break;
      }
    }
    if (k == matrix->Naxes) assert (i == Nline - 1); // we should be done here...

    // Naxes = 3
    // offset = counter[1]*matrix->Naxis[0] + counter[2]*matrix->Naxis[0]*matrix->Naxis[1] + 
    // offset = matrix->Naxis[0]*(counter[1] + matrix->Naxis[1]*(counter[2] + matrix->Naxis[2]*...))

    // determine the offset of the next line relative to the start position
    offset = counter[matrix->Naxes - 1];
    for (k = matrix->Naxes - 2; k >= 0; k--) {
      offset = offset*matrix->Naxis[k] + counter[k];
    }      
  }

  free (counter);
  free (Ztile);

  return (TRUE);
}

// GZIP_2 organizes data in bytes by significance: (A1 A2 A3 A4), (B1 B2 B3 B4) -> (A1 B1), (A2, B2), (A3, B3), (A4, B4)
// NOTE that this organization depends on the machine endian state. 
// raw_pixsize is the bytes / pixel for the input matrix
// place the raw image bytes for the current tile into the tile buffer
// * otile is the counter for the current output tile
// * ztile gives the size of the i-th dimension of the current tile
// * raw_bitpix defines the size and type of the raw buffer
int gfits_collect_gzp2 (Matrix *matrix, char *raw, unsigned long int Nraw, int raw_bitpix, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero) {
  OHANA_UNUSED_PARAM(oblank);
  OHANA_UNUSED_PARAM(zblank);
  OHANA_UNUSED_PARAM(zzero);
  OHANA_UNUSED_PARAM(zscale);

  unsigned long int i, j;
  unsigned int *counter = NULL;
  unsigned long int *Ztile = NULL;

  ALLOCATE (counter, unsigned int, matrix->Naxes); // counter for current row in tile to copy
  ALLOCATE (Ztile, unsigned long int, matrix->Naxes); // true sizes of this tile (in pixels)
  
  int k;
  for (k = 0; k < matrix->Naxes; k++) {
    counter[k] = 0;
    Ztile[k] = MIN ((matrix->Naxis[k] - otile[k]*ztile[k]), ztile[k]);
  }

  // number of lines in the tile (in pixels)
  unsigned long int Nline = 1;
  unsigned long int Npix = matrix->Naxis[0];
  for (k = 1; k < matrix->Naxes; k++) {
    Nline *= Ztile[k];
    Npix  *= matrix->Naxis[k];
  }

  // confirm the tile size:
  assert (Nraw == Ztile[0]*Nline);

  // set the starting point of the tile in the matrix buffer
  // start = otile[0]*ztile[0] + otile[1]*ztile[1]*Naxis[0] + otile[2]*ztile[2]*Naxis[0]*Naxis[1] + ...;
  // start = otile[0]*ztile[0] + Naxis[0]*(otile[1]*ztile[1] + Naxis[1]*(otile[2]*ztile[2] + ...));
  int start = otile[matrix->Naxes-1]*ztile[matrix->Naxes-1];
  for (k = matrix->Naxes - 2; k >= 0; k--) {
    unsigned long int coord = otile[k]*ztile[k];
    start = start*matrix->Naxis[k] + coord;
  }
  
  int size = 0;
  switch (raw_bitpix) {
    case   8: size = 1; break;
    case  16: size = 2; break;
    case  32: size = 4; break;
    case -32: size = 4; break;
    case -64: size = 8; break;
    default: abort();
  }

  // pixel offset in output array relative to tile start
  int offset = 0;
  
  static int pass = 0;
  for (k = 0; k < size; k++) {
    for (i = 0; i < Nline; i++) {
# ifdef BYTE_SWAP      
      char *srcptr = &matrix->buffer[size*(offset + start) + (size - k - 1)];
# else
      char *srcptr = &matrix->buffer[size*(offset + start) + k];
# endif
      char *rawptr = &raw[i*Ztile[0] + k*Nraw];	
      for (j = 0; j < Ztile[0]; j++, srcptr += size, rawptr ++) {		
	if (FALSE && (i == 0) && (j < 4) && (pass == 0)) {
	  fprintf (stderr, "0x%02hhx -> 0x%02hhx\n", (int)(srcptr - matrix->buffer), (int)(rawptr - raw));
	}
	*rawptr = *srcptr; 
	// myAssert (srcptr - matrix->buffer < Npix*size, "memory overrun");
	// myAssert (rawptr - raw < Nraw*size, "memory overrun");
      }

      // update the counters, carrying to the next dimension if needed
      int axis;
      for (axis = 1; axis < matrix->Naxes; axis++) {
	counter[axis] ++;
	if (counter[axis] == Ztile[axis]) {
	  counter[axis] = 0;
	} else {
	  break;
	}
      }
      if (axis == matrix->Naxes) assert (i == Nline - 1); // we should be done here...

      // Naxes = 3
      // offset = counter[1]*matrix->Naxis[0] + counter[2]*matrix->Naxis[0]*matrix->Naxis[1] + 
      // offset = matrix->Naxis[0]*(counter[1] + matrix->Naxis[1]*(counter[2] + matrix->Naxis[2]*...))

      // determine the offset of the next line relative to the start position
      offset = counter[matrix->Naxes - 1];
      for (axis = matrix->Naxes - 2; axis >= 0; axis--) {
	offset = offset*matrix->Naxis[axis] + counter[axis];
      }      
    }
  }
  pass ++;

  free (counter);
  free (Ztile);

  return (TRUE);
}
