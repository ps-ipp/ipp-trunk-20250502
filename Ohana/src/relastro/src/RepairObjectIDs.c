# include "relastro.h"

int RepairObjectIDs_parallel (SkyList *sky);
int RepairObjectIDs_catalog (Catalog *catalog);

// some average entries are missing the PSPS object ID values.  set them correctly here
int RepairObjectIDs (SkyList *skylist, int hostID, char *hostpath) {

  int i;
  Catalog catalog;

  if (PARALLEL && !hostID) {
    RepairObjectIDs_parallel (skylist);
    return TRUE;
  }

  // load data from each region file, only use bright stars
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // define the catalog file name
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);

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

    RepairObjectIDs_catalog (&catalog);

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }
    
    struct timeval now;
    gettimeofday (&now, (void *) NULL);
    char *moddate = ohana_sec_to_date (now.tv_sec);
    gfits_modify (&catalog.header, "REPAIR", "%s", 1, moddate);      
    free (moddate);

    // write the updated detections to disk
    save_catalogs (&catalog, 1);
  }
  return (TRUE);
}

int RepairObjectIDs_parallel_table (HostTable *table, SkyList *sky);

int RepairObjectIDs_parallel (SkyList *sky) {

  // launch the setphot_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    exit (1);
  }    
  
  RepairObjectIDs_parallel_table (table, sky);

  return TRUE;
}      

// CATDIR is supplied globally
# define DEBUG 1
int RepairObjectIDs_parallel_table (HostTable *table, SkyList *sky) {

  // launch the relastro_client jobs to the parallel hosts

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
    strextend (&command, "relastro_client -repair-object-id");
    strextend (&command, "-hostID %d", table->hosts[i].hostID);
    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-hostdir %s", table->hosts[i].pathname);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)       strextend (&command, "-v");
    if (VERBOSE2)      strextend (&command, "-vv");
    if (UPDATE)        strextend (&command, "-update");

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
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
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

int RepairObjectIDs_catalog (Catalog *catalog) {

  myAssert (!catalog->Naverage || catalog->average, "programming error");

  int onePercent = catalog->Naverage / 100;

  for (off_t j = 0; j < catalog->Naverage; j++) {
    if (j % onePercent == 0) fprintf (stderr, ".");

    catalog->average[j].extID = CreatePSPSObjectID (catalog->average[j].R, catalog->average[j].D);
  }

  return TRUE;
}

