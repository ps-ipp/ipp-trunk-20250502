# include "dvolens.h"

# define DEBUG 1

# define TIMESTAMP(TIME){				\
  gettimeofday (&stopTimer, (void *) NULL);		\
  double dtime = DTIME (stopTimer, startTimer);		\
  TIME += dtime;					\
  gettimeofday (&startTimer, (void *) NULL); }

void update_objects () {

  int i;
  int status;
  struct stat filestat;
  Catalog catalog;

  double time1 = 0.0;
  double time2 = 0.0;
  double time3 = 0.0;
  double time4 = 0.0;

  // load the current sky table (layout of all SkyRegions) 
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, 0, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // determine the populated SkyRegions overlapping the requested area (default depth)
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);

  // XXX need to decide how to determine PARALLEL mode...
  // in parallel mode, master: PARALLEL is true and HOST_ID == 0, client: PARALLEL is false, HOST_ID != 0
  // in non-parallel mode, master: PARALLEL is false and HOST_ID == 0
  if (PARALLEL && !HOST_ID) {
    update_objects_parallel (skylist);
    return;
  }

  if (VERBOSE) fprintf (stderr, "re-loading catalog data\n");

  // XXX I need to load enough of the images to load the warp groups

  if (REPAIR_LENSING_IDS) {
    load_images (skylist);
    FindWarpGroups();
  }

  /* load data from each region file */
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    if (UserCatalog && strcasecmp (skylist[0].regions[i]->name, UserCatalog)) continue;

    INITTIME;

    // set up the basic catalog info
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = HOST_ID ? hostfile : skylist[0].filename[i];

    // only update existing db tables
    status = stat (catalog.filename, &filestat);
    if ((status == -1) && (errno == ENOENT)) {
      if (VERBOSE) fprintf (stderr, "no file %s, skipping\n", catalog.filename);
      continue;
    }
    TIMESTAMP(time1);

    catalog.catformat = dvo_catalog_catformat (CATFORMAT);    // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);        // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT | DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();               // set the desired number in case we need to create the catalog

    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (VERBOSE && (catalog.Naverage_disk == 0)) {
	fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
	dvo_catalog_unlock (&catalog);
	dvo_catalog_free (&catalog);
	continue;
    }
    TIMESTAMP(time2);

    update_objects_catalog (&catalog);
    TIMESTAMP(time3);

    struct timeval now;
    gettimeofday (&now, (void *) NULL);
    char *moddate = ohana_sec_to_date (now.tv_sec);
    gfits_modify (&catalog.header, "DVOLENS", "%s", 1, moddate);      
    free (moddate);

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
    TIMESTAMP(time4);

    if (HOST_ID) {
      client_logger_message ("updated catalog file %s\n", catalog.filename);
    }
  }

  fprintf (stderr, "time step 1  %10.3f sec : find catalog\n", time1);
  fprintf (stderr, "time step 2  %10.3f sec : load catalog\n", time2);
  fprintf (stderr, "time step 3  %10.3f sec : update catalog\n", time3);
  fprintf (stderr, "time step 4  %10.3f sec : save catalog\n", time4);

  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
}

int update_objects_parallel (SkyList *sky) {

  // now launch the dvolens_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: problem with parallel host table\n");
    exit (2);
  }

  int i, j;
  for (i = 0; i < table->Nhosts; i++) {

    if (sky->Nregions < table->Nhosts) {
      // do any of the regions want this host?
      int wantThisHost = FALSE;
      for (j = 0; j < sky->Nregions; j++) {
	if (HostTableTestHost (sky->regions[j], table->hosts[i].hostID)) {
	  wantThisHost = TRUE;
	  break;
	}
      }
      if (!wantThisHost) {
	// fprintf (stderr, "skip host %s\n", table->hosts[i].hostname);
	continue;
      }
    }

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    char *command = NULL;
    strextend (&command, "dvolens_client -update-objects");
    strextend (&command, "-hostID %d", table->hosts[i].hostID);
    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-hostdir %s", table->hosts[i].pathname);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    // options & configs which affect dvolens_client -update-catalogs:
    // VERBOSE, VERBOSE2, UPDATE

    if (VERBOSE)            	       strextend (&command, "-v");
    if (VERBOSE2)           	       strextend (&command, "-vv");
    if (UPDATE)             	       strextend (&command, "-update");
    if (REPAIR_LENSING_IDS) 	       strextend (&command, "-repair-lensing-ids");
    if (REPAIR_LENSING_IDS_FROM_WARPS) strextend (&command, "-repair-lensing-ids-from-warps");
    if (UserCatalog)                   strextend (&command, "-catalog %s", UserCatalog);

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) { 
      free (command);
      continue;
    }

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running dvolens_client\n");
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
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the dvolens_client commands above.  when these are done, hit return\n");
    getchar();
  } 
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    int status = HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
    if (!status) {
      fprintf (stderr, "at least one remote client job failed to load data, exiting\n");
      exit (3);
    }
  }

  FreeHostTable (table);
  return (TRUE);
}      
