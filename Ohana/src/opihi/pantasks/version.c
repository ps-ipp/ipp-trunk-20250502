# include "pantasks.h"
static char *name = "$Name: not supported by cvs2svn $";

int version (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  char *tmp;

  gprint (GP_LOG, "\n");
  gprint (GP_LOG, "pantasks version: %s\n", (tmp = strip_version (name))); free (tmp);

  gprint (GP_LOG, "opihi version: %s\n", (tmp = strip_version (opihi_version()))); free (tmp);
  gprint (GP_LOG, "ohana version: %s\n", (tmp = strip_version (ohana_version()))); free (tmp);
  gprint (GP_LOG, "gfits version: %s\n", (tmp = strip_version (gfits_version()))); free (tmp);

  gprint (GP_LOG, "compiled on %s %s\n", __DATE__, __TIME__);
  return (TRUE);
}
