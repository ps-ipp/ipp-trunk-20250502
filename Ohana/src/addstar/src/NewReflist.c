# include "addstar.h"

int NewReflist (int BindSocket) {

  int N, Nstars;
  Stars *stars;
  AddstarClientOptions *options;

  /* accept incoming data set */
  if (!Recv_AddstarClientOptions (BindSocket, &options, &N)) {
    fprintf (stderr, "error: problem receiving options\n");
    goto reject;
  }
  if (N != 1) {
    fprintf (stderr, "error: too many option sets (%d)\n", N);
    goto reject;
  }

  if (!Recv_Stars (BindSocket, &stars, &Nstars)) {
    fprintf (stderr, "error: problem receiving star data\n");
    goto reject;
  }
  fprintf (stderr, "accepted %d stars\n", Nstars);

  /* add to db */
  UpdateDatabase_Reflist (options, stars, Nstars);

  close (BindSocket);
  return (TRUE);

reject:
  close (BindSocket);
  return (FALSE);
}
