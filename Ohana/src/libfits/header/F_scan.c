# include <ohana.h>
# include <gfitsio.h>

// this only scans the regular and non-boolean fields
int gfits_scan (Header *header, char *field, char *mode, int N,...) {

  int status;
  va_list argp;
  
  va_start (argp, N);
  status = gfits_vscan (header, field, mode, N, argp);
  va_end (argp);
  return (status);
}
  
// alternate version for the special types (boolean, comments, COMMENT)
int gfits_scan_alt (Header *header, char *field, char *mode, int N,...) {

  int status;
  va_list argp;
  
  va_start (argp, N);
  status = gfits_vscan_alt (header, field, mode, N, argp);
  va_end (argp);
  return (status);
}
  
// this only scans the regular and non-boolean fields
int gfits_vscan (Header *header, char *field, char *mode, int N, va_list argp) {

  char *p, *q, *s, tmp[81];
  int Nchar, status;
  long long value;
  double fvalue;
  
  /* find the correct line with field */
  p = gfits_header_field (header, field, N);
  if (p == NULL) {
    status = gfits_vscan_hierarch (header, field, mode, N, argp);
    return (status);
  }

  /* all others require '=' in column 8 */
  if (p[8] != '=') return (FALSE);

  s = gfits_keyword_start (p); /* points at first char (not ') */
  q = gfits_keyword_end (p); /* points at following space or ' */

  // these are invalid conditions:
  if (s - p > 80) return FALSE;
  if (q - p > 80) return FALSE;

  /* extract data into char array (exclude containing ' chars) */
  Nchar = MIN (80, MAX (0, (q - s)));
  bzero (tmp, 81);
  memcpy (tmp, s, Nchar);

  // for string types, copy the data to the target pointer
  if (!strcmp (mode, "%s")) {
    stripwhite (tmp);
    strcpy (va_arg (argp, char *), tmp);
    return (TRUE);
  }
  
  /* remaining options are numerical data */
  /* need to interpret 1.0d5 as 1.0e5 */

  if (!strcmp (mode, "%f") || !strcmp (mode, "%lf")) {
    fvalue = strtod (tmp, &q);
    if ((*q == 'd') || (*q == 'D')) fvalue *= pow (10.0, atof (q + 1));

    if (!strcmp (mode, "%f"))   { *va_arg (argp, float *)  = fvalue; return (TRUE); }
    if (!strcmp (mode, "%lf"))  { *va_arg (argp, double *) = fvalue; return (TRUE); }
  }

  value = strtoll (tmp, &q, 0);
  if ((*q == 'd') || (*q == 'D')) value *= pow (10.0, atof (q + 1));

  if (!strcmp (mode, "%d"))   { *va_arg (argp, int *)       	     = value; return (TRUE); }
  if (!strcmp (mode, "%ld"))  { *va_arg (argp, long *)      	     = value; return (TRUE); }
  if (!strcmp (mode, "%lld")) { *va_arg (argp, long long *) 	     = value; return (TRUE); }
  if (!strcmp (mode, "%Ld"))  { *va_arg (argp, long long *) 	     = value; return (TRUE); }
  if (!strcmp (mode, "%u"))   { *va_arg (argp, unsigned *)  	     = value; return (TRUE); }
  if (!strcmp (mode, "%lu"))  { *va_arg (argp, unsigned long *)      = value; return (TRUE); }
  if (!strcmp (mode, "%llu")) { *va_arg (argp, unsigned long long *) = value; return (TRUE); }
  if (!strcmp (mode, "%Lu"))  { *va_arg (argp, unsigned long long *) = value; return (TRUE); }
  if (!strcmp (mode, "%hd"))  { *va_arg (argp, short *)     	     = value; return (TRUE); }

  // XXX is this safe for 64bit off_t and 32bit off_t?
  // XXX the problem is that we read FITS files on many machine types: I need to ensure 
  // that we are portable -- this sseems inconsistent
  if (!strcmp (mode, "%jd"))  { *va_arg (argp, intmax_t *) 	     = value; return (TRUE); }

  /* no valid mode found */
  return (FALSE);
}

// alternate version for the special types (boolean, comments, COMMENT)
int gfits_vscan_alt (Header *header, char *field, char *mode, int N, va_list argp) {

  char *p, *q, *s, tmp[128];
  int Nchar, status;

  /* find the correct line with field */
  p = gfits_header_field (header, field, N);
  if (p == NULL) {
    status = gfits_vscan_hierarch (header, field, mode, N, argp);
    return (status);
  }

  /* non-data entry (COMMENT, HISTORY) */
  if (!strcmp (mode, "%S")) {
    strncpy_nowarn (va_arg (argp, char *), p + 8, FT_HISTORY_LENGTH);
    return (TRUE);
  }

  /* all others require '=' in column 8 */
  if (p[8] != '=') return (FALSE);

  /* comment from data line */
  if (!strcmp (mode, "%C")) {
    q = gfits_keyword_end (p);
    if (!q) return (FALSE);
    q += 3;
    q = MIN (p + 80, q);
    bzero (tmp, 81);
    Nchar = MAX (0, MIN (80, p + 80 - q));
    memcpy (tmp, q, Nchar);
    stripwhite (tmp);
    strcpy (va_arg (argp, char *), tmp);
    return (TRUE);
  }

  /* boolean data, requires int target */
  if (!strcmp (mode, "%t")) {
    s = gfits_keyword_start (p);
    if (*s == 'T') {
      *va_arg (argp, int *) = TRUE;
      return (TRUE);
    }
    if (*s == 'F') {
      *va_arg (argp, int *) = FALSE;   
      return (TRUE);
    }
  }

  /* no valid mode found */
  return (FALSE);
}

# define HIERARCH_LENGTH 71
int gfits_vscan_hierarch (Header *header, char *field, char *mode, int N, va_list argp) {

  char *p, *q, *s, tmp[128];
  int Nchar, Nfield;
  long long value;
  double fvalue;
  
  /* find the correct line with field */
  p = gfits_header_hierarch_field (header, field, N);
  if (p == NULL) return (FALSE);

  /* non-data entries (COMMENT, HISTORY) are not allowed */
  if (!strcmp (mode, "%S")) {
    return (FALSE);
  }

  /* all others require '=' in column p + 1 + strlen(field) */
  Nfield = strlen (field);
  if (p[Nfield + 1] != '=') return (FALSE);

  /* comment from data line */
  if (!strcmp (mode, "%C")) {
    q = gfits_hierarch_keyword_end (p, field);
    if (!q) return (FALSE);
    q += 3;
    q = MIN (p + HIERARCH_LENGTH, q);
    bzero (tmp, 81);
    Nchar = MAX (0, MIN (HIERARCH_LENGTH, p + HIERARCH_LENGTH - q));
    memcpy (tmp, q, Nchar);
    stripwhite (tmp);
    strcpy (va_arg (argp, char *), tmp);
    return (TRUE);
  }

  s = gfits_hierarch_keyword_start (p, field); /* points at first char (not ') */
  q = gfits_hierarch_keyword_end (p, field); /* points at following space or ' */

  // these are invalid conditions:
  if (s - p > 80) return FALSE;
  if (q - p > 80) return FALSE;

  /* boolean data, requires int target */
  if (!strcmp (mode, "%t")) {
    if (*s == 'T') {
      *va_arg (argp, int *) = TRUE;
      return (TRUE);
    }
    if (*s == 'F') {
      *va_arg (argp, int *) = FALSE;   
      return (TRUE);
    }
  }

  /* extract data into char array (exclude containing ' chars) */
  Nchar = MIN (HIERARCH_LENGTH, MAX (0, (q - s)));
  bzero (tmp, 81);
  memcpy (tmp, s, Nchar);

  // for string types, copy the data to the target pointer
  if (!strcmp (mode, "%s")) {
    stripwhite (tmp);
    strcpy (va_arg (argp, char *), tmp);
    return (TRUE);
  }
  
  /* remaining options are numerical data */
  /* need to interpret 1.0d5 as 1.0e5 */

  if (!strcmp (mode, "%f") || !strcmp (mode, "%lf")) {
    fvalue = strtod (tmp, &q);
    if ((*q == 'd') || (*q == 'D')) fvalue *= pow (10.0, atof (q + 1));

    if (!strcmp (mode, "%f"))   { *va_arg (argp, float *)  = fvalue; return (TRUE); }
    if (!strcmp (mode, "%lf"))  { *va_arg (argp, double *) = fvalue; return (TRUE); }
  }

  value = strtoll (tmp, &q, 0);
  if ((*q == 'd') || (*q == 'D')) value *= pow (10.0, atof (q + 1));

  if (!strcmp (mode, "%d"))   	 { *va_arg (argp, int *)       	        = value; return (TRUE); }
  if (!strcmp (mode, "%ld"))  	 { *va_arg (argp, long *)      	        = value; return (TRUE); }
  if (!strcmp (mode, OFF_T_FMT)) { *va_arg (argp, off_t *) 	        = value; return (TRUE); }
  if (!strcmp (mode, "%Ld"))     { *va_arg (argp, long long *) 	        = value; return (TRUE); }
  if (!strcmp (mode, "%u"))      { *va_arg (argp, unsigned *)  	        = value; return (TRUE); }
  if (!strcmp (mode, "%lu"))     { *va_arg (argp, unsigned long *)      = value; return (TRUE); }
  if (!strcmp (mode, "%llu"))    { *va_arg (argp, unsigned long long *) = value; return (TRUE); }
  if (!strcmp (mode, "%Lu"))     { *va_arg (argp, unsigned long long *) = value; return (TRUE); }
  if (!strcmp (mode, "%hd"))     { *va_arg (argp, short *)     	        = value; return (TRUE); }
  if (!strcmp (mode, "%jd"))     { *va_arg (argp, intmax_t *) 	        = value; return (TRUE); }

  /* no valid mode found */
  return (FALSE);
}

/* if the variable argument stuff breaks on another system, look 
at F_modify.c as it has the old code */
