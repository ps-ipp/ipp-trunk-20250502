# include "ckids.h"

// ckids_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// ckids_client -hostname (hostname) -catdir (catdir) -hostdir (hostdir)
// load the SkyTable and HostTable from (catdir) [contains the host responsibilities]
// loop over tables from SkyTable with my host ID
// the calling program tells the client which host they are (or host ID?); we do not actually have to run this on the real host

int main (int argc, char **argv) {

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, IMAGES)
  SetSignals ();
  initialize_ckids_client (argc, argv);

  update_dvo_ckids ();

  exit (0);
}
