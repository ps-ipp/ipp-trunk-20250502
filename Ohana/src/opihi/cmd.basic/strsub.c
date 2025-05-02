# include "basic.h"

int strsub (int argc, char **argv) {

  int N, Nkey, Noutput;
  char *varName;
  char *p, *q, *input, *output, *key, *value;

  // clear all sections
  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: strsub (string) (key) (value) [-var out]\n");
    gprint (GP_ERR, "  replace instances of 'key' with the 'value'\n");
    return (FALSE);
  }

  // input string 
  input = argv[1];
  
  // find this in the string
  key = argv[2];
  Nkey = strlen(key);

  // replace with this
  value = argv[3];

  Noutput = MAX (16, strlen(input));
  ALLOCATE (output, char, Noutput);
  memset (output, 0, Noutput);

  // 0123456789
  // wordKEYnew
  // p = 0, q = 4, q-p = 4 -> p = 7

  p = input;
  while (*p && ((q = strstr (p, key)) != NULL)) {
    output = opihi_append (output, &Noutput, p, q);
    output = opihi_append (output, &Noutput, value, value + strlen(value));
    p = q + Nkey;
  }
  if (*p) {
    output = opihi_append (output, &Noutput, p, p + strlen(p));
  }
  
  if (varName) {
    set_str_variable (varName, output);
  } else {
    gprint (GP_LOG, "%s\n", output);
  }

  free (output);
  return (TRUE);
}
