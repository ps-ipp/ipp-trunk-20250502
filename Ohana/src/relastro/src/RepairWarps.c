# include "relastro.h"

int RepairWarps_parallel (SkyList *sky);

int RepairWarps (SkyList *skylist, int hostID, char *hostpath) {

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    RepairWarps_parallel (skylist);
    return TRUE;
  }

  FindWarpGroups ();

  // load data from each region file, only use bright stars
  for (int i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // define the catalog file name
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);

    Catalog catalog;
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = hostID ? hostfile : skylist[0].filename[i];

    // set up the basic catalog info
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);    // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);        // set the default catmode from config data
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE2, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE2) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    // update the detection coordinates using the new image parameters
    WarpRepairResult result = RepairWarpMeasures (&catalog);

    // track number of corrections and only update if corrections are made
    int Nmods = 0;
    Nmods += result.NfixChipID;
    Nmods += result.NfixStackID;
    Nmods += result.NfixWarpID;
    Nmods += result.NfixWarpImageID;
    Nmods += result.NfixWarpCoord;
    Nmods += result.NmissWarp;
    Nmods += result.NmissStack;
    Nmods += result.NbadWarp;
    Nmods += result.NbadWarpTime;
    Nmods += result.NwarpNoImage;
    if (!Nmods) {
      fprintf (stderr, "no changes to %s, no output\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }
    
    char history[128];
    struct timeval now;
    gettimeofday (&now, (void *) NULL);
    char *moddate = ohana_sec_to_date (now.tv_sec);
    snprintf (history, 128, "repair warp measure.imageID: %s", moddate);
    gfits_modify_alt (&catalog.header, "HISTORY", "%S", 0, history); // adds a new entry
    free (moddate);

    // add metadata to define number of corrections
    char line[128];
    snprintf (line, 60, "ChipID %d, StackID %d, warpID %d", result.NfixChipID, result.NfixStackID, result.NfixWarpID);
    gfits_modify (&catalog.header, "REPAIR_1", "%s", 1, line);
    snprintf (line, 60, "WarpImageID %d, WarpCoord %d, WarpNoImage %d", result.NfixWarpImageID, result.NfixWarpCoord, result.NwarpNoImage);
    gfits_modify (&catalog.header, "REPAIR_2", "%s", 1, line);
    snprintf (line, 60, "missWarp %d, badWarp %d, badWarpTime %d, missStack %d", result.NmissWarp, result.NbadWarp, result.NbadWarpTime, result.NmissStack);
    gfits_modify (&catalog.header, "REPAIR_3", "%s", 1, line);

    // save backup of original cpm file
    if (!dvo_catalog_unlock (catalog.measure_catalog)) {
      fprintf (stderr, "ERROR: failed to unlock cpm table for catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!dvo_catalog_backup (catalog.measure_catalog, ".rp2", FALSE)) {
      fprintf (stderr, "ERROR: failed to make backup cpm table for catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!dvo_catalog_lock (catalog.measure_catalog, catalog.lockmode)) {
      fprintf (stderr, "failed to re-lock cpm table catalog file %s\n", catalog.filename);
      exit (1);
    }

    // write the updated detections to disk
    save_catalogs (&catalog, 1);
  }

  FreeWarpGroups ();
  
  return (TRUE);
}

// launch the setphot_client jobs to the parallel hosts
int RepairWarps_parallel (SkyList *sky) {


  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    

  // launch the relastro_client jobs to the parallel hosts
  for (int i = 0; i < table->Nhosts; i++) {

    if (sky->Nregions < table->Nhosts) {
      // do any of the regions want this host?
      int wantThisHost = FALSE;
      for (int j = 0; j < sky->Nregions; j++) {
	if (HostTableTestHost (sky->regions[j], table->hosts[i].hostID)) {
	  wantThisHost = TRUE;
	  break;
	}
      }
      if (!wantThisHost) continue;
    }

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // options / arguments that can affect relastro_client -repair-warps:
    // VERBOSE, VERBOSE2, UPDATE

    char *command = NULL;
    strextend (&command, "relastro_client -repair-warps -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	       UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)       	 	     strextend (&command, "-v");
    if (VERBOSE2)      	 	     strextend (&command, "-vv");
    if (UPDATE)        	 	     strextend (&command, "-update");
    if (USE_BASIC_CHECK) 	     strextend (&command, "-basic-image-search");
    if (USE_IMAGE_COORDS_FOR_REPAIR) strextend (&command, "-use-image-coords-for-repair");
    if (USE_ALL_IMAGES)              strextend (&command, "-use-all-images");

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) {
      free (command);
      continue;
    }

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running relastro_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	exit (1);
      }
      table->hosts[i].pid = pid; // save for future reference
    }
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the relastro_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  FreeHostTable (table);

  return TRUE;
}      
