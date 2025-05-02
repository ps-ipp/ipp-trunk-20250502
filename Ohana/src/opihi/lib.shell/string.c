# include "opihi.h"

/**********************************************************************/
/* returns a pointer to an isolated string containing the first word,
   removing leading WHITESPACE.  A "word" is a contiguous set of 
   characters from the set: alphanumerics, and any of: / . _ -
   Any other single, non-WHITESPACE characters are considered to be 
   complete words in themselves.  Any characters surrounded by quotes 
   make a single word 
*/

char *thisword (char *string) {

  int i, j, N;
  char *word;

  if (string == (char *) NULL) return ((char *) NULL);

  for (i = 0; OHANA_WHITESPACE (string[i]); i++);
  if (string[i] == 0) return ((char *)NULL);
  if (string[i] == ';') {
    word = strncreate (&string[i], 1);
    return (word);
  }

  /* a string of characters contained within double quotes is 
      a single entry.  check that there are pairs of quotes,
      return only the string between the quotes */
  if (string[i] == '"') { 
    i++;
    if (string[i] == 0) return ((char *) NULL);
    for (j = i; (string[j] != 0) && (string[j] != '"'); j++);
    if (string[j] == 0) {
      gprint (GP_ERR, "misbalanced quotes\n");
      return ((char *)NULL);
    }
    //    for (; (string[j] != 0) && (!OHANA_WHITESPACE(string[j])); j++);
    word = strncreate (&string[i], j - i);
    return (word);
  } 

  /* a string of characters contained within parentheses is 
      a single entry.  check that there are matched pairs of
      parentheses, return string, including exterior parentheses */
  if (string[i] == '(') { 
    i++;
    N = 1;
    if (string[i] == 0) return ((char *) NULL);
    for (j = i; (string[j] != 0) && (N > 0); j++) {
      if (string[j] == '(') {
	N++;
      }
      if (string[j] == ')') {
	N--;
      }
    }
    if ((string[j] == 0) && (N != 0)) {
      gprint (GP_ERR, "misbalanced parenthesis\n");
      return ((char *)NULL);
    }
    word = strncreate (&string[i-1], j - i + 1);
    return (word);
  } 


  for (j = i; (string[j] != 0) && (string[j] != ';') && !OHANA_WHITESPACE(string[j]); j++);
  word = strncreate (&string[i], j - i);
  return (word);

}

/* returns a pointer to an isolated string containing the first command,
   removing leading WHITESPACE.  A command ends with the first non WHITESPACE */
char *thiscomm (char *string) {

  int i, j;
  char *word;

  if (string == (char *) NULL) return ((char *) NULL);

  for (i = 0; OHANA_WHITESPACE (string[i]); i++);
  if (string[i] == 0) return ((char *)NULL);

  for (j = i; ((string[j] != 0) && !OHANA_WHITESPACE (string[j])); j++);
  if (i == j) return ((char *) NULL);

  word = strncreate (&string[i], j - i);
  return (word);

}

/* take a pointer to the beginning of a variable (ie $foo)
and extract only the variable name (eg, foo) */

char *thisvar (char *string) {

  int i, start;
  char *word;

  if (string == (char *) NULL) return ((char *) NULL);
  if (string[0] != '$') return ((char *) NULL);

  /* special case $?name : check that name is valid */
  start = 1;
  if (string[1] == '?') start = 2;

  for (i = start; ISVAR(string[i]); i++);
  if (i == start) return ((char *) NULL);

  /* the ? is part of the variable */
  word = strncreate (&string[1], i - 1);
  return (word);

}

/* take a pointer to the beginning of a variable (ie $foo)
and extract only the variable name (eg, foo) */

char *thisref (char *string) {

  int i, start;
  char *word;

  if (string == (char *) NULL) return ((char *) NULL);
  if (string[0] != '$') return ((char *) NULL);

  /* special case $?name : check that name is valid */
  start = 1;
  if (string[1] == '?') start = 2;

  for (i = start; ISREF(string[i]); i++);
  if (i == start) return ((char *) NULL);

  /* the ? is part of the variable */
  word = strncreate (&string[1], i - 1);
  return (word);

}

/**********************************************************************/
/* returns a pointer to the next word, or (char *) NULL if there is not a next word */
char *nextword (char *string) {

  int i, j;

  if (string == (char *) NULL) return ((char *) NULL);

  for (i = 0; OHANA_WHITESPACE (string[i]); i++);
  if (string[i] == 0) return ((char *)NULL);

  if (string[i] == '"') { 
    i++;
    if (string[i] == 0) return ((char *) NULL);
    for (; (string[i] != 0) && (string[i] != '"'); i++);
    if (string[i] == 0) {
      gprint (GP_ERR, "misbalanced quotes\n");
      return ((char *)NULL);
    }
    i++;
    for (; (string[i] != 0) && OHANA_WHITESPACE (string[i]); i++);
    if (string[i] == 0) return ((char *) NULL);
    return (&string[i]);
  } 

  if (string[i] == '(') { 
    i++; 
    j = 1;
    if (string[i] == 0) return ((char *) NULL);
    for (; (string[i] != 0) && (j > 0); i++) {
      if (string[i] == '(') {
	j++;
      }
      if (string[i] == ')') {
	j--;
      }
    }
    if ((string[i] == 0) && (j != 0)) {
      gprint (GP_ERR, "misbalanced parenthesis\n");
      return ((char *)NULL);
    }
    for (; (string[i] != 0) && OHANA_WHITESPACE (string[i]); i++);
    if (string[i] == 0) return ((char *) NULL);
    return (&string[i]);
  } 

  if (string[i] == ';') i++;
  for (; (string[i] != 0) && (string[i] != ';') && !OHANA_WHITESPACE(string[i]); i++);
  for (; (string[i] != 0) && OHANA_WHITESPACE (string[i]); i++);
  if (string[i] == 0) return ((char *) NULL);

  return (&string[i]);
}

/* returns a pointer to the next command, or (char *) NULL 
   if there is not a next command.  A command is bounded by WHITESPACE */
char *nextcomm (char *string) {

  int i;

  if (string == (char *) NULL) return ((char *) NULL);
  
  for (i = 0; (string[i] != 0) && !OHANA_WHITESPACE (string[i]); i++);
  if (string[i] == 0) return ((char *) NULL);
  
  for (; OHANA_WHITESPACE (string[i]); i++);
  if (string[i] == 0) return ((char *) NULL);

  return (&string[i]);
}

/* returns a pointer to the previous word,
   or (char *) NULL if there is not a previous word
*/
char *lastword (char *string, char *c) {

  if (string == (char *) NULL) return ((char *) NULL);
  if (c == (char *) NULL) return ((char *) NULL);

  for (; !OHANA_WHITESPACE(*c) && (c >= string); c--);
  if (c < string) return ((char *)NULL);

  for (; OHANA_WHITESPACE(*c) && (c >= string); c--);
  if (c < string)
    return ((char *)NULL);
  for (; !OHANA_WHITESPACE(*c) && (c >= string); c--);
  c++;
  return (c);
}



/* take a pointer to the beginning of a variable (ie $fred) and return
   a pointer to the next thing (non WHITESPACE) which is not part of the
   variable extract only the variable name */

char *aftervar (char *string) {

  int i, j, start;

  if (string == (char *) NULL) return ((char *) NULL);
  if (string[0] != '$') return ((char *) NULL);

  /* special case: $?name : test only name */
  start = 1;
  if (string[1] == '?') start = 2;

  for (i = start; ISVAR(string[i]); i++);
  if (i == start) return ((char *) NULL);

  for (j = i; OHANA_WHITESPACE (string[j]); j++);
  if (string[j] == 0) return ((char *)NULL);

  return (&string[j]);

}


/* returns a pointer to the previous var, 
   or (char *) NULL if there is not a previous word
*/
char *lastvar (char *string, char *c) {

  if (string == (char *) NULL) return ((char *) NULL);
  if (c == (char *) NULL) return ((char *) NULL);

  for (; (c >= string) && OHANA_WHITESPACE(*c); c--);
  if (c < string) return ((char *)NULL);

  for (; (c >= string) && ISVAR(*c) ; c--);
  if ((c < string) || (*c != '$')) return ((char *)NULL);

  return (c);
}

// append string defined by range start - stop to the end of output, which
// currently has an allocated size of Noutput.  if this operation would overshoot Noutput, 
// output is reallocated to a sufficiently large size
char *opihi_append (char *output, int *Noutput, char *start, char *stop) {

  int N1, N2, outlen;

  // a NULL end pointer means 'go to end of line'
  if (stop == NULL) {
    stop = start + strlen(start);
  }

  // enough space?
  N1 = strlen(output);
  N2 = stop - start;
  outlen = N1 + N2;
  if (outlen >= *Noutput) {
    *Noutput = outlen + 128;
    REALLOCATE (output, char, *Noutput);
    memset (&output[N1], 0, N2 + 128);
  }

  strncat (output, start, stop - start);
  return output;
}

char *opihi_readline (char *prompt) {

# ifdef OHANA_MEMORY
    char *raw = readline (prompt);
    char *line = strcreate (raw);
    real_free (raw);
    return line;
# else
    char *line = readline (prompt);
    return line;
# endif
}

/* replace all instances of \A with A in line */
void interpolate_slash (char *line) {

  char *in, *out;

  for (in = out = line; *in != 0; in++, out++) {
    if (*in == 0x5c) in++;
    *out = *in;
    if (*in == 0) return;
  }
  *out = *in;
}

// paste together argv[0] .. argv[N] into a single string
char *paste_args (int argc, char **argv) {

  int i;

  int length = 0;
  for (i = 0; i < argc; i++) {
    length += strlen(argv[i]) + 1;
  }
  
  char *string = NULL;
  ALLOCATE (string, char, length);
  string[0] = 0;
  for (i = 0; i < argc; i++) {
    strcat (string, argv[i]);
    if (i < argc - 1) strcat (string, " ");
  }
  return string;
}

int set_list_varname (char *line, char *base, int N, int excelStyle) {

  int i;
    
  // A-Z correspond to 0 - 25

  if (excelStyle) {
    float f = log(26.0);
    float g = (N == 0) ? 0.0 : log(1.0*N);
    int Ndigit = (int) (g / f) + 1;
    if (Ndigit > 10) {
      sprintf (line, "%s:ZZZZZZZZZZ", base);
      return FALSE;
    }
    char name[12];
    memset (name, 0, 12);
    for (i = 0; i < Ndigit; i++) {
      float Npow = Ndigit - i - 1;
      float g = pow(26.0, Npow);
      int V = (int) (N / g);
      name[i] = (Npow == 0.0) ? 'A' + V : 'A' + V - 1;
      N -= V * g;
    }
    sprintf (line, "%s:%s", base, name);
  } else {
    sprintf (line, "%s:%d", base, N);
  }
  return TRUE;
}
