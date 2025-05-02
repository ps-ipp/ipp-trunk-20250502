# include "imregister.h"

/* scan and give a warning for missing entries */
void warn_scan (Header *header, char *field, char *format, int N, void *var) {
  if (!gfits_scan (header, field, format, N, var)) {
    fprintf (stderr, "WARNING: %s not found in header\n", field);
  }
}

/* scan from header into a string of fixed length */
int gfits_scan_nchar (Header *header, int size, char *field, int N,...) {

  char tmpstr[160], *outstr; 
  va_list argp;
  int status;
  
  va_start (argp, N);
  outstr = va_arg (argp, char *);
  va_end (argp);

  status = gfits_scan (header, field, "%s", N, tmpstr); 
  strncpy_nowarn (outstr, tmpstr, size - 1);

  return (status);
} 
     
void warn_scan_nchar (Header *header, int size, char *field, int N, void *var) {
  if (!gfits_scan_nchar (header, size, field, N, var)) {
    fprintf (stderr, "missing %s not found in header\n", field);
  }
}
