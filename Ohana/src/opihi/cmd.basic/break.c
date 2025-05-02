# include "basic.h"

// auto_break is currently a global
int exec_break (int argc, char **argv) {

  int N, value;

  if ((N = get_argument (argc, argv, "-h"))) goto usage;
  if ((N = get_argument (argc, argv, "-help"))) goto usage;
  if ((N = get_argument (argc, argv, "--help"))) goto usage;

  if ((N = get_argument (argc, argv, "-auto"))) {
    remove_argument (N, &argc, argv);
    if (N == argc) {
      if (auto_break) 
	gprint (GP_ERR, "auto break on\n");
      else 
	gprint (GP_ERR, "auto break off\n");
      return (FALSE);
    }
    value = -1;
    if (!strcasecmp (argv[N], "on")) value = 1;
    if (!strcasecmp (argv[N], "off")) value = 0;
    if (value == -1) goto usage;
    auto_break = value;
    return (TRUE);
  }
  
  loop_break = TRUE;
  return (FALSE);
  
usage:
  gprint (GP_ERR, "USAGE: break -auto [on / off]\n");
  return (FALSE);
}
