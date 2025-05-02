# include <dvo.h>

/* The Ftable-TYPE conversion functions determine the format of table based on EXTNAME in header.
   they convert the table to the internal format, and set 'format'.  

   The TYPE-Ftable conversions functions create output tables in the format requested
   by the 'format' function parameter.
*/

/** this file might be more readable if I use macros for the repetative
    constructions below **/

DVOCatFormat FtableGetFormat (FTable *ftable) {

  DVOCatFormat format;
  char extname[80];

  /* read the format of this table */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for average table\n");
    return (DVO_FORMAT_UNDEF);
  }

  if (!strcmp (extname, "DVO_AVERAGE")) {
    format = DVO_FORMAT_INTERNAL;
    return (format);
  }
# define CONVERT_FORMAT(NAME, FORMAT)		\
  if (!strcmp (extname, NAME)) {		\
    format = DVO_FORMAT_##FORMAT;		\
    return (format); }

  CONVERT_FORMAT ("DVO_AVERAGE_ELIXIR", 	 ELIXIR);
  CONVERT_FORMAT ("DVO_AVERAGE_LONEOS", 	 LONEOS);
  CONVERT_FORMAT ("DVO_AVERAGE_PANSTARRS_DEV_0", PANSTARRS_DEV_0);
  CONVERT_FORMAT ("DVO_AVERAGE_PANSTARRS_DEV_1", PANSTARRS_DEV_1);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_DEV_1",       PS1_DEV_1);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_DEV_2",       PS1_DEV_2);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V1",          PS1_V1);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V2",          PS1_V2);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V3",          PS1_V3);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V4",          PS1_V4);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V5",          PS1_V5);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V6",          PS1_V6);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V5_LOAD",     PS1_V5_LOAD);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_REF",         PS1_REF);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_REF_V2",      PS1_REF_V2);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_REF_V3",      PS1_REF_V3);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_SIM",         PS1_SIM);
# undef CONVERT_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);
  return (DVO_FORMAT_UNDEF);
}

/*** Average / FTable conversion functions ***/

Average *FtableToAverage (FTable *ftable, off_t *Naverage, DVOCatFormat *format, SecFilt **primary, char nativeBytes) {

  Average *average;
  char extname[80];

  /* in the Elixir and Loneos cases, we are supplied with an average magnitude in Average
     in these cases, save these values in primary; otherwise set primary to NULL */
  *primary = NULL;

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for average table\n");
    return (FALSE);
  }

  // block to convert broken tables (PS1_V4 made before the Xfix addition)
  if (!strcmp (extname, "DVO_AVERAGE_PS1_V4") && (ftable[0].header[0].Naxis[0] == 120)) {
    Average_PS1_V4alt *tmpAverage;
    tmpAverage = gfits_table_get_Average_PS1_V4alt (ftable, Naverage, NULL);
    if (!tmpAverage) {
      fprintf (stderr, "ERROR: failed to read averages\n");
      exit (2);
    }
    average = Average_PS1_V4alt_ToInternal (tmpAverage, *Naverage);
    free (tmpAverage);
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V4;
    return (average); 
  }

  // block to convert old tables (PS1_V5 versions made during development)
  if (!strcmp (extname, "DVO_AVERAGE_PS1_V5") && (ftable[0].header[0].Naxis[0] == 184)) {
    Average_PS1_V5alt *tmpAverage;
    tmpAverage = gfits_table_get_Average_PS1_V5alt (ftable, Naverage, NULL);
    if (!tmpAverage) {
      fprintf (stderr, "ERROR: failed to read averages\n");
      exit (2);
    }
    average = Average_PS1_V5alt_ToInternal (tmpAverage, *Naverage);
    free (tmpAverage);
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;
    return (average); 
  }

# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    Average_##TYPE *tmpAverage;						\
    tmpAverage = gfits_table_get_Average_##TYPE (ftable, Naverage, NULL, &nativeBytes); \
    if (!tmpAverage) {							\
      fprintf (stderr, "ERROR: failed to read averages\n");		\
      exit (2);								\
    }									\
    average = Average_##TYPE##_ToInternal (tmpAverage, *Naverage, primary); \
    free (tmpAverage);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (average); }

  if (!strcmp (extname, "DVO_AVERAGE")) {
    average = gfits_table_get_Average (ftable, Naverage, NULL, &nativeBytes);
    if (!average) {
      fprintf (stderr, "ERROR: failed to read averages\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (average);
  }

  CONVERT_FORMAT ("DVO_AVERAGE_ELIXIR", 	 ELIXIR, 	  Elixir);
  CONVERT_FORMAT ("DVO_AVERAGE_LONEOS", 	 LONEOS, 	  Loneos);
  CONVERT_FORMAT ("DVO_AVERAGE_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  CONVERT_FORMAT ("DVO_AVERAGE_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V1",          PS1_V1,          PS1_V1);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V2",          PS1_V2,          PS1_V2);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V3",          PS1_V3,          PS1_V3);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V4",          PS1_V4,          PS1_V4);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V5",          PS1_V5,          PS1_V5);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V6",          PS1_V6,          PS1_V6);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_REF",         PS1_REF,         PS1_REF);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  CONVERT_FORMAT ("DVO_AVERAGE_PS1_SIM",         PS1_SIM,         PS1_SIM);
# undef CONVERT_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Naverage = 0;
  return (NULL);
}

int AverageToFtable (FTable *ftable, Average *average, off_t Naverage, DVOCatFormat format, SecFilt *primary, int swapFromNative) {
  
# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    Average_##TYPE *tmpAverage;						\
    tmpAverage = AverageInternalTo_##TYPE (average, Naverage, primary); \
    gfits_table_set_Average_##TYPE (ftable, tmpAverage, Naverage, swapFromNative); \
    free (tmpAverage);							\
    break; }
  
  /* convert from the internal format */
  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      gfits_table_set_Average (ftable, average, Naverage, swapFromNative);
      break; }

      FORMAT_CASE (ELIXIR, 	    Elixir);
      FORMAT_CASE (LONEOS, 	    Loneos);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
      FORMAT_CASE (PS1_V1,          PS1_V1);
      FORMAT_CASE (PS1_V2,          PS1_V2);
      FORMAT_CASE (PS1_V3,          PS1_V3);
      FORMAT_CASE (PS1_V4,          PS1_V4);
      FORMAT_CASE (PS1_V5,          PS1_V5);
      FORMAT_CASE (PS1_V6,          PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,         PS1_REF);
      FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,         PS1_SIM);
# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (average)\n");
      return (FALSE);
  }
  return (TRUE);
}

/*** Measure / FTable conversion functions ***/

// FtableToMeasure needs the Average since old formats stored measure.dR,dD only
// other FtableToFOO conversions accept the average argument for macro construction
Measure *FtableToMeasure (FTable *ftable, Average *average, off_t *Nmeasure, DVOCatFormat *format, char nativeBytes) {

  Measure *measure;
  char extname[80];

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for measure table\n");
    return (FALSE);
  }

  // block to convert broken tables (PS1_V4 made before the Xfix addition)
  if (!strcmp (extname, "DVO_MEASURE_PS1_V4") && (ftable[0].header[0].Naxis[0] == 176)) {
    fprintf (stderr, "reading alt PS1_V4 format\n");
    myAssert (!nativeBytes, "need to implement optional swap");
    Measure_PS1_V4alt *tmpMeasure;
    tmpMeasure = gfits_table_get_Measure_PS1_V4alt (ftable, Nmeasure, NULL);
    if (!tmpMeasure) {
      fprintf (stderr, "ERROR: failed to read measures\n");
      exit (2);
    }
    myAssert (average, "conversion to internal needs average table");
    measure = Measure_PS1_V4alt_ToInternal (average, tmpMeasure, *Nmeasure);
    free (tmpMeasure);
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V4;
    return (measure); 
  }

  // block to convert PV1_V5_0 tables (PS1_V5 made before the XoffCAM,YoffCAM addition)
  if (!strcmp (extname, "DVO_MEASURE_PS1_V5") && (ftable[0].header[0].Naxis[0] == 232)) {
    fprintf (stderr, "reading alt PS1_V5 format\n");
    myAssert (!nativeBytes, "need to implement optional swap");
    Measure_PS1_V5alt *tmpMeasure;
    tmpMeasure = gfits_table_get_Measure_PS1_V5alt (ftable, Nmeasure, NULL);
    if (!tmpMeasure) {
      fprintf (stderr, "ERROR: failed to read measures\n");
      exit (2);
    }
    myAssert (average, "conversion to internal needs average table");
    measure = Measure_PS1_V5alt_ToInternal (average, tmpMeasure, *Nmeasure);
    free (tmpMeasure);
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;
    return (measure); 
  }

# define CONVERT_FORMAT(NAME, FORMAT, TYPE, ABS_COORDS)			\
  if (!strcmp (extname, NAME)) {					\
    Measure_##TYPE *tmpMeasure;						\
    tmpMeasure = gfits_table_get_Measure_##TYPE (ftable, Nmeasure, NULL, &nativeBytes); \
    if (!tmpMeasure) {							\
      fprintf (stderr, "ERROR: failed to read measures\n");		\
      exit (2);								\
    }									\
    myAssert (ABS_COORDS || average, "conversion to internal needs average table"); \
    measure = Measure_##TYPE##_ToInternal (average, tmpMeasure, *Nmeasure); \
    free (tmpMeasure);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (measure); }

  if (!strcmp (extname, "DVO_MEASURE")) {
    measure = gfits_table_get_Measure (ftable, Nmeasure, NULL, &nativeBytes);
    if (!measure) {
      fprintf (stderr, "ERROR: failed to read measures\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (measure);
  }

  CONVERT_FORMAT ("DVO_MEASURE_ELIXIR", 	 ELIXIR, 	  Elixir,          FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_LONEOS", 	 LONEOS,          Loneos,          FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0, FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1, FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1,       FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2,       FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_V1",          PS1_V1,          PS1_V1,          FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_V2",          PS1_V2,          PS1_V2,          FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_V3",          PS1_V3,          PS1_V3,          FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_V4",          PS1_V4,          PS1_V4,          FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_V5",          PS1_V5,          PS1_V5,          TRUE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_V6",          PS1_V6,          PS1_V6,          TRUE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD,     TRUE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_REF",         PS1_REF,         PS1_REF,         FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2,      FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3,      FALSE);
  CONVERT_FORMAT ("DVO_MEASURE_PS1_SIM",         PS1_SIM,         PS1_SIM,         TRUE);
# undef CONVERT_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Nmeasure = 0;
  return (NULL);
}

// MeasureToFtable needs the Average since old formats stored measure.dR,dD only
int MeasureToFtable (FTable *ftable, Average *average, Measure *measure, off_t Nmeasure, DVOCatFormat format, int swapFromNative) {

# define FORMAT_CASE(FORMAT, TYPE, ABS_COORDS)				\
  case DVO_FORMAT_##FORMAT: {						\
    Measure_##TYPE *tmpMeasure;						\
    myAssert (ABS_COORDS || average, "conversion from internal needs average table"); \
    tmpMeasure = MeasureInternalTo_##TYPE (average, measure, Nmeasure); \
    gfits_table_set_Measure_##TYPE (ftable, tmpMeasure, Nmeasure, swapFromNative); \
    free (tmpMeasure);							\
    break; }

  /* convert from the internal format */
  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      gfits_table_set_Measure (ftable, measure, Nmeasure, swapFromNative);
      break; }

      FORMAT_CASE (ELIXIR, 	    Elixir,          FALSE);
      FORMAT_CASE (LONEOS, 	    Loneos,          FALSE);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0, FALSE);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1, FALSE);
      FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1,       FALSE);
      FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2,       FALSE);
      FORMAT_CASE (PS1_V1,          PS1_V1,          FALSE);
      FORMAT_CASE (PS1_V2,          PS1_V2,          FALSE);
      FORMAT_CASE (PS1_V3,          PS1_V3,          FALSE);
      FORMAT_CASE (PS1_V4,          PS1_V4,          FALSE);
      FORMAT_CASE (PS1_V5,          PS1_V5,          TRUE);
      FORMAT_CASE (PS1_V6,          PS1_V6,          TRUE);
      FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD,     TRUE);
      FORMAT_CASE (PS1_REF,         PS1_REF,         FALSE);
      FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2,      FALSE);
      FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3,      FALSE);
      FORMAT_CASE (PS1_SIM,         PS1_SIM,         TRUE);
# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (measure)\n");
      return (FALSE);
  }
  return (TRUE);
}

/*** Missing / FTable conversion functions ***/

Missing *FtableToMissing (FTable *ftable, Average *average, off_t *Nmissing, DVOCatFormat *format, char nativeBytes) {
  OHANA_UNUSED_PARAM(average);

  Missing *missing;
  char extname[80];

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for missing table\n");
    return (FALSE);
  }

# define SKIPPING_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    fprintf (stderr, "ERROR: format %s not defined for missing, skipping\n", NAME); \
    *Nmissing = 0;							\
    return NULL;							\
  }

# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    Missing_##TYPE *tmpMissing;						\
    tmpMissing = gfits_table_get_Missing_##TYPE (ftable, Nmissing, NULL, &nativeBytes); \
    if (!tmpMissing) {							\
      fprintf (stderr, "ERROR: failed to read missing\n");		\
      exit (2);								\
    }									\
    missing = Missing_##TYPE##_ToInternal (tmpMissing, *Nmissing);	\
    free (tmpMissing);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (missing); }

  // XXX the structure is here for future expansion, but no transformations are currently defined
  if (TRUE) {
    missing = gfits_table_get_Missing (ftable, Nmissing, NULL, &nativeBytes);
    if (!missing) {
      fprintf (stderr, "ERROR: failed to read missing\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (missing);
  }

  if (!strcmp (extname, "DVO_MISSING")) {
    missing = gfits_table_get_Missing (ftable, Nmissing, NULL, &nativeBytes);
    if (!missing) {
      fprintf (stderr, "ERROR: failed to read missing\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (missing);
  }

  SKIPPING_FORMAT ("DVO_MISSING_PS1_SIM",         PS1_SIM,         PS1_SIM);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_REF",         PS1_REF,         PS1_REF);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  SKIPPING_FORMAT ("DVO_MISSING_ELIXIR", 	  ELIXIR,  	   Elixir);
  SKIPPING_FORMAT ("DVO_MISSING_LONEOS", 	  LONEOS,          Loneos);
  SKIPPING_FORMAT ("DVO_MISSING_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  SKIPPING_FORMAT ("DVO_MISSING_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_V1",          PS1_V1,          PS1_V1);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_V2",          PS1_V2,          PS1_V2);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_V3",          PS1_V3,          PS1_V3);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_V4",          PS1_V4,          PS1_V4);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_V5",          PS1_V5,          PS1_V5);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_V6",          PS1_V6,          PS1_V6);
  SKIPPING_FORMAT ("DVO_MISSING_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
# undef CONVERT_FORMAT
# undef SKIPPING_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Nmissing = 0;
  return (NULL);
}

/*** SecFilt / FTable conversion functions ***/

SecFilt *FtableToSecFilt (FTable *ftable, Average *average, off_t *Nsecfilt, DVOCatFormat *format, char nativeBytes) {
  OHANA_UNUSED_PARAM(average);

  SecFilt *secfilt;
  char extname[80];

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for secfilt table\n");
    return (FALSE);
  }

  // block to convert old tables (PS1_V5 versions made during development)
  if (!strcmp (extname, "DVO_SECFILT_PS1_V5") && (ftable[0].header[0].Naxis[0] == 160)) {
    SecFilt_PS1_V5alt *tmpSecFilt;
    tmpSecFilt = gfits_table_get_SecFilt_PS1_V5alt (ftable, Nsecfilt, NULL);
    if (!tmpSecFilt) {
      fprintf (stderr, "ERROR: failed to read secfilts\n");
      exit (2);
    }
    secfilt = SecFilt_PS1_V5alt_ToInternal (tmpSecFilt, *Nsecfilt);
    free (tmpSecFilt);
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;
    return (secfilt); 
  }


# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    SecFilt_##TYPE *tmpSecFilt;						\
    tmpSecFilt = gfits_table_get_SecFilt_##TYPE (ftable, Nsecfilt, NULL, &nativeBytes); \
    if (!tmpSecFilt) {							\
      fprintf (stderr, "ERROR: failed to read secfilts\n");		\
      exit (2);								\
    }									\
    secfilt = SecFilt_##TYPE##_ToInternal (tmpSecFilt, *Nsecfilt);	\
    free (tmpSecFilt);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (secfilt); }

  if (!strcmp (extname, "DVO_SECFILT")) {
    secfilt = gfits_table_get_SecFilt (ftable, Nsecfilt, NULL, &nativeBytes);
    if (!secfilt) {
      fprintf (stderr, "ERROR: failed to read secfilts\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (secfilt);
  }

  CONVERT_FORMAT ("DVO_SECFILT_ELIXIR", 	 ELIXIR, 	  Elixir);
  CONVERT_FORMAT ("DVO_SECFILT_LONEOS", 	 LONEOS, 	  Loneos);
  CONVERT_FORMAT ("DVO_SECFILT_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  CONVERT_FORMAT ("DVO_SECFILT_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_V1",          PS1_V1,          PS1_V1);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_V2",          PS1_V2,          PS1_V2);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_V3",          PS1_V3,          PS1_V3);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_V4",          PS1_V4,          PS1_V4);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_V5",          PS1_V5,          PS1_V5);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_V6",          PS1_V6,          PS1_V6);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_REF",         PS1_REF,         PS1_REF);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  CONVERT_FORMAT ("DVO_SECFILT_PS1_SIM",         PS1_SIM,         PS1_SIM);
# undef CONVERT_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Nsecfilt = 0;
  return (NULL);
}

int SecFiltToFtable (FTable *ftable, SecFilt *secfilt, off_t Nsecfilt, DVOCatFormat format, int swapFromNative) {

# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    SecFilt_##TYPE *tmpSecFilt;						\
    tmpSecFilt = SecFiltInternalTo_##TYPE (secfilt, Nsecfilt);		\
    gfits_table_set_SecFilt_##TYPE (ftable, tmpSecFilt, Nsecfilt, swapFromNative); \
    free (tmpSecFilt);							\
    break; }

  /* convert from the internal format */
  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      gfits_table_set_SecFilt (ftable, secfilt, Nsecfilt, swapFromNative);
      break; }

      FORMAT_CASE (ELIXIR, 	    Elixir);
      FORMAT_CASE (LONEOS, 	    Loneos);
      FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
      FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
      FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
      FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
      FORMAT_CASE (PS1_V1,          PS1_V1);
      FORMAT_CASE (PS1_V2,          PS1_V2);
      FORMAT_CASE (PS1_V3,          PS1_V3);
      FORMAT_CASE (PS1_V4,          PS1_V4);
      FORMAT_CASE (PS1_V5,          PS1_V5);
      FORMAT_CASE (PS1_V6,          PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
      FORMAT_CASE (PS1_REF,         PS1_REF);
      FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
      FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
      FORMAT_CASE (PS1_SIM,         PS1_SIM);
# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (secfilt)\n");
      return (FALSE);
  }
  return (TRUE);
}

/*** Lensing / FTable conversion functions ***/

Lensing *FtableToLensing (FTable *ftable, Average *average, off_t *Nlensing, DVOCatFormat *format, char nativeBytes) {
  OHANA_UNUSED_PARAM(average);

  Lensing *lensing;
  char extname[80];

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for lensing table\n");
    return (FALSE);
  }

  if (!strcmp (extname, "DVO_LENSING_PS1_V5") && (ftable[0].header[0].Naxis[0] == 128)) {
    Lensing_PS1_V5_R0 *tmpLensing;						
    tmpLensing = gfits_table_get_Lensing_PS1_V5_R0 (ftable, Nlensing, NULL, &nativeBytes); 
    if (!tmpLensing) {							
      fprintf (stderr, "ERROR: failed to read lensings\n");		
      exit (2);								
    }									
    lensing = Lensing_PS1_V5_R0_ToInternal (tmpLensing, *Nlensing); 
    free (tmpLensing);							
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;					
    return (lensing); }

  if (!strcmp (extname, "DVO_LENSING_PS1_V5") && (ftable[0].header[0].Naxis[0] == 136)) {
    Lensing_PS1_V5_R1 *tmpLensing;						
    tmpLensing = gfits_table_get_Lensing_PS1_V5_R1 (ftable, Nlensing, NULL, &nativeBytes); 
    if (!tmpLensing) {							
      fprintf (stderr, "ERROR: failed to read lensings\n");		
      exit (2);								
    }									
    lensing = Lensing_PS1_V5_R1_ToInternal (tmpLensing, *Nlensing); 
    free (tmpLensing);							
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;					
    return (lensing); }

  if (!strcmp (extname, "DVO_LENSING_PS1_V5") && (ftable[0].header[0].Naxis[0] == 144)) {
    Lensing_PS1_V5_R2 *tmpLensing;						
    tmpLensing = gfits_table_get_Lensing_PS1_V5_R2 (ftable, Nlensing, NULL, &nativeBytes); 
    if (!tmpLensing) {							
      fprintf (stderr, "ERROR: failed to read lensings\n");		
      exit (2);								
    }									
    lensing = Lensing_PS1_V5_R2_ToInternal (tmpLensing, *Nlensing); 
    free (tmpLensing);							
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;					
    return (lensing); }

# define SKIPPING_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    fprintf (stderr, "ERROR: format %s not defined for lensing, skipping\n", NAME); \
    *Nlensing = 0;							\
    return NULL;							\
  }

# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    Lensing_##TYPE *tmpLensing;						\
    tmpLensing = gfits_table_get_Lensing_##TYPE (ftable, Nlensing, NULL, &nativeBytes); \
    if (!tmpLensing) {							\
      fprintf (stderr, "ERROR: failed to read lensings\n");		\
      exit (2);								\
    }									\
    lensing = Lensing_##TYPE##_ToInternal (tmpLensing, *Nlensing);	\
    free (tmpLensing);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (lensing); }

  if (!strcmp (extname, "DVO_LENSING")) {
    lensing = gfits_table_get_Lensing (ftable, Nlensing, NULL, &nativeBytes);
    if (!lensing) {
      fprintf (stderr, "ERROR: failed to read lensings\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (lensing);
  }

  SKIPPING_FORMAT ("DVO_LENSING_PS1_REF",         PS1_REF,         PS1_REF);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  SKIPPING_FORMAT ("DVO_LENSING_ELIXIR", 	  ELIXIR,  	   Elixir);
  SKIPPING_FORMAT ("DVO_LENSING_LONEOS", 	  LONEOS,          Loneos);
  SKIPPING_FORMAT ("DVO_LENSING_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  SKIPPING_FORMAT ("DVO_LENSING_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_V1",          PS1_V1,          PS1_V1);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_V2",          PS1_V2,          PS1_V2);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_V3",          PS1_V3,          PS1_V3);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_V4",          PS1_V4,          PS1_V4);
  CONVERT_FORMAT  ("DVO_LENSING_PS1_V5",          PS1_V5,          PS1_V5_R3);
  CONVERT_FORMAT  ("DVO_LENSING_PS1_V6",          PS1_V6,          PS1_V6);
  CONVERT_FORMAT  ("DVO_LENSING_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
  SKIPPING_FORMAT ("DVO_LENSING_PS1_SIM",         PS1_SIM,         PS1_SIM);
# undef CONVERT_FORMAT
# undef SKIPPING_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Nlensing = 0;
  return (NULL);
}

// LensingToFtable needs the Average since old formats stored lensing.dR,dD only
int LensingToFtable (FTable *ftable, Lensing *lensing, off_t Nlensing, DVOCatFormat format, int swapFromNative) {

# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    Lensing_##TYPE *tmpLensing;						\
    tmpLensing = LensingInternalTo_##TYPE (lensing, Nlensing);		\
    gfits_table_set_Lensing_##TYPE (ftable, tmpLensing, Nlensing, swapFromNative); \
    free (tmpLensing);							\
    break; }

  /* convert from the internal format */
  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      gfits_table_set_Lensing (ftable, lensing, Nlensing, swapFromNative);
      break; }

//    FORMAT_CASE (PS1_REF,         PS1_REF);
//    FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
//    FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
//    FORMAT_CASE (ELIXIR, 	    Elixir);
//    FORMAT_CASE (LONEOS, 	    Loneos);
//    FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
//    FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
//    FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
//    FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
//    FORMAT_CASE (PS1_V1,          PS1_V1);
//    FORMAT_CASE (PS1_V2,          PS1_V2);
//    FORMAT_CASE (PS1_V3,          PS1_V3);
//    FORMAT_CASE (PS1_V4,          PS1_V4);
      FORMAT_CASE (PS1_V5,          PS1_V5_R3);
      FORMAT_CASE (PS1_V6,          PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (lensing)\n");
      return (FALSE);
  }
  return (TRUE);
}

/*** Lensobj / FTable conversion functions ***/

Lensobj *FtableToLensobj (FTable *ftable, Average *average, off_t *Nlensobj, DVOCatFormat *format, char nativeBytes) {
  OHANA_UNUSED_PARAM(average);

  Lensobj *lensobj;
  char extname[80];

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for lensobj table\n");
    return (FALSE);
  }

  if (!strcmp (extname, "DVO_LENSOBJ_PS1_V5") && (ftable[0].header[0].Naxis[0] == 136)) {
    Lensobj_PS1_V5_R0 *tmpLensobj;						
    tmpLensobj = gfits_table_get_Lensobj_PS1_V5_R0 (ftable, Nlensobj, NULL, &nativeBytes); 
    if (!tmpLensobj) {							
      fprintf (stderr, "ERROR: failed to read lensobjs\n");		
      exit (2);								
    }									
    lensobj = Lensobj_PS1_V5_R0_ToInternal (tmpLensobj, *Nlensobj); 
    free (tmpLensobj);							
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;					
    return (lensobj); }

# define SKIPPING_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    fprintf (stderr, "ERROR: format %s not defined for lensobj, skipping\n", NAME); \
    *Nlensobj = 0;							\
    return NULL;							\
  }

# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    Lensobj_##TYPE *tmpLensobj;						\
    tmpLensobj = gfits_table_get_Lensobj_##TYPE (ftable, Nlensobj, NULL, &nativeBytes); \
    if (!tmpLensobj) {							\
      fprintf (stderr, "ERROR: failed to read lensobjs\n");		\
      exit (2);								\
    }									\
    lensobj = Lensobj_##TYPE##_ToInternal (tmpLensobj, *Nlensobj);	\
    free (tmpLensobj);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (lensobj); }

  if (!strcmp (extname, "DVO_LENSOBJ")) {
    lensobj = gfits_table_get_Lensobj (ftable, Nlensobj, NULL, &nativeBytes);
    if (!lensobj) {
      fprintf (stderr, "ERROR: failed to read lensobjs\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (lensobj);
  }

  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_REF",         PS1_REF,         PS1_REF);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  SKIPPING_FORMAT ("DVO_LENSOBJ_ELIXIR", 	  ELIXIR,  	   Elixir);
  SKIPPING_FORMAT ("DVO_LENSOBJ_LONEOS", 	  LONEOS,          Loneos);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_V1",          PS1_V1,          PS1_V1);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_V2",          PS1_V2,          PS1_V2);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_V3",          PS1_V3,          PS1_V3);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_V4",          PS1_V4,          PS1_V4);
  CONVERT_FORMAT  ("DVO_LENSOBJ_PS1_V5",          PS1_V5,          PS1_V5_R1);
  CONVERT_FORMAT  ("DVO_LENSOBJ_PS1_V6",          PS1_V6,          PS1_V6);
  CONVERT_FORMAT  ("DVO_LENSOBJ_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
  SKIPPING_FORMAT ("DVO_LENSOBJ_PS1_SIM",         PS1_SIM,         PS1_SIM);
# undef CONVERT_FORMAT
# undef SKIPPING_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Nlensobj = 0;
  return (NULL);
}

// LensobjToFtable needs the Average since old formats stored lensobj.dR,dD only
int LensobjToFtable (FTable *ftable, Lensobj *lensobj, off_t Nlensobj, DVOCatFormat format, int swapFromNative) {

# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    Lensobj_##TYPE *tmpLensobj;						\
    tmpLensobj = LensobjInternalTo_##TYPE (lensobj, Nlensobj);		\
    gfits_table_set_Lensobj_##TYPE (ftable, tmpLensobj, Nlensobj, swapFromNative); \
    free (tmpLensobj);							\
    break; }

  /* convert from the internal format */
  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      gfits_table_set_Lensobj (ftable, lensobj, Nlensobj, swapFromNative);
      break; }

//    FORMAT_CASE (PS1_REF,         PS1_REF);
//    FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
//    FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
//    FORMAT_CASE (ELIXIR, 	    Elixir);
//    FORMAT_CASE (LONEOS, 	    Loneos);
//    FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
//    FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
//    FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
//    FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
//    FORMAT_CASE (PS1_V1,          PS1_V1);
//    FORMAT_CASE (PS1_V2,          PS1_V2);
//    FORMAT_CASE (PS1_V3,          PS1_V3);
//    FORMAT_CASE (PS1_V4,          PS1_V4);
      FORMAT_CASE (PS1_V5,          PS1_V5_R1);
      FORMAT_CASE (PS1_V6,          PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (lensobj)\n");
      return (FALSE);
  }
  return (TRUE);
}

/*** StarPar / FTable conversion functions ***/

StarPar *FtableToStarPar (FTable *ftable, Average *average, off_t *Nstarpar, DVOCatFormat *format, char nativeBytes) {
  OHANA_UNUSED_PARAM(average);

  StarPar *starpar;
  char extname[80];

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for starpar table\n");
    return (FALSE);
  }

# define SKIPPING_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    fprintf (stderr, "ERROR: format %s not defined for starpar, skipping\n", NAME); \
    *Nstarpar = 0;							\
    return NULL;							\
  }
  
# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    StarPar_##TYPE *tmpStarPar;						\
    tmpStarPar = gfits_table_get_StarPar_##TYPE (ftable, Nstarpar, NULL, &nativeBytes); \
    if (!tmpStarPar) {							\
      fprintf (stderr, "ERROR: failed to read starpar\n");		\
      exit (2);								\
    }									\
    starpar = StarPar_##TYPE##_ToInternal (tmpStarPar, *Nstarpar);	\
    free (tmpStarPar);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (starpar); }

  if (!strcmp (extname, "DVO_STARPAR")) {
    starpar = gfits_table_get_StarPar (ftable, Nstarpar, NULL, &nativeBytes);
    if (!starpar) {
      fprintf (stderr, "ERROR: failed to read starpar\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (starpar);
  }

  CONVERT_FORMAT  ("DVO_STARPAR_PS1_SIM",         PS1_SIM,         PS1_SIM);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_REF",         PS1_REF,         PS1_REF);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  SKIPPING_FORMAT ("DVO_STARPAR_ELIXIR", 	  ELIXIR,  	   Elixir);
  SKIPPING_FORMAT ("DVO_STARPAR_LONEOS", 	  LONEOS,          Loneos);
  SKIPPING_FORMAT ("DVO_STARPAR_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  SKIPPING_FORMAT ("DVO_STARPAR_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_V1",          PS1_V1,          PS1_V1);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_V2",          PS1_V2,          PS1_V2);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_V3",          PS1_V3,          PS1_V3);
  SKIPPING_FORMAT ("DVO_STARPAR_PS1_V4",          PS1_V4,          PS1_V4);
  CONVERT_FORMAT  ("DVO_STARPAR_PS1_V5",          PS1_V5,          PS1_V5);
  CONVERT_FORMAT  ("DVO_STARPAR_PS1_V6",          PS1_V6,          PS1_V6);
  CONVERT_FORMAT  ("DVO_STARPAR_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
# undef CONVERT_FORMAT
# undef SKIPPING_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Nstarpar = 0;
  return (NULL);
}

// StarParToFtable needs the Average since old formats stored starpar.dR,dD only
int StarParToFtable (FTable *ftable, StarPar *starpar, off_t Nstarpar, DVOCatFormat format, int swapFromNative) {

# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    StarPar_##TYPE *tmpStarPar;						\
    tmpStarPar = StarParInternalTo_##TYPE (starpar, Nstarpar);		\
    gfits_table_set_StarPar_##TYPE (ftable, tmpStarPar, Nstarpar, swapFromNative); \
    free (tmpStarPar);							\
    break; }

  /* convert from the internal format */
  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      gfits_table_set_StarPar (ftable, starpar, Nstarpar, swapFromNative);
      break; }

      FORMAT_CASE (PS1_SIM,         PS1_SIM);
//    FORMAT_CASE (PS1_REF,         PS1_REF);
//    FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
//    FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
//    FORMAT_CASE (ELIXIR, 	    Elixir);
//    FORMAT_CASE (LONEOS, 	    Loneos);
//    FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
//    FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
//    FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
//    FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
//    FORMAT_CASE (PS1_V1,          PS1_V1);
//    FORMAT_CASE (PS1_V2,          PS1_V2);
//    FORMAT_CASE (PS1_V3,          PS1_V3);
//    FORMAT_CASE (PS1_V4,          PS1_V4);
      FORMAT_CASE (PS1_V5,          PS1_V5);
      FORMAT_CASE (PS1_V6,          PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (starpar)\n");
      return (FALSE);
  }
  return (TRUE);
}

/*** GalPhot / FTable conversion functions ***/

GalPhot *FtableToGalPhot (FTable *ftable, Average *average, off_t *Ngalphot, DVOCatFormat *format, char nativeBytes) {
  OHANA_UNUSED_PARAM(average);

  GalPhot *galphot;
  char extname[80];

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for galphot table\n");
    return (FALSE);
  }

  if (!strcmp (extname, "DVO_GALPHOT_PS1_V5") && (ftable[0].header[0].Naxis[0] == 72)) {
    GalPhot_PS1_V5_R0 *tmpGalPhot;						
    tmpGalPhot = gfits_table_get_GalPhot_PS1_V5_R0 (ftable, Ngalphot, NULL, &nativeBytes); 
    if (!tmpGalPhot) {							
      fprintf (stderr, "ERROR: failed to read galphots\n");		
      exit (2);								
    }									
    galphot = GalPhot_PS1_V5_R0_ToInternal (tmpGalPhot, *Ngalphot); 
    free (tmpGalPhot);							
    ftable[0].buffer = NULL;
    *format = DVO_FORMAT_PS1_V5;					
    return (galphot); 
  }

# define SKIPPING_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    fprintf (stderr, "ERROR: format %s not defined for galphot, skipping\n", NAME); \
    *Ngalphot = 0;							\
    return NULL;							\
  }

# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    GalPhot_##TYPE *tmpGalPhot;						\
    tmpGalPhot = gfits_table_get_GalPhot_##TYPE (ftable, Ngalphot, NULL, &nativeBytes); \
    if (!tmpGalPhot) {							\
      fprintf (stderr, "ERROR: failed to read galphots\n");		\
      exit (2);								\
    }									\
    galphot = GalPhot_##TYPE##_ToInternal (tmpGalPhot, *Ngalphot);	\
    free (tmpGalPhot);							\
    ftable[0].buffer = NULL;						\
    *format = DVO_FORMAT_##FORMAT;					\
    return (galphot); }

  if (!strcmp (extname, "DVO_GALPHOT")) {
    galphot = gfits_table_get_GalPhot (ftable, Ngalphot, NULL, &nativeBytes);
    if (!galphot) {
      fprintf (stderr, "ERROR: failed to read galphots\n");
      exit (2);
    }
    *format = DVO_FORMAT_INTERNAL;
    return (galphot);
  }

  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_REF",         PS1_REF,         PS1_REF);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  SKIPPING_FORMAT ("DVO_GALPHOT_ELIXIR", 	  ELIXIR,  	   Elixir);
  SKIPPING_FORMAT ("DVO_GALPHOT_LONEOS", 	  LONEOS,          Loneos);
  SKIPPING_FORMAT ("DVO_GALPHOT_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  SKIPPING_FORMAT ("DVO_GALPHOT_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_V1",          PS1_V1,          PS1_V1);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_V2",          PS1_V2,          PS1_V2);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_V3",          PS1_V3,          PS1_V3);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_V4",          PS1_V4,          PS1_V4);
  CONVERT_FORMAT  ("DVO_GALPHOT_PS1_V5",          PS1_V5,          PS1_V5_R1);
  CONVERT_FORMAT  ("DVO_GALPHOT_PS1_V6",          PS1_V6,          PS1_V6);
  CONVERT_FORMAT  ("DVO_GALPHOT_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
  SKIPPING_FORMAT ("DVO_GALPHOT_PS1_SIM",         PS1_SIM,         PS1_SIM);
# undef CONVERT_FORMAT
# undef SKIPPING_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);

  *Ngalphot = 0;
  return (NULL);
}

// GalPhotToFtable needs the Average since old formats stored galphot.dR,dD only
int GalPhotToFtable (FTable *ftable, GalPhot *galphot, off_t Ngalphot, DVOCatFormat format, int swapFromNative) {

# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    GalPhot_##TYPE *tmpGalPhot;						\
    tmpGalPhot = GalPhotInternalTo_##TYPE (galphot, Ngalphot);		\
    gfits_table_set_GalPhot_##TYPE (ftable, tmpGalPhot, Ngalphot, swapFromNative); \
    free (tmpGalPhot);							\
    break; }

  /* convert from the internal format */
  switch (format) {
    case DVO_FORMAT_INTERNAL: {
      gfits_table_set_GalPhot (ftable, galphot, Ngalphot, swapFromNative);
      break; }

//    FORMAT_CASE (PS1_REF,         PS1_REF);
//    FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
//    FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
//    FORMAT_CASE (ELIXIR, 	    Elixir);
//    FORMAT_CASE (LONEOS, 	    Loneos);
//    FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
//    FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
//    FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
//    FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
//    FORMAT_CASE (PS1_V1,          PS1_V1);
//    FORMAT_CASE (PS1_V2,          PS1_V2);
//    FORMAT_CASE (PS1_V3,          PS1_V3);
//    FORMAT_CASE (PS1_V4,          PS1_V4);
      FORMAT_CASE (PS1_V5,          PS1_V5_R1);
      FORMAT_CASE (PS1_V6,          PS1_V6);
      FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (galphot)\n");
      return (FALSE);
  }
  return (TRUE);
}

/*** Image Conversions ***/

// I have loaded the disk db table and now I want to convert to the internal format
// (Image structure), but I onyl 
int FtableToImage (FTable *ftable, Header *theader, DVOCatFormat *format) {

  off_t Nimage;
  char extname[80];

  /* extname may be set from outside if the source is RAW not MEF */
  if (*format == DVO_FORMAT_ELIXIR) {		  // special case for ELIXIR
    Image_Elixir *tmpimage;
    tmpimage = gfits_table_get_Image_Elixir (ftable, &Nimage, NULL, NULL);
    if (!tmpimage) {
      fprintf (stderr, "ERROR: failed to read images\n");
      exit (2);
    }
    off_t Nalloc = gfits_data_pad_size(Nimage*sizeof(Image));
    ftable[0].buffer = (char *) Image_Elixir_ToInternal (tmpimage, Nimage, Nalloc);
    free (tmpimage);
    gfits_free_header (theader);
    gfits_table_mkheader_Image (theader);
    gfits_modify (theader, "NAXIS2", OFF_T_FMT, 1,  Nimage);
    theader[0].Naxis[1] = Nimage;
    ftable[0].datasize = gfits_data_size (theader);
    return (TRUE);
  }

  /* convert to the internal format */
  if (!gfits_scan (ftable[0].header, "EXTNAME", "%s", 1, extname)) {
    fprintf (stderr, "EXTNAME missing for image table\n");
    return (FALSE);
  }

# define CONVERT_FORMAT(NAME, FORMAT, TYPE)				\
  if (!strcmp (extname, NAME)) {					\
    Image_##TYPE *tmpimage;						\
    *format = DVO_FORMAT_##FORMAT;					\
    tmpimage = gfits_table_get_Image_##TYPE (ftable, &Nimage, NULL, NULL); \
    if (!tmpimage) {							\
      fprintf (stderr, "ERROR: failed to read images\n");		\
      exit (2);								\
    }									\
    off_t Nalloc = gfits_data_pad_size(Nimage*sizeof(Image));		\
    ftable[0].buffer = (char *) Image_##TYPE##_ToInternal (tmpimage, Nimage, Nalloc); \
    free (tmpimage);							\
    gfits_free_header (theader);					\
    gfits_table_mkheader_Image (theader);				\
    gfits_modify (theader, "NAXIS2", OFF_T_FMT, 1,  Nimage);		\
    theader[0].Naxis[1] = Nimage;					\
    ftable[0].datasize = gfits_data_size (theader);			\
    return (TRUE); }

  CONVERT_FORMAT ("DVO_IMAGE_ELIXIR", 	       ELIXIR,          Elixir);
  CONVERT_FORMAT ("DVO_IMAGE_LONEOS", 	       LONEOS,          Loneos);
  CONVERT_FORMAT ("DVO_IMAGE_PANSTARRS_DEV_0", PANSTARRS_DEV_0, Panstarrs_DEV_0);
  CONVERT_FORMAT ("DVO_IMAGE_PANSTARRS_DEV_1", PANSTARRS_DEV_1, Panstarrs_DEV_1);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_DEV_1",       PS1_DEV_1,       PS1_DEV_1);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_DEV_2",       PS1_DEV_2,       PS1_DEV_2);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_V1",          PS1_V1,          PS1_V1);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_V2",          PS1_V2,          PS1_V2);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_V3",          PS1_V3,          PS1_V3);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_V4",          PS1_V4,          PS1_V4);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_V5",          PS1_V5,          PS1_V5);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_V6",          PS1_V6,          PS1_V6);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_V5_LOAD",     PS1_V5_LOAD,     PS1_V5_LOAD);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_REF",         PS1_REF,         PS1_REF);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_REF_V2",      PS1_REF_V2,      PS1_REF_V2);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_REF_V3",      PS1_REF_V3,      PS1_REF_V3);
  CONVERT_FORMAT ("DVO_IMAGE_PS1_SIM",         PS1_SIM,         PS1_SIM);

# undef CONVERT_FORMAT

  fprintf (stderr, "table format unknown: %s\n", extname);
  return (FALSE);
}

int ImageToFtable (FTable *ftable, Header *theader, DVOCatFormat format) {

  off_t Nimage;

  Nimage = theader[0].Naxis[1];

# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    Image_##TYPE *tmpImage;						\
    tmpImage = ImageInternalTo_##TYPE ((Image *) ftable[0].buffer, Nimage); \
    free (ftable[0].buffer);						\
    ftable[0].buffer = NULL;						\
    gfits_free_header (ftable->header);					\
    gfits_table_set_Image_##TYPE (ftable, tmpImage, Nimage, TRUE);	\
    free (tmpImage);							\
    break; }

  /* convert from the internal format */
  switch (format) {
    FORMAT_CASE (ELIXIR, 	    Elixir);
    FORMAT_CASE (LONEOS, 	    Loneos);
    FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
    FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
    FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
    FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
    FORMAT_CASE (PS1_V1,          PS1_V1);
    FORMAT_CASE (PS1_V2,          PS1_V2);
    FORMAT_CASE (PS1_V3,          PS1_V3);
    FORMAT_CASE (PS1_V4,          PS1_V4);
    FORMAT_CASE (PS1_V5,          PS1_V5);
    FORMAT_CASE (PS1_V6,          PS1_V6);
    FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
    FORMAT_CASE (PS1_REF,         PS1_REF);
    FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
    FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
    FORMAT_CASE (PS1_SIM,         PS1_SIM);

# undef FORMAT_CASE

    default:
      fprintf (stderr, "table format unknown (image ftable)\n");
      return (FALSE);
  }
  return (TRUE);
}

int ImageToVtable (VTable *vtable, Header *theader, DVOCatFormat format) {

  off_t i, Nrow, Nimage;

  Nrow = vtable[0].Nrow;

# define FORMAT_CASE(FORMAT, TYPE)					\
  case DVO_FORMAT_##FORMAT: {						\
    Image_##TYPE *tmpImage;						\
    /* convert table rows from internal to external format */		\
    for (i = 0; i < Nrow; i++) {					\
      tmpImage = ImageInternalTo_##TYPE ((Image *) vtable[0].buffer[i], 1); \
      gfits_convert_Image_##TYPE (tmpImage, sizeof(Image_##TYPE), 1);	\
      free (vtable[0].buffer[i]);					\
      vtable[0].buffer[i] = (char *) tmpImage;				\
    }									\
    /* convert header from old format to new format */			\
    gfits_scan (theader, "NAXIS2", OFF_T_FMT, 1,  &Nimage);		\
    gfits_free_header (theader);					\
    gfits_table_mkheader_Image_##TYPE (theader);			\
    gfits_modify (theader, "NAXIS2", OFF_T_FMT, 1,  Nimage);		\
    theader[0].Naxis[1] = Nimage;					\
    vtable[0].datasize = gfits_data_size (theader);			\
    return (TRUE); }


  /* convert from the internal format */
  switch (format) {
    FORMAT_CASE (ELIXIR, 	    Elixir);
    FORMAT_CASE (LONEOS, 	    Loneos);
    FORMAT_CASE (PANSTARRS_DEV_0, Panstarrs_DEV_0);
    FORMAT_CASE (PANSTARRS_DEV_1, Panstarrs_DEV_1);
    FORMAT_CASE (PS1_DEV_1,       PS1_DEV_1);
    FORMAT_CASE (PS1_DEV_2,       PS1_DEV_2);
    FORMAT_CASE (PS1_V1,          PS1_V1);
    FORMAT_CASE (PS1_V2,          PS1_V2);
    FORMAT_CASE (PS1_V3,          PS1_V3);
    FORMAT_CASE (PS1_V4,          PS1_V4);
    FORMAT_CASE (PS1_V5,          PS1_V5);
    FORMAT_CASE (PS1_V6,          PS1_V6);
    FORMAT_CASE (PS1_V5_LOAD,     PS1_V5_LOAD);
    FORMAT_CASE (PS1_REF,         PS1_REF);
    FORMAT_CASE (PS1_REF_V2,      PS1_REF_V2);
    FORMAT_CASE (PS1_REF_V3,      PS1_REF_V3);
    FORMAT_CASE (PS1_SIM,         PS1_SIM);

# undef FORMAT_CASE

    default:
      break;
  }
  fprintf (stderr, "table format unknown (image vtable)\n");
  return (FALSE);
}

