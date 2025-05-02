# include "gcompare.h"

char *nextword(string)
char *string;
{
  if (string == (char *) NULL) return ((char *) NULL);

  for (; isspace (*string); string++);
  for (; (*string != 0) && !isspace (*string); string++);
  for (; isspace (*string); string++);
  return (string);
}
