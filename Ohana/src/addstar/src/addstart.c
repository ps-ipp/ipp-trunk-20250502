# include "addstar.h"
# include <pthread.h>

int main (int argc, char **argv) {

  AddstarClientOptions options;
  pthread_t thread;
  DVO_DATA *dataset;

  SetSignals ();
  options = ConfigInit (&argc, argv);
  args_server (argc, argv);

  /* store the sky table in a global for internal use */
  ServerSky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (ServerSky, CATDIR, "cpt");

  VERBOSE = TRUE;

  InitDataset ();

  // launch thread to listen for client data
  pthread_create (&thread, NULL, &ListenClients_Thread, NULL);

  // XXX need to watch for shutdown message
  while (1) {

    dataset = PopDataset ();
    if (dataset == NULL) {
      usleep (50000);
    }

    switch (dataset[0].options[0].mode) {
      case ADDSTAR_MODE_IMAGE:
	UpdateDatabase_Image (dataset[0].options, dataset[0].images, dataset[0].Nimages, dataset[0].mosaic, dataset[0].stars, dataset[0].Nstars);
	continue;

      case ADDSTAR_MODE_REFLIST:
	UpdateDatabase_Reflist (dataset[0].options, dataset[0].stars, dataset[0].Nstars);
	continue;

      case ADDSTAR_MODE_REFCAT:
	UpdateDatabase_Refcat (dataset[0].options, dataset[0].patch, dataset[0].refcat);
	continue;

      default:
	fprintf (stderr, "error: unexpected dataset\n");
    }
  }    
  exit (1);
}
