# include "lightcurve.h"

/*************/
int
scan_line (f, line) 
FILE *f;
char line[];
{

  int i, status;
  char c;
  
  status = EOF + 1;
  
  for (i = 0, c = 0; (c != '\n') && (status != EOF); i++) {
    status = fscanf (f, "%c", &c);
    line[i] = c;
  }
  line[i - 1] = 0;  /* this could make things crash! */

  if (i > 1) {
    status = EOF + 1;
  }

  return (status);

}

