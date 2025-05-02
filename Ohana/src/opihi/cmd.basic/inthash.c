# include "basic.h"

int inthash (int argc, char **argv) {

  int N, modulus, value, input;
  char *varName;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: inthash (word) (modulus) [-var value]\n");
    gprint (GP_ERR, "  treats the word as an integer when applying the modulus\n");
    return (FALSE);
  }

  modulus = atoi(argv[2]);
  if (modulus > 255) {
    gprint (GP_ERR, "for the moment, (modulus) is limited to 255\n");
    return (FALSE);
  }

  input = atoi(argv[1]);
  value = input % modulus;
  
  if (varName) {
    set_int_variable (varName, value);
  } else {
    gprint (GP_LOG, "%d\n", value);
  }

  return (TRUE);
}
