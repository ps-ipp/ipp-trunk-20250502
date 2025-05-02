# include "nightd.h"

/* special words:
   word    variable
   DATE    DateStr
   TIME    TimeStr
*/

int DoCommand (char *command, char *name) {
  
  char *line;

  line = strcreate (command);
  line = ExpandWords (line);
  if (pcommand (line, TIMEOUT)) {
    fprintf (LogFile, "error running %s command\n", name);
    fflush (LogFile);
  }
  free (line);
  return (TRUE);
}
