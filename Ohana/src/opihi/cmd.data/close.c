# include "data.h"

int close_device (int argc, char **argv) {

  int N, kapa;
  char *name;
  /* close current graphics device */

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (!GetGraph (NULL, &kapa, name)) return (FALSE);

  close_kapa (name); 
  FREE (name);

  return (TRUE);
}
