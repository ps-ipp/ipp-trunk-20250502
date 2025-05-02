# include "imregister.h"
# include "spreg.h"

static int RegTimeMode = FALSE;

void SetOutputMode (char *mode) {

  if (!strcmp (mode, "RegTimeMode")) {
    RegTimeMode = TRUE;
    return;
  }
  return;
}

/* given a subset list, write out the selected spectra, if desired */
void OutputSubset (Spectrum *spectrum, off_t Nspectra, off_t *match, off_t Nmatch) {

  if (output.table != (char *) NULL) {
    DumpFitsTable (output.table, spectrum, match, Nmatch);
  } 

  if (output.bintable != (char *) NULL) {
    DumpFitsBintable (output.bintable, spectrum, match, Nmatch);
  } 

  PrintSubset (spectrum, match, Nmatch);
  exit (0);
}

/* write out complete binary FITS table in format of db */
void DumpFitsBintable (char *filename, Spectrum *spectrum, off_t *match, off_t Nmatch) {

  off_t i, j;
  FILE *f;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  Spectrum *subset;

  /* extract subset list to single array */
  ALLOCATE (subset, Spectrum, MAX (1, Nmatch));
  for (i = 0; i < Nmatch; i++){
    j = match[i];
    memcpy (&subset[i], &spectrum[j], sizeof (Spectrum));
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
  gfits_table_set_Spectrum (&ftable, subset, Nmatch, TRUE);

  gfits_fwrite_header   (f, &header);
  gfits_fwrite_matrix   (f, &matrix);
  gfits_fwrite_Theader  (f, &theader);
  gfits_fwrite_table   (f, &ftable);
  fclose (f);
  exit (0);
}

/* write out an ASCII table */
void DumpFitsTable (char *filename, Spectrum *spectrum, off_t *match, off_t Nmatch) {
  
  off_t i;
  char *obsstr, *regstr, *line, *datestr;
  time_t tsecond;
  FILE *f;
  Header header, theader;
  Matrix matrix;
  FTable ftable;
  Spectrum *subset;

  /* create primary header */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);
  
  /* the ASCII table is always a little harder than the binary:
   * we need to build the data line a bit carefully 
   */

  /* create an empty table which we will fill in by hand */
  ftable.header = &theader;
  gfits_table_set_SpectrumASCII (&ftable, NULL, 0, TRUE);
  
  /* add data to table */
  for (i = 0; i < Nmatch; i++) {
    subset = &spectrum[match[i]];
    obsstr = ohana_sec_to_date (subset[0].obstime);
    regstr = ohana_sec_to_date (subset[0].regtime);

    /* we should get an error here if we don't construct this line correctly */
    line = gfits_table_print (&ftable, subset[0].filename, subset[0].pathname, subset[0].instrument, 
			     subset[0].telescope, subset[0].objname, subset[0].extname, 
			     subset[0].ra, subset[0].dec, subset[0].exptime, subset[0].airmass, 
			     subset[0].Ws, subset[0].We, subset[0].dW, 
			     subset[0].Nspec, obsstr, regstr,
			     subset[0].mode, subset[0].state, subset[0].flag);

    gfits_add_rows (&ftable, line, 1, strlen(line));
    free (line);
    free (obsstr);
    free (regstr);
  }

  /* add current date/time to header */
  ohana_str_to_time ("now", &tsecond);
  datestr = ohana_sec_to_date (tsecond);
  gfits_modify (&header,  "DATE", "%s", 1, datestr);
  gfits_modify (&theader, "DATE", "%s", 1, datestr);

  /* open file for output */
  f = fopen (filename, "w");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't open output file %s\n", filename);
    exit (1);
  }

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &ftable);
  fclose (f);

  exit (0);
}

/* Select, TimeMode are global */
int PrintSubset (Spectrum *spectrum, off_t *match, off_t Nmatch) {
  
  off_t i, j;
  char *timestr;

  /* print the selected entries */
  for (j = 0; j < Nmatch; j++) {
    i = match[j];
    
    timestr = RegTimeMode ? ohana_sec_to_date (spectrum[i].regtime) : ohana_sec_to_date (spectrum[i].obstime);

    /* predefined subset of values */
    fprintf (stdout, "%5lld %20s %s %s %s %s ", (long long) i, timestr, spectrum[i].pathname, spectrum[i].filename, spectrum[i].objname, spectrum[i].telescope);
    fprintf (stdout, "%5.1f %5.3f %.1f %.1f %.2f\n", spectrum[i].exptime, spectrum[i].airmass, spectrum[i].Ws, spectrum[i].We, spectrum[i].dW);

    free (timestr);
  }
  return (TRUE);
}

int dump_data (Spectrum *spectrum, off_t Nspectrum) {

  off_t i;

  for (i = 0; i < Nspectrum; i++) {
    fprintf (stdout, "%s %s %f %f %f %f\n", spectrum[i].filename, spectrum[i].objname, spectrum[i].exptime, spectrum[i].airmass, spectrum[i].Ws, spectrum[i].We);
  }

  fprintf (stdout, "SUCCESS\n");
  exit (0);

}
