# include <dvo.h>

int dvo_catalog_load_mef (Catalog *catalog, int VERBOSE) {
  
  off_t Nbytes;
  off_t Nitems, Nexpect;
  int Nsecfilt;

  Header header;
  FTable ftable;

  SecFilt *primary = NULL;

  ftable.header = &header;

  /* the OBJID is a counter that uniquely defines an average entry and never changes.  if
     it is not defined for a legacy database, we can generate them using the existing index values.
     If it is missing, give a warning and recommend the user upgrade the DB */
  if (!gfits_scan (&catalog[0].header, "OBJID",    "%u", 1, &catalog[0].objID)) {
    if (VERBOSE) fprintf (stderr, "WARNING: OBJID is not set for %s: upgrade for full feature set\n", catalog[0].filename);
    catalog[0].objID = 0;
  }
  if (!gfits_scan (&catalog[0].header, "CATID",    "%u", 1, &catalog[0].catID)) {
    if (VERBOSE) fprintf (stderr, "WARNING: CATID is not set for %s: upgrade for full feature set\n", catalog[0].filename);
    catalog[0].catID = 0;
  }

  // NSTARS, average, Naverage_disk

# define GET_TABLE_SIZES(HFIELD, CFIELD, DFIELD, REQUIRED)		\
  off_t N##CFIELD;							\
  if (REQUIRED) {							\
    if (!gfits_scan (&catalog[0].header, HFIELD, OFF_T_FMT, 1, &N##CFIELD)) return (FALSE); \
  } else {								\
    if (!gfits_scan (&catalog[0].header, HFIELD, OFF_T_FMT, 1, &N##CFIELD)) N##CFIELD = 0; \
  }									\
  /* save the current number so we can do partial updates */		\
  catalog[0].DFIELD = N##CFIELD;					\
  /* default values, but we will assign these a valid value before we exit (even if empty) */ \
  catalog[0].CFIELD = NULL;
  
  /* get the components and sizes from the header */
  GET_TABLE_SIZES ("NSTARS",    average,  Naverage_disk,     TRUE);
  GET_TABLE_SIZES ("NMEAS",     measure,  Nmeasure_disk,     TRUE);
  GET_TABLE_SIZES ("NMISS",     missing,  Nmissing_disk,     FALSE);
  GET_TABLE_SIZES ("NLENSING",  lensing,  Nlensing_disk,  FALSE);
  GET_TABLE_SIZES ("NLENSOBJ",  lensobj,  Nlensobj_disk,  FALSE);
  GET_TABLE_SIZES ("NSTARPAR",  starpar,  Nstarpar_disk,  FALSE);
  GET_TABLE_SIZES ("NGALPHOT", galphot, Ngalphot_disk, FALSE);

  /**  Nsecfilt is unusual: it does not list the number of data items in the table
       instead, the number of items is Nsecfilt * Naverage.  **/

  if (!gfits_scan (&catalog[0].header, "NSECFILT", "%d", 1, &Nsecfilt)) Nsecfilt = 0;
  catalog[0].Nsecfilt = Nsecfilt;
  catalog[0].Nsecfilt_disk = Naverage * Nsecfilt;
  catalog[0].secfilt = NULL;

  /*** Average Table ***/

  /* need to read the table header to determine format (whether or not we read averages) */
  /* move pointer past PHU header -- must be already read (load_catalog) */
  Nbytes = catalog[0].header.datasize + gfits_data_size (&catalog[0].header);
  fseeko (catalog[0].f, Nbytes, SEEK_SET);

  /* read Average table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table average header");
    return (FALSE);
  }

  /* read Average table data (or skip) */
  if (catalog[0].catflags & DVO_LOAD_AVERAGE) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table average data");
      return (FALSE);
    }
    /* old versions of DVO stored one of the average magnitudes in Average. we save this if needed */
    catalog[0].average = FtableToAverage (&ftable, &Naverage, &catalog[0].catformat, &primary, FALSE);
    if (Naverage != catalog[0].Naverage_disk) {
      fprintf (stderr, "Warning: mismatch between Naverage in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Naverage,  catalog[0].Naverage_disk);
    }
    catalog[0].Naverage = catalog[0].Naverage_disk;
    catalog[0].Naverage_off = 0;
  } else {
    catalog[0].catformat = FtableGetFormat (&ftable);
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    ALLOCATE (catalog[0].average, Average, 1);
    catalog[0].Naverage = 0;
    catalog[0].Naverage_off = catalog[0].Naverage_disk;
  }
  gfits_free_header (&header);
  /** free the ftable or not? data is being used still..? **/

  /* read Measure table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table measure header");
    return (FALSE);
  }
  /* read Measure table data */
  if (catalog[0].catflags & DVO_LOAD_MEASURE) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table measure data");
      return (FALSE);
    }
    catalog[0].measure = FtableToMeasure (&ftable, catalog[0].average, &catalog[0].Nmeasure, &catalog[0].catformat, FALSE);
    if (Nmeasure != catalog[0].Nmeasure_disk) {
      fprintf (stderr, "Warning: mismatch between Nmeasure in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nmeasure,  catalog[0].Nmeasure_disk);
    }
    catalog[0].Nmeasure = catalog[0].Nmeasure_disk;
    catalog[0].Nmeasure_off = 0;
  } else {
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    ALLOCATE (catalog[0].measure, Measure, 1);
    catalog[0].Nmeasure = 0;
    catalog[0].Nmeasure_off = catalog[0].Nmeasure_disk;
  }

  /* read Missing table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table missing header");
    return (FALSE);
  }
  /* read Missing table data */
  if (catalog[0].catflags & DVO_LOAD_MISSING) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table missing data");
      return (FALSE);
    }
    /* no conversions currently defined */
    catalog[0].missing = gfits_table_get_Missing (&ftable, &catalog[0].Nmissing, NULL, NULL);
    if (!catalog[0].missing) {
      fprintf (stderr, "ERROR: failed to read missing\n");
      exit (2);
    }
    if (Nmissing != catalog[0].Nmissing_disk) {
      fprintf (stderr, "Warning: mismatch between Nmissing in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nmissing,  catalog[0].Nmissing_disk);
    }
    catalog[0].Nmissing = catalog[0].Nmissing_disk;
    catalog[0].Nmissing_off = 0;
  } else {
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    ALLOCATE (catalog[0].missing, Missing, 1);
    catalog[0].Nmissing = 0;
    catalog[0].Nmissing_off = catalog[0].Nmissing_disk;
  }

  /* read secfilt table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table secfilt header");
    return (FALSE);
  }
  /* read secfilt table data */
  if (catalog[0].catflags & DVO_LOAD_SECFILT) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table secfilt data");
      return (FALSE);
    }

    /* how many entries do we expect from the secfilt table? */
    Nexpect = catalog[0].Nsecfilt * catalog[0].Naverage;
    catalog[0].secfilt = FtableToSecFilt (&ftable, catalog[0].average, &Nitems, &catalog[0].catformat, FALSE);
    if (Nexpect != Nitems) {
      fprintf (stderr, "Warning: mismatch between Nsecfilt items in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nexpect,  Nitems);
    }

    /* if primary is defined, we were supplied with one additional average magnitude from Average
       we need to interleave these magnitudes with the secfilt entries just loaded */
    if (primary != NULL) {
      off_t Ntmpfilt, Ntotal, i, j;
      SecFilt *tmpfilt;
      tmpfilt  = catalog[0].secfilt;
      Ntmpfilt = catalog[0].Nsecfilt;
      Nsecfilt = catalog[0].Nsecfilt + 1;
      Ntotal = Nsecfilt * catalog[0].Naverage_disk;
      ALLOCATE (catalog[0].secfilt, SecFilt, Ntotal);
      for (i = 0; i < catalog[0].Naverage_disk; i++) {
	catalog[0].secfilt[i*Nsecfilt + 0] = primary[i];
	for (j = 0; j < Ntmpfilt; j++) {
	  catalog[0].secfilt[i*Nsecfilt + j + 1] = tmpfilt[i*Ntmpfilt + j];
	}
      }		
      catalog[0].Nsecfilt = Nsecfilt;
      catalog[0].Nsecfilt_disk = Ntotal;
      free (tmpfilt);
      free (primary);
    } 
    catalog[0].Nsecfilt_mem = catalog[0].Nsecfilt_disk;
    catalog[0].Nsecfilt_off = 0;
  } else {
    /* no real need to skip the data array here... */
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    if (primary != NULL) {
      free (primary);
      catalog[0].Nsecfilt ++;
      catalog[0].Nsecfilt_disk =  catalog[0].Nsecfilt * catalog[0].Naverage_disk;
    }
    ALLOCATE (catalog[0].secfilt, SecFilt, 1);
    catalog[0].Nsecfilt_mem = 0;
    catalog[0].Nsecfilt_off = catalog[0].Nsecfilt_disk;
  }

  /* read Lensing table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table lensing header");
    return (FALSE);
  }
  /* read Lensing table data */
  if (catalog[0].catflags & DVO_LOAD_LENSING) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table lensing data");
      return (FALSE);
    }
    catalog[0].lensing = FtableToLensing (&ftable, catalog[0].average, &catalog[0].Nlensing, &catalog[0].catformat, FALSE);
    if (Nlensing != catalog[0].Nlensing_disk) {
      fprintf (stderr, "Warning: mismatch between Nlensing in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nlensing,  catalog[0].Nlensing_disk);
    }
    catalog[0].Nlensing = catalog[0].Nlensing_disk;
    catalog[0].Nlensing_off = 0;
  } else {
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    ALLOCATE (catalog[0].lensing, Lensing, 1);
    catalog[0].Nlensing = 0;
    catalog[0].Nlensing_off = catalog[0].Nlensing_disk;
  }

  /* read Lensobj table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table lensobj header");
    return (FALSE);
  }
  /* read Lensobj table data */
  if (catalog[0].catflags & DVO_LOAD_LENSOBJ) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table lensobj data");
      return (FALSE);
    }
    catalog[0].lensobj = FtableToLensobj (&ftable, catalog[0].average, &catalog[0].Nlensobj, &catalog[0].catformat, FALSE);
    if (Nlensobj != catalog[0].Nlensobj_disk) {
      fprintf (stderr, "Warning: mismatch between Nlensobj in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nlensobj,  catalog[0].Nlensobj_disk);
    }
    catalog[0].Nlensobj = catalog[0].Nlensobj_disk;
    catalog[0].Nlensobj_off = 0;
  } else {
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    ALLOCATE (catalog[0].lensobj, Lensobj, 1);
    catalog[0].Nlensobj = 0;
    catalog[0].Nlensobj_off = catalog[0].Nlensobj_disk;
  }

  /* read StarPar table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table starpar header");
    return (FALSE);
  }
  /* read StarPar table data */
  if (catalog[0].catflags & DVO_LOAD_STARPAR) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table starpar data");
      return (FALSE);
    }
    catalog[0].starpar = FtableToStarPar (&ftable, catalog[0].average, &catalog[0].Nstarpar, &catalog[0].catformat, FALSE);
    if (Nstarpar != catalog[0].Nstarpar_disk) {
      fprintf (stderr, "Warning: mismatch between Nstarpar in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Nstarpar,  catalog[0].Nstarpar_disk);
    }
    catalog[0].Nstarpar = catalog[0].Nstarpar_disk;
    catalog[0].Nstarpar_off = 0;
  } else {
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    ALLOCATE (catalog[0].starpar, StarPar, 1);
    catalog[0].Nstarpar = 0;
    catalog[0].Nstarpar_off = catalog[0].Nstarpar_disk;
  }

  /* read GalPhot table header */
  if (!gfits_fread_header (catalog[0].f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read table galphot header");
    return (FALSE);
  }
  /* read GalPhot table data */
  if (catalog[0].catflags & DVO_LOAD_GALPHOT) {
    if (!gfits_fread_ftable_data (catalog[0].f, &ftable, FALSE)) {
      if (VERBOSE) fprintf (stderr, "can't read table galphot data");
      return (FALSE);
    }
    catalog[0].galphot = FtableToGalPhot (&ftable, catalog[0].average, &catalog[0].Ngalphot, &catalog[0].catformat, FALSE);
    if (Ngalphot != catalog[0].Ngalphot_disk) {
      fprintf (stderr, "Warning: mismatch between Ngalphot in PHU and Table headers ("OFF_T_FMT" vs "OFF_T_FMT")\n",  Ngalphot,  catalog[0].Ngalphot_disk);
    }
    catalog[0].Ngalphot = catalog[0].Ngalphot_disk;
    catalog[0].Ngalphot_off = 0;
  } else {
    Nbytes = gfits_data_size (&header);
    fseeko (catalog[0].f, Nbytes, SEEK_CUR);
    ALLOCATE (catalog[0].galphot, GalPhot, 1);
    catalog[0].Ngalphot = 0;
    catalog[0].Ngalphot_off = catalog[0].Ngalphot_disk;
  }

  return (TRUE);
}

/* XXX need to decompose the primary and secfilt entries for Elixir and Loneos */
/* save_catalog_mef writes a complete new file from scratch */
int dvo_catalog_save_mef (Catalog *catalog, char VERBOSE) {

  off_t Nitems;
  FILE *f;
  Matrix matrix;
  Header header;
  FTable ftable;
  SecFilt *primary;
  SecFilt *secfilt;
  off_t i, j, Nallfilt, Ntotal;
  int Nsecfilt;

  if (catalog[0].Naverage == 0) {
    if (VERBOSE) fprintf (stderr, "no stars in catalog, skipping\n");
    return (TRUE);
  }

  /* for the appropriate types, pull out the first secfilt and pass to AverageToFtable as primary */
  if ((catalog[0].catformat == DVO_FORMAT_ELIXIR) || // special case for ELIXIR 
      (catalog[0].catformat == DVO_FORMAT_LONEOS)) { // special case for LONEOS
    Nallfilt = catalog[0].Nsecfilt;
    Nsecfilt = catalog[0].Nsecfilt - 1;
    Ntotal = Nsecfilt * catalog[0].Naverage;
    ALLOCATE (primary, SecFilt, catalog[0].Naverage);
    ALLOCATE (secfilt, SecFilt, Ntotal);

    for (i = 0; i < catalog[0].Naverage; i++) {
      primary[i] = catalog[0].secfilt[i*Nallfilt + 0];
      for (j = 0; j < Nsecfilt; j++) {
	secfilt[i*Nsecfilt + j] = catalog[0].secfilt[i*Nallfilt + j + 1];
      }
    }		
  } else {
    primary = NULL;
    secfilt = catalog[0].secfilt;
    Nsecfilt = catalog[0].Nsecfilt;
  }

  /* make sure header is consistent with data */
  gfits_modify (&catalog[0].header, "NSTARS",   OFF_T_FMT, 1,  catalog[0].Naverage);
  gfits_modify (&catalog[0].header, "NMEAS",    OFF_T_FMT, 1,  catalog[0].Nmeasure);
  gfits_modify (&catalog[0].header, "NMISS",    OFF_T_FMT, 1,  catalog[0].Nmissing);
  gfits_modify (&catalog[0].header, "NSECFILT", "%d",   1,                        Nsecfilt);
  gfits_modify (&catalog[0].header, "NLENSING", OFF_T_FMT, 1,  catalog[0].Nlensing);
  gfits_modify (&catalog[0].header, "NLENSOBJ", OFF_T_FMT, 1,  catalog[0].Nlensobj);
  gfits_modify (&catalog[0].header, "NSTARPAR", OFF_T_FMT, 1,  catalog[0].Nstarpar);
  gfits_modify (&catalog[0].header, "NGALPHOT", OFF_T_FMT, 1,  catalog[0].Ngalphot);

  gfits_modify_alt (&catalog[0].header, "EXTEND",   "%t", 1, TRUE);
  gfits_modify (&catalog[0].header, "OBJID",    "%d", 1, catalog[0].objID);

  f = catalog[0].f;
  /* rewind file pointers and truncate */
  fseeko (f, 0LL, SEEK_SET);
  if (ftruncate (fileno (catalog[0].f), 0)) {
    if (VERBOSE) fprintf (stderr, "failed to truncate file\n");
    goto failure;
  }
  ftable.header = &header;

  /* write table PHU header */
  if (!gfits_fwrite_header  (catalog[0].f, &catalog[0].header)) {
    fprintf (stderr, "can't write primary header");
    goto failure;
  }

  /* this is probably a NOP, do I have to keep it in? */
  gfits_create_matrix (&catalog[0].header, &matrix);
  if (!gfits_fwrite_matrix  (catalog[0].f, &matrix)) {
    fprintf (stderr, "can't write primary matrix");
    goto failure;
  }
  gfits_free_matrix (&matrix);

  /* write out Average table (convert to FITS table format) */
  AverageToFtable (&ftable, catalog[0].average, catalog[0].Naverage, catalog[0].catformat, primary, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* write out Measure table (convert to FITS table format) */
  MeasureToFtable (&ftable, catalog[0].average, catalog[0].measure, catalog[0].Nmeasure, catalog[0].catformat, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* write out Missing table (convert to FITS table format) */
  gfits_table_set_Missing (&ftable, catalog[0].missing, catalog[0].Nmissing, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* write out SecFilt table (convert to FITS table format) */
  Nitems = catalog[0].Naverage * Nsecfilt;
  SecFiltToFtable (&ftable, secfilt, Nitems, catalog[0].catformat, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* write out Lensing table (convert to FITS table format) */
  LensingToFtable (&ftable, catalog[0].lensing, catalog[0].Nlensing, catalog[0].catformat, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* write out Lensobj table (convert to FITS table format) */
  LensobjToFtable (&ftable, catalog[0].lensobj, catalog[0].Nlensobj, catalog[0].catformat, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* write out StarPar table (convert to FITS table format) */
  StarParToFtable (&ftable, catalog[0].starpar, catalog[0].Nstarpar, catalog[0].catformat, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* write out GalPhot table (convert to FITS table format) */
  GalPhotToFtable (&ftable, catalog[0].galphot, catalog[0].Ngalphot, catalog[0].catformat, TRUE);
  if (!gfits_fwrite_Theader (catalog[0].f, &header)) {
    fprintf (stderr, "can't write table header");
    goto failure;
  }
  if (!gfits_fwrite_table (catalog[0].f, &ftable)) {
    fprintf (stderr, "can't write table data");
    goto failure;
  }
  gfits_free_table (&ftable);
  gfits_free_header (&header);

  /* free temp storage */
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }
  return (TRUE);

failure:
  /* free temp storage */
  if (primary != NULL) {
    free (primary);
    free (secfilt);
  }
  return (FALSE);
}

/*
  catalog data is:
  header (bitpix == 8)
  matrix (empty)
  average header
  average table
  measure header
  measure table
  missing header
  missing table
  secfilt header
  secfilt table
  lensing header
  lensing table
  lensobj header
  lensobj table
  starpar header
  starpar table
*/
   
