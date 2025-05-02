# include "astro.h"

int getlst (int argc, char **argv) {

  int N;
  time_t time;
  double jd, lst, longitude;
  char *Variable;

  Variable = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    Variable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (argc != 3) goto syntax;

  if (!ohana_str_to_time (argv[1], &time)) {
      if (Variable != NULL) free (Variable);
      return (FALSE);
  }

  longitude = atof (argv[2]);

  jd = ohana_sec_to_jd (time);
  fprintf (stderr, "jd: %f\n", jd);

  lst = ohana_lst (jd, longitude);

  if (Variable != NULL) {
      set_variable (Variable, lst);
      free (Variable);
      return (TRUE);
  }
  gprint (GP_ERR, "lst: %f\n", lst);
  return (TRUE);

 syntax:
  gprint (GP_ERR, "USAGE: getlst (time) (longitude) [-var name]\n");
  return (FALSE);
}
