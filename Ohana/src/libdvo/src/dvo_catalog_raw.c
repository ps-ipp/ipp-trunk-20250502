# include <dvo.h>

/* read data from raw-style catalog file; set data format values based on file */

int dvo_catalog_load_raw (Catalog *catalog, int VERBOSE) {
  
  off_t Nitems, nitems;
  off_t i, Nmeas, Nmiss, size;
  off_t NewMeasure, Nskip;
  off_t AverageSize, MeasureSize, MissingSize, SecFiltSize;
  FILE *f;
  struct stat filestatus;
  char format[80], telescope[80];
  SecFilt *primary;

  f = catalog[0].f;

  /* move pointer past header -- must be already read (load_catalog) */
  fseeko (f, catalog[0].header.datasize, SEEK_SET);

  /* get the components from the header */
  catalog[0].Naverage = catalog[0].Nmeasure = catalog[0].Nmissing = catalog[0].Nsecfilt = 0;
  if (!gfits_scan (&catalog[0].header, "NSTARS",   OFF_T_FMT, 1,  &catalog[0].Naverage)) return (FALSE);
  if (!gfits_scan (&catalog[0].header, "NMEAS",    OFF_T_FMT, 1,  &catalog[0].Nmeasure)) return (FALSE);
  if (!gfits_scan (&catalog[0].header, "NMISS",    OFF_T_FMT, 1,  &catalog[0].Nmissing)) return (FALSE);
  if (!gfits_scan (&catalog[0].header, "NSECFILT", "%d",   1,               &catalog[0].Nsecfilt)) catalog[0].Nsecfilt = 0;

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

  /* determine catalog format */
  catalog[0].catformat = DVO_FORMAT_UNDEF;
  if (gfits_scan (&catalog[0].header, "FORMAT",  "%s", 1, format)) {
    catalog[0].catformat = dvo_catalog_catformat (format);
    if (catalog[0].catformat != DVO_FORMAT_UNDEF) goto got_format;
  }
  /* special cases: old versions of the DB tables which were poorly identified */
  if (gfits_scan_alt (&catalog[0].header, "NEWMEAS",  "%t", 1, &NewMeasure)) {
    catalog[0].catformat = DVO_FORMAT_ELIXIR; // special case for ELIXIR
    goto got_format;
  }
  if (gfits_scan (&catalog[0].header, "TELESCOP",  "%s", 1, telescope)) {
    if (!strncmp (telescope, "LONEOS", strlen("LONEOS"))) {
      catalog[0].catformat = DVO_FORMAT_LONEOS; // special case for LONEOS
      goto got_format;
    }
    if (!strncmp (telescope, "1.3m McGraw-Hill", strlen("1.3m McGraw-Hill"))) {
      catalog[0].catformat = DVO_FORMAT_ELIXIR; // special case for ELIXIR
      goto got_format;
    }
  }
  if (VERBOSE) fprintf (stderr, "cannot determine catalog format\n");
  return (FALSE);

// this macro generates the case statements for each type
# define FORMAT_CASE(NAME,TYPE) \
  case DVO_FORMAT_##NAME: { \
    AverageSize = sizeof(Average_##TYPE); \
    MeasureSize = sizeof(Measure_##TYPE); \
    SecFiltSize = sizeof(SecFilt_##TYPE); \
    break; }

got_format:
  /* determine datatype sizes */
  switch (catalog[0].catformat) {
    case DVO_FORMAT_INTERNAL: {
      AverageSize = sizeof(Average);
      MeasureSize = sizeof(Measure);
      SecFiltSize = sizeof(SecFilt);
      break;
    }

      FORMAT_CASE (LONEOS, Loneos);
      FORMAT_CASE (ELIXIR, Elixir);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,   PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,   PS1_DEV_2);
      FORMAT_CASE (PS1_V1,      PS1_V1);
      FORMAT_CASE (PS1_V2,      PS1_V2);
      FORMAT_CASE (PS1_V3,      PS1_V3);
      FORMAT_CASE (PS1_V4,      PS1_V4);
      FORMAT_CASE (PS1_V5,      PS1_V5);
      FORMAT_CASE (PS1_V6,      PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD, PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,     PS1_REF);
      FORMAT_CASE (PS1_REF_V2,  PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,  PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,     PS1_SIM);

    default:
      fprintf (stderr, "programming error in phot_catalog_raw\n");
      exit (2);
  }
# undef FORMAT_CASE

  MissingSize = sizeof (Missing);

  /* predicted file size - for double checking data validity */
  size = catalog[0].header.datasize;
  size += AverageSize * catalog[0].Naverage;
  size += MeasureSize * catalog[0].Nmeasure;
  size += MissingSize * catalog[0].Nmissing;
  size += SecFiltSize * catalog[0].Nsecfilt * catalog[0].Naverage;

  /* check that file size makes sense */
  if (stat (catalog[0].filename, &filestatus) == -1) {
    if (VERBOSE) fprintf (stderr, "failed to get status of catalog\n");
    return (FALSE);
  }
  if (size > filestatus.st_size) {
    if (VERBOSE) {
      fprintf (stderr, "star catalog has inconsistent size\n");
      fprintf (stderr, "average: "OFF_T_FMT" = "OFF_T_FMT" bytes\n",  catalog[0].Naverage,  catalog[0].Naverage*AverageSize);
      fprintf (stderr, "measure: "OFF_T_FMT" = "OFF_T_FMT" bytes\n",  catalog[0].Nmeasure,  catalog[0].Nmeasure*MeasureSize);
      fprintf (stderr, "missing: "OFF_T_FMT" = "OFF_T_FMT" bytes\n",  catalog[0].Nmissing,  catalog[0].Nmissing*MissingSize);
      fprintf (stderr, "secfilt: %d = "OFF_T_FMT" bytes\n",  catalog[0].Nsecfilt,  catalog[0].Nsecfilt*SecFiltSize*catalog[0].Naverage);
      fprintf (stderr, "expect: "OFF_T_FMT", found: "OFF_T_FMT"\n",  size,  filestatus.st_size);
    }
    return (FALSE);
  } 
  if (size < filestatus.st_size) {
    if (VERBOSE) fprintf (stderr, "warning: file larger than expected\n");
  } 

  if (catalog[0].Naverage == 0) {
    if (VERBOSE) fprintf (stderr, "no stars yet in catalog %s\n", catalog[0].filename);
    return (TRUE);
  }

  /* read and convert the averages (use a macro to clean this up?) */
  /* old versions of DVO stored one of the average magnitudes in Average. we save this if needed */
  if (catalog[0].catflags & DVO_LOAD_AVERAGE) {
    catalog[0].average = ReadRawAverage (catalog[0].f, catalog[0].Naverage, catalog[0].catformat, &primary);
  } else {
    /* skip over averages */
    Nskip = catalog[0].Naverage * AverageSize;
    fseeko (f, Nskip, SEEK_CUR); 
  }    
  
  /* read and convert the measures (use a macro to clean this up?) */
  if (catalog[0].catflags & DVO_LOAD_MEASURE) {
    catalog[0].measure = ReadRawMeasure (catalog[0].f, catalog[0].average, catalog[0].Nmeasure, catalog[0].catformat);
  } else {
    /* skip over measures */
    Nskip = catalog[0].Nmeasure * MeasureSize;
    fseeko (f, Nskip, SEEK_CUR); 
  }    

  /* read and convert missing */
  if (catalog[0].catflags & DVO_LOAD_MISSING) {
    ALLOCATE (catalog[0].missing, Missing, MAX (catalog[0].Nmissing, 1));
    Nitems = catalog[0].Nmissing;
    nitems = fread (catalog[0].missing, MissingSize, Nitems, f);
    if (nitems != Nitems) {
      if (VERBOSE) fprintf (stderr, "failed to read missing from catalog file %s ("OFF_T_FMT" vs "OFF_T_FMT")\n", catalog[0].filename,  nitems,  Nitems);
      return (FALSE);
    }
    gfits_convert_Missing (catalog[0].missing, MissingSize, Nitems);
  } else {
    /* skip over missings */
    Nskip = catalog[0].Nmissing * MissingSize;
    fseeko (f, Nskip, SEEK_CUR); 
  }
  
  /* read and convert secfilt */
  if (catalog[0].catflags & DVO_LOAD_SECFILT) {
    Nitems = catalog[0].Naverage * catalog[0].Nsecfilt;
    catalog[0].secfilt = ReadRawSecFilt (catalog[0].f, Nitems, catalog[0].catformat);

    /* if primary is defined, we were supplied with one additional average magnitude from Average
       we need to interleave these magnitudes with the secfilt entries just loaded */
    if (primary != NULL) {
      off_t Ntmpfilt, Nsecfilt, Ntotal, i, j;
      SecFilt *tmpfilt;
      tmpfilt  = catalog[0].secfilt;
      Ntmpfilt = catalog[0].Nsecfilt;
      Nsecfilt = catalog[0].Nsecfilt + 1;
      Ntotal = Nsecfilt * catalog[0].Naverage;
      ALLOCATE (catalog[0].secfilt, SecFilt, Ntotal);
      for (i = 0; i < catalog[0].Naverage; i++) {
	catalog[0].secfilt[i*Nsecfilt + 0] = primary[i];
	for (j = 0; j < Ntmpfilt; j++) {
	  catalog[0].secfilt[i*Nsecfilt + j + 1] = tmpfilt[i*Ntmpfilt + j];
	}
      }		
      catalog[0].Nsecfilt = Nsecfilt;
      catalog[0].Nsecfilt_mem = Ntotal;
      free (primary);
    } 

  } else {
    /* skip over secfilts */
    Nskip = catalog[0].Nsecfilt * catalog[0].Naverage * SecFiltSize;
    fseeko (f, Nskip, SEEK_CUR); 
    if (primary != NULL) free (primary);
  }

  if (VERBOSE) fprintf (stderr, "read "OFF_T_FMT" stars from catalog file %s ("OFF_T_FMT" measurements, "OFF_T_FMT" missing, %d secondary filters)\n", 
			 catalog[0].Naverage, 
			catalog[0].filename, 
			 catalog[0].Nmeasure, 
			 catalog[0].Nmissing, 
			 catalog[0].Nsecfilt);

  /* check data integrity */
  if (catalog[0].catflags & DVO_LOAD_AVERAGE) {
    for (i = Nmeas = Nmiss = 0; i < catalog[0].Naverage; i++) {
      Nmeas += catalog[0].average[i].Nmeasure; 
      Nmiss += catalog[0].average[i].Nmissing; 
    }
    if ((Nmeas != catalog[0].Nmeasure) || (Nmiss != catalog[0].Nmissing)) {
      if (VERBOSE) {
	fprintf (stderr, "****** data in catalog %s is corrupt, sums don't check\n", catalog[0].filename);
	fprintf (stderr, "****** Nmeas: "OFF_T_FMT", "OFF_T_FMT"\n",  Nmeas,  catalog[0].Nmeasure);
	fprintf (stderr, "****** Nmiss: "OFF_T_FMT", "OFF_T_FMT"\n",  Nmiss,  catalog[0].Nmissing);
      }
      return (FALSE);
    }
  }

  /* save the current number so we can do partial updates */
  catalog[0].Naverage_disk = catalog[0].Naverage;
  catalog[0].Nmeasure_disk = catalog[0].Nmeasure;
  catalog[0].Nmissing_disk = catalog[0].Nmissing;

  return (TRUE);
}

int dvo_catalog_save_raw (Catalog *catalog, char VERBOSE) {

  off_t Nitems, nitems;
  FILE *f;
  SecFilt *primary;
  SecFilt *secfilt;
  off_t i, j, Nsecfilt, Nallfilt, Ntotal;

  if (catalog[0].Naverage == 0) {
    if (VERBOSE) fprintf (stderr, "no stars in catalog, skipping\n");
    return (TRUE);
  }

  /* for the appropriate types, pull out the first secfilt and pass to WriteRawAverage as primary */
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
  gfits_modify (&catalog[0].header, "NSECFILT", "%d", 1,  catalog[0].Nsecfilt);
  gfits_modify (&catalog[0].header, "OBJID",    "%d", 1, catalog[0].objID);

  /* specify the appropriate data format */
  if (catalog[0].catformat == DVO_FORMAT_INTERNAL)  	  gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "INTERNAL");
  if (catalog[0].catformat == DVO_FORMAT_LONEOS)    	  gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "LONEOS");
  if (catalog[0].catformat == DVO_FORMAT_ELIXIR)    	  gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "ELIXIR");
  if (catalog[0].catformat == DVO_FORMAT_PANSTARRS_DEV_0) gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PANSTARRS_DEV_0");
  if (catalog[0].catformat == DVO_FORMAT_PANSTARRS_DEV_1) gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PANSTARRS_DEV_1");
  if (catalog[0].catformat == DVO_FORMAT_PS1_DEV_1)       gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_DEV_1");
  if (catalog[0].catformat == DVO_FORMAT_PS1_DEV_2)       gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_DEV_2");
  if (catalog[0].catformat == DVO_FORMAT_PS1_V1)          gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_V1");
  if (catalog[0].catformat == DVO_FORMAT_PS1_V2)          gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_V2");
  if (catalog[0].catformat == DVO_FORMAT_PS1_V3)          gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_V3");
  if (catalog[0].catformat == DVO_FORMAT_PS1_V4)          gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_V4");
  if (catalog[0].catformat == DVO_FORMAT_PS1_V5)          gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_V5");
  if (catalog[0].catformat == DVO_FORMAT_PS1_V6)          gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_V6");
  if (catalog[0].catformat == DVO_FORMAT_PS1_V5_LOAD)     gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_V5_LOAD");
  if (catalog[0].catformat == DVO_FORMAT_PS1_REF)         gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_REF");
  if (catalog[0].catformat == DVO_FORMAT_PS1_REF_V2)      gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_REF_V2");
  if (catalog[0].catformat == DVO_FORMAT_PS1_REF_V3)      gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_REF_V3");
  if (catalog[0].catformat == DVO_FORMAT_PS1_SIM)         gfits_modify (&catalog[0].header, "FORMAT", "%s", 1, "PS1_SIM");

  /* rewind file pointers and truncate file */
  f = catalog[0].f;
  fseeko (f, 0, SEEK_SET);
  if (ftruncate (fileno (catalog[0].f), 0)) {
    if (VERBOSE) fprintf (stderr, "failed to truncate file\n");
    goto failure;
  }

  /* write header data (use gfits_write_header?) */
  nitems = fwrite (catalog[0].header.buffer, 1, catalog[0].header.datasize, f);
  if (nitems != catalog[0].header.datasize) {
    if (VERBOSE) fprintf (stderr, "failed to write header\n");
    goto failure;
  }

  /* write averages and measures */
  WriteRawAverage (f, catalog[0].average, catalog[0].Naverage, catalog[0].catformat, primary);
  WriteRawMeasure (f, catalog[0].average, catalog[0].measure, catalog[0].Nmeasure, catalog[0].catformat);

  /* write missing data */
  Nitems = catalog[0].Nmissing;
  gfits_convert_Missing (catalog[0].missing, sizeof(Missing), Nitems);
  nitems = fwrite (catalog[0].missing, sizeof(Missing), Nitems, f);
  if (nitems != Nitems) {
    if (VERBOSE) fprintf (stderr, "failed to write catalog file missing %s\n", catalog[0].filename);
    goto failure;
  }

  Nitems = catalog[0].Naverage * catalog[0].Nsecfilt;
  WriteRawSecFilt (f, catalog[0].secfilt, Nitems, catalog[0].catformat);

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

/*** 
     some warnings:
     - need to be wary of exit condition errors
***/

/** Average / Raw Table conversions **/

/* in the Elixir and Loneos cases, we are supplied with an average magnitude in Average
   in these cases, save these values in primary; otherwise set primary to NULL */

Average *ReadRawAverage (FILE *f, off_t Naverage, char format, SecFilt **primary) {

  Average *average;

  *primary = NULL;

// this macro generates the case statements for each type
# define FORMAT_CASE(NAME,TYPE) \
    case DVO_FORMAT_##NAME: { \
      off_t nitems; \
      Average_##TYPE *tmpAverage; \
      ALLOCATE (tmpAverage, Average_##TYPE, MAX (Naverage, 1)); \
      nitems = fread (tmpAverage, sizeof(Average_##TYPE), Naverage, f); \
      if (nitems != Naverage) { \
	fprintf (stderr, "failed to read averages ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Naverage); \
	return (NULL); \
      } \
      gfits_convert_Average_##TYPE (tmpAverage, sizeof(Average_##TYPE), Naverage); \
      average = Average_##TYPE##_ToInternal (tmpAverage, Naverage, primary); \
      free (tmpAverage); \
      break; } \

  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      off_t nitems;
      ALLOCATE (average, Average, MAX (Naverage, 1));
      nitems = fread (average, sizeof(Average), Naverage, f);
      if (nitems != Naverage) {
	fprintf (stderr, "failed to read averages ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Naverage);
	return (NULL);
      }
      gfits_convert_Average (average, sizeof(Average), Naverage);
      break; }

      FORMAT_CASE (LONEOS, Loneos);
      FORMAT_CASE (ELIXIR, Elixir);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,   PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,   PS1_DEV_2);
      FORMAT_CASE (PS1_V1,      PS1_V1);
      FORMAT_CASE (PS1_V2,      PS1_V2);
      FORMAT_CASE (PS1_V3,      PS1_V3);
      FORMAT_CASE (PS1_V4,      PS1_V4);
      FORMAT_CASE (PS1_V5,      PS1_V5);
      FORMAT_CASE (PS1_V6,      PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD, PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,     PS1_REF);
      FORMAT_CASE (PS1_REF_V2,  PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,  PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,     PS1_SIM);

    default:
      fprintf (stderr, "error reading measures\n");
      return (NULL);
  }
# undef FORMAT_CASE

  return (average);
}

/* accepts and converts internal average formats and outputs 
   raw data in the specified format */
int WriteRawAverage (FILE *f, Average *average, off_t Naverage, char format, SecFilt *primary) {

// this macro generates the case statements for each type
# define FORMAT_CASE(NAME,TYPE) \
    case DVO_FORMAT_##NAME: { \
      off_t nitems; \
      Average_##TYPE *tmpAverage; \
      tmpAverage = AverageInternalTo_##TYPE (average, Naverage, primary); \
      gfits_convert_Average_##TYPE (tmpAverage, sizeof(Average_##TYPE), Naverage); \
      nitems = fwrite (tmpAverage, sizeof(Average_##TYPE), Naverage, f); \
      free (tmpAverage); \
      if (nitems != Naverage) { \
	fprintf (stderr, "failed to write averages ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Naverage); \
	return (FALSE); \
      } \
      break; }

  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      off_t nitems;
      gfits_convert_Average (average, sizeof(Average), Naverage);
      nitems = fwrite (average, sizeof(Average), Naverage, f);
      if (nitems != Naverage) {
	fprintf (stderr, "failed to write averages ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Naverage);
	return (FALSE);
      }
      break; }

      FORMAT_CASE (LONEOS, Loneos);
      FORMAT_CASE (ELIXIR, Elixir);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,   PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,   PS1_DEV_2);
      FORMAT_CASE (PS1_V1,      PS1_V1);
      FORMAT_CASE (PS1_V2,      PS1_V2);
      FORMAT_CASE (PS1_V3,      PS1_V3);
      FORMAT_CASE (PS1_V4,      PS1_V4);
      FORMAT_CASE (PS1_V5,      PS1_V5);
      FORMAT_CASE (PS1_V6,      PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD, PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,     PS1_REF);
      FORMAT_CASE (PS1_REF_V2,  PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,  PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,     PS1_SIM);

    default:
      fprintf (stderr, "error writing averages\n");
      return (FALSE);
  }
# undef FORMAT_CASE

  return (TRUE);
}

/** Average / Raw Table conversions **/

Measure *ReadRawMeasure (FILE *f, Average *average, off_t Nmeasure, char format) {

  Measure *measure;

// this macro generates the case statements for each type
# define FORMAT_CASE(NAME,TYPE) \
    case DVO_FORMAT_##NAME: { \
      off_t nitems; \
      Measure_##TYPE *tmpMeasure; \
      ALLOCATE (tmpMeasure, Measure_##TYPE, MAX (Nmeasure, 1)); \
      nitems = fread (tmpMeasure, sizeof(Measure_##TYPE), Nmeasure, f); \
      if (nitems != Nmeasure) { \
	fprintf (stderr, "failed to read measures ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nmeasure); \
	return (NULL); \
      } \
      gfits_convert_Measure_##TYPE (tmpMeasure, sizeof(Measure_##TYPE), Nmeasure); \
      measure = Measure_##TYPE##_ToInternal (average, tmpMeasure, Nmeasure); \
      free (tmpMeasure); \
      break; }

  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      off_t nitems;
      ALLOCATE (measure, Measure, MAX (Nmeasure, 1));
      nitems = fread (measure, sizeof(Measure), Nmeasure, f);
      if (nitems != Nmeasure) {
	fprintf (stderr, "failed to read measures ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nmeasure);
	return (NULL);
      }
      gfits_convert_Measure (measure, sizeof(Measure), Nmeasure);
      break; }

      FORMAT_CASE (LONEOS, Loneos);
      FORMAT_CASE (ELIXIR, Elixir);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,   PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,   PS1_DEV_2);
      FORMAT_CASE (PS1_V1,      PS1_V1);
      FORMAT_CASE (PS1_V2,      PS1_V2);
      FORMAT_CASE (PS1_V3,      PS1_V3);
      FORMAT_CASE (PS1_V5,      PS1_V5);
      FORMAT_CASE (PS1_V6,      PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD, PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,     PS1_REF);
      FORMAT_CASE (PS1_REF_V2,  PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,  PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,     PS1_SIM);

    default:
      fprintf (stderr, "error reading measures\n");
      return (NULL);
  }
# undef FORMAT_CASE

  return (measure);
}

/* accepts and converts internal measure formats and outputs 
   raw data in the specified format */
int WriteRawMeasure (FILE *f, Average *average, Measure *measure, off_t Nmeasure, char format) {

// this macro generates the case statements for each type
# define FORMAT_CASE(NAME,TYPE) \
    case DVO_FORMAT_##NAME: { \
      off_t nitems; \
      Measure_##TYPE *tmpMeasure; \
      tmpMeasure = MeasureInternalTo_##TYPE (average, measure, Nmeasure); \
      gfits_convert_Measure_##TYPE (tmpMeasure, sizeof(Measure_##TYPE), Nmeasure); \
      nitems = fwrite (tmpMeasure, sizeof(Measure_##TYPE), Nmeasure, f); \
      free (tmpMeasure); \
      if (nitems != Nmeasure) { \
	fprintf (stderr, "failed to write measures ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nmeasure); \
	return (FALSE); \
      } \
      break; }

  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      off_t nitems;
      gfits_convert_Measure (measure, sizeof(Measure), Nmeasure);
      nitems = fwrite (measure, sizeof(Measure), Nmeasure, f);
      if (nitems != Nmeasure) {
	fprintf (stderr, "failed to write measures ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nmeasure);
	return (FALSE);
      }
      break; }

      FORMAT_CASE (LONEOS, Loneos);
      FORMAT_CASE (ELIXIR, Elixir);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,   PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,   PS1_DEV_2);
      FORMAT_CASE (PS1_V1,      PS1_V1);
      FORMAT_CASE (PS1_V2,      PS1_V2);
      FORMAT_CASE (PS1_V3,      PS1_V3);
      FORMAT_CASE (PS1_V4,      PS1_V4);
      FORMAT_CASE (PS1_V5,      PS1_V5);
      FORMAT_CASE (PS1_V6,      PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD, PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,     PS1_REF);
      FORMAT_CASE (PS1_REF_V2,  PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,  PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,     PS1_SIM);

    default:
      fprintf (stderr, "error writing measures\n");
      return (FALSE);
  }
# undef FORMAT_CASE

  return (TRUE);
}

/** SecFilt / Raw Table conversions **/

SecFilt *ReadRawSecFilt (FILE *f, off_t Nsecfilt, char format) {

  SecFilt *secfilt;

// this macro generates the case statements for each type
# define FORMAT_CASE(NAME,TYPE) \
    case DVO_FORMAT_##NAME: { \
      off_t nitems; \
      SecFilt_##TYPE *tmpSecFilt; \
      ALLOCATE (tmpSecFilt, SecFilt_##TYPE, MAX (Nsecfilt, 1)); \
      nitems = fread (tmpSecFilt, sizeof(SecFilt_##TYPE), Nsecfilt, f); \
      if (nitems != Nsecfilt) { \
	fprintf (stderr, "failed to read secfilts ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nsecfilt); \
	return (NULL); \
      } \
      gfits_convert_SecFilt_##TYPE (tmpSecFilt, sizeof(SecFilt_##TYPE), Nsecfilt); \
      secfilt = SecFilt_##TYPE##_ToInternal (tmpSecFilt, Nsecfilt); \
      free (tmpSecFilt); \
      break; }

  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      off_t nitems;
      ALLOCATE (secfilt, SecFilt, MAX (Nsecfilt, 1));
      nitems = fread (secfilt, sizeof(SecFilt), Nsecfilt, f);
      if (nitems != Nsecfilt) {
	fprintf (stderr, "failed to read secfilts ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nsecfilt);
	return (NULL);
      }
      gfits_convert_SecFilt (secfilt, sizeof(SecFilt), Nsecfilt);
      break; }

      FORMAT_CASE (LONEOS, Loneos);
      FORMAT_CASE (ELIXIR, Elixir);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,   PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,   PS1_DEV_2);
      FORMAT_CASE (PS1_V1,      PS1_V1);
      FORMAT_CASE (PS1_V2,      PS1_V2);
      FORMAT_CASE (PS1_V3,      PS1_V3);
      FORMAT_CASE (PS1_V4,      PS1_V4);
      FORMAT_CASE (PS1_V5,      PS1_V5);
      FORMAT_CASE (PS1_V6,      PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD, PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,     PS1_REF);
      FORMAT_CASE (PS1_REF_V2,  PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,  PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,     PS1_SIM);

    default:
      fprintf (stderr, "error reading measures\n");
      return (NULL);
  }
# undef FORMAT_CASE

  return (secfilt);
}

/* accepts and converts internal secfilt formats and outputs 
   raw data in the specified format */
int WriteRawSecFilt (FILE *f, SecFilt *secfilt, off_t Nsecfilt, char format) {

// this macro generates the case statements for each type
# define FORMAT_CASE(NAME,TYPE) \
    case DVO_FORMAT_##NAME: { \
      off_t nitems; \
      SecFilt_##TYPE *tmpSecFilt; \
      tmpSecFilt = SecFiltInternalTo_##TYPE (secfilt, Nsecfilt); \
      gfits_convert_SecFilt_##TYPE (tmpSecFilt, sizeof(SecFilt_##TYPE), Nsecfilt); \
      nitems = fwrite (tmpSecFilt, sizeof(SecFilt_##TYPE), Nsecfilt, f); \
      free (tmpSecFilt); \
      if (nitems != Nsecfilt) { \
	fprintf (stderr, "failed to write secfilts ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nsecfilt); \
	return (FALSE); \
      } \
      break; }

  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      off_t nitems;
      gfits_convert_SecFilt (secfilt, sizeof(SecFilt), Nsecfilt);
      nitems = fwrite (secfilt, sizeof(SecFilt), Nsecfilt, f);
      if (nitems != Nsecfilt) {
	fprintf (stderr, "failed to write secfilts ("OFF_T_FMT" vs "OFF_T_FMT")\n",  nitems,  Nsecfilt);
	return (FALSE);
      }
      break; }

      FORMAT_CASE (LONEOS, Loneos);
      FORMAT_CASE (ELIXIR, Elixir);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,   PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,   PS1_DEV_2);
      FORMAT_CASE (PS1_V1,      PS1_V1);
      FORMAT_CASE (PS1_V2,      PS1_V2);
      FORMAT_CASE (PS1_V3,      PS1_V3);
      FORMAT_CASE (PS1_V4,      PS1_V4);
      FORMAT_CASE (PS1_V5,      PS1_V5);
      FORMAT_CASE (PS1_V6,      PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD, PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,     PS1_REF);
      FORMAT_CASE (PS1_REF_V2,  PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,  PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,     PS1_SIM);

    default:
      fprintf (stderr, "error writing secfilts\n");
      return (FALSE);
  }
# undef FORMAT_CASE

  return (TRUE);
}

