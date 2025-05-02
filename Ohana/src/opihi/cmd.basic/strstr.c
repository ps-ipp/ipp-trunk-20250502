# include "basic.h"

int strstr_func (int argc, char **argv) {

  int N;

  int VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-q"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = FALSE;
  }

  int start = -1;
  char *startName = NULL;
  if ((N = get_argument (argc, argv, "-start"))) {
    remove_argument (N, &argc, argv);
    startName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int length = 0;
  char *lengthName = NULL;
  if ((N = get_argument (argc, argv, "-length"))) {
    remove_argument (N, &argc, argv);
    lengthName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* returns range of the requested string */ 
  if (argc != 3) {
    gprint (GP_ERR, "USAGE: strstr (haystack) (needle) [-start (var)] [-length (var)]\n");
    gprint (GP_ERR, "  return the starting position and length of needle in haystack\n");
    return (FALSE);
  }

  char *c = strstr (argv[1], argv[2]);

  // set the result if we found the needle
  if (c != NULL) {
    start = c - argv[1];
    length = strlen(argv[2]);
  }

  if (startName) set_variable (startName, start);
  if (lengthName) set_variable (lengthName, length);

  if (VERBOSE) gprint (GP_ERR, "start: %d, length: %d\n", start, length);

  return (TRUE);
}
