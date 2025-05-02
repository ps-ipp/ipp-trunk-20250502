# include "imregister.h"
# include "imreg.h"

static int RegTimeMode = FALSE;
static int CCDSeq = FALSE;
static int PTstyle = FALSE;

void SetOutputMode (char *mode) {

  if (!strcmp (mode, "RegTimeMode")) {
    RegTimeMode = TRUE;
    return;
  }
  if (!strcmp (mode, "CCDSeq")) {
    CCDSeq = TRUE;
    return;
  }
  if (!strcmp (mode, "PTstyle")) {
    PTstyle = TRUE;
    return;
  }

}

/* given a subset list, write out the selected images, if desired */
void OutputSubset (RegImage *image, off_t Nimage, off_t *match, off_t Nmatch) {

  if (output.table != (char *) NULL) {
    DumpFitsTable (output.table, image, match, Nmatch);
  } 

  if (output.bintable != (char *) NULL) {
    DumpFitsBintable (output.bintable, image, match, Nmatch);
  } 

  if (output.cadctable != (char *) NULL) {
    DumpCADCTable (output.cadctable, image, match, Nmatch);
  } 

  PrintSubset (image, match, Nmatch);
  if (output.verbose) fprintf (stderr, "SUCCESS\n");

  exit (0);
}

/* write out complete binary FITS table in format of db */
void DumpFitsBintable (char *filename, RegImage *image, off_t *match, off_t Nmatch) {

  off_t i, j;
  FILE *f;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  RegImage *subset;

  /* extract subset list to single array */
  ALLOCATE (subset, RegImage, MAX (1, Nmatch));
  for (i = 0; i < Nmatch; i++){
    j = match[i];
    memcpy (&subset[i], &image[j], sizeof (RegImage));
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
  gfits_table_set_RegImage (&ftable, subset, Nmatch, TRUE);

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &ftable);
  fclose (f);
  exit (0);
}

void DumpFitsTable (char *filename, RegImage *image, off_t *match, off_t Nmatch) {
  
  off_t i;
  char *obsstr, *regstr, *line, dummy[64];
  char *modestr, *typestr, *ccdstr, *datestr;
  time_t tsecond;
  Header header, theader;
  Matrix matrix;
  FTable table;
  RegImage *subset;

  bzero (dummy, 64);
  memset (dummy, ' ', 63);

  /* create primary header */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);
  
  /* create table header */
  gfits_create_table_header (&theader, "TABLE", "IMAGE_DATABASE");
      
  /* add current date/time to header */
  ohana_str_to_time ("now", &tsecond);
  datestr = ohana_sec_to_date (tsecond);
  gfits_modify (&header,  "DATE", "%s", 1, datestr);
  gfits_modify (&theader, "DATE", "%s", 1, datestr);

  /* define table layout */
  gfits_define_table_column (&theader, "A64",   "FILE",       "filename in db",        "");
  gfits_define_table_column (&theader, "A128",  "PATH",       "fullpath in db",        "");
  gfits_define_table_column (&theader, "A32",   "FILTER",     "filter name",           "");
  gfits_define_table_column (&theader, "A32",   "INSTRUMENT", "instrument",            "");
  gfits_define_table_column (&theader, "A6",    "CCD",        "ccd identifier",        "");
  gfits_define_table_column (&theader, "A6",    "MODE",       "mef/split/etc",         "");
  gfits_define_table_column (&theader, "A8",    "TYPE",       "object/flat/bias/etc",  "");
  gfits_define_table_column (&theader, "A25",   "JUNK",       "space for expansion",   "");
  gfits_define_table_column (&theader, "F6.1",  "EXPTIME",    "exposure time",        "seconds");
  gfits_define_table_column (&theader, "F5.3",  "AIRMASS",    "airmass",              "");
  gfits_define_table_column (&theader, "F7.1",  "SKY",        "background level",     "counts / pixel");
  gfits_define_table_column (&theader, "F6.1",  "BIAS",       "bias level",           "counts / pixel");
  gfits_define_table_column (&theader, "F5.2",  "FWHM",       "image quality",        "pixels");
  gfits_define_table_column (&theader, "F5.1",  "TELFOCUS",   "telescope focus",      "microns");
  gfits_define_table_column (&theader, "F5.1",  "XPROBE",     "bonnette probe x pos", "microns");
  gfits_define_table_column (&theader, "F5.1",  "YPROBE",     "bonnette probe y pos", "microns");
  gfits_define_table_column (&theader, "F5.1",  "ZPROBE",     "bonnette focus",       "microns");
  gfits_define_table_column (&theader, "F5.1",  "DETTEMP",    "detector temperature", "deg celcius");
  gfits_define_table_column (&theader, "F5.1",  "TELTEMP0",   "other temperature",    "deg celcius");
  gfits_define_table_column (&theader, "F5.1",  "TELTEMP1",   "other temperature",    "deg celcius");
  gfits_define_table_column (&theader, "F5.1",  "TELTEMP2",   "other temperature",    "deg celcius");
  gfits_define_table_column (&theader, "F5.1",  "TELTEMP3",   "other temperature",    "deg celcius");
  gfits_define_table_column (&theader, "F5.1",  "ROTANGLE",   "camear rotation angle", "degrees");
  gfits_define_table_column (&theader, "F10.6", "RA",         "image ra",              "degrees");
  gfits_define_table_column (&theader, "F10.6", "DEC",        "image dec",             "degrees");
  gfits_define_table_column (&theader, "A20",   "OBS_TIME",   "time of measurement",   "seconds since Jan 1, 1970 UT");
  gfits_define_table_column (&theader, "A20",   "REG_TIME",   "time of registration",  "seconds since Jan 1, 1970 UT");

  /* create table, add data values */
  gfits_create_table (&theader, &table);
  
  /* add data to table */
  for (i = 0; i < Nmatch; i++) {
    subset = &image[match[i]];
    obsstr   = ohana_sec_to_date (subset[0].obstime);
    regstr   = ohana_sec_to_date (subset[0].regtime);
    typestr  = get_type_name(subset[0].type);
    modestr  = get_mode_name(subset[0].mode);
    ccdstr   = ccds[(int)subset[0].ccd];

    line = gfits_table_print (&table, subset[0].pathname, subset[0].filename, 
			     subset[0].filter, subset[0].instrument, ccdstr,
			     modestr, typestr, dummy, 
			     subset[0].exptime, subset[0].airmass, 
			     subset[0].sky, subset[0].bias, subset[0].fwhm, 
			     subset[0].telfocus, subset[0].xprobe, subset[0].yprobe, subset[0].zprobe, 
			     subset[0].dettemp, 
			     subset[0].teltemp_0, subset[0].teltemp_1, subset[0].teltemp_2, subset[0].teltemp_3,
			     subset[0].rotangle, 
			     subset[0].ra, subset[0].dec, 
			     obsstr, regstr);

    gfits_add_rows (&table, line, 1, strlen(line));
    free (line);
    free (obsstr);
    free (regstr);
  }

  gfits_write_header  (filename, &header);
  gfits_write_matrix  (filename, &matrix);
  gfits_write_Theader (filename, &theader);
  gfits_write_table   (filename, &table);
  exit (0);
}

/* Select, TimeMode are global */
int PrintSubset (RegImage *image, off_t *match, off_t Nmatch) {
  
  char ccdstr[64];  
  off_t i, j;
  char *modestr, *typestr, *timestr, *root, *path;

  /* print the selected entries */
  for (j = 0; j < Nmatch; j++) {
    
    i = match[j];

    modestr = get_mode_name (image[i].mode);
    typestr = get_type_name (image[i].type);
    timestr = RegTimeMode ? ohana_sec_to_date (image[i].regtime) : ohana_sec_to_date (image[i].obstime);

    if (CCDSeq) {
      sprintf (ccdstr, "%02d", image[i].ccd);
    } else {
      if ((image[i].ccd < 0) || (image[i].ccd >= Nccd)) {
	sprintf (ccdstr, "%02d", image[i].ccd);
      } else {
      sprintf (ccdstr, "%s", ccds[(int)image[i].ccd]);
      }
    }      

    if (PTstyle) {
      root = filerootname (image[i].filename);

      /* do i want ccdstr? add a dot? 654321o.ccd00.ext */
      if (image[i].mode == M_MEF) {
	fprintf (stdout, "%s/%s %s/%s%02d %s %s\n", image[i].pathname, image[i].filename, root, root, image[i].ccd, ccdstr, modestr);
      }

      if (image[i].mode == M_SPLIT) {
	path = basename (image[i].pathname);
	fprintf (stdout, "%s/%s %s/%s %s %s\n", image[i].pathname, image[i].filename, path, root, ccdstr, "SPLIT");
      }

      if ((image[i].mode == M_SINGLE) || (image[i].mode == M_CUBE)) {
	fprintf (stdout, "%s/%s %s 0 %s\n", image[i].pathname, image[i].filename, root, modestr);
      }
    } else {
      /* this is somewhat poor: I have predefined a subset of value, and I can't guarantee that 'filter' has no spaces */
      fprintf (stdout, "%5lld %6s %6s %s  ", (long long) i, typestr, modestr, ccdstr);
      fprintf (stdout, "%s %s  ", image[i].pathname, image[i].filename);
      fprintf (stdout, "%s %f %s %f %f %f\n", image[i].filter, image[i].exptime, 
	       timestr, image[i].fwhm, image[i].bias, image[i].sky);
    }

    free (timestr);
  }
  return (TRUE);
}

int dump_data (RegImage *image, off_t Nimage) {

  off_t i;

  for (i = 0; i < Nimage; i++) {
    fprintf (stdout, "%s %f %f %f %s\n", image[i].filename, image[i].fwhm, image[i].sky, image[i].bias, image[i].filter);
  }

  fprintf (stdout, "SUCCESS\n");
  exit (0);

}
