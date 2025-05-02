# include "addstar.h"

int main (int argc, char **argv) {

  int Nstars, Nimages;
  int BindSocket;
  AddstarClientOptions options;
  Stars *stars;
  Image *images;
  Coords MOSAIC;

  /* load config and options */
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_client (argc, argv, options);

  /* set up server connection */
  BindSocket = GetClientSocket (HOSTNAME);
  SendCommand (BindSocket, strlen(PASSWORD), PASSWORD);

  /* send new data to server */
  switch (options.mode) {
    case ADDSTAR_MODE_IMAGE:
      /* load data */
      stars = LoadStars (argv[1], &Nstars, &images, &Nimages, &options);

      // set and update the imageID sequence
      UpdateImageIDs (stars, Nstars, images, Nimages);

      /* send data to server */
      SendCommand (BindSocket, 5, "IMAGE");
      Send_AddstarClientOptions (BindSocket, &options, 1, TRUE);
      Send_Image (BindSocket, images, Nimages, FALSE);
      if (options.mosaic) {
	// XXX : this is broken due to an API mod for GetRegisteredMosaic
	// GetRegisteredMosaic (&MOSAIC);
	abort();
	Send_Coords (BindSocket, &MOSAIC, 1, FALSE);
      }
      Send_Stars (BindSocket, stars, Nstars, FALSE);
      break;

    case ADDSTAR_MODE_REFLIST:
      /* load data */
      stars = grefstars (argv[1], options.photcode, &Nstars);
      
      /* send data to server */
      SendCommand (BindSocket, 5, "REFLS");
      Send_AddstarClientOptions (BindSocket, &options, 1, TRUE);
      Send_Stars (BindSocket, stars, Nstars, FALSE);
      break;

    case ADDSTAR_MODE_REFCAT:
      /* send data to server */
      SendCommand (BindSocket, 5, "REFCT");
      Send_AddstarClientOptions (BindSocket, &options, 1, TRUE);
      Send_SkyRegion (BindSocket, &UserPatch, 1, TRUE);
      SendMessage (BindSocket, argv[1]);
      break;

    default:
      fprintf (stderr, "unknown addstar mode\n");
      exit (1);
  }
  exit (0);
}
