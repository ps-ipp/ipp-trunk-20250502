# include "dvolens.h"

// dvolens_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// dvolens_client -update-objects : calculate mean lensing parameters

int main (int argc, char **argv) {

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, etc) 
  initialize_client (argc, argv);
  client_logger_init (HOSTDIR);

  switch (MODE) {
    case MODE_UPDATE_OBJECTS:
      client_logger_message ("start dvolens_client\n");
      update_objects ();
      client_logger_message ("updated objects\n");
      FREE (HOSTDIR);
      FREE (CATDIR);
      FREE (UserCatalog);
      free_images();
      FreeWarpGroups();
      ohana_memcheck (VERBOSE);
      ohana_memdump (VERBOSE);
      break;

    default:
      fprintf (stderr, "ERROR: no valid dvolens mode chosen\n");
      exit (2);
  }
  client_logger_message ("done with dvolens_client\n");

  exit (0);
}
