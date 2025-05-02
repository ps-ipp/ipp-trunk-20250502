# include "addstar.h"

int NewReflist_Thread (int BindSocket) {

  int N, Nstars;
  Stars *stars;
  AddstarClientOptions *options;
  DVO_DATA *dataset;

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

  /* create new dataset to store the incoming data */
  ALLOCATE (dataset, DVO_DATA, 1);
  dataset[0].options = options;
  dataset[0].patch   = NULL;
  dataset[0].refcat  = NULL;
  dataset[0].images  = NULL;
  dataset[0].mosaic  = NULL;
  dataset[0].stars   = stars;
  dataset[0].Nstars  = Nstars;

  /* place on dataset stack */
  PushDataset (dataset);

  close (BindSocket);
  return (TRUE);

reject:
  close (BindSocket);
  return (FALSE);
}
