# include "setposangle.h"

// setposangle_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// setposangle_client -hostname (hostname) -catdir (catdir) -hostdir (hostdir) -images (images)
// load the SkyTable and HostTable from (catdir) [contains the host responsibilities]
// load the image data from (images) [contains the zp infomation to apply]
// loop over tables from SkyTable with my host ID
// the calling program tells the client which host they are (or host ID?); we do not actually have to run this on the real host

int main (int argc, char **argv) {

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, IMAGES)
  SetSignals ();
  initialize_setposangle_client (argc, argv);

  // load the image subset table from the specified location
  off_t Nimage;
  ImageSubset *subset = ImageSubsetLoad (IMAGES, &Nimage);

  update_dvo_setposangle (subset, Nimage);

  exit (0);
}
