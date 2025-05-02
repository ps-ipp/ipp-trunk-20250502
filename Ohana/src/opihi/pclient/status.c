# include "pclient.h"

int status (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  char status_string[64];

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: status\n");
    return (FALSE);
  }
  
  if (ChildStatus == PCLIENT_NONE)  strcpy (status_string, "NONE");
  if (ChildStatus == PCLIENT_BUSY)  strcpy (status_string, "BUSY");
  if (ChildStatus == PCLIENT_EXIT)  strcpy (status_string, "EXIT");
  if (ChildStatus == PCLIENT_CRASH) strcpy (status_string, "CRASH");

  gprint (GP_LOG, "STATUS %s\n", status_string);
  gprint (GP_LOG, "EXITST %d\n", ChildExitStatus);
  gprint (GP_LOG, "STDOUT %d\n", child_stdout.Nbuffer);
  gprint (GP_LOG, "STDERR %d\n", child_stderr.Nbuffer);

  set_str_variable ("JOBSTATUS", status_string);
  return (TRUE);
}
