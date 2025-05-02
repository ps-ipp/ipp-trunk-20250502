# include "addstar.h"

// pass in options and skylist?
int resort_catalogs (AddstarClientOptions *options, SkyTable *sky) {

  INITTIME;

  if (options->only_images) {
    fprintf (stderr, "-image (only images) makes no sense with -resort\n");
    exit (2);
  }
  if (options->only_match) {
    fprintf (stderr, "-only-match makes no sense with -resort\n");
    exit (2);
  }
  if (options->nosort == 1) {
    fprintf (stderr, "-nosort makes no sense with -resort\n");
    exit (2);
  }
  if (options->update) {
    fprintf (stderr, "-update makes no sense with -resort\n");
    exit (2);
  }
  if (options->calibrate) {
    fprintf (stderr, "-cal (calibrate) makes no sense with -resort\n");
    exit (2);
  }

  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);
  
  // in these cases, limit the sky catalogs to an existing subset
  if (options->existing_regions) {
    SkyList *tmp;
    tmp = SkyListExistingSubset (skylist, CATDIR);
    SkyListFree (skylist);
    skylist = tmp;
  }
  if (VERBOSE) fprintf (stderr, "writing to "OFF_T_FMT" regions\n",  skylist[0].Nregions);

  if (PARALLEL && !HOST_ID) {
    resort_catalogs_parallel (options, skylist);
    return TRUE;
  }

  int ForceSort = (options->nosort == 3);

  if (NTHREADS == 0) {
    resort_unthreaded (skylist, ForceSort);
  } else {
    resort_threaded (skylist, ForceSort);
  }

  MARKTIME ("SUCCESS: elapsed time %9.4f sec\n", dtime);
  return TRUE;
}

// CATDIR is supplied globally
# define DEBUG 1
int resort_catalogs_parallel (AddstarClientOptions *options, SkyList *sky) {

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    char *command = NULL;
    strextend (&command, "addstar_client -resort");
    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-hostID %d", table->hosts[i].hostID);
    strextend (&command, "-hostdir %s", table->hosts[i].pathname);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)                   strextend (&command, "-v");
    if (NTHREADS)                  strextend (&command, "-threads %d", NTHREADS);
    if (OLD_RESORT)                strextend (&command, "-old-resort");
    if (options->nosort == 3)      strextend (&command, "-force-sort");
    if (options->existing_regions) strextend (&command, "-existing-regions");

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running addstar_client\n");
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
    fprintf (stderr, "run the addstar_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}      
