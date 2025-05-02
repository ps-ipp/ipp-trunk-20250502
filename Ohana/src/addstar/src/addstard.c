# include "addstar.h"

int main (int argc, char **argv) {

  int status, InitSocket, BindSocket;
  SockAddress Address;
  IOBuffer message;
  AddstarClientOptions options;

  SetSignals ();
  options = ConfigInit (&argc, argv);
  args_server (argc, argv);

  /* store the sky table in a global for internal use */
  ServerSky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (ServerSky, CATDIR, "cpt");

  /* if we separate the incoming data from db update, spawn db thread here */

  VERBOSE = TRUE;
  InitSocket = InitServerSocket (&Address);
  
  while (1) {

    /* wait for clients to make connection */
    BindSocket = WaitServerSocket (InitSocket, &Address, VALID_IP, NVALID_ID);
    if (BindSocket == -1) continue;

    /* validate : wait for password */
    if (!CheckPassword (BindSocket)) continue;
    
    /* accept command : XXX EAM : long-enough timeout? */
    status = ExpectCommand (BindSocket, 5, 0.1, &message);
    if (status != 0) {
      if (VERBOSE) fprintf (stderr, "failed connection\n");
      FreeIOBuffer (&message);
      close (BindSocket);
      continue;
    }

    /* message options */
    if (!strcmp (message.buffer, "IMAGE")) {
      fprintf (stderr, "Image\n");
      NewImage (BindSocket);
      continue;
    }
    if (!strcmp (message.buffer, "REFLS")) {
      fprintf (stderr, "Reflist\n");
      NewReflist (BindSocket);
      continue;
    }
    if (!strcmp (message.buffer, "REFCT")) {
      fprintf (stderr, "Refcat\n");
      NewRefcat (BindSocket);
      continue;
    }
    if (!strcmp (message.buffer, "EXIT")) {
      fprintf (stderr, "Exit\n");
      exit (2);
    }
  }    
  exit (1);
}
