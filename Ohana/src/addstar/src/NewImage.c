# include "addstar.h"

int NewImage (int BindSocket) {

  int N, Nstars, Nimages;
  Stars *stars;
  Image *images;
  Coords *mosaic;
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
    saveMosaicCoords (mosaic);
  }    

  if (!Recv_Stars (BindSocket, &stars, &Nstars)) {
    fprintf (stderr, "error: problem receiving star data\n");
    goto reject;
  }
  fprintf (stderr, "accepted %d images, %d stars\n", Nimages, Nstars);

  /* add to db */
  UpdateDatabase_Image (options, images, Nimages, mosaic, stars, Nstars);

  close (BindSocket);
  return (TRUE);

reject:
  close (BindSocket);
  return (FALSE);
}
