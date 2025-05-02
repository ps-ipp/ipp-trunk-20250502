# include <ohana.h>
# include <gfitsio.h>

// this is only valid for the regular and non-boolean fields
int gfits_modify (Header *header, char *field, char *mode, int N,...) {
  
  /* this function expects one more argument, the value to be written */
  /* this function is extremely similar to gfits_print, except it allows for changing an existing field. */

  // the output string needs to be long enough to keep the compiler from
  // complaining, even though the string cannot be more than 80 chars
  char comment[82], string[200], data[82];
  char *p, *qe;
  va_list argp;
  
  va_start (argp, N);
  bzero (data, 82);
  bzero (string, 200);
  bzero (comment, 82);

  if (mode[0] != '%') {
    fprintf (stderr, "gfits_print: weird mode:  %s\n", mode);
    return (FALSE);
  }

  /* find location of desired entry */
  p = gfits_header_field (header, field, N);
  if (p == NULL)  {
    /* new entry, find the END of the header */
    p = gfits_header_field (header, "END", 1);
    if (p == NULL) return (FALSE); 
    
    /* is there enough space for 1 more line? */
    if (header[0].datasize - (p - (header[0].buffer)) < 2*FT_LINE_LENGTH) {
      header[0].datasize += FT_RECORD_SIZE;
      REALLOCATE (header[0].buffer, char, header[0].datasize);
      p = gfits_header_field (header, "END", 1);
      if (p == NULL) return (FALSE); 
      memset (p + FT_LINE_LENGTH, ' ', FT_RECORD_SIZE);
    }
    
    /* push END line back 1 */
    memmove ((p + FT_LINE_LENGTH), p, FT_LINE_LENGTH);
    memset (p, ' ', FT_LINE_LENGTH);
  } else {
    /* old entry, save the comment region (is this skipping a character for non-strings?) */
    qe = gfits_keyword_end (p);
    qe += 3;
    qe = MIN (p + 80, qe);
    strncpy_nowarn (comment, qe, p + 80 - qe);
  }
  gfits_pad_ending (comment, 0x20, 82);  /* comment must contain spaces to the end */

  /* write the numeric modes */
  if (!strcmp (mode, "%d"))   { snprintf (string, 200, "%-8s= %20d / %-s ",    field, va_arg (argp, int),       	 comment); goto found_it; }
  if (!strcmp (mode, "%ld"))  { snprintf (string, 200, "%-8s= %20ld / %-s ",   field, va_arg (argp, long),      	 comment); goto found_it; }
  if (!strcmp (mode, "%lld")) { snprintf (string, 200, "%-8s= %20lld / %-s ",  field, va_arg (argp, long long), 	 comment); goto found_it; }
  if (!strcmp (mode, "%Ld"))  { snprintf (string, 200, "%-8s= %20lld / %-s ",  field, va_arg (argp, long long), 	 comment); goto found_it; }
  if (!strcmp (mode, "%u"))   { snprintf (string, 200, "%-8s= %20u / %-s ",    field, va_arg (argp, unsigned),  	 comment); goto found_it; }
  if (!strcmp (mode, "%lu"))  { snprintf (string, 200, "%-8s= %20lu / %-s ",   field, va_arg (argp, unsigned long),      comment); goto found_it; }
  if (!strcmp (mode, "%llu")) { snprintf (string, 200, "%-8s= %20llu / %-s ",  field, va_arg (argp, unsigned long long), comment); goto found_it; }
  if (!strcmp (mode, "%Lu"))  { snprintf (string, 200, "%-8s= %20llu / %-s ",  field, va_arg (argp, unsigned long long), comment); goto found_it; }
  if (!strcmp (mode, "%hd"))  { snprintf (string, 200, "%-8s= %20d / %-s ",    field, va_arg (argp, int),      	         comment); goto found_it; } 
  if (!strcmp (mode, "%f"))   { snprintf (string, 200, "%-8s= %20.10f / %-s ", field, va_arg (argp, double),   	         comment); goto found_it; } 
  if (!strcmp (mode, "%lf"))  { snprintf (string, 200, "%-8s= %20.10f / %-s ", field, va_arg (argp, double),   	         comment); goto found_it; } 
  if (!strcmp (mode, "%e"))   { snprintf (string, 200, "%-8s= %20.10E / %-s ", field, va_arg (argp, double),   	         comment); goto found_it; } 
  if (!strcmp (mode, "%le"))  { snprintf (string, 200, "%-8s= %20.10E / %-s ", field, va_arg (argp, double),   	         comment); goto found_it; } 
  if (!strcmp (mode, "%g"))   { snprintf (string, 200, "%-8s= %20.10G / %-s ", field, va_arg (argp, double),   	         comment); goto found_it; } 
  if (!strcmp (mode, "%lg"))  { snprintf (string, 200, "%-8s= %20.10G / %-s ", field, va_arg (argp, double),   	         comment); goto found_it; } 
  if (!strcmp (mode, "%jd"))  { snprintf (string, 200, "%-8s= %20jd / %-s ",   field, va_arg (argp, intmax_t), 	         comment); goto found_it; }

  /* string value.  Quotes must be at least 8 chars apart */
  if (!strcmp (mode, "%s")) {
    char *ptr = va_arg (argp, char *);
    if (!ptr) goto invalid;
    strncpy_nowarn (data, ptr, 68);
    snprintf (string, 200, "%-8s= '%-8s' / %-s ", field, data, comment);
    goto found_it;
  }

  /* failed to find mode */
invalid:
  va_end (argp);
  return (FALSE);

found_it:
  memcpy (p, string, 80); // do not use strncpy_nowarn: do NOT set last byte to NULL
  va_end (argp);
  return (TRUE);
}

// alternate version for the special types (boolean, comments, COMMENT)
int gfits_modify_alt (Header *header, char *field, char *mode, int N,...) {
  
  /* this function expects one more argument, the value to be written */
  /* this function is extremely similar to gfits_print, except it allows for changing an existing field. */

  char comment[82], string[200], data[82];
  char *p, *qs, *qe;
  va_list argp;
  
  va_start (argp, N);
  bzero (data, 82);
  bzero (comment, 82);
  bzero (string, 200);

  if (mode[0] != '%') {
    fprintf (stderr, "gfits_print: weird mode:  %s\n", mode);
    return (FALSE);
  }

  /* find location of desired entry */
  p = gfits_header_field (header, field, N);
  if (p == NULL)  {
    /* new entry, find the END of the header */
    p = gfits_header_field (header, "END", 1);
    if (p == NULL) return (FALSE); 
    
    /* is there enough space for 1 more line? */
    if (header[0].datasize - (p - (header[0].buffer)) < 2*FT_LINE_LENGTH) {
      header[0].datasize += FT_RECORD_SIZE;
      REALLOCATE (header[0].buffer, char, header[0].datasize);
      p = gfits_header_field (header, "END", 1);
      if (p == NULL) return (FALSE); 
      memset (p + FT_LINE_LENGTH, ' ', FT_RECORD_SIZE);
    }
    
    /* push END line back 1 */
    memmove ((p + FT_LINE_LENGTH), p, FT_LINE_LENGTH);
    memset (p, ' ', FT_LINE_LENGTH);
  } else {
    /* old entry, save the comment region (is this skipping a character for non-strings?) */
    qe = gfits_keyword_end (p);
    qe += 3;
    qe = MIN (p + 80, qe);
    strncpy_nowarn (comment, qe, p + 80 - qe);
  }
  gfits_pad_ending (comment, 0x20, 82);  /* comment must contain spaces to the end */

  /* write the boolean mode */
  if (!strcmp (mode, "%t")) {
    if (va_arg (argp, int)) 
      snprintf (string, 200, "%-8s= %18s T / %-s ", field, " ", comment);
    else
      snprintf (string, 200, "%-8s= %18s F / %-s ", field, " ", comment);
  }

  /* comment type of value.  */
  if (!strcmp (mode, "%S")) {
    char *ptr = va_arg (argp, char *);
    if (!ptr) goto invalid;
    strncpy_nowarn (data, ptr, 71);
    snprintf (string, 200, "%-8s %-71s", field, data);
  }  

  /* just a comment associated with a value.  this is assumes a fixed format position for the comment */
  if (!strcmp (mode, "%C")) { 
    qe = gfits_keyword_end (p);
    qs = gfits_keyword_start (p);

    /* keep ' on ends, if there */
    if (qe[0]  == 0x27) qe ++;
    if (qs[-1] == 0x27) qs --;

    strncpy_nowarn (data, qs, MAX (MIN (qe - qs, 71), 0));

    char *ptr = va_arg (argp, char *);
    if (!ptr) goto invalid;
    strncpy_nowarn (comment, ptr, 80);
    gfits_pad_ending (comment, 0x20, 82);  /* comment must contain spaces to the end */
    snprintf (string, 200, "%-8s= %s / %-s", field, data, comment);
    /* this will keep the original line, but truncate the comment */
  }

  memcpy (p, string, 80); // do not use strncpy_nowarn: do NOT set last byte to NULL
  va_end (argp);
  return (TRUE);

invalid:
  va_end (argp);
  return (FALSE);
}

/* given a FITS card line, return pointer to the start of the data area */
char *gfits_keyword_start (char *line) {

  char *c;

  /* find the end of the existing data region */
  c = line + 8;
  if (*c != '=') return (c);  /* no data in COMMENT field */
  
  c += 2;
  /* advance pointer over WHITESPACE */
  while ((*c == ' ') && (c < line + 80)) { c++; }
  
  /* skip one quote mark */
  if ((*c == 0x27) && (c < line + 80)) { c++; }

  return (c);
}

/* given a FITS card line, return pointer to the end of the data area */
char *gfits_keyword_end (char *line) {

  int done;
  char *c1, *c2;

  /* find the end of the existing data region */
  c1 = line + 8;
  if (*c1 != '=') return (c1);  /* no data in COMMENT field */
  c1 += 2;

  /* advance pointer over WHITESPACE */
  while ((*c1 == ' ') && (c1 < line + 80)) { c1++; }
  
  if (c1[0] == 0x27) { /* entry a string, skip over 'fred' */
    for (done = FALSE, c2 = c1 + 1; !done && (c2 < line + 80); c2++) {
      if ((c2[0] == 0x27) && (c2[1] == 0x27)) {
	c2 += 2;
	continue;
      }
      if (c2[0] == 0x27) {
	c1 = c2;
	done = TRUE;
      }
    }
    if (!done) { /* error in line: mismatched ' chars, return fixed position */
      c1 = line + 30;
    }
  } else {
    while ((*c1 != ' ') && (c1 < line + 80)) c1++;
  }
  return (c1);
}

/* given a pointer to the FITS HIERARCH card field name, return pointer to the start of the data area */
char *gfits_hierarch_keyword_start (char *line, char *field) {

  char *c;

  /* find the end of the existing data region */
  c = line + strlen(field) + 1;
  if (*c != '=') return (c);  /* non-data fields are not allowed */
  
  c += 2;
  /* advance pointer over WHITESPACE */
  while ((*c == ' ') && (c < line + 71)) { c++; }
  
  /* skip one quote mark */
  if ((*c == 0x27) && (c < line + 71)) { c++; }

  return (c);
}

/* given a pointer to the FITS HIERARCH card field name, return pointer to the end of the data area */
char *gfits_hierarch_keyword_end (char *line, char *field) {

  int done;
  char *c1, *c2;

  /* find the end of the existing data region */
  c1 = line + strlen(field) + 1;
  if (*c1 != '=') return (NULL);  /* non-data fields are not allowed */
  c1 += 2;

  /* advance pointer over WHITESPACE */
  while ((*c1 == ' ') && (c1 < line + 71)) { c1++; }
  
  if (c1[0] == 0x27) { /* entry a string, skip over 'fred' */
    for (done = FALSE, c2 = c1 + 1; !done && (c2 < line + 71); c2++) {
      if ((c2[0] == 0x27) && (c2[1] == 0x27)) {
	c2 += 2;
	continue;
      }
      if (c2[0] == 0x27) {
	c1 = c2;
	done = TRUE;
      }
    }
    if (!done) { /* error in line: mismatched ' chars, return fixed position */
      c1 = line + 30;
    }
  } else {
    while ((*c1 != ' ') && (c1 < line + 71)) c1++;
  }
  return (c1);
}

/* according to the FITS guidelines, a string is supposed to start with a single quote
   on the 11th character.  I'll be generous and allow the string to start at the 10th.
   I read the string into tmp then strip off the leading single quote and spaces.
   a string of "  '  fred is here  '  " should become "fred is here".
   a string of "  fred is here  " should also become "fred is here".
   the remaining problem:
   "'O''HARA'" should be kept as O''HARA.  need to test for double single quotes
   d -- /(0x2f: comment)  (*(p + i + 10) != 0x2f) &&  && (*(p + i + 10) != 0x27)
 */

// XXX this is an absurd patch on a gcc bug: memset has a problem is value != 0 and N is constant
void myMemset (char *ptr, int value, size_t N) {

  char *p = ptr;
  
  size_t i = 0;
  for (i = 0; i < N; i++, p++) {
    *p = value;
  }
}

/* fill 'line' with Nbyte space from first NULL to last byte with value */
void gfits_pad_ending (char *line, char value, int Nbyte) {

  line[Nbyte-1] = 0;
  
  char *p = line + strlen (line);
  size_t N = MAX (Nbyte - strlen (line) - 1, 0);
  myMemset (p, value, N);
}


/* in this routine, the value field will expand as needed, forcing out the comment
   the total line is limited by snprintf to only 80 chars + EOL */
