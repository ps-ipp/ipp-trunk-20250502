# include "pantasks.h"

int server_shutdown (int argc, char **argv) {

  int status, server;
  IOBuffer message;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: shutdown now\n");
    return (FALSE);
  }
  if (strcmp (argv[1], "now")) {
    gprint (GP_ERR, "USAGE: shutdown now\n");
    return (FALSE);
  }
  
  server = getServer ();
  
  if (!server) return (TRUE);

  // send the command to the server instead
  status = SendMessage (server, "exit");
  fprintf (stderr, "exit status of %d\n", status);

  // try to read from the server until we get a status of 0
  status = ExpectMessage (server, 2.0, &message);
  fprintf (stderr, "message status of %d\n", status);

  // XXX I should probably loop until we get an exit status of -3 or EPIPE

  // close connection with server
  multicommand_StopServer ();

  return (TRUE);
}

