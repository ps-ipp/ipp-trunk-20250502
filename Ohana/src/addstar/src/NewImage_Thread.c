# include "addstar.h"

int NewImage_Thread (int BindSocket) {

  int N, Nstars, Nimages;
  Stars *stars;
  Image *images;
  Coords *mosaic;
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

  if (!Recv_Image (BindSocket, &images, &Nimages)) {
    fprintf (stderr, "error: problem receiving image data\n");
    goto reject;
  }

  if (options[0].mosaic) {
    if (!Recv_Coords (BindSocket, &mosaic, &N)) {
      fprintf (stderr, "error: problem receiving mosaic coordinates\n");
      goto reject;
    }
    if (N != 1) {
      fprintf (stderr, "error: invalid number of mosaic coords (%d)\n", N);
      goto reject;
    }
  }    

  if (!Recv_Stars (BindSocket, &stars, &Nstars)) {
    fprintf (stderr, "error: problem receiving star data\n");
    goto reject;
  }
  fprintf (stderr, "accepted %d, %d stars\n", Nimages, Nstars);

  /* create new dataset to store the incoming data */
  ALLOCATE (dataset, DVO_DATA, 1);
  dataset[0].options = options;
  dataset[0].patch   = NULL;
  dataset[0].refcat  = NULL;
  dataset[0].images  = images;
  dataset[0].Nimages = Nimages;
  dataset[0].mosaic  = mosaic;
  dataset[0].stars   = stars;
  dataset[0].Nstars  = Nstars;

  /* place on dataset stack */
  PushDataset (dataset);

  /* close connection, return */
  close (BindSocket);
  return (TRUE);

reject:
  close (BindSocket);
  return (FALSE);
}
