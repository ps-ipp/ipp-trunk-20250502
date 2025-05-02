# include "dvopsps.h"
# define USE_MYSQL 1
// # define SAVE_REMOTE 1

// we have two ways of writing to the mysql database:
// SAVE_REMOTE = 0 : each client sends the detections directly to the mysql server
// SAVE_REMOTE = 1 : each client saves the detections to disk and these are loaded by the main program and sent to the mysql server

// determine the relevant catalogs, launch parallel clients if desired
int insert_detections_dvopsps () {

  SkyTable *sky = NULL;
  SkyList *skylist = NULL;
  Catalog catalog;
  off_t i;

  // load the current sky table (layout of all SkyRegions) 
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  if (PARALLEL && !HOST_ID) {
    int status = insert_detections_dvopsps_parallel (sky);
    return status;
  }

  // determine the populated SkyRegions overlapping the requested area (default depth)
  if (SINGLE_CPT) {
      skylist = SkyRegionByCPT (sky, SINGLE_CPT);
  } else {
      skylist = SkyListByPatch (sky, -1, &UserPatch);
  }
  myAssert (skylist, "ooops!");

# if (USE_MYSQL)
  // NOTE: mysql connection happens here since each dvopsps_client makes its own connection
  MYSQL  mysqlBase;
  MYSQL *mysqlReal = NULL;
  if (!HOST_ID || !SAVE_REMOTE) {
    mysqlReal = mysql_dvopsps_connect (&mysqlBase);
    if (!mysqlReal) {
      fprintf (stderr, "failed to connect to mysql\n");
      exit (1);
    }
  }
# else
  MYSQL *mysqlReal = NULL;
# endif

  if (SAVE_REMOTE && HOST_ID) {
    init_detections ();
  }

  int status = TRUE;
  // select measurements for each populated catalog
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    char localFilename[1024];
    snprintf (localFilename, 1024, "%s/%s.cpt", HOSTDIR, skylist->regions[i]->name);

    // set up the basic catalog info
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? localFilename : skylist[0].filename[i];
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
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
    if (SAVE_REMOTE && HOST_ID) {
      if (!append_detections_dvopsps_catalog (&catalog)) {
	fprintf (stderr, "ERROR: failure to append detections to output catalog\n");
	status = FALSE;
      }
    } else {
      if (!insert_detections_dvopsps_catalog (&catalog, mysqlReal)) {
	fprintf (stderr, "ERROR: failure to insert detections into mysql database\n");
	status = FALSE;
      }
    }

    // NOTE : unlike setastrom or relphot, this program is read-only wrt dvo
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);

    if (!status) {
      fprintf (stderr, "failure to insert data in mysql\n");
      exit (2);
    }
  }

  if (SAVE_REMOTE && HOST_ID) {
    if (!save_detections_dvopsps ()) {
      fprintf (stderr, "ERROR: failure to save output file with detections\n");
      status = FALSE;
    }
  }

  return (status);
}      

int insert_detections_dvopsps_parallel (SkyTable *sky) {

  // launch the dvopsps_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    if (SAVE_REMOTE) { 
      // XXX need to uniquify the result file
      ALLOCATE (table->hosts[i].results, char, 1024);
      snprintf (table->hosts[i].results, 1024, "%s/dvopsps.%s.det.dat", table->hosts[i].pathname, uniquer);
    }

    char *command = NULL;
    strextend (&command, "dvopsps_client detections -hostID %d -catdir %s -hostdir %s -region %f %f %f %f", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    strextend (&command, "-dbhost %s", DATABASE_HOST);
    strextend (&command, "-dbuser %s", DATABASE_USER);
    strextend (&command, "-dbpass %s", DATABASE_PASS);
    strextend (&command, "-dbname %s", DATABASE_NAME);

    if (VERBOSE)     { strextend (&command, "-v");}
    if (SAVE_REMOTE) { strextend (&command, "-save %s", table->hosts[i].results);} 

    // some filters -- these are the detections we skip
    if (TIME_START) { strextend (&command, "-time-start %s", TIME_START);} 
    if (TIME_END)   { strextend (&command, "-time-end %s", TIME_END);} 
    
    if (PHOTCODE_START > 0)      { strextend (&command, "-photcode-start %d", PHOTCODE_START);} 
    if (PHOTCODE_END <= INT_MAX) { strextend (&command, "-photcode-end %d", PHOTCODE_END);} 

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
	fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
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
    if (VERBOSE) fprintf (stderr, "done reading remote tables, status: %d\n", status);
    if (!status) return FALSE;
  }

  if (SAVE_REMOTE) {
    int Ndetections = 0;
    Detections *detections = NULL;

    // NOTE: mysql connection happens here since each dvopsps_client makes its own connection
    MYSQL  mysqlBase;
    MYSQL *mysql = NULL;
    mysql = mysql_dvopsps_connect (&mysqlBase);
    if (!mysql) {
      fprintf (stderr, "failed to connect to mysql\n");
      exit (1);
    }

    // explicitly delete all entries
    char query[256];
    sprintf (query, "delete from dvoDetectionFull");
    if (mysql_query (mysql, query)) {
      fprintf (stderr, "failed to delete existing entries\n");
      fprintf (stderr, "%s\n", mysql_error (mysql));
      return FALSE;
    }
    if (VERBOSE) fprintf (stderr, "deleted existing entries\n");
    
    for (i = 0; i < table->Nhosts; i++) {
      if ((detections = DetectionsLoad (table->hosts[i].results, &Ndetections)) == NULL) {
	// failed to get the data from this host.  This can happen for various reasons.  Give the user a chance to try again...
	// fprintf (stderr, "you may run the command manually\n");
	fprintf (stderr, "failed to read data from %s\n", table->hosts[i].hostname);
	return FALSE;
      }
      if (VERBOSE) fprintf (stderr, "read %d detections from %s\n", Ndetections, table->hosts[i].hostname);
      free (table->hosts[i].results);
      table->hosts[i].results = NULL;
    
      if (!insert_detections_mysql_array (mysql, detections, Ndetections)) {
	fprintf (stderr, "failed to insert data for %s\n", table->hosts[i].hostname);
	return FALSE;
      }

      free (detections);
    }
  }

  return (TRUE);
}      

int insert_detections_mysql_array (MYSQL *mysql, Detections *detections, int Ndetections) {

  int i;
  IOBuffer buffer;
  buffer.Nalloc = 0;

  if (Ndetections == 0) return TRUE;

  insert_detections_mysql_init (&buffer);

  int status = TRUE;
  int Ninsert = 0;

  INITTIME;
  for (i = 0; i < Ndetections; i++) {

    // XXX check return status
    insert_detections_mysql_detvalue (&buffer, &detections[i]);
    Ninsert ++;
    if (buffer.Nbuffer > MAX_BUFFER) {
      if (!insert_detections_mysql_commit (&buffer, mysql)) {
	fprintf (stderr, "failure to insert detections in mysql (commit)\n");
	status = FALSE;
      }
      buffer.Nbuffer = 0;
      bzero (buffer.buffer, buffer.Nalloc);
      insert_detections_mysql_init (&buffer);
      Ninsert = 0;
    }
  }
  if (Ninsert > 0) {
    // insert the remaining detections loaded in the buffer
    if (!insert_detections_mysql_commit (&buffer, mysql)) {
      fprintf (stderr, "failure to insert detections in mysql (commit 2)\n");
      status = FALSE;
    }
  }
  FreeIOBuffer (&buffer);

  if (!status) {
    MARKTIME("-- failed to insert %d rows in %f sec\n", Ndetections, dtime);
    return FALSE;
  }

  MARKTIME("-- inserted %d rows in %f sec\n", Ndetections, dtime);
  return TRUE;
}

