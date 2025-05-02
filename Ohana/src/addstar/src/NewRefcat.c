# include "addstar.h"

int NewRefcat (int BindSocket) {

  int N, status;
  AddstarClientOptions *options;
  IOBuffer message;
  SkyRegion *patch;

  /* accept incoming data set */
  if (!Recv_AddstarClientOptions (BindSocket, &options, &N)) {
    fprintf (stderr, "error: problem receiving options\n");
    goto reject;
  }
  if (N != 1) {
    fprintf (stderr, "error: too many option sets (%d)\n", N);
    goto reject;
  }

  if (!Recv_SkyRegion (BindSocket, &patch, &N)) {
    fprintf (stderr, "error: problem receiving patch\n");
    goto reject;
  }
  if (N != 1) {
    fprintf (stderr, "error: too many patches (%d)\n", N);
    goto reject;
  }

  status = ExpectMessage (BindSocket, 0.25, &message);
  if (status != 0) {
    if (VERBOSE) fprintf (stderr, "failed connection\n");
    FreeIOBuffer (&message);
    goto reject;
  }

  /* add to db */
  UpdateDatabase_Refcat (options, patch, message.buffer);

  close (BindSocket);
  return (TRUE);

reject:
  close (BindSocket);
  return (FALSE);
}
