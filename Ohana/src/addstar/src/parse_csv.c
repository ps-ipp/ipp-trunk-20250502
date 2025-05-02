# include "addstar.h"
# include "gaia_dr2.h"

// I want a function which takes a pointer to the start of a line
// and an entry number N and returns the Nth element, along with the pointer
// to the following (or to th

char *dparse_csv_rpt (double *X, int Nwant, int Nlast, char *line, int *status) {

  int i;
  char *word;
  char *ptr;

  // only scan forward for NX from this point
  int NX = Nwant - Nlast + 1;

  // scan for the NX word
  word = line;
  for (i = 0; i < NX - 1; i++) {
    word = parse_nextword_csv (word);
  }
  
  if (word[0] == '"') word[0] = ' ';
  if (word[0] == ',') {
    *X = NAN;
    *status = 1;
    return word;
  }

  // return FALSE if this field is not consistent with a double
  *X = strtod (word, &ptr);

  // default result is success
  *status = 1;

  // if the leading value is '-', return -1 so we can handle -00
  if (word[0] == '-') { *status = -1; }

  // if ptr = word, then there is not actually a valid number here
  if (ptr == word) { *status = FALSE; *X = NAN; } 

  return word;
}

char *iparse_csv_rpt (int *X, int Nwant, int Nlast, char *line, int *status) {

  int i;
  char *word;
  char *ptr;

  // only scan forward for NX from this point
  int NX = Nwant - Nlast + 1;

  // scan for the NX word
  word = line;
  for (i = 0; i < NX - 1; i++) {
    word = parse_nextword_csv (word);
  }
  
  if (word[0] == '"') word[0] = ' ';
  if (word[0] == ',') {
    *X = 0;
    *status = 1;
    return word;
  }

  *X = strtol (word, &ptr, 0);

  // default result is success
  *status = 1;

  // if the leading value is '-', return -1 so we can handle -00
  if (word[0] == '-') { *status = -1; }

  // if ptr = word, then there is not actually a valid number here
  if (ptr == word) { *status = FALSE; *X = 0; } 

  return word;
}

char *jparse_csv_rpt (uint64_t *X, int Nwant, int Nlast, char *line, int *status) {

  int i;
  char *word;
  char *ptr;

  // only scan forward for NX from this point
  int NX = Nwant - Nlast + 1;

  // scan for the NX word
  word = line;
  for (i = 0; i < NX - 1; i++) {
    word = parse_nextword_csv (word);
  }
  
  if (word[0] == '"') word[0] = ' ';
  if (word[0] == ',') {
    *X = 0;
    *status = 1;
    return word;
  }

  *X = strtoll (word, &ptr, 0);

  // default result is success
  *status = 1;

  // if the leading value is '-', return -1 so we can handle -00
  if (word[0] == '-') { *status = -1; }

  // if ptr = word, then there is not actually a valid number here
  if (ptr == word) { *status = FALSE; *X = 0; } 

  return word;
}

