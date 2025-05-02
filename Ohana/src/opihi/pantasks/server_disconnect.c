# include "pantasks.h"

int server_disconnect (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  int server;

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: disconnect\n");
    return (FALSE);
  }
  
  server = getServer ();
  
  if (!server) return (TRUE);

  // close connection with server
  multicommand_StopServer ();

  return (TRUE);
}
