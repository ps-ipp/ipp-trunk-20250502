# include "pantasks.h"
# define DEBUG 0

// this static var is only used by InitPassword and CheckPassword below.
// Both functions are only called by the main thread.
static char PASSWORD[256];

int InitPassword () {

  VarConfig ("PASSWORD", "%s", PASSWORD);
  return (TRUE);
}

int CheckPassword (int BindSocket) {

  IOBuffer message;
  int status;

  if (DEBUG) gprint (GP_ERR, "waiting for password %s\n", PASSWORD);

  status = ExpectCommand (BindSocket, strlen(PASSWORD), 0.1, &message);
  if (status != 0) {
    if (DEBUG) gprint (GP_ERR, "failed connection\n");
    FreeIOBuffer (&message);
    close (BindSocket);
    return (FALSE);
  }
  if (strncmp (message.buffer, PASSWORD, strlen(PASSWORD))) {
    if (DEBUG) gprint (GP_ERR, "invalid password\n");
    close (BindSocket);
    return (FALSE);
  }
  if (DEBUG) gprint (GP_ERR, "accepted password (%s)\n", message.buffer);
  
  return (TRUE);
}
