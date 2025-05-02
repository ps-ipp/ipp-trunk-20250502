# include "addstar.h"

int NewRefcat_Thread (int BindSocket) {

  int N, status;
  AddstarClientOptions *options;
  IOBuffer message;
  SkyRegion *patch;
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

  /* create new dataset to store the incoming data */
  ALLOCATE (dataset, DVO_DATA, 1);
  dataset[0].options = options;
  dataset[0].patch   = patch;
  dataset[0].refcat  = message.buffer;
  dataset[0].images  = NULL;
  dataset[0].mosaic  = NULL;
  dataset[0].stars   = NULL;
  dataset[0].Nstars  = 0;

  /* place on dataset stack */
  PushDataset (dataset);

  close (BindSocket);
  return (TRUE);

reject:
  close (BindSocket);
  return (FALSE);
}
