# include "imregister.h"
# include "imphot.h"

/* write out complete binary FITS table in format of db */
int DumpFitsBintable (char *filename, Image *image, off_t *match, off_t Nmatch) {

  off_t i, j;
  FILE *f;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  Image *subset;

  ALLOCATE (subset, Image, MAX (1, Nmatch));
  for (i = 0; i < Nmatch; i++){
    j = match[i];
    memcpy (&subset[i], &image[j], sizeof (Image));
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
  // gfits_table_set_Image (&ftable, subset, Nmatch, TRUE);

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &ftable);
  fclose (f);
  exit (0);
}

int DumpFitsTable (char *filename, Image *image, off_t *match, off_t Nmatch) {
  
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  Image *subset;
  FILE *f;
  char *startstr, *filtstr, *datestr, *line;
  off_t i;
  double zp, dzp, ra, dec, airmass, sky;
  time_t tsecond;

  /* create primary header */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);
  
  /* the ASCII table is always a little harder than the binary:
   * we need to build the data line a bit carefully 
   */

  /* create table header */
  gfits_create_table_header (&theader, "TABLE", "ZPTS");

  /* add current date/time to header */
  ohana_str_to_time ("now", &tsecond);
  datestr = ohana_sec_to_date (tsecond);
  gfits_modify (&header,  "DATE", "%s", 1, datestr);
  gfits_modify (&theader, "DATE", "%s", 1, datestr);
  
  /* define table layout */
  gfits_define_table_column (&theader, "A20",   "START_TIME", "start time of measurement", "yyyy/mm/dd,hh:mm:ss");
  gfits_define_table_column (&theader, "A10",   "FILTER",     "filter and camera name",    "");
  gfits_define_table_column (&theader, "F8.4",  "ZP_OBS",     "measured zero point",       "mag");
  gfits_define_table_column (&theader, "F7.4",  "ZP_ERR",     "error on zero point",       "mag");
  gfits_define_table_column (&theader, "F11.6", "RA",         "RA (J2000)",                "dec. degrees");
  gfits_define_table_column (&theader, "F11.6", "DEC",        "DEC (J2000)",               "dec. degrees");
  gfits_define_table_column (&theader, "F7.3",  "C_AIRMASS",  "airmass coeff",             "mag per airmass"); 
  gfits_define_table_column (&theader, "F7.1",  "SKY",        "median sky flux",           "counts");
  gfits_define_table_column (&theader, "I6",    "NSTAR",      "Number of stars in image",  "stars");

  /* define TNULL, TNVAL values */
  gfits_modify (&theader, "TNULL1",  "%s", 1, "NULL"); /* START_TIME */
  gfits_modify (&theader, "TNULL2",  "%s", 1, "NULL"); /* FILTER     */
  gfits_modify (&theader, "TNULL3",  "%s", 1, "NaN");  /* ZP_OBS     */
  gfits_modify (&theader, "TNULL4",  "%s", 1, "NaN");  /* ZP_ERR     */
  gfits_modify (&theader, "TNULL5",  "%s", 1, "NaN");  /* RA         */
  gfits_modify (&theader, "TNULL6",  "%s", 1, "NaN");  /* DEC        */
  gfits_modify (&theader, "TNULL7",  "%s", 1, "NaN");  /* C_AIRMASS  */
  gfits_modify (&theader, "TNULL8",  "%s", 1, "NaN");  /* SKY        */
  gfits_modify (&theader, "TNULL9",  "%s", 1,  "-1");  /* NSTAR      */

  gfits_modify (&theader, "TNVAL1",  "%s", 1, "NA");   /* START_TIME */
  gfits_modify (&theader, "TNVAL2",  "%s", 1, "NA");   /* FILTER     */
  gfits_modify (&theader, "TNVAL3",  "%s", 1, "Inf");  /* ZP_OBS     */
  gfits_modify (&theader, "TNVAL4",  "%s", 1, "Inf");  /* ZP_ERR     */
  gfits_modify (&theader, "TNVAL5",  "%s", 1, "Inf");  /* RA         */
  gfits_modify (&theader, "TNVAL6",  "%s", 1, "Inf");  /* DEC        */
  gfits_modify (&theader, "TNVAL7",  "%s", 1, "Inf");  /* C_AIRMASS  */
  gfits_modify (&theader, "TNVAL8",  "%s", 1, "Inf");  /* SKY        */
  gfits_modify (&theader, "TNVAL9",  "%s", 1,  "-2");  /* NSTAR      */

  /* add data to table */
  for (i = 0; i < Nmatch; i++) {
    subset   = &image[match[i]];
    startstr = ohana_sec_to_date (subset[0].tzero);
    filtstr  = GetPhotcodeNamebyCode (subset[0].photcode);
    zp       = subset[0].McalPSF;
    dzp      = subset[0].dMcal;
    XY_to_RD (&ra, &dec, 0.0, 0.0, &subset[0].coords);
    airmass  = subset[0].secz;
    sky      = 0.0; // subset[0].Myyyy + 0x8000;

    /* we should get an error here if we don't construct this line correctly */
    line = gfits_table_print (&ftable, startstr, filtstr, zp, dzp, ra, dec, airmass, sky, subset[0].nstar);
    gfits_add_rows (&ftable, line, 1, strlen(line));
    free (line);
    free (startstr);
  }

  /* write data to output file */
  f = fopen (filename, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "Failure writing fits table\n");
    exit (1);
  }
  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &ftable);
  fclose (f);
  return (TRUE);
}
