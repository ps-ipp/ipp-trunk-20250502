# include "basic.h"

int strhash (int argc, char **argv) {

  int i, N, Nchar, Nsum, modulus, value;
  char *varName;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: strhash (string) (modulus) [-var value]\n");
    return (FALSE);
  }

  modulus = atoi(argv[2]);
  if (modulus > 255) {
    gprint (GP_ERR, "for the moment, (modulus) is limited to 255\n");
    return (FALSE);
  }

  Nchar = strlen (argv[1]);

  Nsum = 0;
  for (i = 0; i < Nchar; i++) {
    Nsum += (argv[1][i] % modulus);
  }
  
  value = Nsum % modulus;
  
  if (varName) {
    set_int_variable (varName, value);
  } else {
    gprint (GP_LOG, "%d\n", value);
  }

  return (TRUE);
}
