# include "Ximage.h"

/************** CheckDisplayName *************/
char *CheckDisplayName (int *argc, char **argv) {

  char *name = NULL;
  int N;

  if ((N = get_argument (*argc, argv, "-d"))) {
    if (*argc <= N + 1) {
      fprintf (stderr, "error: usage is [-display/-d] DisplayName\n");
      exit (2);
    }
    remove_argument(N, argc, argv);
    name = strcreate (argv[N]);
    remove_argument(N, argc, argv);
    return (name);
  }

  if ((N = get_argument (*argc, argv, "-display"))) {
    if (*argc <= N + 1) {
      fprintf (stderr, "error: usage is [-display/-d] DisplayName\n");
      exit (2);
    }
    remove_argument(N, argc, argv);
    name = strcreate (argv[N]);
    remove_argument(N, argc, argv);
    return (name);
  }
  return (NULL);
}
