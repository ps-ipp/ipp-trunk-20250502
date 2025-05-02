# include "fiximids.h"

int update_dvo_fiximids (ImageSubset *image, off_t Nimage) {

  SkyTable *sky = NULL;
  SkyList *skylist = NULL;
  Catalog catalog;
  off_t i;

  // load the current sky table (layout of all SkyRegions) 
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // generate index tables
  initImageIndex (image, Nimage);

  if (PARALLEL && !HOST_ID) {
    update_dvo_fiximids_parallel (sky, image, Nimage);
    return TRUE;
  }

  // determine the populated SkyRegions overlapping the requested area (default depth)
  if (SINGLE_CPT) {
      skylist = SkyRegionByCPT (sky, SINGLE_CPT);
  } else {
      skylist = SkyListByPatch (sky, -1, &UserPatch);
  }
  myAssert (skylist, "ooops!");

  // update measurements for each populated catalog
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    char localFilename[1024];
    snprintf (localFilename, 1024, "%s/%s.cpt", HOSTDIR, skylist->regions[i]->name);

    // set up the basic catalog info
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? localFilename : skylist[0].filename[i];
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

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

    update_catalog_fiximids (&catalog);

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }
    
    if (VERBOSE) fprintf (stderr, "saving catalog %s\n", catalog.filename);
    
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }

  // for the remote clients, we need to dump out the results table (Nvalid, Ninvalid)
  if (HOST_ID && IMSTATS_FILE) {
    ImageValidSave (IMSTATS_FILE);
  }

  return (TRUE);
}      

# define DEBUG 1

int update_dvo_fiximids_parallel (SkyTable *sky, ImageSubset *image, off_t Nimage) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  // write out the subset table of image information
  char imageFile[512];
  snprintf (imageFile, 512, "%s/fiximids.images.%s.dat", CATDIR, uniquer);

  if (!ImageSubsetSave (imageFile, image, Nimage)) {
    fprintf (stderr, "failed to write image subset\n");
    exit (1);
  }

  // now launch the fiximids_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    ALLOCATE (table->hosts[i].results, char, 1024);
    snprintf (table->hosts[i].results, 1024, "%s/fiximids.results.%s.dat", table->hosts[i].pathname, uniquer);

    char *command = NULL;
    strextend (&command, "fiximids_client -hostID %d -catdir %s -hostdir %s -images %s -region %f %f %f %f", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, imageFile,
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    int saveImstats = VERBOSE_IMSTATS || SUMMARY_IMSTATS;

    if (VERBOSE)     { strextend (&command, "-v"); }
    if (saveImstats) { strextend (&command, "-imstats %s", table->hosts[i].results); }
    if (SINGLE_CPT)  { strextend (&command, "-cpt %s", SINGLE_CPT); }
    if (UPDATE)      { strextend (&command, "-update"); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running fiximids_client\n");
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

  // wait for the remote jobs to be completed
  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the fiximids_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    int status = HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
    if (!status) return FALSE;
  }

  if (VERBOSE_IMSTATS || SUMMARY_IMSTATS) {
    for (i = 0; i < table->Nhosts; i++) {
      while (!ImageValidLoad (table->hosts[i].results)) {
	// failed to get the data from this host.  This can happen for various reasons.  Give the user a chance to try again...
	fprintf (stderr, "failed to read data from %s, stopping operations until this can be fixed\n", table->hosts[i].hostname);
	fprintf (stderr, "you may run the command manually and send this process the CONT signal\n");
	int pid = getpid();
	kill (pid, SIGSTOP);
	fprintf (stderr, "retrying %s\n", table->hosts[i].results);
      }
    }
  }

  return (TRUE);
}      

