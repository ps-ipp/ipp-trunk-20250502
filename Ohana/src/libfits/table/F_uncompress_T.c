# include <ohana.h>
# include <gfitsio.h>
# include <zlib.h>

# define ESCAPE { fprintf (stderr, "error in %s @ line %d\n", __func__, __LINE__); goto escape; }

int gfits_distribute_table_data (FTable *table, TableField *field, char *raw, int row_start, int Nrows);
int gfits_distribute_table_gzp2 (FTable *table, TableField *field, char *raw, int row_start, int Nrows);

# define OHANA_MEMCHECK 0
# define VERBOSE_DUMP 0

int gfits_dump_raw_table (FTable *table, char *message) {
# if (VERBOSE_DUMP)
  fprintf (stderr, "%s data:\n", message);
  for (int i = 0; i < table->header->Naxis[0]*table->header->Naxis[1]; i++) {
    // for (i = 0; i < 16; i++) {
    fprintf (stderr, "%02hhx", table->buffer[i]);
    if (i % 2) fprintf (stderr, " ");
    if (i % 32 == 31) fprintf (stderr, "\n");
  }
  fprintf (stderr, "\n");
# else
  OHANA_UNUSED_PARAM(table);
  OHANA_UNUSED_PARAM(message);
# endif
  return TRUE;
}

int gfits_dump_cmp_table (FTable *table, char *message) {
# if (VERBOSE_DUMP)
  fprintf (stderr, "%s pntr:\n", message);
  for (int i = 0; i < table->header->Naxis[0]*table->header->Naxis[1]; i++) {
    fprintf (stderr, "%02hhx", table->buffer[i]);
    if (i % 2) fprintf (stderr, " ");
    if (i % 32 == 31) fprintf (stderr, "\n");
  }
  fprintf (stderr, "\n");

  fprintf (stderr, "%s data:\n", message);
  for (int i = 0; i < table->header->pcount; i++) {
    fprintf (stderr, "%02hhx", table->buffer[table->heap_start + i]);
    if (i % 2) fprintf (stderr, " ");
    if (i % 32 == 31) fprintf (stderr, "\n");
  }
  fprintf (stderr, "\n");
# else
  OHANA_UNUSED_PARAM(table);
  OHANA_UNUSED_PARAM(message);
# endif
  return TRUE;
}

static float timeSum1 = 0.0;
static float timeSum2 = 0.0;
static float timeSum2a = 0.0;
static float timeSum2b = 0.0;
static float timeSum2c = 0.0;
static float timeSum2d = 0.0;
static float timeSum2e = 0.0;
static float timeSum2f = 0.0;
static float timeSum3 = 0.0;

void gfits_uncompress_timing () {
  return;

  fprintf (stderr, "unc times: %f %f %f\n", timeSum1, timeSum2, timeSum3);
  fprintf (stderr, "unc times: %f %f %f %f %f %f\n", timeSum2a, timeSum2b, timeSum2c, timeSum2d, timeSum2e, timeSum2f);

  timeSum1 = 0.0;
  timeSum2 = 0.0;
  timeSum2a = 0.0;
  timeSum2b = 0.0;
  timeSum2c = 0.0;
  timeSum2d = 0.0;
  timeSum2e = 0.0;
  timeSum2f = 0.0;
  timeSum3 = 0.0;
}

int gfits_uncompress_table (FTable *srctable, FTable *tgttable) {

  char keyword[256];

  char *raw = NULL;
  char *zdata = NULL;
  TableField *fields = NULL;

  Header *srcheader = srctable->header;
  Header *tgtheader = tgttable->header;

  // XXX EAM:
  struct timeval startTimer, stopTimer;
  float dtime;
  gettimeofday (&startTimer, (void *) NULL);

  int ztilelen;
  if (!gfits_scan (srcheader, "ZTILELEN", "%d", 1, &ztilelen)) ESCAPE;
  // XXX if ZTILELEN is missing, it should have the value of ZNAXIS2

  // EXTNAME is not mandatory, keep it if it exists:
  char extname[80];
  if (!gfits_scan (srcheader, "EXTNAME", "%s", 1, extname)) {
    strcpy (extname, "UNCOMPRESSED_TABLE");
  }

  // creates an empty (Naxes = 2, Naxis[i] = 0) table header with XTENSION = BINTABLE, EXTNAME = COMPRESSED_TABLE
  if (!gfits_create_table_header (tgtheader, "BINTABLE", extname)) ESCAPE;

  int Nfields;
  if (!gfits_scan (srcheader, "TFIELDS", "%d", 1, &Nfields)) ESCAPE;

  // below we generate a fake table with tmpbuffers.  allocated it to the max needed size
  int max_width = 0;
  int offset = 0;

  ALLOCATE (fields, TableField, Nfields);
  for (int i = 0; i < Nfields; i++) {
    snprintf (keyword, 80, "TTYPE%d", i+1);
    if (!gfits_scan (srcheader, keyword, "%s", 1, fields[i].ttype)) ESCAPE;
    if (!gfits_scan_alt (srcheader, keyword, "%C", 1, fields[i].ttype_cmt)) ESCAPE;

    // TUNIT is not mandatory
    snprintf (keyword, 80, "TUNIT%d", i+1);
    if (!gfits_scan (srcheader, keyword, "%s", 1, fields[i].tunit)) {
      fields[i].tunit[0] = 0;
    } else {
      if (!gfits_scan_alt (srcheader, keyword, "%C", 1, fields[i].tunit_cmt)) ESCAPE;
    }
    
    snprintf (keyword, 80, "ZFORM%d", i+1);
    if (!gfits_scan (srcheader, keyword, "%s", 1, fields[i].tformat)) ESCAPE;
    if (!gfits_scan_alt (srcheader, keyword, "%C", 1, fields[i].tformat_cmt)) ESCAPE;

    snprintf (keyword, 80, "ZCTYP%d", i+1);
    if (!gfits_scan (srcheader, keyword, "%s", 1, fields[i].zctype)) {
      strcpy (fields[i].zctype, "GZIP_2"); // XXX NOTE: not yet supported
    }

    if (!gfits_bintable_format (fields[i].tformat, fields[i].datatype, &fields[i].Nvalues, &fields[i].pixsize)) ESCAPE;
    fields[i].rowsize = fields[i].Nvalues*fields[i].pixsize;
    max_width = MAX(max_width, fields[i].rowsize);

    fields[i].offset = offset;
    offset += fields[i].rowsize;

    if (!gfits_varlength_column_define (srctable, &fields[i].zdef, i + 1)) ESCAPE;

    if (!gfits_define_bintable_column (tgtheader, fields[i].tformat, fields[i].ttype, fields[i].ttype_cmt, fields[i].tunit, 1.0, 0.0)) ESCAPE;
  }    

  // allocates the default data array (tgttable->buffer)
  if (!gfits_create_table (tgtheader, tgttable)) ESCAPE;

  // copy original header to output header (XXX this needs to be finished)
  if (!gfits_copy_keywords_compress (srcheader, tgtheader)) ESCAPE;
  
  off_t Nx, Ny;
  if (!gfits_scan (srcheader, "ZNAXIS1", OFF_T_FMT, 1, &Nx)) ESCAPE;
  if (!gfits_scan (srcheader, "ZNAXIS2", OFF_T_FMT, 1, &Ny)) ESCAPE;
  myAssert (Nx == tgtheader->Naxis[0], "table definition error?");

  // XXX TIMER 1
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum1 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  // this allocates the pointer data array for the full set of tiles (NOT the heap) by
  // calling with an empty data array, data values are not actually copied (this is done
  // below after decompression)
  for (int i = 0; i < Nfields; i++) {
    if (!gfits_set_bintable_column (tgtheader, tgttable, fields[i].ttype, NULL, Ny)) ESCAPE;
  }
    
  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  int Ntile = srcheader->Naxis[1];
  int ztilelast;
  if (Ntile == 0) {
    ztilelast = 0;
  } else {
    ztilelast = (tgtheader->Naxis[1] % ztilelen) ? tgtheader->Naxis[1] % ztilelen : ztilelen;
  }

  // allocate the intermediate storage buffers
  int Nraw_alloc = max_width*ztilelen + 1000;
  ALLOCATE (raw,   char, Nraw_alloc);
  ALLOCATE (zdata, char, max_width*ztilelen);

  // uncompress the data : extract a tile's compressed data, uncompress the tile, insert into full table
  // each tile -> 1 row of the output table
  
  gfits_dump_cmp_table (srctable, "unc");

  // XXX TIMER 2
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum2 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  for (off_t row = 0; row < srcheader->Naxis[1]; row++) {

    for (int i = 0; i < Nfields; i++) {

      // gfits_uncompress_data can take values specific to the compression mode
      // optname, optvalue = NULL, Noptions = 0

      // XXX note that this function currently byteswaps.  I should redo the code to make
      // byteswap of the varlength data segment a separate function

      gettimeofday (&startTimer, (void *) NULL);

      off_t Nzdata;
      char *zdata = gfits_varlength_column_pointer (srctable, &fields[i].zdef, row, &Nzdata);
      if (!zdata) ESCAPE;
    
      if (VERBOSE_DUMP) {
	fprintf (stderr, "u4: ");
	for (int k = 0; k < Nzdata; k++) {
	  fprintf (stderr, "%02hhx", zdata[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      // XXX TIMER 2a
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2a += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (strcasecmp(fields[i].zctype, "NONE_2") && 
	  strcasecmp(fields[i].zctype, "GZIP_1") && 
	  strcasecmp(fields[i].zctype, "GZIP_2") && 
	  strcasecmp(fields[i].zctype, "RICE_1") && 
	  strcasecmp(fields[i].zctype, "RICE_ONE")) {
	// Nzdata is number of bytes
	if (!gfits_byteswap_zdata (zdata, Nzdata, fields[i].pixsize)) ESCAPE;
      }

      // XXX TIMER 2b
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2b += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (VERBOSE_DUMP) {
	fprintf (stderr, "u3: ");
	for (int k = 0; k < Nzdata; k++) {
	  fprintf (stderr, "%02hhx", zdata[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      // XXX TIMER 2c
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2c += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      unsigned long int Nrows = (row == Ntile - 1) ? ztilelast : ztilelen;
      unsigned long int Nraw = Nrows*fields[i].Nvalues*fields[i].pixsize; // expected number of bytes
      if (!gfits_uncompress_data (zdata, Nzdata, fields[i].zctype, NULL, NULL, 0, raw, &Nraw, Nraw_alloc, fields[i].pixsize)) ESCAPE;

      if (VERBOSE_DUMP) {
	fprintf (stderr, "u2: ");
	for (unsigned long int k = 0; k < Nraw*fields[i].pixsize; k++) {
	  fprintf (stderr, "%02hhx", raw[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      // XXX TIMER 2d
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2d += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (!strcasecmp(fields[i].zctype, "GZIP_1")) {
	myAssert ((fields[i].zdef.format != 'C') && (fields[i].zdef.format != 'M'), "swap is probably wrong for C or M columns");
	if (!gfits_byteswap_zdata (raw, Nraw * fields[i].pixsize, fields[i].pixsize)) ESCAPE;
      }

      // XXX TIMER 2e
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2e += dtime;
      gettimeofday (&startTimer, (void *) NULL);
  
      if (VERBOSE_DUMP) {
	fprintf (stderr, "u1: ");
	for (unsigned long int k = 0; k < Nraw*fields[i].pixsize; k++) {
	  fprintf (stderr, "%02hhx", raw[k]);
	  if (k % 2) fprintf (stderr, " ");
	  // if (k % 32 == 31) fprintf (stderr, "\n");
	}
	fprintf (stderr, "\n");
      }

      // int valid = (row == 0) && (i == 0);

      off_t row_start = row*ztilelen; 
      // ^- first row for this tile & field
      // NOTE: assumes each tile has the same length, ex the last (true for now)

      // copy the raw pixels from their native matrix locations to the temporary output buffer
      if (!strcasecmp(fields[i].zctype, "GZIP_2") || !strcasecmp(fields[i].zctype, "NONE_2")) {
	if (!gfits_distribute_table_gzp2 (tgttable, &fields[i], raw, row_start, Nrows)) ESCAPE;
      } else {
	if (!gfits_distribute_table_data (tgttable, &fields[i], raw, row_start, Nrows)) ESCAPE;
      }      

      // XXX TIMER 2f
      gettimeofday (&stopTimer, (void *) NULL); 
      dtime = DTIME (stopTimer, startTimer);
      timeSum2f += dtime;
      gettimeofday (&startTimer, (void *) NULL);
    }
  }

  // XXX TIMER 3
  gettimeofday (&stopTimer, (void *) NULL); 
  dtime = DTIME (stopTimer, startTimer);
  timeSum3 += dtime;
  gettimeofday (&startTimer, (void *) NULL);

  gfits_dump_raw_table (tgttable, "unc");

  free (raw);
  free (zdata);
  free (fields);
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
int gfits_distribute_table_data (FTable *table, TableField *field, char *raw, int row_start, int Nrows) {

  // we are copying NN rows into the column which starts at XX and has MM bytes per row

  off_t Nx = table->header->Naxis[0];

  char *tblbuffer = &table->buffer[Nx*row_start + field->offset];

  int rowsize = field->rowsize;

  if (VERBOSE) fprintf (stderr, "distribute: ");
  for (off_t i = 0; i < Nrows; i++, tblbuffer += Nx) {
    memcpy (tblbuffer, &raw[i*rowsize], rowsize);
# if (VERBOSE)
    for (int j = 0; j < field->rowsize; j++) fprintf (stderr, "0x%02hhx ", table->buffer[Nx*row + field->offset + j]);
# endif
  }
  if (VERBOSE) fprintf (stderr, "\n");

  return (TRUE);
}

int gfits_distribute_table_gzp2 (FTable *table, TableField *field, char *raw, int row_start, int Nrows) {

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
    for (off_t i = 0; i < Nrows; i++, tblptr_start += Nx) {
      char *tblptr = tblptr_start;
      for (off_t j = 0; j < Nvalues; j++, tblptr += pixsize, rawptr++) {
	*tblptr = *rawptr;
      }
    }
  }
  return (TRUE);
}

int gfits_distribute_table_gzp2_alt (FTable *table, TableField *field, char *raw, int row_start, int Nrows) {
  
  // we are copying NN rows into the column which starts at XX and has MM bytes per row
  
  off_t Nx = table->header->Naxis[0];
  
  for (off_t k = 0; k < field->pixsize; k++) {
    for (off_t i = 0; i < Nrows; i++) {
      int row = row_start + i;
      char *rawptr = &raw[i*field->Nvalues + k*Nrows*field->Nvalues];
# ifdef BYTE_SWAP     
      char *tblptr = &table->buffer[Nx*row + field->offset + (field->pixsize - k - 1)];
# else
      char *tblptr = &table->buffer[Nx*row + field->offset + k];
# endif
      for (off_t j = 0; j < field->Nvalues; j++, tblptr += field->pixsize, rawptr++) {
	*tblptr = *rawptr;
      }
    }
  }
  return (TRUE);
}
