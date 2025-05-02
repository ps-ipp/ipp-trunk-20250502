# include "imregister.h"
static char *name = "$Name: not supported by cvs2svn $";

void get_version (int argc, char **argv, char *version) {

  if (!get_argument (argc, argv, "-version")) return;

  fprintf (stderr, "%s\n", version);
  fprintf (stderr, "%s\n", name);

  fprintf (stderr, "ohana: %s\n", ohana_version());
  fprintf (stderr, "fits:  %s\n", gfits_version());
  exit (2);

}
