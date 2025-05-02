# include <dvo.h>
# define OHANA_MEMCHECK 0

void gfits_compress_timing ();
void gfits_uncompress_timing ();

// return options: 
// * error (cannot lock, open, read, etc)
// * empty (file is not found)
// * ok

// utility function to see if we should compress this table)
int output_is_compressed (int start, int Nrows, int Ntotal, DVOCatCompress catcompress) {
  int fullWrite = (start == 0) && (Nrows == Ntotal);
  if (fullWrite && catcompress) return TRUE;  // we want to compress, so do not swap
  return FALSE;
}

int byteswap_varlength_ftable (Header *header, FTable *table) {
  int i, Nfields;
  if (!gfits_scan (header, "TFIELDS", "%d", 1, &Nfields)) return FALSE;
  for (i = 0; i < Nfields; i++) {
    if (!gfits_byteswap_varlength_column (table, i+1)) return FALSE;
  }
  return TRUE;
}

int dvo_catalog_secfilt_to_primary (Catalog *catalog, SecFilt **myPrimary, SecFilt **mySecfilt, int *myNsecfilt) {

  off_t i, j, Nallfilt, Nsecfilt, Ntotal;

  SecFilt *primary;
  SecFilt *secfilt;

  if (catalog[0].secfilt == NULL) {       
    fprintf (stderr, "missing secfilt, cannot build output averages (dvo_catalog_split.c)\n");
    exit (1);
  }
  secfilt = catalog[0].secfilt;

  // XXX this translation only works if we have loaded / created a matched average/secfilt set
  assert (catalog[0].Nsecfilt_mem == catalog[0].Nsecfilt*catalog[0].Naverage);

  Nallfilt = catalog[0].Nsecfilt;
  Nsecfilt = catalog[0].Nsecfilt - 1;
  Ntotal = Nsecfilt * catalog[0].Naverage;
  ALLOCATE (primary, SecFilt, catalog[0].Naverage);
  ALLOCATE (secfilt, SecFilt, Ntotal);

  for (i = 0; i < catalog[0].Naverage; i++) {
    primary[i] = secfilt[i*Nallfilt + 0];
    for (j = 0; j < Nsecfilt; j++) {
      secfilt[i*Nsecfilt + j] = catalog[0].secfilt[i*Nallfilt + j + 1];
    }
  }		
  catalog[0].Nsecfilt --;
  catalog[0].Nsecfilt_mem = catalog[0].Naverage*catalog[0].Nsecfilt;

  *myPrimary = primary;
  *mySecfilt = secfilt;
  *myNsecfilt = Nsecfilt;
  return (TRUE);
}

int dvo_catalog_primary_to_secfilt (Catalog *catalog, SecFilt *primary, off_t Naves) {

  off_t Ntmpfilt, Nsecfilt, Ntotal, i, j;
  SecFilt *tmpfilt;

  tmpfilt  = catalog[0].secfilt;
  Ntmpfilt = catalog[0].Nsecfilt;

  // we do NOT modify Nsecfilt_disk; this operation only modifies in in-memory values

  catalog[0].Nsecfilt ++;
  Nsecfilt = catalog[0].Nsecfilt;
  Ntotal = Nsecfilt * Naves;

  catalog[0].Nsecfilt_mem = Ntotal;

  ALLOCATE (catalog[0].secfilt, SecFilt, Ntotal);
  for (i = 0; i < Naves; i++) {
    catalog[0].secfilt[i*Nsecfilt + 0] = primary[i];
    for (j = 0; j < Ntmpfilt; j++) {
      catalog[0].secfilt[i*Nsecfilt + j + 1] = tmpfilt[i*Ntmpfilt + j];
    }
  }		
  free (tmpfilt);
  free (primary);
  return (TRUE);
}

int dvo_catalog_save_subcat (Catalog *catalog, FTable *ftable, off_t start, off_t Nrows, off_t Ndisk, off_t Ntotal, int VERBOSE) {

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);
  
  if (!catalog->f) return TRUE;

  /* rewind file pointers and truncate (file is still open) */
  if (fseeko (catalog->f, 0LL, SEEK_SET)) {
    perror ("dvo_catalog_save_subset: ");
    fprintf (stderr, "failed to seek to beginning\n");
    return FALSE;
  }

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  // write PHU header
  if (!gfits_fwrite_header  (catalog->f, &catalog->header)) {
    fprintf (stderr, "can't write primary header\n");
    return (FALSE);
  }
  off_t fullsize = catalog->header.datasize;

  if (fflush (catalog->f)) {
    perror ("fflush: ");
    fprintf (stderr, "failed to flush file %s\n", catalog->filename);
    return FALSE;
  }

# if (1)
  // write the PHU matrix; this is probably a NOP, do I have to keep it in?
  Matrix matrix;
  gfits_create_matrix (&catalog->header, &matrix);
  if (!gfits_fwrite_matrix  (catalog->f, &matrix)) {
    fprintf (stderr, "can't write primary matrix\n");
    gfits_free_matrix (&matrix);
    return (FALSE);
  }
  fullsize += matrix.datasize;
  gfits_free_matrix (&matrix);
# endif

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  FTable *outtable = ftable;
  FTable cmptable;
  Header cmpheader;
  gfits_init_header (&cmpheader);
  gfits_init_table (&cmptable);
  cmptable.header = &cmpheader;
  
  int fullWrite = (start == 0) && (Nrows == Ntotal);
  int isCompressed = output_is_compressed (start, Nrows, Ntotal, catalog->catcompress);

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  if (isCompressed) {
    // fprintf (stderr, "compress %s\n", catalog->filename);
    char *compressMode = dvo_catalog_compress_string (catalog->catcompress);
    // I should test how Ntile affects fpack/funpack and speed
    if (!gfits_compress_table (ftable, &cmptable, 10000, compressMode)) {
      fprintf (stderr, "compression failure\n");
      free (compressMode);
      return (FALSE);
    }
    if (VERBOSE) gfits_compress_timing ();
    if (!byteswap_varlength_ftable (&cmpheader, &cmptable)) {
      fprintf (stderr, "failed to swap varlength column\n");
      free (compressMode);
      return FALSE;
    }
    if (!gfits_modify (cmptable.header, "DVO_CMP", "%s", 1, compressMode)) {
      fprintf (stderr, "can't save compression mode\n");
      free (compressMode);
      return (FALSE);
    }
    free (compressMode);
    outtable = &cmptable;
  }

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  // write the table data
  if (fullWrite) {
    if (!gfits_fwrite_Theader (catalog->f, outtable->header)) {
      fprintf (stderr, "can't write table header\n");
      return (FALSE);
    }
    if (fflush (catalog->f)) {
      perror ("fflush: ");
      fprintf (stderr, "failed to flush file %s\n", catalog->filename);
      return FALSE;
    }
    if (!gfits_fwrite_table (catalog->f, outtable)) {
      fprintf (stderr, "can't write table data\n");
      return (FALSE);
    }
  } else {
    if (!gfits_fwrite_ftable_range (catalog->f, outtable, start, Nrows, Ndisk, Ntotal)) {
      fprintf (stderr, "can't write table data (range)\n");
      return (FALSE);
    }
  }
  fullsize += outtable->datasize + outtable->header->datasize;

  // XXX test of repeated failure
  if (0) {
    Matrix myMatrix;

    // write the PHU matrix; this is probably a NOP, do I have to keep it in?
    gfits_create_matrix (&catalog->header, &myMatrix);

    int niter;
    for (niter = 0; niter < 5; niter ++) {
      char name[1024];
      snprintf (name, 1024, "%s.v.%d", catalog->filename, niter);
      FILE *f = fopen (name, "w");

      myAssert (gfits_fwrite_header  (f, &catalog->header), "failed to write header");
      myAssert (gfits_fwrite_matrix  (f, &myMatrix), "failed to write matrix");
      myAssert (gfits_fwrite_Theader (f, outtable->header), "can't write table header");
      myAssert (gfits_fwrite_table (f, outtable), "can't write table data");
      myAssert (!fflush (f), "failed to flush");
      myAssert (!fclose (f), "failed to close");
      int fd = fileno(f);
      myAssert (fsync(fd), "failed to sync");
      fprintf (stderr, "wrote to %s\n", name);
    }
    gfits_free_matrix (&myMatrix);
  }

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  // since we init'ed the structures above, these operations are safe whether or not we compressed the table
  gfits_free_header (&cmpheader);
  gfits_free_table (&cmptable);

  if (fflush (catalog->f)) {
    perror ("fflush: ");
    fprintf (stderr, "failed to flush file %s\n", catalog->filename);
    return FALSE;
  }

  // if the output file will be completely re-written, truncate to total datasize
  if (fullWrite) {
    int fd = fileno (catalog->f);
    if (ftruncate (fd, fullsize)) {
      perror ("dvo_catalog_save_subset: ");
      return (FALSE);
    }
  }

  return (TRUE);
}

int dvo_catalog_open_subcat (Catalog *catalog, Catalog **Subcat, Header *header, char *name, int VERBOSE) {

  off_t Nskip;
  int status;
  char *path, string[256];
  Catalog *subcat;

  /* in split mode, we need to init & open the corresponding measure file (even if we do not read
   * any data in at this stage) */
  ALLOCATE (subcat, Catalog, 1);
  dvo_catalog_init (subcat, TRUE);

  *Subcat = subcat;

  /* needed to find the split files below */
  path = pathname (catalog[0].filename);

  /* get split filename from main header (paths relative to cpt file) */
  if (!gfits_scan (&catalog[0].header, name,  "%s", 1, string)) {
    free (path);
    // databases created prior to ~2014.07.01 did not have the LENSING, LENSOBJ, STARPAR paths in their headers.
    // in these cases, we do not try to lock or open the relevant file
    if (!strcmp (name, "LENSING"))  return (DVO_CAT_OPEN_EMPTY);
    if (!strcmp (name, "LENSOBJ"))  return (DVO_CAT_OPEN_EMPTY);
    if (!strcmp (name, "STARPAR"))  return (DVO_CAT_OPEN_EMPTY);
    if (!strcmp (name, "GALPHOT")) return (DVO_CAT_OPEN_EMPTY);
    return (DVO_CAT_OPEN_FAIL);
  }
  int Nchar = strlen(path) + strlen(string) + 10;
  ALLOCATE (subcat[0].filename, char, Nchar);
  snprintf (subcat[0].filename, Nchar, "%s/%s", path, string);
  free (path);

  // inherit compression mode for this subcat;
  subcat->catcompress = catalog->catcompress;

  /* lock & open catalog file */
  status = dvo_catalog_lock (subcat, catalog[0].lockmode);
  if (status != DVO_CAT_OPEN_OK) {
    if (VERBOSE) {
      if (status == DVO_CAT_OPEN_EMPTY) {
	fprintf (stderr, "%s (%s) is empty\n", name, subcat[0].filename);
      } else {
	fprintf (stderr, "failure to lock %s (%s)\n", name, subcat[0].filename);
      }
    }
    return (status);
  }

  /* read PHU */
  if (!gfits_load_header (subcat[0].f, &subcat[0].header)) {
    if (VERBOSE) fprintf (stderr, "error reading PHU %s header: %s\n", name, subcat[0].filename);
    return (DVO_CAT_OPEN_FAIL);
  }
  Nskip = gfits_data_size (&subcat[0].header);
  if (fseeko (subcat[0].f, Nskip, SEEK_CUR)) { perror ("fseeko: "); exit (1); }

  /* read table header */
  if (!gfits_fread_header (subcat[0].f, header)) {
    if (VERBOSE) fprintf (stderr, "can't read %s table header\n", name);
    gfits_free_header (&subcat[0].header);
    return (DVO_CAT_OPEN_FAIL);
  }
  return (DVO_CAT_OPEN_OK);
}

// ftable must already exist and have a valid, loaded header
int gfits_fread_uncompressed (Catalog *catalog, FTable *ftable, char *nativeOrder, char VERBOSE) {

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  // fread_ftable_data requires the header

  *nativeOrder = FALSE;
  if (!gfits_fread_ftable_data (catalog->f, ftable, FALSE)) { 
    if (VERBOSE) fprintf (stderr, "can't read table data\n");
    return FALSE;
  }
  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);
  // NOTE: fread_ftable returns an unswapped table; uncompress swaps the bytes into native order
    
  if (gfits_extension_is_compressed_table (ftable->header)) {
    FTable rawtable;
    Header rawheader;
    gfits_init_table (&rawtable);
    gfits_init_header (&rawheader);
    rawtable.header = &rawheader;

    // NOTE: uncompress swaps the data bytes into native order, but needs the varlength columns native
    if (!byteswap_varlength_ftable (ftable->header, ftable)) {
      fprintf (stderr, "failed to swap varlength column\n");
      return FALSE;
    }
    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    if (!gfits_uncompress_table (ftable, &rawtable)) {
      if (VERBOSE) fprintf (stderr, "failed to uncompress table\n");
      gfits_free_table (ftable);
      return FALSE;
    }
    if (VERBOSE) gfits_uncompress_timing ();
    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    char compressMode[256];
    if (!gfits_scan (ftable->header, "DVO_CMP", "%s", 1, compressMode)) {
      strcpy (compressMode, "AUTO");
    }
    catalog->catcompress = dvo_catalog_catcompress (compressMode); // if any table is compressed, set all to a compress state?

    // free the buffers 
    gfits_free_header (ftable->header);
    gfits_free_table (ftable); 
    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);
    
    // copies values but does not allocate new memory
    Header *outheader = ftable->header;
    gfits_copy_header_ptr (&rawheader, outheader);
    gfits_copy_ftable_ptr (&rawtable, ftable); // this replaces ftable->header
    ftable->header = outheader;
    *nativeOrder = TRUE;
  }
  return TRUE;
}

// LOAD_SUBCAT(measure,MEASURE,Measure)

# define LOAD_SUBCAT(FIELD, NAME, STRUCT) 				\
  {									\
    Header header;							\
    FTable ftable;							\
    off_t Nitems;							\
    gfits_init_table  (&ftable);					\
    gfits_init_header (&header);					\
    ftable.header = &header;						\
    int status = DVO_CAT_OPEN_EMPTY;					\
    if (!(catalog[0].catflags & DVO_SKIP_##NAME)) {			\
      status = dvo_catalog_open_subcat (catalog, &catalog[0].FIELD##_catalog, ftable.header, #NAME, VERBOSE); \
      if (status == DVO_CAT_OPEN_FAIL) {				\
        if (VERBOSE) fprintf (stderr, "can't open subcat file for %s\n", #NAME); \
	return (FALSE);							\
      }									\
      if ((status == DVO_CAT_OPEN_EMPTY) && (catalog[0].N##FIELD##_disk > 0)) { \
        if (VERBOSE) fprintf (stderr, "metadata mismatch for %s\n", #NAME); \
	return (FALSE);							\
      }									\
    }									\
    if ((status != DVO_CAT_OPEN_EMPTY) && (catalog[0].catflags & DVO_LOAD_##NAME)) { \
      char nativeOrder = FALSE;						\
      /* read table data */						\
      if (!gfits_fread_uncompressed (catalog[0].FIELD##_catalog, &ftable, &nativeOrder, VERBOSE)) { \
	if (VERBOSE) fprintf (stderr, "can't read table %s data\n", #FIELD); \
	gfits_free_header (&header);					\
	return (FALSE);							\
      }									\
      /* convert data format to internal : returns number of row read in Nvalues */ \
      catalog[0].FIELD = FtableTo##STRUCT (&ftable, catalog[0].average, &Nitems, &catalog[0].catformat, nativeOrder); \
	if (Nitems != catalog[0].N##FIELD##_disk) {			\
	  fprintf (stderr, "Warning: mismatch between N%s in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n", #FIELD, Nitems, catalog[0].N##FIELD##_disk); \
	}								\
	catalog[0].N##FIELD = catalog[0].N##FIELD##_disk;		\
	  catalog[0].N##FIELD##_off = 0;				\
    } else {								\
      if (catalog[0].FIELD##_catalog) {					\
	gfits_free_header (&catalog[0].FIELD##_catalog[0].header);	\
      } else {								\
	ALLOCATE (catalog[0].FIELD##_catalog, Catalog, 1);		\
	dvo_catalog_init (catalog[0].FIELD##_catalog, TRUE);		\
      }									\
      gfits_create_header (&catalog[0].FIELD##_catalog[0].header);	\
      ALLOCATE (catalog[0].FIELD, STRUCT, 1);				\
      catalog[0].N##FIELD = 0;						\
	catalog[0].N##FIELD##_off = catalog[0].N##FIELD##_disk;		\
    }									\
    gfits_free_header (&header);					\
  }

int dvo_catalog_load_split (Catalog *catalog, int VERBOSE) {

  off_t Nbytes;
  off_t Naverage;
  off_t Nmeasure;
  off_t Nmissing;
  off_t Nlensing;
  off_t Nlensobj;
  off_t Nstarpar;
  off_t Ngalphot;
  int Nsecfilt;
  SecFilt *primary;

  primary = NULL;

  /* get the components from the header - these duplicate information in the split files (NAXIS2) */
  // NSTARS, NMEAS, NMISS are required; NLENSING, NLENSOBJ, NSTARPAR, NGALPHOT are not (0 if not found)
  if (!gfits_scan (&catalog[0].header, "NSTARS",    OFF_T_FMT, 1,  &Naverage))  return (FALSE);
  if (!gfits_scan (&catalog[0].header, "NMEAS",     OFF_T_FMT, 1,  &Nmeasure))  return (FALSE);
  if (!gfits_scan (&catalog[0].header, "NMISS",     OFF_T_FMT, 1,  &Nmissing))  return (FALSE);
  if (!gfits_scan (&catalog[0].header, "NSECFILT",       "%d", 1,  &Nsecfilt))  Nsecfilt  = 0;
  if (!gfits_scan (&catalog[0].header, "NLENSING",  OFF_T_FMT, 1,  &Nlensing))  Nlensing  = 0;
  if (!gfits_scan (&catalog[0].header, "NLENSOBJ",  OFF_T_FMT, 1,  &Nlensobj))  Nlensobj  = 0;
  if (!gfits_scan (&catalog[0].header, "NSTARPAR",  OFF_T_FMT, 1,  &Nstarpar))  Nstarpar  = 0;
  if (!gfits_scan (&catalog[0].header, "NGALPHOT",  OFF_T_FMT, 1,  &Ngalphot))  Ngalphot  = 0;

  /* the OBJID is a counter that uniquely defines an average entry and never changes.  if
     it is not defined for a legacy database, we can generate them using the existing index values.
     If it is missing, give a warning and recommend the user upgrade the DB */
  if (!gfits_scan (&catalog[0].header, "OBJID",    "%u", 1, &catalog[0].objID)) {
    if (VERBOSE) fprintf (stderr, "WARNING: OBJID is not set for this database: upgrade for full feature set\n");
    catalog[0].objID = 0;
  }
  if (!gfits_scan (&catalog[0].header, "CATID",    "%u", 1, &catalog[0].catID)) {
    if (VERBOSE) fprintf (stderr, "WARNING: CATID is not set for this database: upgrade for full feature set\n");
    catalog[0].catID = 0;
  }

  /* save the current number so we can do partial updates */
  catalog[0].Naverage_disk = Naverage;
  catalog[0].Nmeasure_disk = Nmeasure;
  catalog[0].Nmissing_disk = Nmissing;
  catalog[0].Nsecfilt_disk = Naverage * Nsecfilt;
  catalog[0].Nlensing_disk = Nlensing;
  catalog[0].Nlensobj_disk = Nlensobj;
  catalog[0].Nstarpar_disk = Nstarpar;
  catalog[0].Ngalphot_disk = Ngalphot;

  /* default values, but we will assign these a valid value before we exit (even if empty) */
  catalog[0].average  = NULL;
  catalog[0].measure  = NULL;
  catalog[0].missing  = NULL;
  catalog[0].secfilt  = NULL;
  catalog[0].lensing  = NULL;
  catalog[0].lensobj  = NULL;
  catalog[0].starpar  = NULL;
  catalog[0].galphot = NULL;

  /*** Average Table ***/

  /* need to read the table header to determine format (whether or not we read averages) */
  /* move pointer past PHU header -- must be already read (load_catalog) */
  Nbytes = catalog[0].header.datasize + gfits_data_size (&catalog[0].header);
  if (fseeko (catalog[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

  /* ftable header storage for below */
  char nativeOrder;
  Header header;
  FTable ftable;
  gfits_init_table  (&ftable);
  gfits_init_header (&header);
  ftable.header = &header;

  /* read Average table header */
  if (!gfits_fread_header (catalog[0].f, &header)) { 
    if (VERBOSE) fprintf (stderr, "can't read table average header\n");
    return (FALSE);
  }
  if (catalog[0].catflags & DVO_LOAD_AVERAGE) {
    if (!gfits_fread_uncompressed (catalog, &ftable, &nativeOrder, VERBOSE)) {
      if (VERBOSE) fprintf (stderr, "can't read Average table\n");
      gfits_free_header (&header);
      return FALSE;
    }

    /* convert the disk version of the table to the internal version.  Old versions of DVO stored
     * one of the average magnitudes in Average.  We save this in case it is needed below.  NOTE:
     * primary is only used if we read in the secfilt table, otherwise it should be freed */
    catalog[0].average = FtableToAverage (&ftable, &Naverage, &catalog[0].catformat, &primary, nativeOrder);
    if (Naverage != catalog[0].Naverage_disk) {
      fprintf (stderr, "Warning: mismatch between Naverage in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Naverage,  catalog[0].Naverage_disk);
    }
    catalog[0].Naverage = catalog[0].Naverage_disk;
    catalog[0].Naverage_off = 0;
  } else {
    catalog[0].catformat = FtableGetFormat (&ftable);
    ALLOCATE (catalog[0].average, Average, 1);
    catalog[0].Naverage = 0;
    catalog[0].Naverage_off = catalog[0].Naverage_disk;
  }
  gfits_free_header (&header);

  /*** Measure Table ***/
  
  LOAD_SUBCAT(measure,MEASURE,Measure);
  LOAD_SUBCAT(missing,MISSING,Missing);
  LOAD_SUBCAT(secfilt,SECFILT,SecFilt);
  LOAD_SUBCAT(lensing,LENSING,Lensing);
  LOAD_SUBCAT(lensobj,LENSOBJ,Lensobj);
  LOAD_SUBCAT(starpar,STARPAR,StarPar);
  LOAD_SUBCAT(galphot,GALPHOT,GalPhot);

  /**  catalog->Nsecfilt is unusual: it does not list the number of data items in the
       table instead, the number of items is Nsecfilt * Naverage, and is stored in
       Nsecfilt_mem. fix these below **/

  catalog[0].Nsecfilt_mem = catalog[0].Nsecfilt;
  catalog[0].Nsecfilt  = Nsecfilt;

  /** some old formats stored one of the secfilt values in the average table. 
      repair this below **/

  if (primary) {
    if (catalog[0].Nsecfilt_mem) {
      dvo_catalog_primary_to_secfilt (catalog, primary, catalog[0].Naverage_disk);
    } else {
      free (primary);
      catalog[0].Nsecfilt ++;
    }
  }

  return (TRUE);
}

// I need to always read both average and secfilt at the same time to correctly manage the
// primary secfilt values...
int dvo_catalog_load_segment_split (Catalog *catalog, int VERBOSE, off_t start, off_t Nrows) {

  off_t Nbytes;
  off_t Naverage, Nexpect, Nitems, Nmeasure, Nmissing, Nlensing, Nlensobj, Nstarpar, Ngalphot;
  Header header;
  FTable ftable;
  SecFilt *primary;

  /* ftable header storage for below */
  ftable.header = &header;
  ftable.buffer = NULL;
  header.buffer = NULL;
  primary = NULL;

  /*** Average (& SecFilt) Table ***/
  if (catalog[0].catflags & DVO_LOAD_AVERAGE) {

    /*** load the Average data ***/
    /* move pointer past header and matrix -- must be already read (load_catalog) */
    Nbytes = catalog[0].header.datasize + gfits_data_size (&catalog[0].header);
    if (fseeko (catalog[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read Average table header */
    if (!gfits_fread_header (catalog[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table average header");
      return (FALSE);
    }
    /* read Average table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (catalog[0].f, FALSE, FALSE, &ftable, start, Nrows)) {
      if (VERBOSE) fprintf (stderr, "can't read table average data");
      return (FALSE);
    }

    /* convert the saved version of the table to the internal version.  Old versions of DVO stored
     * one of the average magnitudes in Average.  We save this in case it is needed below.  NOTE:
     * primary is only used if we read in the secfilt table, otherwise it should be freed */
    catalog[0].average = FtableToAverage (&ftable, &Naverage, &catalog[0].catformat, &primary, FALSE);
    if (Naverage != Nrows) {
      // XXX this condition denotes the eof has been reached; not an error or a warning
      // fprintf (stderr, "Warning: mismatch between Naverage in PHU and Table headers (%d vs %d)\n", Naverage, Nrows);
    }
    gfits_free_header (&header);
    catalog[0].Naverage = Naverage;
    catalog[0].Naverage_off = start;

    /*** load the secfilt data ***/
    Catalog *subcat = catalog[0].secfilt_catalog;

    /* move pointer past header -- must be already read (load_catalog) */
    Nbytes = subcat[0].header.datasize + gfits_data_size (&subcat[0].header);
    if (fseeko (subcat[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read Secfilt table header */
    if (!gfits_fread_header (subcat[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table measure header");
      return (FALSE);
    }
    /* read Secfilt table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (subcat[0].f, FALSE, FALSE, &ftable, start*catalog[0].Nsecfilt, catalog[0].Naverage*catalog[0].Nsecfilt)) {
      if (VERBOSE) fprintf (stderr, "can't read table measure data");
      return (FALSE);
    }

    Nexpect = catalog[0].Naverage * catalog[0].Nsecfilt;
    catalog[0].secfilt = FtableToSecFilt (&ftable, catalog[0].average, &Nitems, &catalog[0].catformat, FALSE);
    if (Nitems != Nexpect) {
      fprintf (stderr, "Warning: mismatch between Nsecfilt items in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nitems,  Nexpect);
    }
    catalog[0].Nsecfilt_mem = catalog[0].Naverage * catalog[0].Nsecfilt;
    catalog[0].Nsecfilt_off = start               * catalog[0].Nsecfilt;

    /* if primary is defined, we were supplied with one additional average magnitude from Average
       we need to interleave these magnitudes with the secfilt entries just loaded */
    if (primary != NULL) {
      dvo_catalog_primary_to_secfilt (catalog, primary, Nrows);
    } 
    gfits_free_header (&header);
  }

  // XXX check the open status of the catalog
  if (catalog[0].catflags & DVO_LOAD_MEASURE) {

    Catalog *subcat = catalog[0].measure_catalog;

    /* move pointer past header -- must be already read (load_catalog) */
    Nbytes = subcat[0].header.datasize + gfits_data_size (&subcat[0].header);
    if (fseeko (subcat[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read Measure table header */
    if (!gfits_fread_header (subcat[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table measure header");
      return (FALSE);
    }
    /* read Measure table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (subcat[0].f, FALSE, FALSE, &ftable, start, Nrows)) {
      if (VERBOSE) fprintf (stderr, "can't read table measure data");
      return (FALSE);
    }

    /* convert data format to internal : returns number of row read in Nmeasure */
    catalog[0].measure = FtableToMeasure (&ftable, catalog[0].average, &Nmeasure, &catalog[0].catformat, FALSE);
    if (Nmeasure != Nrows) {
      // XXX this condition denotes the eof has been reached; not an error or a warning
      // fprintf (stderr, "Warning: mismatch between Nmeasure in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nmeasure,  Nrows);
    }
    gfits_free_header (&header);
    catalog[0].Nmeasure = Nmeasure;
    catalog[0].Nmeasure_off = start;
  }

  // XXX check the open status of the catalog?
  if (catalog[0].catflags & DVO_LOAD_MISSING) {

    Catalog *subcat = catalog[0].missing_catalog;

    /* move pointer past header -- must be already read (load_catalog) */
    Nbytes = subcat[0].header.datasize + gfits_data_size (&subcat[0].header);
    if (fseeko (subcat[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read Missing table header */
    if (!gfits_fread_header (subcat[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table missing header");
      return (FALSE);
    }
    /* read Missing table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (subcat[0].f, FALSE, FALSE, &ftable, start, Nrows)) {
      if (VERBOSE) fprintf (stderr, "can't read table missing data");
      return (FALSE);
    }

    /* no conversions currently defined : this just does the byte swap */
    catalog[0].missing = gfits_table_get_Missing (&ftable, &Nmissing, NULL, NULL);
    if (!catalog[0].missing) {
      fprintf (stderr, "ERROR: failed to read missing\n");
      exit (2);
    }
    if (Nmissing != Nrows) {
      fprintf (stderr, "Warning: mismatch between Nmissing in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nmissing,  Nrows);
    }
    gfits_free_header (&header);
    catalog[0].Nmissing = Nmissing;
    catalog[0].Nmissing_off = start;
  }

  // XXX check the open status of the catalog
  if (catalog[0].catflags & DVO_LOAD_LENSING) {

    Catalog *subcat = catalog[0].lensing_catalog;

    /* move pointer past header -- must be already read (load_catalog) */
    Nbytes = subcat[0].header.datasize + gfits_data_size (&subcat[0].header);
    if (fseeko (subcat[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read Lensing table header */
    if (!gfits_fread_header (subcat[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table lensing header");
      return (FALSE);
    }
    /* read Lensing table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (subcat[0].f, FALSE, FALSE, &ftable, start, Nrows)) {
      if (VERBOSE) fprintf (stderr, "can't read table lensing data");
      return (FALSE);
    }

    /* convert data format to internal : returns number of row read in Nlensing */
    catalog[0].lensing = FtableToLensing (&ftable, catalog[0].average, &Nlensing, &catalog[0].catformat, FALSE);
    if (Nlensing != Nrows) {
      // XXX this condition denotes the eof has been reached; not an error or a warning
      // fprintf (stderr, "Warning: mismatch between Nlensing in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nlensing,  Nrows);
    }
    gfits_free_header (&header);
    catalog[0].Nlensing = Nlensing;
    catalog[0].Nlensing_off = start;
  }

  // XXX check the open status of the catalog
  if (catalog[0].catflags & DVO_LOAD_LENSOBJ) {

    Catalog *subcat = catalog[0].lensobj_catalog;

    /* move pointer past header -- must be already read (load_catalog) */
    Nbytes = subcat[0].header.datasize + gfits_data_size (&subcat[0].header);
    if (fseeko (subcat[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read Lensobj table header */
    if (!gfits_fread_header (subcat[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table lensobj header");
      return (FALSE);
    }
    /* read Lensobj table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (subcat[0].f, FALSE, FALSE, &ftable, start, Nrows)) {
      if (VERBOSE) fprintf (stderr, "can't read table lensobj data");
      return (FALSE);
    }

    /* convert data format to internal : returns number of row read in Nlensobj */
    catalog[0].lensobj = FtableToLensobj (&ftable, catalog[0].average, &Nlensobj, &catalog[0].catformat, FALSE);
    if (Nlensobj != Nrows) {
      // XXX this condition denotes the eof has been reached; not an error or a warning
      // fprintf (stderr, "Warning: mismatch between Nlensobj in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nlensobj,  Nrows);
    }
    gfits_free_header (&header);
    catalog[0].Nlensobj = Nlensobj;
    catalog[0].Nlensobj_off = start;
  }

  // XXX check the open status of the catalog
  if (catalog[0].catflags & DVO_LOAD_STARPAR) {

    Catalog *subcat = catalog[0].starpar_catalog;

    /* move pointer past header -- must be already read (load_catalog) */
    Nbytes = subcat[0].header.datasize + gfits_data_size (&subcat[0].header);
    if (fseeko (subcat[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read StarPar table header */
    if (!gfits_fread_header (subcat[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table starpar header");
      return (FALSE);
    }
    /* read StarPar table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (subcat[0].f, FALSE, FALSE, &ftable, start, Nrows)) {
      if (VERBOSE) fprintf (stderr, "can't read table starpar data");
      return (FALSE);
    }

    /* convert data format to internal : returns number of row read in Nstarpar */
    catalog[0].starpar = FtableToStarPar (&ftable, catalog[0].average, &Nstarpar, &catalog[0].catformat, FALSE);
    if (Nstarpar != Nrows) {
      // XXX this condition denotes the eof has been reached; not an error or a warning
      // fprintf (stderr, "Warning: mismatch between Nstarpar in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nstarpar,  Nrows);
    }
    gfits_free_header (&header);
    catalog[0].Nstarpar = Nstarpar;
    catalog[0].Nstarpar_off = start;
  }

  // XXX check the open status of the catalog
  if (catalog[0].catflags & DVO_LOAD_GALPHOT) {

    Catalog *subcat = catalog[0].galphot_catalog;

    /* move pointer past header -- must be already read (load_catalog) */
    Nbytes = subcat[0].header.datasize + gfits_data_size (&subcat[0].header);
    if (fseeko (subcat[0].f, Nbytes, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* read GalPhot table header */
    if (!gfits_fread_header (subcat[0].f, &header)) {
      if (VERBOSE) fprintf (stderr, "can't read table galphot header");
      return (FALSE);
    }
    /* read GalPhot table data : format is irrelevant here */
    if (!gfits_fread_ftable_range (subcat[0].f, FALSE, FALSE, &ftable, start, Nrows)) {
      if (VERBOSE) fprintf (stderr, "can't read table galphot data");
      return (FALSE);
    }

    /* convert data format to internal : returns number of row read in Ngalphot */
    catalog[0].galphot = FtableToGalPhot (&ftable, catalog[0].average, &Ngalphot, &catalog[0].catformat, FALSE);
    if (Ngalphot != Nrows) {
      // XXX this condition denotes the eof has been reached; not an error or a warning
      // fprintf (stderr, "Warning: mismatch between Ngalphot in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Ngalphot,  Nrows);
    }
    gfits_free_header (&header);
    catalog[0].Ngalphot = Ngalphot;
    catalog[0].Ngalphot_off = start;
  }

  return (TRUE);
}

# if (0)
void compare_file_ptr (FILE *copy_ptr, FILE *real_ptr, char *name) {
  return;

  int i;
  
  char *real = (char *) real_ptr;
  char *copy = (char *) copy_ptr;

  for (i = 0; i < sizeof(FILE); i++, real++, copy++) {
    if (*real != *copy) {
      fprintf (stderr, "file pointers %s differ @ %d\n", name, (int) ((char *) real - (char *) real_ptr));
    }
  }
}
# endif

/* save_catalog_split writes all data currently in memory to disk */
// this function cannot shrink the output file sizes
int dvo_catalog_save_split (Catalog *catalog, char VERBOSE) {

  Header header;
  FTable ftable;
  SecFilt *primary, *secfilt;
  int Nsecfilt;
  off_t Naverage_disk_new, Nmeasure_disk_new, Nmissing_disk_new, Nsecfilt_disk_new, Nlensing_disk_new, Nlensobj_disk_new, Nstarpar_disk_new, Ngalphot_disk_new;

  ftable.header = &header;
  ftable.buffer = NULL;
  header.buffer = NULL;
  primary = NULL;
  
  // skip empty catalogs: it is illegal to have Measures without corresponding Averages
  Naverage_disk_new = MAX (catalog[0].Naverage_disk, catalog[0].Naverage + catalog[0].Naverage_off);
  if (Naverage_disk_new == 0) {
    if (VERBOSE) fprintf (stderr, "no stars in catalog, skipping\n");
    return (TRUE);
  }

  // for the appropriate types, pull out the first secfilt and pass to AverageToFtable as primary
  switch (catalog[0].catformat) {
    case DVO_FORMAT_ELIXIR: // special case for ELIXIR
    case DVO_FORMAT_LONEOS: // special case for LONEOS
      dvo_catalog_secfilt_to_primary (catalog, &primary, &secfilt, &Nsecfilt);
      break;
    default:
      primary = NULL;
      secfilt = catalog[0].secfilt;
      Nsecfilt = catalog[0].Nsecfilt;
      break;
  }

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  Nmeasure_disk_new = MAX (catalog[0].Nmeasure_disk, catalog[0].Nmeasure + catalog[0].Nmeasure_off);
  Nmissing_disk_new = MAX (catalog[0].Nmissing_disk, catalog[0].Nmissing + catalog[0].Nmissing_off);
  Nsecfilt_disk_new = MAX (catalog[0].Nsecfilt_disk, catalog[0].Naverage*Nsecfilt + catalog[0].Nsecfilt_off);
  Nlensing_disk_new = MAX (catalog[0].Nlensing_disk, catalog[0].Nlensing + catalog[0].Nlensing_off);
  Nlensobj_disk_new = MAX (catalog[0].Nlensobj_disk, catalog[0].Nlensobj + catalog[0].Nlensobj_off);
  Nstarpar_disk_new = MAX (catalog[0].Nstarpar_disk, catalog[0].Nstarpar + catalog[0].Nstarpar_off);
  Ngalphot_disk_new = MAX (catalog[0].Ngalphot_disk, catalog[0].Ngalphot + catalog[0].Ngalphot_off);

  /* make sure header is consistent with data */
  gfits_modify (&catalog[0].header, "NSTARS",    OFF_T_FMT, 1,  Naverage_disk_new);
  gfits_modify (&catalog[0].header, "NMEAS",     OFF_T_FMT, 1,  Nmeasure_disk_new);
  gfits_modify (&catalog[0].header, "NMISS",     OFF_T_FMT, 1,  Nmissing_disk_new);
  gfits_modify (&catalog[0].header, "NSECFILT",       "%d", 1,  Nsecfilt);
  gfits_modify (&catalog[0].header, "NLENSING",  OFF_T_FMT, 1,  Nlensing_disk_new);
  gfits_modify (&catalog[0].header, "NLENSOBJ",  OFF_T_FMT, 1,  Nlensobj_disk_new);
  gfits_modify (&catalog[0].header, "NSTARPAR",  OFF_T_FMT, 1,  Nstarpar_disk_new);
  gfits_modify (&catalog[0].header, "NGALPHOT",  OFF_T_FMT, 1,  Ngalphot_disk_new);

  gfits_modify_alt (&catalog[0].header, "EXTEND",   "%t", 1, TRUE);
  gfits_modify (&catalog[0].header, "OBJID",    "%d", 1, catalog[0].objID);

  /* in split mode, we can save only part of the data */ 

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  /*** Average Table ***/
  if ((catalog[0].catflags & DVO_LOAD_AVERAGE) && (catalog[0].average != NULL)) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Naverage_off; // first disk row to write
    off_t Nrows  = catalog[0].Naverage - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Naverage);
    assert (catalog[0].Naverage_disk >= catalog[0].Naverage_off);

    /* convert internal to external format : also results in a byte-swapped, scaled output
       table.  Note that ftable is newly allocated.
    */
    
    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Naverage_disk_new, catalog->catcompress);

    // convert to external table format
    if (!AverageToFtable (&ftable, &catalog[0].average[first], Nrows, catalog[0].catformat, primary, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    if (!dvo_catalog_save_subcat (catalog, &ftable, start, Nrows, catalog[0].Naverage_disk, Naverage_disk_new, VERBOSE)) {
      fprintf (stderr, "failure writing Average table\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    gfits_free_header (&header);
    gfits_free_table (&ftable);
  } else {
    // even if we do not save the average table, we need to keep the header in sync
    /* rewind file pointers and truncate (file is still open) */
    if (fseeko (catalog[0].f, 0, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* write table PHU header - always write this out */
    /* XXX EAM : check if disk file size has changed */
    if (!gfits_fwrite_header  (catalog[0].f, &catalog[0].header)) {
      fprintf (stderr, "can't write primary header");
      goto failure;
    }
  }

  /*** Measure Table ***/
  if ((catalog[0].catflags & DVO_LOAD_MEASURE) && (catalog[0].measure != NULL)) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nmeasure_off; // first disk row to write
    off_t Nrows  = catalog[0].Nmeasure - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nmeasure);
    assert (catalog[0].Nmeasure_disk >= catalog[0].Nmeasure_off);

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nmeasure_disk_new, catalog->catcompress);

    // convert to external table format
    if (!MeasureToFtable (&ftable, catalog[0].average, &catalog[0].measure[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // write out Measure table
    catalog->measure_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].measure_catalog, &ftable, start, Nrows, catalog[0].Nmeasure_disk, Nmeasure_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing Measure table\n");
      goto failure;
    }
    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Missing Table ***/
  if ((catalog[0].catflags & DVO_LOAD_MISSING) && (catalog[0].missing != NULL)) {

    if (catalog[0].Nmissing_off != 0) {
      fprintf (stderr, "inconsistency: Missing table cannot be written in segments\n");
      goto failure;
    }

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (catalog[0].Nmissing_off, catalog[0].Nmissing, Nmissing_disk_new, catalog->catcompress);

    // convert to external table format
    if (!gfits_table_set_Missing (&ftable, catalog[0].missing, catalog[0].Nmissing, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Missing table (must write out entire table)
    catalog->missing_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].missing_catalog, &ftable, 0, catalog[0].Nmissing, catalog[0].Nmissing, catalog[0].Nmissing, VERBOSE)) {
      fprintf (stderr, "trouble writing Missing Table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Secfilt Table ***/
  if ((catalog[0].catflags & DVO_LOAD_SECFILT) && (catalog[0].secfilt != NULL)) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nsecfilt_off; // first disk row to write
    off_t Nitems = catalog[0].Naverage*Nsecfilt;
    off_t Nrows  = Nitems - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= Nitems);
    assert (catalog[0].Nsecfilt_disk >= catalog[0].Nsecfilt_off);
    // XXX check these for consistency...

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nsecfilt_disk_new, catalog->catcompress);

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // convert to external table format
    if (!SecFiltToFtable (&ftable, &secfilt[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // write out SecFilt table
    catalog->secfilt_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].secfilt_catalog, &ftable, start, Nrows, catalog[0].Nsecfilt_disk, Nsecfilt_disk_new, VERBOSE)) {
      fprintf (stderr, "failure writing SecFilt table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Lensing Table ***/
  if ((catalog[0].catflags & DVO_LOAD_LENSING) && catalog[0].lensing && Nlensing_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nlensing_off; // first disk row to write
    off_t Nrows  = catalog[0].Nlensing - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nlensing);
    assert (catalog[0].Nlensing_disk >= catalog[0].Nlensing_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nlensing_disk_new, catalog->catcompress);

    // convert to external table format
    if (!LensingToFtable (&ftable, &catalog[0].lensing[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Lensing table
    catalog->lensing_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].lensing_catalog, &ftable, start, Nrows, catalog[0].Nlensing_disk, Nlensing_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing Lensing table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Lensobj Table ***/
  if ((catalog[0].catflags & DVO_LOAD_LENSOBJ) && catalog[0].lensobj && Nlensobj_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nlensobj_off; // first disk row to write
    off_t Nrows  = catalog[0].Nlensobj - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nlensobj);
    assert (catalog[0].Nlensobj_disk >= catalog[0].Nlensobj_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nlensobj_disk_new, catalog->catcompress);

    // convert to external table format
    if (!LensobjToFtable (&ftable, &catalog[0].lensobj[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Lensobj table
    catalog->lensobj_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].lensobj_catalog, &ftable, start, Nrows, catalog[0].Nlensobj_disk, Nlensobj_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing Lensobj table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** StarPar Table ***/
  if ((catalog[0].catflags & DVO_LOAD_STARPAR) && catalog[0].starpar && Nstarpar_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nstarpar_off; // first disk row to write
    off_t Nrows  = catalog[0].Nstarpar - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nstarpar);
    assert (catalog[0].Nstarpar_disk >= catalog[0].Nstarpar_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nstarpar_disk_new, catalog->catcompress);

    // convert to external table format
    if (!StarParToFtable (&ftable, &catalog[0].starpar[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out StarPar table
    catalog->starpar_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].starpar_catalog, &ftable, start, Nrows, catalog[0].Nstarpar_disk, Nstarpar_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing StarPar table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** GalPhot Table ***/
  if ((catalog[0].catflags & DVO_LOAD_GALPHOT) && catalog[0].galphot && Ngalphot_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Ngalphot_off; // first disk row to write
    off_t Nrows  = catalog[0].Ngalphot - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Ngalphot);
    assert (catalog[0].Ngalphot_disk >= catalog[0].Ngalphot_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Ngalphot_disk_new, catalog->catcompress);

    // convert to external table format
    if (!GalPhotToFtable (&ftable, &catalog[0].galphot[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out GalPhot table
    catalog->galphot_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].galphot_catalog, &ftable, start, Nrows, catalog[0].Ngalphot_disk, Ngalphot_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing GalPhot table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /* free temp storage */
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }
  return (TRUE);

 failure:
  /* free temp storage */
  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }
  return (FALSE);
}

/* update_catalog_split only writes new lines to file. */
int dvo_catalog_update_split (Catalog *catalog, char VERBOSE) {

  off_t Nitems;
  Header header;
  FTable ftable;
  SecFilt *primary, *secfilt;
  int Nsecfilt;
  off_t Naverage_disk_new, Nmeasure_disk_new, Nmissing_disk_new, Nsecfilt_disk_new, Nlensing_disk_new, Nlensobj_disk_new, Nstarpar_disk_new, Ngalphot_disk_new;
  off_t first, start, Nrows;

  ftable.header = &header;
  ftable.buffer = NULL;
  header.buffer = NULL;

  // skip empty catalogs: it is illegal to have Measures without corresponding Averages
  Naverage_disk_new = MAX (catalog[0].Naverage_disk, catalog[0].Naverage + catalog[0].Naverage_off);
  if (Naverage_disk_new == 0) {
    if (VERBOSE) fprintf (stderr, "no stars in catalog, skipping\n");
    return (TRUE);
  }

  // for the appropriate types, pull out the first secfilt and pass to AverageToFtable as primary 
  switch (catalog[0].catformat) {
    case DVO_FORMAT_ELIXIR: // special case for ELIXIR
    case DVO_FORMAT_LONEOS: // special case for LONEOS
      dvo_catalog_secfilt_to_primary (catalog, &primary, &secfilt, &Nsecfilt);
      break;
    default:
      primary = NULL;
      secfilt = catalog[0].secfilt;
      Nsecfilt = catalog[0].Nsecfilt;
      break;
  }

  Nmeasure_disk_new = MAX (catalog[0].Nmeasure_disk, catalog[0].Nmeasure + catalog[0].Nmeasure_off);
  Nmissing_disk_new = MAX (catalog[0].Nmissing_disk, catalog[0].Nmissing + catalog[0].Nmissing_off);
  Nsecfilt_disk_new = MAX (catalog[0].Nsecfilt_disk, catalog[0].Naverage*Nsecfilt + catalog[0].Nsecfilt_off);
  Nlensing_disk_new = MAX (catalog[0].Nlensing_disk, catalog[0].Nlensing + catalog[0].Nlensing_off);
  Nlensobj_disk_new = MAX (catalog[0].Nlensobj_disk, catalog[0].Nlensobj + catalog[0].Nlensobj_off);
  Nstarpar_disk_new = MAX (catalog[0].Nstarpar_disk, catalog[0].Nstarpar + catalog[0].Nstarpar_off);
  Ngalphot_disk_new = MAX (catalog[0].Ngalphot_disk, catalog[0].Ngalphot + catalog[0].Ngalphot_off);

  /* make sure header is consistent with data */
  gfits_modify (&catalog[0].header, "NSTARS",   OFF_T_FMT, 1,  Naverage_disk_new);
  gfits_modify (&catalog[0].header, "NMEAS",    OFF_T_FMT, 1,  Nmeasure_disk_new);
  gfits_modify (&catalog[0].header, "NMISS",    OFF_T_FMT, 1,  Nmissing_disk_new);
  gfits_modify (&catalog[0].header, "NSECFILT", "%d", 1, Nsecfilt);
  gfits_modify (&catalog[0].header, "NLENSING", OFF_T_FMT, 1,  Nlensing_disk_new);
  gfits_modify (&catalog[0].header, "NLENSOBJ", OFF_T_FMT, 1,  Nlensobj_disk_new);
  gfits_modify (&catalog[0].header, "NSTARPAR", OFF_T_FMT, 1,  Nstarpar_disk_new);
  gfits_modify (&catalog[0].header, "NGALPHOT", OFF_T_FMT, 1,  Ngalphot_disk_new);

  gfits_modify_alt (&catalog[0].header, "EXTEND",   "%t", 1, TRUE);
  gfits_modify (&catalog[0].header, "OBJID",    "%d", 1, catalog[0].objID);

  /* in split mode, we can save only part of the data */ 

  /*** Average Table ***/
  if (catalog[0].average != NULL) {

    first  = catalog[0].Naverage_disk - catalog[0].Naverage_off; // first row to write (memory)
    start  = catalog[0].Naverage_disk;                        // first row to write (disk)
    Nrows  = catalog[0].Naverage - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Naverage);
    assert (catalog[0].Naverage_disk >= catalog[0].Naverage_off);

    /* convert internal to external format */
    if (!AverageToFtable (&ftable, &catalog[0].average[first], Nrows, catalog[0].catformat, primary, TRUE)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    if (!dvo_catalog_save_subcat (catalog, &ftable, start, Nrows, catalog[0].Naverage_disk, Naverage_disk_new, VERBOSE)) {
      fprintf (stderr, "failure writing Average table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  } else {
    // even if we do not save the average table, we need to keep the header in sync
    /* rewind file pointers and truncate (file is still open) */
    if (fseeko (catalog[0].f, 0, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* write table PHU header - always write this out */
    /* XXX EAM : check if disk file size has changed */
    if (!gfits_fwrite_header  (catalog[0].f, &catalog[0].header)) {
      fprintf (stderr, "can't write primary header");
      goto failure;
    }
  }

  /*** Measure Table ***/
  if ((catalog[0].catflags & DVO_LOAD_MEASURE) && (catalog[0].measure != NULL)) {

    first  = 0;                    // first row in memory to write
    start  = catalog[0].Nmeasure_off; // first disk row to write
    Nrows  = catalog[0].Nmeasure - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nmeasure);
    assert (catalog[0].Nmeasure_disk >= catalog[0].Nmeasure_off);

    // convert to external table format (note that the block above does not damage or free catalog.average) 
    if (!MeasureToFtable (&ftable, catalog[0].average, &catalog[0].measure[first], Nrows, catalog[0].catformat, TRUE)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Measure table
    if (!dvo_catalog_save_subcat (catalog[0].measure_catalog, &ftable, start, Nrows, catalog[0].Nmeasure_disk, Nmeasure_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing Measure table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Missing Table ***/
  if ((catalog[0].catflags & DVO_LOAD_MISSING) && (catalog[0].missing != NULL)) {

    if (catalog[0].Nmissing_off != 0) {
      fprintf (stderr, "inconsistency: Missing table cannot be written in segments\n");
      goto failure;
    }

    // convert to external table format
    if (!gfits_table_set_Missing (&ftable, catalog[0].missing, catalog[0].Nmissing, TRUE)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Missing table (must write out entire table)
    if (!dvo_catalog_save_subcat (catalog[0].missing_catalog, &ftable, 0, catalog[0].Nmissing, catalog[0].Nmissing, catalog[0].Nmissing, VERBOSE)) {
      fprintf (stderr, "trouble writing Missing Table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Secfilt Table ***/
  if ((catalog[0].catflags & DVO_LOAD_SECFILT) && (catalog[0].secfilt != NULL)) {

    first  = 0;                    // first row in memory to write
    start  = catalog[0].Nsecfilt_off; // first disk row to write
    Nitems = catalog[0].Nsecfilt_mem;
    Nrows  = Nitems - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= Nitems);
    assert (catalog[0].Nsecfilt_disk >= catalog[0].Nsecfilt_off);
    // XXX check these for consistency...

    // convert to external table format
    if (!SecFiltToFtable (&ftable, &secfilt[first], Nrows, catalog[0].catformat, TRUE)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out SecFilt table
    if (!dvo_catalog_save_subcat (catalog[0].secfilt_catalog, &ftable, start, Nrows, catalog[0].Nsecfilt_disk, Nsecfilt_disk_new, VERBOSE)) {
      fprintf (stderr, "failure writing SecFilt table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Lensing Table (optional, do not save if not loaded) ***/
  if ((catalog[0].catflags & DVO_LOAD_LENSING) && catalog[0].lensing_catalog->f && catalog[0].lensing) {

    first  = 0;                    // first row in memory to write
    start  = catalog[0].Nlensing_off; // first disk row to write
    Nrows  = catalog[0].Nlensing - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nlensing);
    assert (catalog[0].Nlensing_disk >= catalog[0].Nlensing_off);

    if (catalog[0].Nlensing) {
      // convert to external table format (note that the block above does not damage or free catalog.average) 
      if (!LensingToFtable (&ftable, &catalog[0].lensing[first], Nrows, catalog[0].catformat, TRUE)) {
	fprintf (stderr, "trouble converting format\n");
	goto failure;
      }

      // write out Lensing table
      if (!dvo_catalog_save_subcat (catalog[0].lensing_catalog, &ftable, start, Nrows, catalog[0].Nlensing_disk, Nlensing_disk_new, VERBOSE)) {
	fprintf (stderr, "trouble writing Lensing table\n");
	goto failure;
      }
      gfits_free_header (&header);
      gfits_free_table (&ftable);
    }
  }

  /*** Lensobj Table (optional, do not save if not loaded) ***/
  if ((catalog[0].catflags & DVO_LOAD_LENSOBJ) && catalog[0].lensobj_catalog->f && catalog[0].lensobj) {

    first  = 0;                    // first row in memory to write
    start  = catalog[0].Nlensobj_off; // first disk row to write
    Nrows  = catalog[0].Nlensobj - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nlensobj);
    assert (catalog[0].Nlensobj_disk >= catalog[0].Nlensobj_off);

    if (catalog[0].Nlensobj) {
      // convert to external table format (note that the block above does not damage or free catalog.average) 
      if (!LensobjToFtable (&ftable, &catalog[0].lensobj[first], Nrows, catalog[0].catformat, TRUE)) {
	fprintf (stderr, "trouble converting format\n");
	goto failure;
      }

      // write out Lensobj table
      if (!dvo_catalog_save_subcat (catalog[0].lensobj_catalog, &ftable, start, Nrows, catalog[0].Nlensobj_disk, Nlensobj_disk_new, VERBOSE)) {
	fprintf (stderr, "trouble writing Lensobj table\n");
	goto failure;
      }
      gfits_free_header (&header);
      gfits_free_table (&ftable);
    }
  }

  /*** StarPar Table (optional, do not save if not loaded) ***/
  if ((catalog[0].catflags & DVO_LOAD_STARPAR) && catalog[0].starpar_catalog->f && catalog[0].starpar) {

    first  = 0;                    // first row in memory to write
    start  = catalog[0].Nstarpar_off; // first disk row to write
    Nrows  = catalog[0].Nstarpar - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nstarpar);
    assert (catalog[0].Nstarpar_disk >= catalog[0].Nstarpar_off);

    if (catalog[0].Nstarpar) {
      // convert to external table format (note that the block above does not damage or free catalog.average) 
      if (!StarParToFtable (&ftable, &catalog[0].starpar[first], Nrows, catalog[0].catformat, TRUE)) {
	fprintf (stderr, "trouble converting format\n");
	goto failure;
      }

      // write out StarPar table
      if (!dvo_catalog_save_subcat (catalog[0].starpar_catalog, &ftable, start, Nrows, catalog[0].Nstarpar_disk, Nstarpar_disk_new, VERBOSE)) {
	fprintf (stderr, "trouble writing StarPar table\n");
	goto failure;
      }
      gfits_free_header (&header);
      gfits_free_table (&ftable);
    }
  }

  /*** GalPhot Table (optional, do not save if not loaded) ***/
  if ((catalog[0].catflags & DVO_LOAD_GALPHOT) && catalog[0].galphot_catalog->f && catalog[0].galphot) {

    first  = 0;                    // first row in memory to write
    start  = catalog[0].Ngalphot_off; // first disk row to write
    Nrows  = catalog[0].Ngalphot - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Ngalphot);
    assert (catalog[0].Ngalphot_disk >= catalog[0].Ngalphot_off);

    if (catalog[0].Ngalphot) {
      // convert to external table format (note that the block above does not damage or free catalog.average) 
      if (!GalPhotToFtable (&ftable, &catalog[0].galphot[first], Nrows, catalog[0].catformat, TRUE)) {
	fprintf (stderr, "trouble converting format\n");
	goto failure;
      }

      // write out GalPhot table
      if (!dvo_catalog_save_subcat (catalog[0].galphot_catalog, &ftable, start, Nrows, catalog[0].Ngalphot_disk, Ngalphot_disk_new, VERBOSE)) {
	fprintf (stderr, "trouble writing GalPhot table\n");
	goto failure;
      }
      gfits_free_header (&header);
      gfits_free_table (&ftable);
    }
  }

  /* free temp storage */
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }
  return (TRUE);

 failure:
  /* free temp storage */
  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }
  return (FALSE);
}

/* save_catalog_split writes all data currently in memory to disk, truncating if the disk file was larger */
int dvo_catalog_save_split_complete (Catalog *catalog, char VERBOSE) {

  Header header;
  FTable ftable;
  SecFilt *primary, *secfilt;
  int Nsecfilt;
  off_t Naverage_disk_new, Nmeasure_disk_new, Nmissing_disk_new, Nsecfilt_disk_new, Nlensing_disk_new, Nlensobj_disk_new, Nstarpar_disk_new, Ngalphot_disk_new;

  ftable.header = &header;
  ftable.buffer = NULL;
  header.buffer = NULL;
  primary = NULL;
  
  // it is illegal to use this function in a partial load mode
  if (catalog[0].Naverage_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (Average) was loaded\n");
    goto failure;
  }
  if (catalog[0].Nmeasure_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (Measure) was loaded\n");
    goto failure;
  }
  if (catalog[0].Nmissing_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (Missing) was loaded\n");
    goto failure;
  }
  if (catalog[0].Nsecfilt_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (Secfilt) was loaded\n");
    goto failure;
  }
  if (catalog[0].Nlensing_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (Lensing) was loaded\n");
    goto failure;
  }
  if (catalog[0].Nlensobj_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (Lensobj) was loaded\n");
    goto failure;
  }
  if (catalog[0].Nstarpar_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (StarPar) was loaded\n");
    goto failure;
  }
  if (catalog[0].Ngalphot_off > 0) {
    fprintf (stderr, "ERROR: only partial catalog (GalPhot) was loaded\n");
    goto failure;
  }

  // skip empty catalogs: it is illegal to have Measures without corresponding Averages
  Naverage_disk_new = catalog[0].Naverage;
  if (Naverage_disk_new == 0) {
    if (VERBOSE) fprintf (stderr, "resulting catalog is empty; delete it\n");
    // unlink ();
    return (TRUE);
  }

  // for the appropriate types, pull out the first secfilt and pass to AverageToFtable as primary
  switch (catalog[0].catformat) {
    case DVO_FORMAT_ELIXIR: // special case for ELIXIR
    case DVO_FORMAT_LONEOS: // special case for LONEOS
      dvo_catalog_secfilt_to_primary (catalog, &primary, &secfilt, &Nsecfilt);
      break;
    default:
      primary = NULL;
      secfilt = catalog[0].secfilt;
      Nsecfilt = catalog[0].Nsecfilt;
      break;
  }

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  Nmeasure_disk_new = catalog[0].Nmeasure;
  Nmissing_disk_new = catalog[0].Nmissing;
  Nsecfilt_disk_new = catalog[0].Naverage*Nsecfilt;
  Nlensing_disk_new = catalog[0].Nlensing;
  Nlensobj_disk_new = catalog[0].Nlensobj;
  Nstarpar_disk_new = catalog[0].Nstarpar;
  Ngalphot_disk_new = catalog[0].Ngalphot;

  /* make sure header is consistent with data */
  gfits_modify (&catalog[0].header, "NSTARS",    OFF_T_FMT, 1,  Naverage_disk_new);
  gfits_modify (&catalog[0].header, "NMEAS",     OFF_T_FMT, 1,  Nmeasure_disk_new);
  gfits_modify (&catalog[0].header, "NMISS",     OFF_T_FMT, 1,  Nmissing_disk_new);
  gfits_modify (&catalog[0].header, "NSECFILT",       "%d", 1,  Nsecfilt);
  gfits_modify (&catalog[0].header, "NLENSING",  OFF_T_FMT, 1,  Nlensing_disk_new);
  gfits_modify (&catalog[0].header, "NLENSOBJ",  OFF_T_FMT, 1,  Nlensobj_disk_new);
  gfits_modify (&catalog[0].header, "NSTARPAR",  OFF_T_FMT, 1,  Nstarpar_disk_new);
  gfits_modify (&catalog[0].header, "NGALPHOT",  OFF_T_FMT, 1,  Ngalphot_disk_new);

  gfits_modify_alt (&catalog[0].header, "EXTEND",   "%t", 1, TRUE);
  gfits_modify (&catalog[0].header, "OBJID",    "%d", 1, catalog[0].objID);

  /* in split mode, we can save only part of the data, but not using this function (use e.g., dvo_catalog_update_split) */ 

  if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

  /*** Average Table ***/
  if ((catalog[0].catflags & DVO_LOAD_AVERAGE) && (catalog[0].average != NULL)) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Naverage_off; // first disk row to write
    off_t Nrows  = catalog[0].Naverage - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Naverage);
    assert (catalog[0].Naverage_disk >= catalog[0].Naverage_off);

    /* convert internal to external format : also results in a byte-swapped, scaled output
       table.  Note that ftable is newly allocated.
    */
    
    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Naverage_disk_new, catalog->catcompress);

    /* convert internal to external format */
    if (!AverageToFtable (&ftable, &catalog[0].average[first], Nrows, catalog[0].catformat, primary, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    if (!dvo_catalog_save_subcat (catalog, &ftable, start, Nrows, catalog[0].Naverage_disk, Naverage_disk_new, VERBOSE)) {
      fprintf (stderr, "failure writing Average table\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    gfits_free_header (&header);
    gfits_free_table (&ftable);
  } else {
    // even if we do not save the average table, we need to keep the header in sync
    /* rewind file pointers and truncate (file is still open) */
    if (fseeko (catalog[0].f, 0, SEEK_SET)) { perror ("fseeko: "); exit (1); }

    /* write table PHU header - always write this out */
    /* XXX EAM : check if disk file size has changed */
    if (!gfits_fwrite_header  (catalog[0].f, &catalog[0].header)) {
      fprintf (stderr, "can't write primary header");
      goto failure;
    }
   }

  /*** Measure Table ***/
  if ((catalog[0].catflags & DVO_LOAD_MEASURE) && (catalog[0].measure != NULL)) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nmeasure_off; // first disk row to write
    off_t Nrows  = catalog[0].Nmeasure - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nmeasure);
    assert (catalog[0].Nmeasure_disk >= catalog[0].Nmeasure_off);

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nmeasure_disk_new, catalog->catcompress);

    // convert to external table format
    // XXX does catalog.measure have averef correctly set up?
    if (!MeasureToFtable (&ftable, catalog[0].average, &catalog[0].measure[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // write out Measure table
    catalog->measure_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].measure_catalog, &ftable, start, Nrows, catalog[0].Nmeasure_disk, Nmeasure_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing Measure table\n");
      goto failure;
    }
    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /* missing table CANNOT be written unsorted, thus it is always written 
     out in full */

  /*** Missing Table ***/
  if ((catalog[0].catflags & DVO_LOAD_MISSING) && (catalog[0].missing != NULL)) {

    if (catalog[0].Nmissing_off != 0) {
      fprintf (stderr, "inconsistency: Missing table cannot be written in segments\n");
      goto failure;
    }

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (catalog[0].Nmissing_off, catalog[0].Nmissing, Nmissing_disk_new, catalog->catcompress);

    // convert to external table format
    if (!gfits_table_set_Missing (&ftable, catalog[0].missing, catalog[0].Nmissing, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Missing table (must write out entire table)
    catalog->missing_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].missing_catalog, &ftable, 0, catalog[0].Nmissing, catalog[0].Nmissing, catalog[0].Nmissing, VERBOSE)) {
      fprintf (stderr, "trouble writing Missing Table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Secfilt Table ***/
  if ((catalog[0].catflags & DVO_LOAD_SECFILT) && (catalog[0].secfilt != NULL)) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nsecfilt_off; // first disk row to write
    off_t Nitems = catalog[0].Naverage*Nsecfilt;
    off_t Nrows  = Nitems - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= Nitems);
    assert (catalog[0].Nsecfilt_disk >= catalog[0].Nsecfilt_off);
    // XXX check these for consistency...

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nsecfilt_disk_new, catalog->catcompress);

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // convert to external table format
    if (!SecFiltToFtable (&ftable, &secfilt[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    if (OHANA_MEMCHECK) ohana_memcheck (TRUE);

    // write out SecFilt table
    catalog->secfilt_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].secfilt_catalog, &ftable, start, Nrows, catalog[0].Nsecfilt_disk, Nsecfilt_disk_new, VERBOSE)) {
      fprintf (stderr, "failure writing SecFilt table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Lensing Table ***/
  if ((catalog[0].catflags & DVO_LOAD_LENSING) && catalog[0].lensing && Nlensing_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nlensing_off; // first disk row to write
    off_t Nrows  = catalog[0].Nlensing - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nlensing);
    assert (catalog[0].Nlensing_disk >= catalog[0].Nlensing_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nlensing_disk_new, catalog->catcompress);

    // convert to external table format
    if (!LensingToFtable (&ftable, &catalog[0].lensing[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Lensing table
    catalog->lensing_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].lensing_catalog, &ftable, start, Nrows, catalog[0].Nlensing_disk, Nlensing_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing Lensing table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** Lensobj Table ***/
  if ((catalog[0].catflags & DVO_LOAD_LENSOBJ) && catalog[0].lensobj && Nlensobj_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nlensobj_off; // first disk row to write
    off_t Nrows  = catalog[0].Nlensobj - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nlensobj);
    assert (catalog[0].Nlensobj_disk >= catalog[0].Nlensobj_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nlensobj_disk_new, catalog->catcompress);

    // convert to external table format
    if (!LensobjToFtable (&ftable, &catalog[0].lensobj[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out Lensobj table
    catalog->lensobj_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].lensobj_catalog, &ftable, start, Nrows, catalog[0].Nlensobj_disk, Nlensobj_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing Lensobj table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** StarPar Table ***/
  if ((catalog[0].catflags & DVO_LOAD_STARPAR) && catalog[0].starpar && Nstarpar_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Nstarpar_off; // first disk row to write
    off_t Nrows  = catalog[0].Nstarpar - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Nstarpar);
    assert (catalog[0].Nstarpar_disk >= catalog[0].Nstarpar_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Nstarpar_disk_new, catalog->catcompress);

    // convert to external table format
    if (!StarParToFtable (&ftable, &catalog[0].starpar[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out StarPar table
    catalog->starpar_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].starpar_catalog, &ftable, start, Nrows, catalog[0].Nstarpar_disk, Nstarpar_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing StarPar table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /*** GalPhot Table ***/
  if ((catalog[0].catflags & DVO_LOAD_GALPHOT) && catalog[0].galphot && Ngalphot_disk_new) {

    off_t first  = 0;                    // first row in memory to write
    off_t start  = catalog[0].Ngalphot_off; // first disk row to write
    off_t Nrows  = catalog[0].Ngalphot - first;

    assert (Nrows >= 0);
    assert (first >= 0);
    assert (first <= catalog[0].Ngalphot);
    assert (catalog[0].Ngalphot_disk >= catalog[0].Ngalphot_off);

    // if we are going to compress, we need to receive unswapped data -- 
    int swapFromNative = !output_is_compressed (start, Nrows, Ngalphot_disk_new, catalog->catcompress);

    // convert to external table format
    if (!GalPhotToFtable (&ftable, &catalog[0].galphot[first], Nrows, catalog[0].catformat, swapFromNative)) {
      fprintf (stderr, "trouble converting format\n");
      goto failure;
    }

    // write out GalPhot table
    catalog->galphot_catalog->catcompress = catalog->catcompress; // XXX this is a bit of a hack, should be done in an api
    if (!dvo_catalog_save_subcat (catalog[0].galphot_catalog, &ftable, start, Nrows, catalog[0].Ngalphot_disk, Ngalphot_disk_new, VERBOSE)) {
      fprintf (stderr, "trouble writing GalPhot table\n");
      goto failure;
    }
    gfits_free_header (&header);
    gfits_free_table (&ftable);
  }

  /* free temp storage */
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }

  return (TRUE);

 failure:
  /* free temp storage */
  gfits_free_header (&header);
  gfits_free_table (&ftable);
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }
  return (FALSE);
}

/* in split mode, extra files are linked to catalog->measure_catalog, etc.  Each
   has a valid filename, f, header.  The primary catalog data pointers are set
   to point at the corresponding entries from the elements on the chain. An
   unloaded entry has a null pointer here.
*/

/* XXX EAM : this file needs work on the error exit conditions and memory leaks, esp under errors */

/* XXX EAM : update is not efficient.  MeasureToFtable should only 
   convert the new rows (Nmeasure_disk to Nmeasure). the resulting
   table represents the end rows of the ftable.  we need to define
   the vtable based on the ftable, but with Ny = Nmeasure */  
  


// * convert to an ftable
// * optionally write the PHU header/matrix
// * advance to the start of the output data block:
// ** Nx * catalog[0].Nmeasure_off
// * write out the ftable data block
// * if Nmeasure_off + Nmeasure >= Nmeasure_disk, update padding
// ** start = Nmeasure_off
// ** Nrows = Nmeasure

