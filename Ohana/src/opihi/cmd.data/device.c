# include "data.h"

int device (int argc, char **argv) {

  int N, kapa;
  char *name;;
  /* set / get current graphics device */

  int QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    remove_argument (N, &argc, argv);
    QUIET = TRUE;
  }

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (name == NULL) {
    name = GetKapaName ();
    if (name == NULL) {
      gprint (GP_ERR, "no device defined\n");
      return (FALSE);
    }
  } else {
    if (!GetGraph (NULL, &kapa, name)) return (FALSE);
  }
  if (!QUIET) gprint (GP_ERR, "kapa %s\n", name); 

  return (TRUE);
}
