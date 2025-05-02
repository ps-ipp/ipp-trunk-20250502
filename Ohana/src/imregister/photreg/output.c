# include "imregister.h"
# include "photreg.h"

static char PhotError[] = "unknown";
static char PhotNA[] = "unknown";

/* given a subset list, write out the selected images, if desired */
void OutputSubset (PhotPars *photdata, off_t Nphotdata, off_t *match, off_t Nmatch) {

  if (output.table != (char *) NULL) {
    DumpFitsTable (output.table, photdata, match, Nmatch);
  } 

  if (output.bintable != (char *) NULL) {
    DumpFitsBintable (output.bintable, photdata, match, Nmatch);
  } 

  PrintSubset (photdata, match, Nmatch);
  if (output.verbose) fprintf (stderr, "SUCCESS\n");

  exit (0);
}

/* write out complete binary FITS table in format of db */
void DumpFitsBintable (char *filename, PhotPars *photdata, off_t *match, off_t Nmatch) {

  off_t i, j;
  FILE *f;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  PhotPars *subset;

  /* extract subset of input data */
  ALLOCATE (subset, PhotPars, MAX (1, Nmatch));
  for (i = 0; i < Nmatch; i++){
    j = match[i];
    memcpy (&subset[i], &photdata[j], sizeof (PhotPars));
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
  gfits_table_set_PhotPars (&ftable, subset, Nmatch, TRUE);

  /* EXTNAME is set to ZERO_POINTS_3.0 by default */
  if (!strcmp (output.db, "trans")) {
    gfits_modify (&theader, "EXTNAME", "%s", 1, "TRANS_POINTS_3.0");
  }

  gfits_write_header  (filename, &header);
  gfits_write_matrix  (filename, &matrix);
  gfits_write_Theader (filename, &theader);
  gfits_write_table   (filename, &ftable);
  fclose (f);
  exit (0);
}

/** add ASCII table to autocode? **/
void DumpFitsTable (char *filename, PhotPars *photdata, off_t *index, off_t Nkeep) {
  
  Header header, theader;
  Matrix matrix;
  FTable table;
  PhotPars *newdata;
  FILE *f;
  char *startstr, *stopstr, *datestr, *line;
  char *c1, *c2, *code, *photsys, *extname;
  off_t i;
  time_t tsecond;

  /* create primary header */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);
  
  /* create table header */
  if (!strcmp (output.db, "phot")) {
    extname = strcreate ("IMAGE_ZPTS");
  } else {
    extname = strcreate ("SUMMARY_ZPTS");
  }

  gfits_create_table_header (&theader, "TABLE", extname);
    
  /* add current date/time to header */
  ohana_str_to_time ("now", &tsecond);
  datestr = ohana_sec_to_date ((unsigned int) tsecond);
  gfits_modify (&header,  "DATE", "%s", 1, datestr);
  gfits_modify (&theader, "DATE", "%s", 1, datestr);
  free (datestr);
     
  /* define table layout */
  gfits_define_table_column (&theader, "F8.4", "ZP_OBS",       "measured zero point",       "mag");
  gfits_define_table_column (&theader, "F8.4", "ZP_REF",       "nominal zero point",        "mag");
  gfits_define_table_column (&theader, "F7.4", "ZP_ERR",       "error on zero point",       "mag");
  gfits_define_table_column (&theader, "F7.3", "C_AIRMASS",    "airmass coeff",             "mag per airmass"); 
  gfits_define_table_column (&theader, "F6.3", "C_COLOR",      "color coeff",               "mag per mag");
  gfits_define_table_column (&theader, "A20",  "START_TIME",   "start time of measurement", "yyyy/mm/dd,hh:mm:ss");
  gfits_define_table_column (&theader, "A20",  "STOP_TIME",    "stop time of measurement",  "yyyy/mm/dd,hh:mm:ss");
  gfits_define_table_column (&theader, "A12",  "C1_NAME",      "filter 1 for color",        "");
  gfits_define_table_column (&theader, "A12",  "C2_NAME",      "filter 2 for color",        "");
  gfits_define_table_column (&theader, "I6",   "NSTARS",       "number of stars used",      "");
  gfits_define_table_column (&theader, "I6",   "NTIMES",       "number of unique images",   "");
  gfits_define_table_column (&theader, "A12",  "INT_PHOT_SYS", "internal photom system",    "");
  gfits_define_table_column (&theader, "A12",  "REF_PHOT_SYS", "reference photom system",   "");
  gfits_define_table_column (&theader, "A70",  "LABEL",        "data label",                "");
  
  /* define TNULL, TNVAL values */
  gfits_modify (&theader, "TNULL1",  "%s", 1, "NaN");   /* ZP_OBS     */
  gfits_modify (&theader, "TNULL2",  "%s", 1, "NaN");   /* ZP_REF     */
  gfits_modify (&theader, "TNULL3",  "%s", 1, "NaN");   /* ZP_ERR     */
  gfits_modify (&theader, "TNULL4",  "%s", 1, "NaN");   /* C_AIRMASS  */
  gfits_modify (&theader, "TNULL5",  "%s", 1, "NaN");   /* C_COLOR    */
  gfits_modify (&theader, "TNULL6",  "%s", 1, "NULL");  /* START_TIME */
  gfits_modify (&theader, "TNULL7",  "%s", 1, "NULL");  /* STOP_TIME  */
  gfits_modify (&theader, "TNULL8",  "%s", 1, "NULL");  /* C1_NAME    */
  gfits_modify (&theader, "TNULL9",  "%s", 1, "NULL");  /* C2_NAME    */
  gfits_modify (&theader, "TNULL10", "%s", 1, "NULL");  /* NSTARS     */
  gfits_modify (&theader, "TNULL11", "%s", 1, "NULL");  /* NTIMES     */
  gfits_modify (&theader, "TNULL12", "%s", 1, "NULL");  /* INT_PHOT_SYS */
  gfits_modify (&theader, "TNULL13", "%s", 1, "NULL");  /* REF_PHOT_SYS */
  gfits_modify (&theader, "TNULL14", "%s", 1, "NULL");  /* LABEL      */

  gfits_modify (&theader, "TNVAL1",  "%s", 1, "Inf");   /* ZP_OBS       */
  gfits_modify (&theader, "TNVAL2",  "%s", 1, "Inf");   /* ZP_REF       */
  gfits_modify (&theader, "TNVAL3",  "%s", 1, "Inf");   /* ZP_ERR       */
  gfits_modify (&theader, "TNVAL4",  "%s", 1, "Inf");   /* C_AIRMASS    */
  gfits_modify (&theader, "TNVAL5",  "%s", 1, "Inf");   /* C_COLOR      */
  gfits_modify (&theader, "TNVAL6",  "%s", 1, "NA");    /* START_TIME   */
  gfits_modify (&theader, "TNVAL7",  "%s", 1, "NA");    /* STOP_TIME    */
  gfits_modify (&theader, "TNVAL8",  "%s", 1, "NA");    /* C1_NAME      */
  gfits_modify (&theader, "TNVAL9",  "%s", 1, "NA");    /* C2_NAME      */
  gfits_modify (&theader, "TNVAL10", "%s", 1, "NA");    /* NSTARS       */
  gfits_modify (&theader, "TNVAL11", "%s", 1, "NA");    /* NTIMES       */
  gfits_modify (&theader, "TNVAL12", "%s", 1, "NA");    /* INT_PHOT_SYS */
  gfits_modify (&theader, "TNVAL13", "%s", 1, "NA");    /* REF_PHOT_SYS */
  gfits_modify (&theader, "TNVAL14", "%s", 1, "NA");    /* LABEL        */

  /* create table, add data values */
  gfits_create_table (&theader, &table);
  
  /* add data to table */
  for (i = 0; i < Nkeep; i++) {
    newdata = &photdata[index[i]];
    startstr = ohana_sec_to_date (newdata[0].tstart);
    stopstr = ohana_sec_to_date (newdata[0].tstop);
    code    = GetPhotcodeNamebyCode (newdata[0].photcode);
    photsys = GetPhotcodeNamebyCode (newdata[0].refcode);
    c1      = GetPhotcodeNamebyCode (newdata[0].c1);
    c2      = GetPhotcodeNamebyCode (newdata[0].c2);
    if (code    == (char *) NULL) code    = PhotNA;
    if (photsys == (char *) NULL) photsys = PhotNA;
    if (c1      == (char *) NULL) c1      = PhotNA;
    if (c2      == (char *) NULL) c2      = PhotNA;

    line = gfits_table_print (&table, newdata[0].ZP, newdata[0].ZPo, newdata[0].dZP, 
	      newdata[0].K, newdata[0].X, startstr, stopstr,
	      c1, c2, newdata[0].Nmeas, newdata[0].Ntime, code, photsys, newdata[0].label);
    if (!gfits_add_rows (&table, line, 1, strlen(line))) {
      fprintf (stderr, "error writing dataline");
      exit (1);
    }

    free (line);
    free (startstr);
    free (stopstr);
  }
 
  f = fopen (filename, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "Failure writing fits table\n");
    exit (1);
  }
  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &table);
  fclose (f);
  exit (0);
}

/* Select, TimeMode are global */
int PrintSubset (PhotPars *photdata, off_t *match, off_t Nmatch) {
  
  off_t i, j;
  char *photstr, *timestr, *c1, *c2, *refcode;
  
  /* print the selected entries */
  for (j = 0; j < Nmatch; j++) {
    
    i = match[j];
    
    /* convert UNIX time to Elixir-style date string */
    timestr = ohana_sec_to_date (photdata[i].tstart);
    
    if (output.photcodenames) {
      /* convert photcode to filter name */
      photstr = GetPhotcodeNamebyCode (photdata[i].photcode);
      refcode = GetPhotcodeNamebyCode (photdata[i].refcode);
      c1      = GetPhotcodeNamebyCode (photdata[i].c1);
      c2      = GetPhotcodeNamebyCode (photdata[i].c2);

      if (photstr == NULL) photstr = PhotError;
      if (refcode == NULL) refcode = PhotError;
      if (c1      == NULL) c1      = PhotError;
      if (c2      == NULL) c2      = PhotError;
      
      fprintf (stdout, "%s %s  %7.4f %7.4f %7.4f  %3d %3d  %7.4f %7.4f  %s  %s %s %s\n", 
	       photstr, timestr, photdata[i].ZP, photdata[i].ZPo, photdata[i].dZP, photdata[i].Nmeas, photdata[i].Ntime, 
	       photdata[i].X, photdata[i].K, refcode, c1, c2, photdata[i].label); 
    } else {
      fprintf (stdout, "%4d %s  %7.4f %7.4f %7.4f  %3d %3d  %7.4f %7.4f  %4d %4d %4d  %s\n", 
	       photdata[i].photcode, timestr, photdata[i].ZP, photdata[i].ZPo, photdata[i].dZP, photdata[i].Nmeas, 
	       photdata[i].Ntime, photdata[i].X, photdata[i].K, 
	       photdata[i].refcode, photdata[i].c1, photdata[i].c2, photdata[i].label); 
    }
      
    free (timestr);
  }
  return (TRUE);
}

/**** the pre-autocode implementation of this program failed to byte-swap refcode!
      any tables from before the new implementation have to be treated carefully
****/
