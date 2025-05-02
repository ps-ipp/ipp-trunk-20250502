# include "addstar.h"
# include "loadICRF.h"
# define DEBUG 1

int loadICRF_save_remote (ICRF_Stars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname, AddstarClientOptions *options) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  // if this region is a parallel thing, save and launch remote
  if (!PARALLEL) { 
    loadICRF_catalog (stars, Nstars, region, fullname, options);
  } else {
    int N = hosts->index[region->hostID];
    HostInfo *hostMach = &hosts->hosts[N];

    // save to a unique filename
    char filename[1024]; // CATDIR/tmpdir/ICRF.PID.index.fits
    snprintf (filename, 1024, "%s/tmpdir/ICRF.%s.%05d.fits", CATDIR, uniquer, region->index);
    if (!check_file_access (filename, FALSE, TRUE, TRUE)) exit (2);

    // write the data to the given FITS file
    loadICRF_save_stars (filename, stars, Nstars);

    int slot = -1;
    while (slot == -1) {
      slot = find_empty_slot ();
      if (slot == -1) {
	usleep (50000);
	slot = harvest_host();
	myAssert (slot != -2, "we should not call harvest_host here if we have open slots");
      }
    }

    // allocate a host for this job
    HostInfo *host = NULL;
    ALLOCATE (host, HostInfo, 1);
    InitHost (host);

    // we want to run this job on the host described by hostMach.  copy
    // immutable data from hostMach to a locally allocated host:
    host->hostID = hostMach->hostID;
    host->hostname = strcreate (hostMach->hostname);
    host->pathname = strcreate (hostMach->pathname);
    InitIOBuffer (&host->stdout, 1000);
    InitIOBuffer (&host->stderr, 1000);

    // got a valid slot, so launch a new host

    // need to generate the remote command
    char *command = NULL;
    strextend (&command, "loadICRF_client");
    strextend (&command, "-hostID %d", host->hostID);
    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-hostdir %s", host->pathname);
    strextend (&command, "-cpt %s", region->name);
    strextend (&command, "-input %s", filename);
    strextend (&command, "-D ADDSTAR_RADIUS %f", options->radius);

    fprintf (stderr, "command: %s\n", command);

    // launch the job on the remote machine (no handshake)
    int errorInfo = 0;
    int pid = rconnect ("ssh", host->hostname, command, host->stdio, &errorInfo, FALSE);
    if (!pid) {
      if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", host->hostname, errorInfo);
      exit (1);
    }
    host->pid = pid; // save for future reference
    fprintf (stderr, "launch cpt %s on %s @ %d\n", region->name, host->pathname, pid);
    
    save_remote_host (host);
  }
  return TRUE;
}
