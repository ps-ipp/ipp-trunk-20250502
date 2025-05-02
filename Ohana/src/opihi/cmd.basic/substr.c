# include "basic.h"

int substr_func (int argc, char **argv) {

  int N1, N2, len;
  char *c, *string;

  if ((argc != 4) && (argc != 5)) {
    gprint (GP_ERR, "USAGE: substr (string) Nstart Nlength [var]\n");
    return (FALSE);
  }

  N1 = atof (argv[2]);
  N2 = atof (argv[3]);

  // add a range check here
  if ((N1 < 0) || (N1 >=  strlen(argv[1]))) {
      gprint (GP_ERR, "ERROR: start value out of range in substr command\n");
      return (FALSE);
  }
  if ((N2 < 0) || ((N2+N1) >  strlen(argv[1]))) {
      gprint (GP_ERR, "ERROR: length out of range in substr command\n");
      return (FALSE);
  }

  len = strlen (argv[1]);
  if ((N1 >= len) || (N1 + N2 > len)) {
    c = (char *) NULL;
  } else {
    c = strncreate (&argv[1][N1], N2);
  }

  if (c == (char *) NULL) {
    string = strcreate ("");
  } else {
    string = strcreate (c);
  }

  if (argc == 5) {
    set_str_variable (argv[4], string);
  } else {
    gprint (GP_ERR, "%s\n", string);
  }
  free (c);
  free (string);
  return (TRUE);

}
