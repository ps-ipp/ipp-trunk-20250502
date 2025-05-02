# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>
# define VERBOSE_DUMP 0

int gfits_distribute_gzp2 (Matrix *matrix, char *raw, unsigned long int Nraw, int raw_bitpix, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero);

# define ESCAPE {							\
    fprintf (stderr, "error in %s @ line %d\n", __func__, __LINE__);	\
    if (ztile != NULL) free (ztile);					\
    if (optname != NULL) {						\
      int opt;								\
      for (opt = 0; opt < Noptions; opt++) {				\
	free (optname[opt]);						\
	free (optvalue[opt]);						\
      }									\
      free (optname);							\
      free (optvalue);							\
    }									\
    if (out != NULL) free (out);					\
    if (otile != NULL) free (otile);					\
    if (ntile != NULL) free (ntile);					\
    return (FALSE); }

# define MOD_KEYWORD(ZNAME,NAME,TYPE,IN,OUT) {	   \
    if (gfits_scan (header, ZNAME, TYPE, 1, IN)) { \
      gfits_modify (header, NAME, TYPE, 1, OUT);   \
    }						   \
    gfits_delete (header, ZNAME, 1); }

# define MOD_KEYWORD_ALT(ZNAME,NAME,TYPE,IN,OUT) {     \
    if (gfits_scan_alt (header, ZNAME, TYPE, 1, IN)) { \
      gfits_modify_alt (header, NAME, TYPE, 1, OUT);   \
    }						       \
    gfits_delete (header, ZNAME, 1); }

# define MOD_KEYWORD_REQUIRED(ZNAME,NAME,TYPE,IN,OUT) {		\
    if (!gfits_scan (header, ZNAME, TYPE, 1, IN)) ESCAPE;	\
    gfits_delete (header, ZNAME, 1);				\
    gfits_modify (header, NAME, TYPE, 1, OUT); }

# define DUMP_ROWBYTES(BUF, INFO)	       \
  if (VERBOSE_DUMP && (row == 0)) {	       \
    int k;				       \
    fprintf (stderr, "%s: ", INFO);	       \
    for (k = 0; k < 64; k++) {		       \
      fprintf (stderr, "%02hhx", BUF[k]);    \
      if (k % 4 == 3) fprintf (stderr, " ");   \
    } fprintf (stderr, "\n"); }

int gfits_uncompress_image (Header *header, Matrix *matrix, FTable *ftable) {

  int status, zimage;
  off_t Nzdata, Nzrows, zcol;
  unsigned long int max_tile_size;
  char cmptype[80];
  char zaxis[10], naxis[10], key[10], word[81], exttype[81], checksum[81], datasum[81];
  int Noptions, NOPTIONS, zblank, oblank;
  VarLengthColumn zdef;
  float zscale, zzero;

  unsigned long int *ztile = NULL;
  int *otile = NULL;
  int *ntile = NULL;
  char **optname = NULL;
  char **optvalue = NULL;
  char *out = NULL;
  char *zdata = NULL;

  Noptions = 0;

  // is ZIMAGE present?
  // NOTE target of %t must be int length
  status = gfits_scan_alt (ftable->header, "ZIMAGE", "%t", 1, &zimage);
  if (!status || !zimage) ESCAPE;

  // copy original header to output header
  gfits_copy_header (ftable->header, header);

  // delete ZIMAGE from output header
  gfits_delete (header, "ZIMAGE", 1);

  // extract compression-specific keywords, update header as needed.
  if (!gfits_scan (header, "ZCMPTYPE", "%s", 1, cmptype)) ESCAPE;
  gfits_delete (header, "ZCMPTYPE", 1);

  MOD_KEYWORD_REQUIRED ("ZBITPIX", "BITPIX", "%d", &header->bitpix, header->bitpix);
  MOD_KEYWORD_REQUIRED ("ZNAXIS",  "NAXIS",  "%d", &header->Naxes,  header->Naxes);

  int axis;
  for (axis = 0; axis < header->Naxes; axis++) {
    snprintf_nowarn (zaxis, 10, "ZNAXIS%d", axis + 1);
    snprintf_nowarn (naxis, 10, "NAXIS%d",  axis + 1);
    MOD_KEYWORD_REQUIRED (zaxis,  naxis,  OFF_T_FMT,  &header->Naxis[axis],  header->Naxis[axis]);
  }    

  // set up the tile sizes : ztile is the default size of the tile in the Nth dimension
  // the actual tile size may be smaller at the edge of a dimension.  if the ZTILEn
  // entries are not found, default to [Nx,1,1,...]
  ALLOCATE (ztile, unsigned long int, header->Naxes);
  if (!gfits_scan (header, "ZTILE1", "%lu", 1, &ztile[0])) {
    ztile[0] = header->Naxis[0];
    for (axis = 1; axis < header->Naxes; axis++) {
      ztile[axis] = 1;
    }
  } else {
    gfits_delete (header, "ZTILE1", 1);
    // if ZTILE1 exists, all ZTILEn must exist:
    for (axis = 1; axis < header->Naxes; axis++) {
      snprintf_nowarn (key, 10, "ZTILE%d", axis + 1); 
      if (!gfits_scan (header, key, "%lu", 1, &ztile[axis])) ESCAPE;
      gfits_delete (header, key, 1);
    }
  }

  // search for algorithm-specific keywords. these are used to control compression options
  // note the difference between the keywords (1 indexed) and the variables (0 indexed)
  NOPTIONS = 10;
  ALLOCATE (optname, char *, NOPTIONS);
  ALLOCATE (optvalue, char *, NOPTIONS);
  for (Noptions = 0; TRUE; Noptions++) {
    snprintf_nowarn (key, 10, "ZNAME%d", Noptions + 1); 
    if (!gfits_scan (header, key, "%s", 1, word)) break;
    gfits_delete (header, key, 1);
    optname[Noptions] = strcreate (word);

    snprintf_nowarn (key, 10, "ZVAL%d", Noptions + 1); 
    if (!gfits_scan (header, key, "%s", 1, word)) ESCAPE;
    gfits_delete (header, key, 1);
    optvalue[Noptions] = strcreate (word);

    if (Noptions == NOPTIONS - 1) {
      NOPTIONS += 10;
      REALLOCATE (optname, char *, NOPTIONS);
      REALLOCATE (optvalue, char *, NOPTIONS);
    }
  }

  // check for ZMASKCMP (not yet supported 2021.01.23)
  char zmaskcmp[81];
  int have_zmaskcmp = gfits_scan (header, "ZMASKCMP", "%s", 1, zmaskcmp);
  if (have_zmaskcmp) {
    fprintf (stderr, "null pixels were compressed with %s, not implemented in Ohana\n", zmaskcmp);
    ESCAPE;
  }

  // NOTE target of %t must be int length
  int zsimple;
  int have_zsimple  = gfits_scan_alt (header, "ZSIMPLE", "%t", 1, &zsimple);
  int have_ztension = gfits_scan (header, "ZTENSION", "%s", 1, exttype);

  // this is a bogus case: we cannot have both keywords
  if (have_zsimple && have_ztension) ESCAPE;

  // if neither are present, we have an image that is not really following the standard:
  // assume it is a PHU
  if (!have_zsimple && !have_ztension) {
    header->simple = TRUE;
    gfits_extended_to_primary (header, header->simple, "Image data");

    MOD_KEYWORD_ALT ("ZEXTEND",  "EXTEND",   "%t", &header->extend, header->extend);
    MOD_KEYWORD_ALT ("ZBLOCKED", "BLOCKED",  "%t", &header->extend, header->extend);
  }

  // have_zsimple : image comes from a PHU
  if (have_zsimple) {
    header->simple = zsimple;
    gfits_delete (header, "ZSIMPLE", 1);
    gfits_extended_to_primary (header, header->simple, "Image data");

    MOD_KEYWORD_ALT ("ZEXTEND",  "EXTEND",   "%t", &header->extend, header->extend);
    MOD_KEYWORD_ALT ("ZBLOCKED", "BLOCKED",  "%t", &header->extend, header->extend);
  } 

  // have_ztension : image comes from an extension
  if (have_ztension) {
    gfits_delete (header, "ZTENSION", 1);
    gfits_modify_extended (header, exttype, "Image extension");

    // we may have an uncompressed PCOUNT / GCOUNT value, otherwise set to 0,1
    if (gfits_scan (header, "ZPCOUNT", OFF_T_FMT, 1, &header->pcount)) {
	gfits_delete (header, "ZPCOUNT", 1);
	gfits_modify (header, "PCOUNT", OFF_T_FMT, 1, header->pcount);
    } else {
	header->pcount = 0;
	gfits_modify (header, "PCOUNT", OFF_T_FMT, 1, header->pcount);
    }
    if (gfits_scan (header, "ZGCOUNT", "%d", 1, &header->gcount)) {
	gfits_delete (header, "ZGCOUNT", 1);
	gfits_modify (header, "GCOUNT", "%d", 1, header->gcount);
    } else {
	header->pcount = 1;
	gfits_modify (header, "GCOUNT", "%d", 1, header->gcount);
    }
  } else {
    // if this is a PHU (whether or not ZSIMPLE is present), PCOUNT & GCOUNT must be basic (0,1)
    header->pcount = 0;
    header->gcount = 1;
    gfits_modify (header, "PCOUNT", OFF_T_FMT, 1, header->pcount);
    gfits_modify (header, "GCOUNT", "%d", 1, header->gcount);
  }

  MOD_KEYWORD ("ZHECKSUM", "CHECKSUM", "%s", checksum,        checksum);
  MOD_KEYWORD ("ZDATASUM", "DATASUM",  "%s", datasum,         datasum);

  // get other basic keywords (default values supplied)
  zscale = 1;      gfits_scan (header, "ZSCALE", "%f", 1, &zscale);
  zblank = 32767;  gfits_scan (header, "ZBLANK", "%d", 1, &zblank);
  oblank = 32767;  gfits_scan (header, "BLANK", "%d", 1, &oblank);
  zzero = 0;       gfits_scan (header, "ZZERO", "%f", 1, &zzero);

  // find the COMPRESSED_DATA column (format should be 1PB, 1PI, 1PJ)
  // XXX is it required that this be the only column? -- not if ZMASKCMP exists (not yet supported)
  int colnum;
  for (colnum = 1; TRUE; colnum++) {
    snprintf_nowarn (key, 10, "TTYPE%d", colnum); 
    if (!gfits_scan (ftable->header, key, "%s", 1, word)) ESCAPE;
    if (!strcmp (word, "COMPRESSED_DATA")) break;
  }
  zcol = colnum;

  if (!gfits_varlength_column_define (ftable, &zdef, zcol)) ESCAPE;
  gfits_delete (header, "TFIELDS", 1);
  snprintf_nowarn (key, 10, "TTYPE"OFF_T_FMT,  zcol); 
  gfits_delete (header, key, 1);
  snprintf_nowarn (key, 10, "TFORM"OFF_T_FMT,  zcol); 
  gfits_delete (header, key, 1);

  // create the output image
  gfits_create_matrix (header, matrix);

  // counters for tile numbers
  ALLOCATE (otile, int, matrix->Naxes);
  ALLOCATE (ntile, int, matrix->Naxes);
  max_tile_size = 1;
  for (axis = 0; axis < matrix->Naxes; axis++) {
    otile[axis] = 0;
    ntile[axis] = (matrix->Naxis[axis] % ztile[axis]) ? (matrix->Naxis[axis] / ztile[axis] + 1) : (matrix->Naxis[axis] / ztile[axis]);
    max_tile_size *= ztile[axis];

    // ztile[axis] is the default (or max) tile size in the i-th dimension
    // ntile[axis] is the number of tiles in the i-th dimension
    // otile[axis] is the current output tile counter in the i-th dimension
  }

  // this takes place in three major steps:
  // 1) read the table data : zdef.format tells the size of the varlength heap element
  // 2) uncompress the data : pixel size depends on compression method
  // 3) distribute data to the tiles : output pixel size depends on header.bitpix

  // heapdata -> zdata -> odata -> idata

  // heapdata.pixsize : zdef.format
  // zdata.pixsize : depends on compression method (cmp_pixsize below)
  // odata.pixsize : depends on compression method (raw_pixsize below)
  // idata.pixsize : depends on header.bitpix

  // size of an element in the vartable heap section
  // zdata_pixsize = gfits_vartable_heap_pixsize (zdef.format); XXX

  // depending on the compression method, the data may need to be byteswapped either before
  // or after uncompression:

  // size of a pixel in the compressed data (cmp_pixsize), used for byteswap below
  int cmp_pixsize = gfits_compressed_data_pixsize (cmptype, header[0].bitpix, optname, optvalue, Noptions);

  // size of a pixel in the raw uncomprssed buffer (raw_pixsize) after uncompression (last option says RICE ignores bitpix)
  int raw_pixsize = gfits_uncompressed_data_pixsize (cmptype, header[0].bitpix, optname, optvalue, Noptions, FALSE);

  // we also need the bitpix value for the raw uncompressed buffer to distinguish int and floats
  int raw_bitpix  = gfits_uncompressed_data_bitpix (cmptype, header[0].bitpix, optname, optvalue, Noptions, FALSE);

  // size of a pixel in the final image (not needed)
  // int tile_pixsize = abs(header[0].bitpix) / 8;
  // fprintf (stderr, "raw_pixsize: %d, cmp_pixsize: %d, tile_pixsize: %d, raw_bitpix: %d\n", raw_pixsize, cmp_pixsize, tile_pixsize, raw_bitpix);

  unsigned long int Nout_alloc = raw_pixsize*max_tile_size;
  ALLOCATE (out, char, Nout_alloc);

  // uncompress the data
  Nzrows = ftable->header->Naxis[1];
  for (off_t row = 0; row < Nzrows; row++) {

    DUMP_ROWBYTES (ftable->buffer, "tbl dat");

    // expected output size for this tile
    unsigned long Nout = raw_pixsize*gfits_tile_size (matrix, otile, ztile);

    zdata = gfits_varlength_column_pointer (ftable, &zdef, row, &Nzdata);
    if (!zdata) ESCAPE;

    DUMP_ROWBYTES (zdata, "cmp dat");

    // byteswap the compressed data before uncompressing.
    // compression algorithms which do not need to byteswap here return 1 for cmp_pixsize
    if (!gfits_byteswap_zdata (zdata, Nzdata, cmp_pixsize)) ESCAPE;

    DUMP_ROWBYTES (zdata, "cmp swp");
    
    // gfits_uncompress_data uncompresses from zdata to the temporary output buffer which must be allocated
    // Note: the tile must not be > 2GB
    // Nout going in is the expected number of bytes 
    if (!gfits_uncompress_data ((char *)zdata, Nzdata, cmptype, optname, optvalue, Noptions, out, &Nout, Nout_alloc, raw_pixsize)) ESCAPE;

    DUMP_ROWBYTES (out, "unc swp");
    
    if (!strcasecmp(cmptype, "GZIP_1")) {
      // Nout is number of pixels
      if (!gfits_byteswap_zdata (out, (off_t) (Nout * raw_pixsize), raw_pixsize)) ESCAPE;
    }
    
    DUMP_ROWBYTES (out, "unc raw");

    // copy the uncompressed pixels into their correct locations 	    
    if (!strcasecmp(cmptype, "GZIP_2") || !strcasecmp(cmptype, "NONE_2")) {
      if (!gfits_distribute_gzp2 (matrix, out, Nout, raw_bitpix, otile, oblank, ztile, zblank, zscale, zzero)) ESCAPE;
    } else {
      if (!gfits_distribute_data (matrix, out, Nout, raw_bitpix, otile, oblank, ztile, zblank, zscale, zzero)) ESCAPE;
    }

    DUMP_ROWBYTES (matrix->buffer, "out raw");

    // update the tile counters, carrying to the next dimension if needed
    for (axis = 0; axis < matrix->Naxes; axis++) {
      otile[axis] ++;
      if (otile[axis] == ntile[axis]) {
	otile[axis] = 0;
      } else {
	break;
      }
    }
  }

  FREE (ztile);
  if (optname != NULL) {
    int opt;
    for (opt = 0; opt < Noptions; opt++) {
      FREE (optname[opt]);
      FREE (optvalue[opt]);
    }
    FREE (optname);
    FREE (optvalue);
  }
  FREE (out);
  FREE (otile); 
  FREE (ntile); 
  return (TRUE);
}

// raw_bitpix is the data size/type for the temporary buffer of uncompressed data
// matrix->bitpix is the data size/type of the output image data
int gfits_distribute_data (Matrix *matrix, char *raw, unsigned long int Nraw, int raw_bitpix, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero) {

  int axis;
  unsigned int *counter = NULL;
  unsigned long int *Ztile = NULL;

  ALLOCATE (counter, unsigned int, matrix->Naxes);
  ALLOCATE (Ztile, unsigned long int, matrix->Naxes);

  // counter for current row in tile to copy
  // true sizes of this tile (in pixels)
  for (axis = 0; axis < matrix->Naxes; axis++) {
    counter[axis] = 0;
    Ztile[axis] = MIN ((matrix->Naxis[axis] - otile[axis]*ztile[axis]), ztile[axis]);
  }

  // number of lines in the tile (in pixels)
  unsigned long int Nline = 1;
  for (axis = 1; axis < matrix->Naxes; axis++) {
    Nline *= Ztile[axis];
  }

  // double check reported size (needs the Isize)
  assert (Nraw == Ztile[0]*Nline);

  // set the starting point of the tile:
  // start = otile[0]*ztile[0] + otile[1]*ztile[1]*Naxis[0] + otile[2]*ztile[2]*Naxis[0]*Naxis[1] + ...;
  // start = otile[0]*ztile[0] + Naxis[0]*(otile[1]*ztile[1] + Naxis[1]*(otile[2]*ztile[2] + ...));
  unsigned long int start = otile[matrix->Naxes-1]*ztile[matrix->Naxes-1];
  for (axis = matrix->Naxes - 2; axis >= 0; axis--) {
    unsigned long int coord = otile[axis]*ztile[axis];
    start = start*matrix->Naxis[axis] + coord;
  }
  
  // pixel offset in output array relative to tile start
  unsigned long int offset = 0;

  int directCopy = (zzero == 0.0) && (zscale == 1.0);

# define SCALE_AND_DIST_INT_PRINT(TYPE, SIZE) {				\
    unsigned long j;							\
    TYPE *TILEptr = (TYPE *) &matrix->buffer[SIZE*(offset + start)];	\
    for (j = 0; j < Ztile[0]; j++, TILEptr++, RAWptr++) {		\
      if (*TILEptr == zblank) {						\
	*TILEptr = oblank;						\
      } else {								\
	*TILEptr = directCopy ? *RAWptr : *RAWptr * zscale + zzero;	\
	fprintf (stderr, "dis: %08x : %08x\n", *TILEptr, *RAWptr); \
      } } }

  // we need to set up switches for all of the possible combinations:
  // this macro is used at the inner switch to run the actual loop
# define SCALE_AND_DIST_INT(OTYPE, OSIZE) {				\
    unsigned long j;							\
    OTYPE *TILEptr = (OTYPE *) &matrix->buffer[OSIZE*(offset + start)];	\
    for (j = 0; j < Ztile[0]; j++, TILEptr++, RAWptr++) {		\
      if (*TILEptr == zblank) {						\
	*TILEptr = oblank;						\
      } else {								\
	if (directCopy) {						\
	  *TILEptr = *RAWptr;						\
	} else {							\
	  *TILEptr = *RAWptr * zscale + zzero;				\
	} } } }

  // we need to set up switches for all of the possible combinations:
  // this macro is used at the inner switch to run the actual loop
# define SCALE_AND_DIST_FLOAT(OTYPE, OSIZE) {				\
    unsigned long j;							\
    OTYPE *TILEptr = (OTYPE *) &matrix->buffer[OSIZE*(offset + start)];	\
    for (j = 0; j < Ztile[0]; j++, TILEptr++, RAWptr++) {		\
      if (!isfinite(*TILEptr)) {					\
	*TILEptr = oblank;						\
      } else {								\
	if (directCopy) {						\
	  *TILEptr = *RAWptr;						\
	} else {							\
	  *TILEptr = *RAWptr * zscale + zzero;				\
	} } } }
  
  // this macro sets up the outer switch and calls above macro with all output bitpix options
# define SETUP_RAWSIZE(ITYPE, ISIZE) {		   \
    ITYPE *RAWptr = (ITYPE *) &raw[i*ISIZE*Ztile[0]]; \
    switch (matrix->bitpix) {			   \
      case   8: SCALE_AND_DIST_INT   (char,   1); break; \
      case  16: SCALE_AND_DIST_INT   (short,  2); break; \
      case  32: SCALE_AND_DIST_INT   (int,    4); break; \
      case -32: SCALE_AND_DIST_FLOAT (float,  4); break; \
      case -64: SCALE_AND_DIST_FLOAT (double, 8); break; \
      default: 	abort(); \
    } }

  // this macro sets up the outer switch and calls above macro with all output bitpix options
# define SETUP_RAWSIZE_PRINT(TYPE, SIZE) {		   \
    TYPE *RAWptr = (TYPE *) &raw[i*SIZE*Ztile[0]]; \
    switch (matrix->bitpix) {			   \
      case   8: SCALE_AND_DIST_INT   (char,   1); break; \
      case  16: SCALE_AND_DIST_INT   (short,  2); break; \
      case  32: SCALE_AND_DIST_INT   (int,    4); break; \
      case -32: SCALE_AND_DIST_FLOAT (float,  4); break; \
      case -64: SCALE_AND_DIST_FLOAT (double, 8); break; \
      default: 	abort(); \
    } }

  // loop over lines in the tile
  unsigned long int i;
  for (i = 0; i < Nline; i++) {
    switch (raw_bitpix) {
      case   8: SETUP_RAWSIZE (char,   1); break;
      case  16: SETUP_RAWSIZE (short,  2); break; 
      case  32:	SETUP_RAWSIZE (int,    4); break;
      case -32:	SETUP_RAWSIZE (float,  4); break;
      case -64:	SETUP_RAWSIZE (double, 8); break;
      default:	abort();
    }

    // update the counters, carrying to the next dimension if needed
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

  free (counter);
  free (Ztile);
  return (TRUE);
}

// raw_bitpix is the data size/type for the temporary buffer of uncompressed data
// matrix->bitpix is the data size/type of the output image data
// The gzip2 algorithm organizes the compressed data in groups by significant byte sequence (most significant byte of all pixels first, etc).
// gfits_distribute_gzp2 needs to shuffle the bytes back into the correct location in the tile.
int gfits_distribute_gzp2 (Matrix *matrix, char *raw, unsigned long int Nraw, int raw_bitpix, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero) {
  // unused parameters are supplied to make this function have the same API as gfits_distribute_data()
  OHANA_UNUSED_PARAM(oblank);
  OHANA_UNUSED_PARAM(zblank);
  OHANA_UNUSED_PARAM(zzero);
  OHANA_UNUSED_PARAM(zscale);

  int axis;
  unsigned int *counter = NULL;
  unsigned long int *Ztile = NULL;

  ALLOCATE (counter, unsigned int, matrix->Naxes);
  ALLOCATE (Ztile, unsigned long int, matrix->Naxes);

  // counter for current row in tile to copy
  // true sizes of this tile (in pixels)
  for (axis = 0; axis < matrix->Naxes; axis++) {
    counter[axis] = 0;
    Ztile[axis] = MIN ((matrix->Naxis[axis] - otile[axis]*ztile[axis]), ztile[axis]);
  }

  // number of lines in the tile (in pixels)
  unsigned long int Nline = 1;
  unsigned long int Npix = matrix->Naxis[0];
  for (axis = 1; axis < matrix->Naxes; axis++) {
    Nline *= Ztile[axis];
    Npix  *= matrix->Naxis[axis];
  }

  // double check reported size (needs the Isize)
  assert (Nraw == Ztile[0]*Nline);

  // set the starting point of the tile:
  // start = otile[0]*ztile[0] + otile[1]*ztile[1]*Naxis[0] + otile[2]*ztile[2]*Naxis[0]*Naxis[1] + ...;
  // start = otile[0]*ztile[0] + Naxis[0]*(otile[1]*ztile[1] + Naxis[1]*(otile[2]*ztile[2] + ...));
  unsigned long int start = otile[matrix->Naxes-1]*ztile[matrix->Naxes-1];
  for (axis = matrix->Naxes - 2; axis >= 0; axis--) {
    unsigned long int coord = otile[axis]*ztile[axis];
    start = start*matrix->Naxis[axis] + coord;
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
  unsigned long int offset = 0;

  static int pass = 0;
  int k;
  for (k = 0; k < size; k++) {
    unsigned long int i;
    for (i = 0; i < Nline; i++) {
# ifdef BYTE_SWAP      
      char *srcptr = &matrix->buffer[size*(offset + start) + (size - k - 1)];
# else
      char *srcptr = &matrix->buffer[size*(offset + start) + k];
# endif
      char *rawptr = &raw[i*Ztile[0] + k*Nraw];	
      unsigned long j;
      for (j = 0; j < Ztile[0]; j++, srcptr += size, rawptr ++) {		
	if (FALSE && (i == 0) && (j < 4) && (pass == 0)) {
	  fprintf (stderr, "0x%02hhx -> 0x%02hhx\n", (int)(srcptr - matrix->buffer), (int)(rawptr - raw));
	}
	*srcptr = *rawptr; 
	// myAssert (srcptr - matrix->buffer < Npix*size, "memory overrun");
	// myAssert (rawptr - raw < Nraw*size, "memory overrun");
      }

      // update the counters, carrying to the next dimension if needed
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

