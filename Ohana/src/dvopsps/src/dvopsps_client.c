# include "dvopsps.h"

// dvopsps_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// dvopsps_client -hostname (hostname) -catdir (catdir) -hostdir (hostdir) -images (images)
// load the SkyTable and HostTable from (catdir) [contains the host responsibilities]
// loop over tables from SkyTable with my host ID
// the calling program tells the client which host they are (or host ID?); we do not actually have to run this on the real host

int main (int argc, char **argv) {

  int status = -1;

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, IMAGES)
  initialize_dvopsps_client (argc, argv);

  if (!strcasecmp (argv[1], "detections")) {
    status = insert_detections_dvopsps ();
  }
  if (!strcasecmp (argv[1], "objects")) {
    status = insert_objects_dvopsps ();
  }
  if (!strcasecmp (argv[1], "diffobj")) {
    status = insert_diffobj_dvopsps ();
  }
  if (!strcasecmp (argv[1], "forced_warp_objects")) {
    status = insert_FWobjects_dvopsps ();
  }
  if (!strcasecmp (argv[1], "forced_galaxy_shape")) {
    status = insert_FGshape_dvopsps ();
  }
  if (status == -1) {
    fprintf (stderr, "invalid mode, should be : detections, skytable\n");
    exit (2);
  }

  if (!status) exit (1);
  exit (0);
}
