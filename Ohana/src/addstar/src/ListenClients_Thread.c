# include "addstar.h"

/* wait for incoming messages from clients */
void *ListenClients_Thread (void *data) {

  int status, InitSocket, BindSocket;
  SockAddress Address;
  IOBuffer message;
  
  /* if we have multiple threads, each one creates its own socket? */
  InitSocket = InitServerSocket (&Address);
  
  while (1) {

    /* wait for clients to make connection */
    BindSocket = WaitServerSocket (InitSocket, &Address, VALID_IP, NVALID_IP);
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
      NewImage_Thread (BindSocket);
      continue;
    }
    if (!strcmp (message.buffer, "REFLS")) {
      fprintf (stderr, "Reflist\n");
      NewReflist_Thread (BindSocket);
      continue;
    }
    if (!strcmp (message.buffer, "REFCT")) {
      fprintf (stderr, "Refcat\n");
      NewRefcat_Thread (BindSocket);
      continue;
    }
    if (!strcmp (message.buffer, "EXIT")) {
      /* need to send this signal to the main thread */
      fprintf (stderr, "Exit\n");
      exit (2);
    }
  }    
  exit (1);
}
