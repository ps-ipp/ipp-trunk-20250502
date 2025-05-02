# include "imregister.h"
# include "detrend.h"

char *RandomPath (char *dBPath);
extern double drand48();

int OutputSubset (DetReg *image, off_t Nimage, Match *match, off_t Nmatch) {

  if (output.table != (char *) NULL) {
    DumpFitsTable (output.table, image, match, Nmatch);
  } 
  if (output.bintable != (char *) NULL) {
    DumpFitsBintable (output.bintable, image, match, Nmatch);
  } 
  PrintSubset (image, match, Nmatch);
  if (output.verbose) fprintf (stderr, "SUCCESS\n");
  exit (0);
}

/* write out complete binary FITS table in format of db */
int DumpFitsBintable (char *filename, DetReg *image, Match *match, off_t Nmatch) {

  off_t i, j;
  FILE *f;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  DetReg *subset;

  ALLOCATE (subset, DetReg, MAX (1, Nmatch));
  for (i = 0; i < Nmatch; i++){
    j = match[i].image;
    memcpy (&subset[i], &image[j], sizeof (DetReg));
  }

  /* open file for output */
  f = fopen (filename, "w");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't open output file %s\n", filename);
    exit (1);
  }

  /* create primary header */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);

  ftable.header = &theader;
  gfits_table_set_DetReg (&ftable, subset, Nmatch, TRUE);

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &ftable);
  fclose (f);
  exit (0);
}

int DumpFitsTable (char *filename, DetReg *detdata, Match *match, off_t Nmatch) {
  
  Header header, theader;
  Matrix matrix;
  FTable table;
  DetReg *newdata;
  FILE *f;
  char *startstr, *stopstr, *regstr, *line, key[33], ccdinfo[16];
  char *filtstr, *typestr, *modestr, *ccdstr, *datestr, *p;
  off_t i;
  time_t tsecond;

  /* create primary header */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);
  
  /* create table header */
  gfits_create_table_header (&theader, "TABLE", "MASTER_DETREND");
      
  /* add current date/time to header */
  ohana_str_to_time ("now", &tsecond);
  datestr = ohana_sec_to_date (tsecond);
  gfits_modify (&header,  "DATE", "%s", 1, datestr);
  gfits_modify (&theader, "DATE", "%s", 1, datestr);
  free (datestr);

  /* define table layout */
  gfits_define_table_column (&theader, "A32",  "KEY",        "unique identifier",         "");
  gfits_define_table_column (&theader, "A20",  "START_TIME", "start time of measurement", "yyyy/mm/dd,hh:mm:ss");
  gfits_define_table_column (&theader, "A20",  "STOP_TIME",  "stop time of measurement",  "yyyy/mm/dd,hh:mm:ss");
  gfits_define_table_column (&theader, "A20",  "REG_TIME",   "time of registration",      "yyyy/mm/dd,hh:mm:ss");
  gfits_define_table_column (&theader, "F7.1", "EXPTIME",    "exposure time",             "seconds"); 
  gfits_define_table_column (&theader, "A10",  "IMAGETYP",   "detrend type",              "");
  gfits_define_table_column (&theader, "A10",  "FILTER",     "filter name",               "");
  gfits_define_table_column (&theader, "A7",   "CCDINFO",    "ccd information",                  "");
  gfits_define_table_column (&theader, "A7",   "MODE",       "data format mode",                  "");
  gfits_define_table_column (&theader, "I3",   "VERSION",    "image version number",      "");
  gfits_define_table_column (&theader, "I3",   "ORDER",      "selection order",           "");
  gfits_define_table_column (&theader, "A64",  "LABEL",      "data label",                "");
  gfits_define_table_column (&theader, "A256", "PATH",       "filename in db",            "");
  
  /* define TNULL, TNVAL values */
  gfits_modify (&theader, "TNULL1",  "%s", 1, "NULL");  /* KEY        */
  gfits_modify (&theader, "TNULL2",  "%s", 1, "NULL");  /* START_TIME */
  gfits_modify (&theader, "TNULL3",  "%s", 1, "NULL");  /* STOP_TIME  */
  gfits_modify (&theader, "TNULL4",  "%s", 1, "NULL");  /* REG_TIME   */
  gfits_modify (&theader, "TNULL5",  "%s", 1, "NaN");   /* EXPTIME    */
  gfits_modify (&theader, "TNULL6",  "%s", 1, "NULL");  /* IMAGETYP   */
  gfits_modify (&theader, "TNULL7",  "%s", 1, "NULL");  /* FILTER     */
  gfits_modify (&theader, "TNULL8",  "%s", 1, "NULL");  /* CCDINFO    */
  gfits_modify (&theader, "TNULL9",  "%s", 1, "NULL");  /* MODE       */
  gfits_modify (&theader, "TNULL10", "%s", 1, "-100");  /* VERSION    */
  gfits_modify (&theader, "TNULL11", "%s", 1, "-100");  /* ORDER      */
  gfits_modify (&theader, "TNULL12", "%s", 1, "NULL");  /* LABEL      */
  gfits_modify (&theader, "TNULL13", "%s", 1, "NULL");  /* PATH       */

  gfits_modify (&theader, "TNVAL1",  "%s", 1, "NA");    /* KEY        */
  gfits_modify (&theader, "TNVAL2",  "%s", 1, "NA");    /* START_TIME */
  gfits_modify (&theader, "TNVAL3",  "%s", 1, "NA");    /* STOP_TIME  */
  gfits_modify (&theader, "TNVAL4",  "%s", 1, "NA");    /* REG_TIME   */
  gfits_modify (&theader, "TNVAL5",  "%s", 1, "Inf");   /* EXPTIME    */
  gfits_modify (&theader, "TNVAL6",  "%s", 1, "NA");    /* IMAGETYP   */
  gfits_modify (&theader, "TNVAL7",  "%s", 1, "NA");    /* FILTER     */
  gfits_modify (&theader, "TNVAL8",  "%s", 1, "NA");    /* CCDINFO    */
  gfits_modify (&theader, "TNVAL9",  "%s", 1, "NA");    /* MODE       */
  gfits_modify (&theader, "TNVAL10", "%s", 1, "-200");  /* VERSION    */
  gfits_modify (&theader, "TNVAL11", "%s", 1, "-200");  /* ORDER      */
  gfits_modify (&theader, "TNVAL12", "%s", 1, "NA");    /* LABEL      */
  gfits_modify (&theader, "TNVAL13", "%s", 1, "NA");    /* PATH       */

  /* create table, add data values */
  gfits_create_table (&theader, &table);
  
  /* add data to table */
  for (i = 0; i < Nmatch; i++) {
    newdata = &detdata[match[i].image];

    /* key = 02Bk02.flat.V.00.00 */
    p = strrchr (newdata[0].filename, '/');
    if (p == (char *) NULL) {
      p = newdata[0].filename;
    } else {
      p ++;
    }
    bzero (key, 33);
    strncpy_nowarn (key, p, 32);
    if ((p = strrchr (key, '.')) != (char *) NULL) *p = 0;

    startstr = ohana_sec_to_date (newdata[0].tstart);
    stopstr  = ohana_sec_to_date (newdata[0].tstop);
    regstr   = ohana_sec_to_date (newdata[0].treg);
    typestr  = get_type_name(newdata[0].type);
    modestr  = get_mode_name(newdata[0].mode);
    filtstr  = filterhash[newdata[0].filter];

    if (newdata[0].mode == M_SPLIT) {
      ccdstr   = ccds[newdata[0].ccd];
    } else {
      sprintf (ccdinfo, "%-3d", newdata[0].ccd);
      ccdstr   = ccdinfo;
    }

    line = gfits_table_print (&table, key, startstr, stopstr, regstr, 
			     newdata[0].exptime, typestr, filtstr, ccdstr, modestr,
			     newdata[0].Nentry, newdata[0].Norder, 
			     newdata[0].label, newdata[0].filename);

    gfits_add_rows (&table, line, 1, strlen(line));
    free (line);
    free (startstr);
    free (stopstr);
    free (regstr);
  }

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "Failure writing fits table\n");
    return (FALSE);
  }
  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &table);
  fclose (f);
  exit (0);
}

/* Select, TimeMode are global */
int PrintSubset (DetReg *detdata, Match *match, off_t Nmatch) {
  
  char *dBPath, *Path, *typestr, *filtstr, filename[512];
  char *timestr, *modestr, *ccdstr, ccdinfo[16], ccdformat[16];
  off_t i, j;
  int Nc;
  struct timeval now;
  long A;

  gettimeofday (&now, NULL);
  A = now.tv_usec + now.tv_sec;
  srand48 (A);

  Nc = strlen (ccds[0]);
  sprintf (ccdformat, "%%%dd", Nc);
  dBPath = get_dBPath ();

  /* list matched images */
  for (j = 0; j < Nmatch; j++) {
    i = match[j].image;
    switch (output.TimeMode) {
    case START:
      timestr = ohana_sec_to_date (detdata[i].tstart);
      break;
    case STOP:
      timestr = ohana_sec_to_date (detdata[i].tstop);
      break;
    case REG:
      timestr = ohana_sec_to_date (detdata[i].treg);
      break;
    default:
      if (output.verbose) fprintf (stderr, "ERROR: bad TimeMode\n");
      exit (1);
    }

    typestr = get_type_name(detdata[i].type);
    modestr = get_mode_name(detdata[i].mode);
    filtstr = filterhash[MIN (MAX (detdata[i].filter, 0), NFILTER - 1)];

    if ((detdata[i].mode == M_SPLIT) && (detdata[i].ccd >= -1) && (detdata[i].ccd < Nccd)) {
      ccdstr   = ccds[detdata[i].ccd];
    } else {
      sprintf (ccdinfo, ccdformat, detdata[i].ccd);
      ccdstr   = ccdinfo;
    }

    /* output mode (Select vs List) */
    if (output.Chipname && criteria[0].CCDSelect && detdata[i].mode == M_MEF) {
      snprintf (filename, 512, "%s[%s]", detdata[i].filename, ccds[criteria[0].CCD]);
    } else {
      strcpy (filename, detdata[i].filename);
    }

    if (output.Select) {
      if (detdata[i].altpath) {
	Path = RandomPath (dBPath);
      } else {
	Path = dBPath;
      }
      fprintf (stdout, "%s/%s\n", Path, filename);
    } else {
      fprintf (stdout, "%-40s = %19s %7s %6s %6s %s %2d %2d %6.1f  %20s  %1d\n", 
	       filename, timestr, modestr, typestr, filtstr, ccdstr,
	       detdata[i].Nentry, detdata[i].Norder, detdata[i].exptime, detdata[i].label, detdata[i].altpath);
    }
    free (timestr);
  }  
  return (TRUE);
}

char *RandomPath (char *dBPath) {
  
  int N;

  N = (NDetrendAltDB + 0.99)*drand48();
  if (N == NDetrendAltDB) return (dBPath);
  return (DetrendAltDB[N]);
}

int PrintCriteria () {

  int i;
  char *start, *stop;

  for (i = 0; i < Ncriteria; i++) {
    start = ohana_sec_to_date (criteria[i].tstart);
    stop  = ohana_sec_to_date (criteria[i].tstop);
    
    fprintf (stdout, "%19s %19s %7s %6s %s %6.1f\n", 
	     start, stop, get_type_name(criteria[i].Type), filterhash[criteria[i].Filter], 
	     ccds[criteria[i].CCD], criteria[i].Exptime);

    free (start);
    free (stop);
  }

  exit (0);
}
