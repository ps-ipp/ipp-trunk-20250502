# include "basic.h"

int basename_opihi (int argc, char **argv) {

  int N;
  char *baseName, *varName, *suffixName;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  suffixName = NULL;
  if ((N = get_argument (argc, argv, "-suffix"))) {
    remove_argument (N, &argc, argv);
    suffixName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: dirname (path) [-var name] [-suffix suffix]\n");
    return (FALSE);
  }

  baseName = filebasename (argv[1]);

  // strip suffix, if supplied
  if (suffixName != NULL) {
      if (strlen(baseName) > strlen(suffixName)) {
	  char *ptr = baseName + strlen(baseName) - strlen(suffixName);
	  if (!strcmp (ptr, suffixName)) {
	      *ptr = 0;
	  }
      }
  }

  if (varName == NULL) {
    gprint (GP_LOG, "%s\n", baseName);
  } else {
    set_str_variable (varName, baseName);
    free (varName);
  }    

  free (baseName);
  return (TRUE);
}

// XXX need to add mode option
// XXX need to respect umask (need umask command?)
