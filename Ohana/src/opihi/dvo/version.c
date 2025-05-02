# include "dvoshell.h"
static char *name = "$Name: not supported by cvs2svn $";

int version (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  char *tmp;

  gprint (GP_LOG, "\n");
  gprint (GP_LOG, "dvo version: %s\n", (tmp = strip_version (name))); free (tmp);

  gprint (GP_LOG, "opihi version: %s\n", (tmp = strip_version (opihi_version()))); free (tmp);
  gprint (GP_LOG, "libohana version: %s\n", (tmp = strip_version (ohana_version()))); free (tmp);
  gprint (GP_LOG, "libdvo version: %s\n", (tmp = strip_version (libdvo_version()))); free (tmp);
  gprint (GP_LOG, "libfits version: %s\n", (tmp = strip_version (gfits_version()))); free (tmp);

  gprint (GP_LOG, "compiled on %s %s\n", __DATE__, __TIME__);
  return (TRUE);
}
