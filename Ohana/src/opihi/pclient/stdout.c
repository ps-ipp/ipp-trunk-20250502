# include "pclient.h"

int stdout_pclient (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: stdout\n");
    gprint (GP_LOG, "STATUS %d\n", -1);
    return (FALSE);
  }
  
  fwrite (child_stdout.buffer, 1, child_stdout.Nbuffer, stdout);
  gprint (GP_LOG, "STATUS %d\n", 0);
  return (TRUE);

}

int stderr_pclient (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: stderr\n");
    gprint (GP_LOG, "STATUS %d\n", -1);
    return (FALSE);
  }
  
  fwrite (child_stderr.buffer, 1, child_stderr.Nbuffer, stdout);
  gprint (GP_LOG, "STATUS %d\n", 0);
  return (TRUE);

}
