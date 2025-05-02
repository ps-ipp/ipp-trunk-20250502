# include "addstar.h"
# include "setobjflags.h"
# define DEBUG 1

int setobjflags_save_remote (MyStars *stars, int Nstars, HostTable *hosts, SkyRegion *region, char *fullname) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  // if this region is a parallel thing, save and launch remote
  if (!PARALLEL) { 
    setobjflags_catalog (stars, Nstars, region, fullname);
  } else {
    int N = hosts->index[region->hostID];
    HostInfo *hostMach = &hosts->hosts[N];

    // save to a unique filename
    char filename[DVO_MAX_PATH]; // CATDIR/tmpdir/starpar.PID.index.fits
    snprintf (filename, DVO_MAX_PATH, "%s/tmpdir/objflags.%s.%05d.fits", CATDIR, uniquer, region->index);

    // write the data to the given FITS file
    setobjflags_save_stars (filename, stars, Nstars);

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
    strextend (&command, "setobjflags_client");
    strextend (&command, "-hostID %d", host->hostID);
    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-hostdir %s", host->pathname);
    strextend (&command, "-cpt %s", region->name);
    strextend (&command, "-input %s", filename);

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) {
      free (command);
      fprintf (stderr, "run the setobjflags_client command above. when done, hit return\n");
      getchar ();
      save_remote_host (host);
      return TRUE;
    }

    // launch the job on the remote machine (no handshake)
    int errorInfo = 0;
    int pid = rconnect ("ssh", host->hostname, command, host->stdio, &errorInfo, FALSE);
    if (!pid) {
      if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", host->hostname, errorInfo);
      exit (1);
    }
    host->pid = pid; // save for future reference
    free (command);
    
    save_remote_host (host);
  }
  return TRUE;
}
