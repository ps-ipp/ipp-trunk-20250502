# include "dvopsps.h"
# define USE_MYSQL 1

// determine the relevant catalogs, launch parallel clients if desired
int insert_FWobjects_dvopsps () {

  SkyTable *sky = NULL;
  SkyList *skylist = NULL;
  Catalog catalog;
  off_t i;

  // load the current sky table (layout of all SkyRegions) 
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // determine the populated SkyRegions overlapping the requested area (default depth)
  if (SINGLE_CPT) {
      skylist = SkyRegionByCPT (sky, SINGLE_CPT);
  } else {
      skylist = SkyListByPatch (sky, -1, &UserPatch);
  }
  myAssert (skylist, "ooops!");

  // pass the skylist to the parallel version, and only launch on relevant hosts 
  if (PARALLEL && !HOST_ID) {
    int status = insert_FWobjects_dvopsps_parallel (skylist);
    return status;
  }

# if (USE_MYSQL)
  // NOTE: mysql connection happens here since each dvopsps_client makes its own connection
  MYSQL  mysqlBase;
  MYSQL *mysqlReal = mysql_dvopsps_connect (&mysqlBase);
  if (!mysqlReal) {
    fprintf (stderr, "failed to connect to mysql\n");
    exit (1);
  }
# else
  MYSQL *mysqlReal = NULL;
# endif

  // select measurements for each populated catalog
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    char localFilename[1024];
    snprintf_nowarn (localFilename, 1024, "%s/%s.cpt", HOSTDIR, skylist->regions[i]->name);

    // set up the basic catalog info
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? localFilename : skylist[0].filename[i];
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_LENSOBJ;
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();

    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    // NOTE: this is where the real action happens
    insert_FWobjects_dvopsps_catalog (&catalog, skylist->regions[i]->name, mysqlReal);

    // NOTE : unlike setastrom or relphot, this program is read-only wrt dvo
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
  }

  if (0) {
    char query[256];
    MYSQL_RES *result;

    sprintf (query, "commit;");
    if (mysql_query (mysqlReal, query)) {
      fprintf (stderr, "failed to turn off autocommit\n");
      fprintf (stderr, "%s\n", mysql_error (mysqlReal));
      return FALSE;
    }
    result = mysql_store_result (mysqlReal);
    mysql_free_result (result);
  }
    
  return (TRUE);
}      

# define DEBUG 1

int insert_FWobjects_dvopsps_parallel (SkyList *sky) {

  // launch the dvopsps_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);

  // if sky->Nregions < 0.5*Nhosts, check if the host used before launching...

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

    char command[1024];
    snprintf_nowarn (command, 1024, "dvopsps_client forced_warp_objects -hostID %d -catdir %s -hostdir %s -region %f %f %f %f", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    char tmpline[1024];
    snprintf_nowarn (tmpline, 1024, "%s -dbhost %s", command, DATABASE_HOST); strcpy (command, tmpline);
    snprintf_nowarn (tmpline, 1024, "%s -dbuser %s", command, DATABASE_USER); strcpy (command, tmpline);
    snprintf_nowarn (tmpline, 1024, "%s -dbpass %s", command, DATABASE_PASS); strcpy (command, tmpline);
    snprintf_nowarn (tmpline, 1024, "%s -dbname %s", command, DATABASE_NAME); strcpy (command, tmpline);

    if (VERBOSE)    { snprintf_nowarn (tmpline, 1024, "%s -v",         command);             strcpy (command, tmpline); }
    if (TEST_MODE)  { snprintf_nowarn (tmpline, 1024, "%s -test-mode", command);             strcpy (command, tmpline); }
    if (SINGLE_CPT) { snprintf_nowarn (tmpline, 1024, "%s -cpt %s",    command, SINGLE_CPT); strcpy (command, tmpline); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running dvopsps_client\n");
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
    fprintf (stderr, "run the dvopsps_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    int status = HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
    if (!status) return FALSE;
  }

  return (TRUE);
}      
