# include "dvoshell.h"
# include <glob.h>
# define DVO_MAX_PATH 1024

// functions to manage the remote hosts
int remote (int argc, char **argv) {
  
  int N;

  int VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  int ReadVectors = TRUE;
  if ((N = get_argument (argc, argv, "-skip-result"))) {
    remove_argument (N, &argc, argv);
    ReadVectors = FALSE;
  }

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: remote (command)\n");
    gprint (GP_ERR, "  launch (command) on the parallel hosts\n");
    gprint (GP_ERR, "  options:\n");
    gprint (GP_ERR, "  -v : verbose mode:\n");
    gprint (GP_ERR, "  -skip-result : do not try to read from the result file\n");
    gprint (GP_ERR, "OR:    remote -reload (uniquer)\n");
    gprint (GP_ERR, "       (reloads the remote host results into vectors as if a parallel command were run)\n");
    gprint (GP_ERR, "OR:    remote -get-results (uniquer)\n");
    gprint (GP_ERR, "       (generates the list of remote result filenames and status variables)\n");
    gprint (GP_ERR, "       (RESULT_FILE:i is the filenme, RESULT_STATUS:i is the dvo_client exit status)\n");
    return FALSE;
  }

  // we can call any command remotely, but the collection of macros will
  // not automatically be passed along.  if we want to run a specific macro,
  // need to point at the relevant input file and have that get loaded

  // if we specified a remote result file, the function above assumes that this is a FITS table
  // with a set of vectors to load.

  if ((N = get_argument (argc, argv, "-reload"))) {
    remove_argument (N, &argc, argv);
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: remote -reload (uniquer)\n");
      gprint (GP_ERR, " (uniquer) is the element in the middle of the results file\n");
      gprint (GP_ERR, " eg: dvo.results.XXXXX.YYYYY.fits\n");
      return FALSE;
    }
    int status = HostTableReloadResults (argv[1], VERBOSE);
    return status;
  }

  if ((N = get_argument (argc, argv, "-get-results"))) {
    remove_argument (N, &argc, argv);
    if (argc != 2) {
      gprint (GP_ERR, "USAGE: remote -get-results (uniquer)\n");
      gprint (GP_ERR, " (uniquer) is the element in the middle of the results file\n");
      gprint (GP_ERR, " eg: dvo.results.XXXXX.YYYYY.fits\n");
      return FALSE;
    }
    int status = HostTableGetResults (argv[1], VERBOSE);
    return status;
  }

  // load the list of hosts
  SkyTable *sky = GetSkyTable();
  if (!sky) {
    gprint (GP_ERR, "failed to load sky table for database\n");
    return FALSE;
  }
  SkyList *skylist = NULL;
  ALLOCATE (skylist, SkyList, 1);
  skylist[0].Nregions = sky[0].Nregions;
  strcpy (skylist[0].hosts, sky[0].hosts);

  // strip of the 'remote' and send the remaining arguments to the remote machine
  int status = HostTableParallelOps (skylist, argc - 1, &argv[1], NULL, ReadVectors, 0, VERBOSE);
  free (skylist);
  return status;
}
