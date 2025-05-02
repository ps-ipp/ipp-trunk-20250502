# include <ohana.h>
# include <gfitsio.h>

// this only prints the regular and non-boolean fields
int gfits_print (Header *header, char *field, char *mode, int N,...) {
  
  /* this function expects one more argument, the value to be written */

  static char blank[] = " ";
  char string[200], line[80];
  char *p;
  va_list argp;  

  va_start (argp, N);
  
  if (mode[0] != '%') {
    fprintf (stderr, "gfits_print: weird mode:  %s\n", mode);
    return (FALSE);
  }

  /* this is supposed to create a new field, not modify an old one.  */
  p = gfits_header_field (header, field, N);
  if (p != NULL) return (FALSE);

  /* find the END of the header */
  p = gfits_header_field (header, "END", 1);
  if (p == NULL) return (FALSE); 

  /* is there enough space for 1 more line? */
  if (header[0].datasize - (p - (header[0].buffer)) < 2*FT_LINE_LENGTH) {
    header[0].datasize += FT_RECORD_SIZE;
    REALLOCATE (header[0].buffer, char, header[0].datasize);
    /* re-find the "END" marker, in case new memory block is used */
    p = gfits_header_field (header, "END", 1);
    if (p == NULL) return (FALSE); 
    memset (p + FT_LINE_LENGTH, ' ', FT_RECORD_SIZE);
  }

  /* push END line back 1 */
  memmove ((p + FT_LINE_LENGTH), p, FT_LINE_LENGTH);
  memset (p, ' ', FT_LINE_LENGTH);

  /* write the new FITS card, setting the comment to be blank */
  /* in these lines, the value field will expand as needed, forcing out the comment
     the total line is limited by snprintf to only 80 chars + EOL */

  /* write the numeric modes */
  if (!strcmp (mode, "%d"))  { snprintf (string, 200, "%-8s= %20d / %46s ",    field, va_arg (argp, int),                blank); goto found_it; }
  if (!strcmp (mode, "%ld")) { snprintf (string, 200, "%-8s= %20ld / %46s ",   field, va_arg (argp, long),               blank); goto found_it; }
  if (!strcmp (mode, "%lld")){ snprintf (string, 200, "%-8s= %20lld / %46s ",  field, va_arg (argp, long long),          blank); goto found_it; }
  if (!strcmp (mode, "%Ld")) { snprintf (string, 200, "%-8s= %20lld / %46s ",  field, va_arg (argp, long long),          blank); goto found_it; }
  if (!strcmp (mode, "%u"))  { snprintf (string, 200, "%-8s= %20u / %46s ",    field, va_arg (argp, unsigned),           blank); goto found_it; }
  if (!strcmp (mode, "%lu")) { snprintf (string, 200, "%-8s= %20lu / %46s ",   field, va_arg (argp, unsigned long),      blank); goto found_it; }
  if (!strcmp (mode, "%llu")){ snprintf (string, 200, "%-8s= %20llu / %46s ",  field, va_arg (argp, unsigned long long), blank); goto found_it; }
  if (!strcmp (mode, "%Lu")) { snprintf (string, 200, "%-8s= %20llu / %46s ",  field, va_arg (argp, unsigned long long), blank); goto found_it; }
  if (!strcmp (mode, "%hd")) { snprintf (string, 200, "%-8s= %20d / %46s ",    field, va_arg (argp, int),                blank); goto found_it; }
  if (!strcmp (mode, "%f"))  { snprintf (string, 200, "%-8s= %20.10f / %46s ", field, va_arg (argp, double),             blank); goto found_it; }
  if (!strcmp (mode, "%lf")) { snprintf (string, 200, "%-8s= %20.10f / %46s ", field, va_arg (argp, double),             blank); goto found_it; }
  if (!strcmp (mode, "%e"))  { snprintf (string, 200, "%-8s= %20.10E / %46s ", field, va_arg (argp, double),             blank); goto found_it; }
  if (!strcmp (mode, "%le")) { snprintf (string, 200, "%-8s= %20.10E / %46s ", field, va_arg (argp, double),             blank); goto found_it; }
  if (!strcmp (mode, "%g"))  { snprintf (string, 200, "%-8s= %20.10G / %46s ", field, va_arg (argp, double),             blank); goto found_it; }
  if (!strcmp (mode, "%lg")) { snprintf (string, 200, "%-8s= %20.10G / %46s ", field, va_arg (argp, double),             blank); goto found_it; }
  if (!strcmp (mode, "%jd")) { snprintf (string, 200, "%-8s= %20jd / %46s ",   field, va_arg (argp, intmax_t),           blank); goto found_it; }

  /* string value.  Quotes must be at least 8 chars apart.  Longer lines will this should be fixed to allow arbitrary string lengths, up to 69 chars */
  if (!strcmp (mode, "%s")) {
    char *ptr = va_arg (argp, char *);
    if (!ptr) goto invalid;
    strcpy (line, ptr);
    line[68] = 0;
    snprintf (string, 200, "%-8s= '%-8s' / %46s ", field, line, blank);
    goto found_it;
  }

invalid:
  va_end (argp);
  return (FALSE);

found_it:
  memcpy (p, string, 80); // do not use strncpy_nowarn: do NOT set last byte to NULL
  va_end (argp);
  return (TRUE);
}

// alternate version for the special types (boolean, comments, COMMENT)
int gfits_print_alt (Header *header, char *field, char *mode, int N,...) {
  
  /* this function expects one more argument, the value to be written */

  static char blank[] = " ";
  char string[200], line[80];
  char *p, a;
  va_list argp;  

  va_start (argp, N);
  
  if (mode[0] != '%') {
    fprintf (stderr, "gfits_print: weird mode:  %s\n", mode);
    return (FALSE);
  }

  /* this is supposed to create a new field, not modify an old one.  */
  p = gfits_header_field (header, field, N);
  if (p != NULL) return (FALSE);

  /* find the END of the header */
  p = gfits_header_field (header, "END", 1);
  if (p == NULL) return (FALSE); 

  /* is there enough space for 1 more line? */
  if (header[0].datasize - (p - (header[0].buffer)) < 2*FT_LINE_LENGTH) {
    header[0].datasize += FT_RECORD_SIZE;
    REALLOCATE (header[0].buffer, char, header[0].datasize);
    /* re-find the "END" marker, in case new memory block is used */
    p = gfits_header_field (header, "END", 1);
    if (p == NULL) return (FALSE); 
    memset (p + FT_LINE_LENGTH, ' ', FT_RECORD_SIZE);
  }

  /* push END line back 1 */
  memmove ((p + FT_LINE_LENGTH), p, FT_LINE_LENGTH);
  memset (p, ' ', FT_LINE_LENGTH);

  /* write the boolean mode */
  if (!strcmp (mode, "%t")) {
    a = va_arg (argp, int);
    if (a == 1)
      snprintf (string, 200, "%-8s= %18s T / %46s ", field, blank, blank);
    else
      snprintf (string, 200, "%-8s= %18s F / %46s ", field, blank, blank);
  }

  /* comment type of value.  */
  if (!strcmp (mode, "%S")) {
    /* we are forcing the entry to be <= 69 char long */
    char *ptr = va_arg (argp, char *);
    if (!ptr) goto invalid;
    bzero (line, 80);
    strncpy_nowarn (line, ptr, 71);
    snprintf (string, 200, "%-8s %-71s", field, line);
  }  

  /*
maximal string line:
12345678901234567890123456789012345678901234567890123456789012345678901234567890 .
KEYWORD = '01234567890123456789012345678901234567890123456789012345678901234567'

maximal comment line:
12345678901234567890123456789012345678901234567890123456789012345678901234567890 .
KEYWORD  01234567890123456789012345678901234567890123456789012345678901234567890
  */

  /* can't write the comment in gfits_print - use gfits_modify */
  if (!strcmp (mode, "%C")) goto invalid;

  memcpy (p, string, 80); // do not use strncpy_nowarn: do NOT set last byte to NULL
  
  va_end (argp);
  return (TRUE);

invalid:
  va_end (argp);
  return (FALSE);
}
