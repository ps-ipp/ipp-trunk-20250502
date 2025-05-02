# include "imregister.h"

void ConfigFilter () {

  int i, N, code, NFILT, Nfilt, Nfield;
  char *c, line[256], name[64];
  FILE *f;

  /* open filter list file */
  f = fopen (FilterList, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "error reading photcodes\n");
    exit (1);
  }

  /* allocate dataspace needed */
  NFILT = 100;
  Nfilt = 0;
  ALLOCATE (filternum,  int,    NFILT);
  ALLOCATE (filtername, char *, NFILT);
  for (i = Nfilt; i < NFILT; i++) {
    ALLOCATE (filtername[i], char, 64);
  }

  while (scan_line (f, line) != EOF) {
    for (c = line; isspace (*c); c++);
    if (*c == '#') continue;
    Nfield = sscanf (c, "%d %s", &code, name);
    if (Nfield != 2) { continue; }

    filternum[Nfilt] = code;
    strcpy (filtername[Nfilt], name);
    Nfilt ++;

    if (Nfilt == NFILT - 1) {
      NFILT += 100;
      REALLOCATE (filternum,  int,    NFILT);
      REALLOCATE (filtername, char *, NFILT);
      for (i = Nfilt; i < NFILT; i++) {
	REALLOCATE (filtername[i], char, 64);
      }
    }
  }
  fclose (f);

  /* make filter hash table (using first available entries) */
  ALLOCATE (filterhash, char *, Nfilt);
  for (i = 0; i < Nfilt; i++) {
    filterhash[i] = (char *) NULL;
  }

  for (i = 0; i < Nfilt; i++) {
    N = filternum[i];
    if (filterhash[N] != (char *) NULL) continue;
    filterhash[N] = filtername[i];
  }

  NFILTER = Nfilt;
  /* we now have NFILTER set, and filternum & filtername arrays filled */

}  

/* convert filter string to fixed filter names (convert all spaces to .) */
int MatchFilterList (char *line) {

  char *p;
  int i, blank;

  /* convert spaces to . */
  blank = FALSE;
  p = line;
  for (i = 0; i < strlen (line); i++, p++) {
    *p = line[i];
    if (OHANA_WHITESPACE(line[i])) { 
      *p = '.';
      if (blank) p--;
      if (!blank) blank = TRUE;
    } else {
      blank = FALSE;
    }
  }
  *p = 0;

  /* find defined filter name */
  for (i = 0; i < NFILTER; i++) {
    if (!strcasecmp (line, filtername[i])) {
      /* careful: line[80] */
      strcpy (line, filterhash[filternum[i]]);
      return (TRUE);
    }
  }      
  fprintf (stderr, "unknown filter %s\n", line);
  return (FALSE);
}
