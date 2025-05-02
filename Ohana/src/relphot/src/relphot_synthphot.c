# include "relphot.h"

// 1) Look for objects which have only synthetic photometry for any of the grizy filters.
// 2) apply spatial zero point offsets to synth photometry, then set mean mag based on synth.

int relphot_synthphot_parallel (SkyList *sky);

int relphot_synthphot (SkyList *skylist, int hostID, char *hostpath) {

  off_t i;

  Catalog catalog;

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    relphot_synthphot_parallel (skylist);
    return TRUE;
  }

  // load the ZP corrections here
  SynthZeroPointsLoad (SYNTH_ZERO_POINTS);
  SynthZeroPoints *zpts = SynthZeroPointsGet ();
  if (!zpts) exit (2);

  // load data from each region file, only use bright stars
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // set up the basic catalog info
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename    = hostID ? hostfile : skylist[0].filename[i];
    catalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt    = GetPhotcodeNsecfilt ();

    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    relphot_synthphot_catalog (&catalog, zpts);

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }
    
    if (VERBOSE) fprintf (stderr, "saving catalog %s\n", catalog.filename);
    
    // we can optionally convert output format here
    // but it would be better to define a dvo crawler program to do this
    // catalog.catformat = DVO_FORMAT_PS1_V1;  
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }
  
  return (TRUE);
}

// CATDIR is supplied globally
# define DEBUG 1
int relphot_synthphot_parallel (SkyList *sky) {

  // launch the setphot_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  // ensure that the path for the zero point is absolute
  { 
    char *tmppath = abspath (SYNTH_ZERO_POINTS, DVO_MAX_PATH);
    free (SYNTH_ZERO_POINTS);
    SYNTH_ZERO_POINTS = tmppath;
  }

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // options / arguments that can affect relphot_client -synthphot_means:
    // VERBOSE, VERBOSE2

    char *command = NULL;
    strextend (&command, "relphot_client -synthphot_means %s", SYNTH_ZERO_POINTS);
    strextend (&command, "-hostID %d -D CATDIR %s -hostdir %s", table->hosts[i].hostID, CATDIR, table->hosts[i].pathname);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)       { strextend (&command, "-v");      }
    if (VERBOSE2)      { strextend (&command, "-vv");     }
    if (UPDATE)        { strextend (&command, "-update"); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running relphot_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	exit (1);
      }
      table->hosts[i].pid = pid; // save for future reference
    }
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the relphot_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}      
