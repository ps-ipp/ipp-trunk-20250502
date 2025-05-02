# include <ohana.h>

// several snprintf statements below my truncate their output
// gcc (since 8.1) warns if the output may be truncated.
// the following NOOP function is used to fool the compiler
// see snprintf_nowarn in ohana.h
void myNOOP (void) { }

/* Strip WHITESPACE from the start and end of STRING. */
int stripwhite (char *string) {

  int i;

  if (string == (char *) NULL) return (FALSE);

  for (i = 0; OHANA_WHITESPACE (string[i]); i++);
  if (i) memmove (string, string + i, strlen(string+i)+1);
  for (i = strlen (string) - 1; (i > 0) && OHANA_WHITESPACE (string[i]); i--);
  string[++i] = 0;
  return (i);
}

/* compare two strings either as strings, or as numbers if both are
   pure numeric strings (base 10) */
int strnumcmp (char *str1, char *str2) {

  char *end1;
  char *end2;
  int num1, num2;
  int value;
 
  value = FALSE;
  num1 = strtol (str1, &end1, 10);
  num2 = strtol (str2, &end2, 10);

  if (!end1[0] && !end2[0] && (num1 == num2)) {
    value = TRUE;
  }
  if (!strcmp (str1, str2)) {
    value = TRUE;
  }
  
  return (value);
}

// gcc (Ubuntu 20.04) complains when strncpy is used to copy a fraction of a buffer,
// potentially skipping the ending NULL.  To avoid the error, manually copy
// and (to ensure the buffer ends in a NULL) set the last + 1 byte to NUL: 
// WARNING : len(dest) must be >= n + 1
char *strncpy_nowarn (char *dest, char *src, size_t n) {

  size_t i;
  
  char *d = dest;
  char *s = src;
  for (i = 0; i < n && *s != 0; i++, d++, s++) { *d = *s; }
  for ( ; i <= n; i++, d++) { *d = 0; }
  
  return dest;
}

/* create a new string from this string */
char *strcreate (char *string) {

  char *line;

  if (string == (char *) NULL) return ((char *) NULL);
  
  ALLOCATE (line, char, MAX (1, strlen(string)) + 1);
  line = strcpy (line, string);

  return (line);
}

/* create a new string of length n from this string */
char *strncreate (char *string, int n) {

  char *line;

  if (string == (char *) NULL) return ((char *) NULL);
  if (n < 0) return NULL;
  
  ALLOCATE (line, char, n + 1);
  memcpy (line, string, n);
  line[n] = 0;
  return (line);
}

// extend input as needed to add the formatted pieces
// if input is NULL, allocate a new line
// the result of the format must be < 1024 bytes
int strextend (char **input, char *format,...) {

  int Nchar;
  char tmpextra[1024], tmpline, *output;
  va_list argp;

  va_start (argp, format);
  Nchar = vsnprintf (tmpextra, 1024, format, argp);
  if (Nchar > 1024 - 1) return FALSE;

  if (*input) {
    Nchar = snprintf (&tmpline, 0, "%s %s", *input, tmpextra);
    ALLOCATE (output, char, Nchar + 1);
    snprintf (output, Nchar + 1, "%s %s", *input, tmpextra);
    free (*input);
  } else {
    Nchar = strlen(tmpextra) + 1;
    ALLOCATE (output, char, Nchar);
    strcpy (output, tmpextra);
  }
  *input = output;

  return TRUE;
}

// replace a single entry of 'match' in the string with 'with'
// (quick-and-dirty regex for a common case...)
char *strsubs (char *string, char *match, char *with) {

  int N1, N2, N3;
  char *root;
  char *out;
  char *ext;

  if (string == NULL) return NULL;
  if (match == NULL) return NULL;
  if (with == NULL) return NULL;

  root = strstr (string, match);
  if (root == NULL) {
    return (strcreate (string));
  }
  N1 = root - string;

  N2 = strlen (with);

  ext = root + strlen(match);
  N3 = strlen (ext);

  ALLOCATE (out, char, N1 + N2 + N3 + 1);

  strncpy_nowarn (&out[0],     string, N1);
  strncpy_nowarn (&out[N1],    with,   N2);
  strncpy_nowarn (&out[N1+N2], ext,    N3);

  return out;
}

# if 0
// replace a single entry of 'match' in the string with 'with'
// (quick-and-dirty regex for a common case...)
char *strrsubs (char *string, char *match, char *with) {

  char *root;
  char *out;

  if (string == NULL) return NULL;
  if (match == NULL) return NULL;
  if (with == NULL) return NULL;

  root = strstr (string, match);
  if (root == NULL) return NULL;

  ext = string + strlen (match);
  ALLOCATE (out, char *, strlen(with) + strlen(ext) + 1);
  strcpy (out, with);
  strcat (out, ext);

  return out;
}
# endif

int scan_line (FILE *f, char *line) {

  int i, status;
  char c;
  
  status = EOF + 1;
  
  for (i = 0, c = 0; (c != '\n') && (c != '\r') && (status != EOF); i++) {
    status = fscanf (f, "%c", &c);
    line[i] = c;
  }
  if (ferror(f)) {
    perror("scan_line: ");
    return EOF;
  }
  line[i - 1] = 0;  /* this could make things crash! */

  if (i > 1) {
    status = EOF + 1;
  }

  return (status);
}

int scan_line_maxlen (FILE *f, char *line, int maxlen) {

  int i, status;
  char c;
  
  status = EOF + 1;
  
  for (i = 0, c = 0; (c != '\n') && (c != '\r') && (status != EOF) && (i < maxlen); i++) {
    status = fscanf (f, "%c", &c);
    line[i] = c;
  }
  if (i > 0) line[i - 1] = 0;  /* this could make things crash! */

  if (i > 1) {
    status = EOF + 1;
  }

  return (status);
}

char *parse_nextword (char *string) {

  if (string == (char *) NULL) return ((char *) NULL);

  for (; isspace (*string); string++);
  for (; (*string != 0) && !isspace (*string); string++);
  for (; isspace (*string); string++);
  return (string);
}

// advance to the next word, separated by commas:
// AA,BB,CC : go from AA to BB to CC to 0
// AA,,CC : go from AA to , to CC to 0
// ,,, : go from , to , to , to 0

char *parse_nextword_csv (char *string) {

  if (string == (char *) NULL) return ((char *) NULL);

  for (; (*string != 0) && (*string != ','); string++);
  if (*string == ',') string ++;
  return (string);
}

int dparse (double *X, int NX, char *line) {

  int i;
  char *word;
  char *ptr;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword (word);

  *X = strtod (word, &ptr);
  if (ptr == word) return (FALSE);
  if (word[0] == '-') return (-1);
  return (1);
}

int dparse_csv (double *X, int NX, char *line) {

  int i;
  char *word;
  char *ptr;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword_csv (word);
  
  if (word[0] == '"') word[0] = ' ';
  if (word[0] == ',') {
      *X = NAN;
      return 1;
  }

  *X = strtod (word, &ptr);
  if (ptr == word) return (FALSE);
  if (word[0] == '-') return (-1);
  return (1);
}

int iparse (int *X, int NX, char *line) {

  int i;
  char *word;
  char *ptr;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword (word);

  *X = strtol (word, &ptr, 0);
  if (ptr == word) return (FALSE);
  if (word[0] == '-') return (-1);
  return (1);
}

int iparse_csv (int *X, int NX, char *line) {

  int i;
  char *word;
  char *ptr;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword_csv (word);
  
  if (word[0] == '"') word[0] = ' ';
  if (word[0] == ',') {
      *X = 0;
      return 1;
  }

  *X = strtol (word, &ptr, 0);
  if (ptr == word) return (FALSE);
  if (word[0] == '-') return (-1);
  return (1);
}

int charparse (char *X, int NX, char *line) {

  int i;
  char *word;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword (word);

  *X = word[0];
  return (1);
}

int charparse_csv (char *X, int NX, char *line) {

  int i;
  char *word;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword_csv (word);
  
  if (word[0] == '"') word[0] = word[1];
  if (word[0] == ',') {
      *X = 0;
      return 1;
  }

  *X = word[0];
  return (1);
}

// return a pointer to the start of the desired field
char *ptrparse (int NX, char *line) {

  int i;
  char *word;

  word = line;
  for (i = 0; i < NX - 1; i++) {
    word = parse_nextword (word);
  }
  return word;
}

char *ptrparse_csv (int NX, char *line) {

  int i;
  char *word;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword_csv (word);
  
  if (word[0] == '"') word ++;
  if (word[0] == ',') return NULL;
  return word;
}

int tparse (time_t *X, int NX, char *line) {

  int i, status;
  char *word;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword (word);

  status = ohana_str_to_time (word, X);
  if (!status) return (FALSE);
  return (TRUE);
}

int tparse_csv (time_t *X, int NX, char *line) {

  int i, status;
  char *word;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword_csv (word);
  
  if (word[0] == '"') word[0] = ' ';
  if (word[0] == ',') {
      *X = 0;
      return 1;
  }

  status = ohana_str_to_time (word, X);
  if (!status) return (FALSE);
  return (TRUE);
}

int fparse (float *X, int NX, char *line) {

  int i;
  char *word;
  char *ptr;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = parse_nextword (word);

  *X = strtod (word, &ptr);
  if (ptr == word) return (FALSE);
  if (word[0] == '-') return (-1);
  return (1);
}

int get_argument (int argc, char **argv, char *arg) {

  int i;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], arg))
      return (i);
  }
  
  return (0);
}

int remove_argument (int N, int *argc, char **argv) {

  int i;

  if (N) {
    (*argc)--;
    for (i = N; i < *argc; i++) {
      argv[i] = argv[i+1];
    }
  }

  return (N);
}

void uppercase (char *string) {

  unsigned int i;
    
  for (i = 0; i < strlen (string); i++) string[i] = toupper (string[i]);

}

/* expect a line of the form "$Name: not supported by cvs2svn $", strip out contents */
char *strip_version (char *input) {

  char *p, *q;

  p = strstr (input, "$Name:");
  if (p == NULL) return (strcreate ("NONE"));

  q = strcreate (input + 6);
  p = strrchr (q, '$');
  if (p != NULL) *p = 0;
  stripwhite (q);
  if (*q == 0) {
    free (q);
    q = strcreate ("NONE");
  }

  return (q);
}

// return a newly allocated string containing the first complete set of non-whitespace
char *getword (char *string) {

  int i, j;
  char *word;

  if (!string) return (NULL);

  // find the end of the whitespace (is there any non-whitespace?)
  for (i = 0; OHANA_WHITESPACE (string[i]); i++);
  if (!string[i]) return (NULL);

  for (j = i; string[j] && !OHANA_WHITESPACE(string[j]); j++);
  word = strncreate (&string[i], j - i);
  return (word);
}

// returns a pointer to the next word, or NULL if there is not a next word
char *skipword (char *string) {

  int i;

  if (!string) return (NULL);

  // find the end of the whitespace (is there any non-whitespace?)
  for (i = 0; OHANA_WHITESPACE (string[i]); i++);
  if (!string[i]) return (NULL);

  // find the end of the non-whitespace (this word)
  while (string[i] && !OHANA_WHITESPACE(string[i])) i++;

  // find the end of the following whitespace
  while (string[i] &&  OHANA_WHITESPACE(string[i])) i++;
  if (!string[i]) return (NULL);

  return (&string[i]);
}

