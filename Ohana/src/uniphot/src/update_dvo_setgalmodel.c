# include "setgalmodel.h"

int update_dvo_setgalmodel (void) {

  SkyTable *sky = NULL;
  SkyList *skylist = NULL;
  Catalog catalog;
  off_t i;

  // load the current sky table (layout of all SkyRegions) 
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  if (PARALLEL && !HOST_ID) {
    int status = update_dvo_setgalmodel_parallel (sky);
    return status;
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
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_STARPAR;
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

    update_catalog_setgalmodel (&catalog);

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }
    
    // modify the output format as desired (ignore current format on disk)
    if (UPDATE_CATFORMAT) {
      catalog.catformat = dvo_catalog_catformat (UPDATE_CATFORMAT);
    }

    if (VERBOSE) fprintf (stderr, "saving catalog %s\n", catalog.filename);
    
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }
  return (TRUE);
}      

# define DEBUG 1

int update_dvo_setgalmodel_parallel (SkyTable *sky) {

  // now launch the setgalmodel_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    char *command = NULL;
    strextend (&command, "setgalmodel_client -hostID %d -catdir %s -hostdir %s -region %f %f %f %f", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)       	  { strextend (&command, "-v"); }
    if (TEST_SCALE != 1.0){ strextend (&command, "-testing %f", TEST_SCALE); }
    if (UPDATE)        	  { strextend (&command, "-update"); }
    if (UPDATE_CATFORMAT) { strextend (&command, "-update-catformat %s", UPDATE_CATFORMAT); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running setgalmodel_client\n");
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
    fprintf (stderr, "run the setgalmodel_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    int status = HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
    if (!status) return FALSE;
  }

  return (TRUE);
}      
