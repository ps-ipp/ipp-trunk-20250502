# include "addstar.h"

int CheckPassword (int BindSocket) {

  IOBuffer message;
  int status;

  status = ExpectCommand (BindSocket, strlen(PASSWORD), 0.1, &message);
  if (status != 0) {
    if (VERBOSE) fprintf (stderr, "failed connection\n");
    FreeIOBuffer (&message);
    close (BindSocket);
    return (FALSE);
  }
  if (strncmp (message.buffer, PASSWORD, strlen(PASSWORD))) {
    if (VERBOSE) fprintf (stderr, "invalid password\n");
    close (BindSocket);
    return (FALSE);
  }
  
  return (TRUE);
}
