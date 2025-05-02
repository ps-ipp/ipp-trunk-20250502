# include "lightcurve.h"

int parse (X, NX, line)
double *X;
int NX;
char *line;
{

  int i;
  char *word;
  char *ptr;

  word = line;
  for (i = 0; i < NX - 1; i++)
    word = nextword (word);

  *X = strtod (word, &ptr);
  if (ptr == word)
    return (FALSE);
  else
    return (TRUE);
}


char *nextword(string)
char *string;
{
  if (string == (char *) NULL) return ((char *) NULL);

  for (; isspace (*string); string++);
  for (; (*string != 0) && !isspace (*string); string++);
  for (; isspace (*string); string++);
  return (string);
}

