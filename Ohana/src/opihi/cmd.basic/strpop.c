# include "basic.h"

int strpop (int argc, char **argv) {

  char *p, *q, *string;

  if ((argc != 2) && (argc != 3)) {
    gprint (GP_ERR, "USAGE: strpop (var) [out]\n");
    gprint (GP_ERR, "   return the first word from the given variable, leaving behind the remaining words\n");
    return (FALSE);
  }

  /* string is a copy of the value on the variable stack */
  string = get_variable (argv[1]);
  if (string == NULL) return (FALSE);
  
  /* thisword is an allocated string */
  p = thisword (string);

  q = nextword (string);
  if (q == NULL) {
    set_str_variable (argv[1], "NULL");
  } else {
    set_str_variable (argv[1], q);
  }
  
  if (argc == 3) {
    set_str_variable (argv[2], p);
  } else {
    gprint (GP_LOG, "%s\n", p);
  }

  free (p);
  free (string);
  return (TRUE);
}
