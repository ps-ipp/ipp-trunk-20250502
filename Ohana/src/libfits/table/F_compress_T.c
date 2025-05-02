# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>
# define VERBOSE_DUMP 0
# define OHANA_MEMCHECK 0
# define OHANA_MEMCHECK_SUPER_VERBOSE 0

// the user needs to specify the various compression options:
// ztilelen (may be 0)
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

int gfits_collect_table_data (FTable *table, TableField *field, char *raw, unsigned long int row_start, unsigned long int Nrows);
int gfits_collect_table_gzp2 (FTable *table, TableField *field, char *raw, unsigned long int row_start, unsigned long int Nrows);

static float timeSum1 = 0.0;
static float timeSum2 = 0.0;
static float timeSum2a = 0.0;
static float timeSum2b = 0.0;
static float timeSum2c = 0.0; // do the compression
static float timeSum2d = 0.0;
static float timeSum2e = 0.0; // insert data in table
static float timeSum3 = 0.0;

void gfits_compress_timing () {
  return;

  fprintf (stderr, "cmp times: %f %f %f\n", timeSum1, timeSum2, timeSum3);
  fprintf (stderr, "cmp times: %f %f %f %f %f\n", timeSum2a, timeSum2b, timeSum2c, timeSum2d, timeSum2e);

  timeSum1 = 0.0;
  timeSum2 = 0.0;
  timeSum2a = 0.0;
  timeSum2b = 0.0;
  timeSum2c = 0.0;
  timeSum2d = 0.0;
  timeSum2e = 0.0;
  timeSum3 = 0.0;
}

int gfits_compress_table (FTable *srctable, FTable *tgttable, unsigned long int ztilelen, char *zcmptype) {

  char keyword[81];

  char *raw = NULL;
  char *zdata = NULL;
  TableField *fields = NULL;

  Header *srcheader = srctable->header;
  Header *tgtheader = tgttable->header;

  // XXX EAM:
  struct timeval startTimer, stopTimer;
  float dtime;
  gettimeofday (&startTimer, (void *) NULL);

  unsigned long int Ntile, ztilelast;
  if (!ztilelen) ztilelen = srcheader->Naxis[1];

  // avoid tiles with size > 31bit (2GB)
  // maximum tile size = ztilelen * 8
  unsigned long maxTileSize = 0x7fffffff / 8;
  if (ztilelen >= maxTileSize) {
    long nScale = ztilelen / maxTileSize;
    if (ztilelen % nScale) nScale ++;
    ztilelen = ztilelen / nScale;
  }

  if (!srcheader->Naxis[1]) {
    Ntile = 0;
    ztilelast = 0;
  } else {
    if (srcheader->Naxis[1] % ztilelen) {
      Ntile = srcheader->Naxis[1] / ztilelen + 1;
      ztilelast = srcheader->Naxis[1] % ztilelen;
    } else {
      Ntile = srcheader->Naxis[1] / ztilelen;
      ztilelast = ztilelen;
    }
  }

  // EXTNAME is not mandatory, but I need to keep it
  char extname[80];
  if (!gfits_scan (srcheader, "EXTNAME", "%s", 1, extname)) {
    strcpy (extname, "COMPRESSED_TABLE");
  }

  // creates an empty (Naxes = 2, Naxis[i] = 0) table header with XTENSION = BINTABLE, EXTNAME = COMPRESSED_TABLE
  if (!gfits_create_table_header (tgtheader, "BINTABLE", extname)) ESCAPE;

  int Nfields;
  if (!gfits_scan (srcheader, "TFIELDS", "%d", 1, &Nfields)) ESCAPE;

  // XXX TIMER 1
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum1 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  ALLOCATE_ZERO (fields, TableField, Nfields);

  for (int field = 0; field < Nfields; field++) {
    snprintf (keyword, 80, "TTYPE%d", field+1);
    if (!gfits_scan (srcheader, keyword, "%s", 1, fields[field].ttype)) ESCAPE;
    if (!gfits_scan_alt (srcheader, keyword, "%C", 1, fields[field].ttype_cmt)) ESCAPE;

    // TUNIT is not mandatory
    snprintf (keyword, 80, "TUNIT%d", field+1);
    if (!gfits_scan (srcheader, keyword, "%s", 1, fields[field].tunit)) {
      fields[field].tunit[0] = 0;
    } else {
      if (!gfits_scan_alt (srcheader, keyword, "%C", 1, fields[field].tunit_cmt)) ESCAPE;
    }
    
    snprintf (keyword, 80, "TFORM%d", field+1);
    if (!gfits_scan (srcheader, keyword, "%s", 1, fields[field].tformat)) ESCAPE;
    if (!gfits_scan_alt (srcheader, keyword, "%C", 1, fields[field].tformat_cmt)) ESCAPE;

    // for now we set all fields to the requested type.  since I do not yet know the column data types I cannot yet assign automatic cmptypes
    strcpy (fields[field].zctype, zcmptype);

    // by using "P" format, we are limited to 32bit pointers (2GB heap)
    if (!gfits_define_bintable_column (tgtheader, "1QB(0)", fields[field].ttype, fields[field].ttype_cmt, fields[field].tunit, 1.0, 0.0)) ESCAPE;
  }    

  // allocates the default data array (tgttable->buffer)
  if (!gfits_create_table (tgtheader, tgttable)) ESCAPE;

  // copy original header to output header (XXX this needs to be finished)
  if (!gfits_copy_keywords_compress (srcheader, tgtheader)) ESCAPE;
  
  // this allocates the pointer data array for the full set of tiles (NOT the heap)
  char *tmpbuffer = NULL;
  ALLOCATE_ZERO (tmpbuffer, char, 16*Ntile); // need space for Ntile entries, each of width 16 bytes (2 long)
  for (int field = 0; field < Nfields; field++) {
    if (!gfits_set_bintable_column (tgtheader, tgttable, fields[field].ttype, tmpbuffer, Ntile)) ESCAPE;
  }
  free (tmpbuffer);
    
  // add ZIMAGE from output header
  if (!gfits_modify_alt (tgtheader, "ZTABLE", "%t", 1, TRUE)) ESCAPE;

  // define compression-specific keywords, update header as needed.
  if (!gfits_modify (tgtheader, "ZTILELEN", "%lu", 1, ztilelen)) ESCAPE;

  if (!gfits_modify (tgtheader, "ZNAXIS1", OFF_T_FMT, 1, srcheader->Naxis[0])) ESCAPE;
  if (!gfits_modify (tgtheader, "ZNAXIS2", OFF_T_FMT, 1, srcheader->Naxis[1])) ESCAPE;

  off_t pcount;
  if (!gfits_scan (srcheader, "PCOUNT", OFF_T_FMT, 1, &pcount)) ESCAPE;
  if (!gfits_modify (tgtheader, "ZPCOUNT", OFF_T_FMT, 1, pcount)) ESCAPE;

  off_t theap;
  if (gfits_scan (srcheader, "THEAP", OFF_T_FMT, 1, &theap)) {
    if (!gfits_modify (tgtheader, "ZTHEAP", OFF_T_FMT, 1, theap)) ESCAPE;
  }
  
  int max_width = 0;
  int offset = 0; // bytes from first column of first field to the current field (accumulate to set)

  for (int field = 0; field < Nfields; field++) {
    snprintf (keyword, 81, "ZFORM%d", field+1);
    if (!gfits_modify (tgtheader, keyword, "%s", 1, fields[field].tformat)) ESCAPE;
    if (!gfits_varlength_column_define (tgttable, &fields[field].zdef, field+1)) ESCAPE;
    if (!gfits_bintable_format (fields[field].tformat, fields[field].datatype, &fields[field].Nvalues, &fields[field].pixsize)) ESCAPE; // 
    fields[field].rowsize = fields[field].Nvalues*fields[field].pixsize;
    max_width = MAX(max_width, fields[field].rowsize);

    if (!strcasecmp (fields[field].zctype, "AUTO")) {
      if (!strcmp (fields[field].datatype, "short") ||
	  !strcmp (fields[field].datatype, "float") || 
	  !strcmp (fields[field].datatype, "double")|| 
	  !strcmp (fields[field].datatype, "int64_t")) {
	strcpy (fields[field].zctype, "GZIP_2");
	goto got_cmptype;
      }
      if (!strcmp (fields[field].datatype, "int")) {
	strcpy (fields[field].zctype, "RICE_1");
	goto got_cmptype;
      }
      if (!strcmp (fields[field].datatype, "gfbyte") ||
	  !strcmp (fields[field].datatype, "char")) {
	strcpy (fields[field].zctype, "GZIP_1");
	goto got_cmptype;
      }
    }
  got_cmptype:


    // OVERRIDE: RICE can only be used on integer fields
    if (!strcasecmp (fields[field].zctype, "RICE_1") || !strcasecmp (fields[field].zctype, "RICE_ONE")) {
      if (!strcmp (fields[field].datatype, "float") || !strcmp (fields[field].datatype, "double") || !strcmp (fields[field].datatype, "int64_t")) {
	strcpy (fields[field].zctype, "GZIP_2");
      }
    }

    // OVERRIDE: GZIP_2 invalid for B (use GZIP_1)
    if (!strcasecmp (fields[field].zctype, "GZIP_2") && !strcmp (fields[field].datatype, "gfbyte")) {
      strcpy (fields[field].zctype, "GZIP_1");
    }

    // compression type per column (XXX for now we are just using zcmptype, as set above)
    snprintf (keyword, 81, "ZCTYP%d", field+1);
    if (!gfits_modify (tgtheader, keyword, "%s", 1, fields[field].zctype)) ESCAPE;

    fields[field].offset = offset;
    offset += fields[field].rowsize;
  }

  // allocate the intermediate storage buffers
  unsigned long int Nzdata_alloc = 2*max_width*ztilelen + 100;
  ALLOCATE (raw,   char, max_width*ztilelen);
  ALLOCATE (zdata, char, Nzdata_alloc);

  gfits_dump_raw_table (srctable, "cmp");

  // XXX TIMER 2
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum2 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  // compress the data : copy into a tile, compress the tile, then add to the output table
  // each tile -> 1 row of the output table
  for (unsigned long int tile = 0; tile < Ntile; tile++) {

    for (int field = 0; field < Nfields; field++) {

      gettimeofday (&startTimer, (void *) NULL);

      unsigned long int Nrows = (tile == Ntile - 1) ? ztilelast : ztilelen;
      unsigned long int row_start = tile*ztilelen; 
      // ^- first row for this tile & field
      // NOTE: assumes each tile has the same length, ex the last (true for now)

      // copy the raw pixels from their native matrix locations to the temporary output buffer
      if (!strcasecmp(fields[field].zctype, "GZIP_2") || !strcasecmp(fields[field].zctype, "NONE_2")) {
	if (!gfits_collect_table_gzp2 (srctable, &fields[field], raw, row_start, Nrows)) ESCAPE;
      } else {
	if (!gfits_collect_table_data (srctable, &fields[field], raw, row_start, Nrows)) ESCAPE;
      }
      
      if (OHANA_MEMCHECK_SUPER_VERBOSE) ohana_memcheck (TRUE);

      // XXX TIMER 2a
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2a += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (VERBOSE_DUMP) {
	fprintf (stderr, "c1: ");
	for (unsigned long int k = 0; k < Nrows*fields[field].Nvalues*fields[field].pixsize; k++) {
	  fprintf (stderr, "%02hhx", raw[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      if (OHANA_MEMCHECK_SUPER_VERBOSE) ohana_memcheck (TRUE);
      // optname, optvalue = NULL, Noptions = 0
      
      unsigned long int Nraw = Nrows*fields[field].Nvalues; // number of pixels
      if (!strcasecmp(fields[field].zctype, "GZIP_1")) {
	if (!gfits_byteswap_zdata (raw, Nraw * fields[field].pixsize, fields[field].pixsize)) ESCAPE;
      }

      // XXX TIMER 2b
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2b += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (VERBOSE_DUMP) {
	fprintf (stderr, "c2: ");
	for (unsigned long int k = 0; k < Nrows*fields[field].Nvalues*fields[field].pixsize; k++) {
	  fprintf (stderr, "%02hhx", raw[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      unsigned long int Nzdata = Nzdata_alloc; // available space, replaced with actual output size on compression
      if (!gfits_compress_data (zdata, &Nzdata, fields[field].zctype, NULL, NULL, 0, raw, Nraw, fields[field].pixsize, 0, 0)) ESCAPE;
      
      // XXX TIMER 2c
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2c += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (VERBOSE_DUMP) {
	fprintf (stderr, "c3: ");
	for (unsigned long int k = 0; k < Nzdata; k++) {
	  fprintf (stderr, "%02hhx", zdata[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      if (strcasecmp(fields[field].zctype, "NONE_2") && // NONE and NONE_1 not swapped?
	  strcasecmp(fields[field].zctype, "GZIP_1") &&
	  strcasecmp(fields[field].zctype, "GZIP_2") && 
	  strcasecmp(fields[field].zctype, "RICE_1") && 
	  strcasecmp(fields[field].zctype, "RICE_ONE")) {
	if (!gfits_byteswap_zdata (zdata, Nzdata, fields[field].pixsize)) ESCAPE;
      }

      // XXX TIMER 2d
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2d += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (VERBOSE_DUMP) {
	fprintf (stderr, "c4: ");
	for (unsigned long int k = 0; k < Nzdata; k++) {
	  fprintf (stderr, "%02hhx", zdata[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      if (!gfits_varlength_column_add_data (tgttable, zdata, Nzdata, tile, &fields[field].zdef)) ESCAPE;
      // XXX TIMER 2e
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2e += dtime;
      gettimeofday (&startTimer, (void *) NULL);
 
    }
  }
  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  for (int field = 0; field < Nfields; field++) {
    if (!gfits_varlength_column_finish (tgttable, &fields[field].zdef)) ESCAPE;
  }

  // XXX TIMER 3
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum3 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  gfits_dump_cmp_table (tgttable, "cmp");

  FREE (raw);
  FREE (zdata);
  FREE (fields);
  return (TRUE);

escape:
  FREE (raw);
  FREE (zdata);
  FREE (fields);
  return FALSE;
}

# define VERBOSE 0

// raw_pixsize is the bytes / pixel for the input matrix
// place the raw image bytes for the current tile into the tile buffer
int gfits_collect_table_data (FTable *table, TableField *field, char *raw, unsigned long int row_start, unsigned long int Nrows) {

  // we are copying NN rows into the column which starts at XX and has MM bytes per row

  off_t Nx = table->header->Naxis[0];

  char *tblbuffer = &table->buffer[Nx*row_start + field->offset];

  int rowsize = field->rowsize;

  if (VERBOSE) fprintf (stderr, "distribute: ");
  for (unsigned long i = 0; i < Nrows; i++, tblbuffer += Nx) {
    memcpy (&raw[i*rowsize], tblbuffer, rowsize);
# if (VERBOSE)
    for (int j = 0; j < field->rowsize; j++) fprintf (stderr, "0x%02hhx ", table->buffer[Nx*row + field->offset + j]);
# endif
  }
  if (VERBOSE) fprintf (stderr, "\n");

  return (TRUE);
}

int gfits_collect_table_data_alt (FTable *table, TableField *field, char *raw, unsigned long int row_start, unsigned long int Nrows) {

  // we are copying NN rows of the column which starts at XX and has MM bytes per row

  off_t Nx = table->header->Naxis[0];

  if (VERBOSE) fprintf (stderr, "collect: ");
  for (unsigned long i = 0; i < Nrows; i++) {
    int row = row_start + i;
    memcpy (&raw[i*field->rowsize], &table->buffer[Nx*row + field->offset], field->rowsize);
# if (VERBOSE)
    int j; for (j = 0; j < field->rowsize; j++) fprintf (stderr, "0x%02hhx ", table->buffer[Nx*row + field->offset + j]);
# endif
  }
  if (VERBOSE) fprintf (stderr, "\n");

  return (TRUE);
}

int gfits_collect_table_gzp2 (FTable *table, TableField *field, char *raw, unsigned long int row_start, unsigned long int Nrows) {

  // we are copying NN rows into the column which starts at XX and has MM bytes per row

  off_t Nx = table->header->Naxis[0];
  int Nvalues = field->Nvalues;
  int pixsize = field->pixsize;
  int offset = field->offset;

  char *rawptr = raw;

  for (off_t k = 0; k < field->pixsize; k++) {
# ifdef BYTE_SWAP      
    char *tblptr_start = &table->buffer[Nx*row_start + offset + (pixsize - k - 1)];
# else
    char *tblptr_start = &table->buffer[Nx*row_start + offset + k];
# endif
    for (unsigned long int i = 0; i < Nrows; i++, tblptr_start += Nx) {
      char *tblptr = tblptr_start;
      for (off_t j = 0; j < Nvalues; j++, tblptr += pixsize, rawptr++) {
	*rawptr = *tblptr;
      }
    }
  }
  return (TRUE);
}

int gfits_collect_table_gzp2_alt (FTable *table, TableField *field, char *raw, unsigned long int row_start, unsigned long int Nrows) {

  // we are copying NN rows into the column which starts at XX and has MM bytes per row

  off_t Nx = table->header->Naxis[0];

  for (off_t k = 0; k < field->pixsize; k++) {
    for (unsigned long int i = 0; i < Nrows; i++) {
      unsigned long int row = row_start + i;
      char *rawptr = &raw[i*field->Nvalues + k*Nrows*field->Nvalues];
# ifdef BYTE_SWAP      
      char *tblptr = &table->buffer[Nx*row + field->offset + (field->pixsize - k - 1)];
# else
      char *tblptr = &table->buffer[Nx*row + field->offset + k];
# endif
      for (off_t j = 0; j < field->Nvalues; j++, tblptr += field->pixsize, rawptr++) {
	*rawptr = *tblptr;
	myAssert (rawptr >= raw, "oops");
	myAssert ((unsigned long int)(rawptr - raw) < Nrows*field->Nvalues*field->pixsize, "oops");
	myAssert (tblptr - table->buffer < table->header->Naxis[0]*table->header->Naxis[1], "oops");
      }
    }
  }
  return (TRUE);
}
