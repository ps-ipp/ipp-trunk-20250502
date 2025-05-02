# include "dvoshell.h"

# define DEBUG 0
# define PARALLEL_MANUAL 0
# define PARALLEL_SERIAL 0
# define DVO_MAX_PATH 1024

# define DIE(WHO,MSG) { perror(WHO); myAbort(MSG); }

int HostTableLaunchJobs (SkyList *sky, HostTable *table, char *basecmd, char *options, int VERBOSE) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

  // write the dvo comment (host independent)
  char commandBase[DVO_MAX_PATH];
  snprintf (commandBase, DVO_MAX_PATH, "dvo.command.%s.txt", uniquer);
  char *commandFile = abspath(commandBase, DVO_MAX_PATH);

  FILE *f = fopen (commandFile, "w");
  fprintf (f, "%s\n", basecmd);
  if (fflush (f)) DIE("flush", "failed to flush");

  int fd = fileno (f);
  if (fsync (fd)) DIE("fsync", "failed to fsync");

  if (fclose (f)) DIE("close", "failed to close");

  // force NFS to write the file to disk
  int state;
  f = fsetlockfile (commandFile, 0.5, LCK_XCLD, &state);
  fclearlockfile (commandFile, f, LCK_XCLD, &state);

  int top_status = TRUE;
  int i, j;
  for (i = 0; i < table->Nhosts; i++) {

      if (sky && (sky->Nregions < table->Nhosts)) {
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
      // fprintf (stderr, "not skip host %s\n", table->hosts[i].hostname);
    }

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // need to save the results filename with the uniquer
    // XXX a bit of a waste (but only 1024 * 60 bytes or so
    ALLOCATE (table->hosts[i].results, char, DVO_MAX_PATH);
    snprintf (table->hosts[i].results, DVO_MAX_PATH, "%s/dvo.results.%s.%04d.fits", table->hosts[i].pathname, uniquer, table->hosts[i].hostID);

    char command[1024];
    snprintf (command, 1024, "dvo_client %s -result %s %s -hostID %d -hostdir %s", commandFile, table->hosts[i].results, options, table->hosts[i].hostID, table->hosts[i].pathname);

    if (VERBOSE) gprint (GP_ERR, "command: %s\n", command);

    if (PARALLEL_MANUAL) {
      continue;
    }

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	gprint (GP_ERR, "ERROR running relphot_client\n");
	top_status = FALSE;
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	gprint (GP_ERR, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	top_status = FALSE;
	continue;
      }
      table->hosts[i].pid = pid; // save for future reference
    }
  }
  free (commandFile);
  return top_status;
}

// bundle the arguments into a command and pass to dvo_client.  

// the normal ending step expects there to be a result file from the clients, and to load
// this into vectors in the main shell.  'Nelements' is a temp hack : for most commands,
// the result vectors are concatenated, but for avmatch, the vectors are merged by index
// into a pre-known length.  this is probably not a solution to a general problem..

// an alternative ending step ignores the result files and instead saves the names into
// the list 'result:n' for the user to access as desired
int HostTableParallelOps (SkyList *sky, int argc, char **argv, char *ResultFile, int ReadVectors, int Nelements, int VERBOSE) {

  int i;

  // XX // load the list of hosts
  // XX SkyTable *sky = GetSkyTable();
  // XX if (!sky) {
  // XX   gprint (GP_ERR, "failed to load sky table for database\n");
  // XX   return FALSE;
  // XX }

  char *tmppath = GetCATDIR ();
  if (!tmppath) {
    gprint (GP_ERR, "failed to get CATDIR for database\n");
    return FALSE;
  }

  char *CATDIR = abspath (tmppath, DVO_MAX_PATH);
  if (!CATDIR) {
    gprint (GP_ERR, "failed to make an absolute path from %s (too long)\n", tmppath);
    return FALSE;
  }

  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    gprint (GP_ERR, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    return FALSE;
  }    

  // other things I need to append?
  char *basecmd = paste_args (argc, argv);

  // determine the sky region 
  // XXX EAM 20141230 : this needs to respect the -region selection
  double Rmin, Rmax, Dmin, Dmax;
  get_skyregion (&Rmin, &Rmax, &Dmin, &Dmax);

  // determine time reference and format
  char *TimeRef = get_variable ("TIMEREF");
  if (!TimeRef) {
    gprint (GP_ERR, "failed to find TIMEREF variable\n");
    return FALSE;
  }

  char *TimeFormat = get_variable ("TIMEFORMAT");
  if (!TimeFormat) {
    gprint (GP_ERR, "failed to find TIMEFORMAT variable\n");
    return FALSE;
  }

  char tmp;
  char *options = NULL;
  int length = snprintf (&tmp, 0, "-D CATDIR %s -time %s %s -skyregion %f %f %f %f", CATDIR, TimeRef, TimeFormat, Rmin, Rmax, Dmin, Dmax);

  ALLOCATE (options, char, length);
  snprintf (options, length, "-D CATDIR %s -time %s %s -skyregion %f %f %f %f", CATDIR, TimeRef, TimeFormat, Rmin, Rmax, Dmin, Dmax);

  // launch this command remotely
  HostTableLaunchJobs (sky, table, basecmd, options, VERBOSE);
  free (options);
  free (basecmd);

  if (PARALLEL_MANUAL) {
    gprint (GP_ERR, "run the relphot_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  // create the result file list
  char name[256];
  snprintf (name, 256, "RESULT_FILE:n");
  set_int_variable (name, table->Nhosts);
  snprintf (name, 256, "RESULT_DATA:n");
  set_int_variable (name, table->Nhosts);
  snprintf (name, 256, "RESULT_STATUS:n");
  set_int_variable (name, table->Nhosts);

  // load fields from file
  int    Nvec = 0;
  Vector **vec = NULL;
  for (i = 0; i < table->Nhosts; i++) {

    snprintf (name, 256, "RESULT_FILE:%d", i);
    set_str_variable (name, table->hosts[i].results);

    // DATA : 0 (unread), 1 (read)
    snprintf (name, 256, "RESULT_DATA:%d", i);
    set_int_variable (name, 0);

    // STATUS : 0 (normal exit), -1 (crash), N (failure exit status)
    snprintf (name, 256, "RESULT_STATUS:%d", i);
    set_int_variable (name, table->hosts[i].status);

    if (table->hosts[i].status) continue; 
    if (!table->hosts[i].pid) continue;

    if (ReadVectors) {
      int    Ninvec = 0;
      Vector **invec = ReadVectorTableFITS (table->hosts[i].results, "RESULT", &Ninvec);
      if (!invec) {
	// failed to read the file, now what?
	gprint (GP_ERR, "failed to read remote result file : %s\n", table->hosts[i].results);
	free (table->hosts[i].results);
	table->hosts[i].results = NULL;
	continue;
      }
      // free (table->hosts[i].results);
      // table->hosts[i].results = NULL;
      set_int_variable (name, 1); // result file has been read

      if (Nelements == 0) {
	vec = MergeVectors (vec, &Nvec, invec, Ninvec);
	if (vec != invec) {
	  FreeVectorArray (invec, Ninvec);
	}
      } else {
	vec = MergeVectorsByIndex (vec, &Nvec, invec, Ninvec, Nelements);
	FreeVectorArray (invec, Ninvec);
      }
    } else {
      // free (table->hosts[i].results);
      // table->hosts[i].results = NULL;
    }
  }

  // write vectors to a table (this is used by parallel dvo operations, but can be used elsewhere)
  if (ResultFile) {
    int status = WriteVectorTableFITS (ResultFile, "RESULT", NULL, vec, Nvec, FALSE, FALSE, NULL, 0);
    if (!status) {
      gprint (GP_ERR, "failed to write result file %s\n", ResultFile);
      return FALSE;
    }
  }

  for (i = 0; i < Nvec; i++) {
    AssignVector (vec[i], vec[i]->name, ANYVECTOR, TRUE);
  }
  free (vec);

  FreeHostTable (table);
  return TRUE;
}

// re-gather the remote results files: this can be used in case one of the clients failed,
// and has since been re-run
int HostTableReloadResults (char *uniquer, int VERBOSE) {
  OHANA_UNUSED_PARAM(VERBOSE);

  int i;

  // load the list of hosts
  SkyTable *sky = GetSkyTable();
  if (!sky) {
    gprint (GP_ERR, "failed to load sky table for database\n");
    return FALSE;
  }

  char *CATDIR = GetCATDIR ();
  if (!CATDIR) {
    gprint (GP_ERR, "failed to get CATDIR for database\n");
    return FALSE;
  }

  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    gprint (GP_ERR, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    return FALSE;
  }    

  // load fields from file
  int    Nvec = 0;
  Vector **vec = NULL;
  for (i = 0; i < table->Nhosts; i++) {
    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // need to save the results filename with the uniquer
    // XXX a bit of a waste (but only 1024 * 60 bytes or so
    ALLOCATE (table->hosts[i].results, char, DVO_MAX_PATH);
    snprintf (table->hosts[i].results, DVO_MAX_PATH, "%s/dvo.results.%s.%04d.fits", table->hosts[i].pathname, uniquer, table->hosts[i].hostID);

    int    Ninvec = 0;
    Vector **invec = ReadVectorTableFITS (table->hosts[i].results, "RESULT", &Ninvec);
    if (!invec) {
      // failed to read the file, now what?
      gprint (GP_ERR, "failed to read remote result file : %s\n", table->hosts[i].results);
      free (table->hosts[i].results);
      table->hosts[i].results = NULL;
      continue;
    }
    free (table->hosts[i].results);
    table->hosts[i].results = NULL;

    // fprintf (stderr, "%s : %d\n", table->hosts[i].pathname, invec[0]->Nelements);

    vec = MergeVectors (vec, &Nvec, invec, Ninvec);
    if (vec != invec) {
      FreeVectorArray (invec, Ninvec);
    }
  }

  for (i = 0; i < Nvec; i++) {
    AssignVector (vec[i], vec[i]->name, ANYVECTOR, TRUE);
  }
  free (vec);

  free (table);
  return TRUE;
}

// re-gather the remote results files: this can be used in case one of the clients failed,
// and has since been re-run
int HostTableGetResults (char *uniquer, int VERBOSE) {
  OHANA_UNUSED_PARAM(VERBOSE);

  int i;

  // load the list of hosts
  SkyTable *sky = GetSkyTable();
  if (!sky) {
    gprint (GP_ERR, "failed to load sky table for database\n");
    return FALSE;
  }

  char *CATDIR = GetCATDIR ();
  if (!CATDIR) {
    gprint (GP_ERR, "failed to get CATDIR for database\n");
    return FALSE;
  }

  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    gprint (GP_ERR, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
    return FALSE;
  }    

  // create the result file list
  char name[256];
  snprintf (name, 256, "RESULT_FILE:n");
  set_int_variable (name, table->Nhosts);
  snprintf (name, 256, "RESULT_DATA:n");
  set_int_variable (name, table->Nhosts);
  snprintf (name, 256, "RESULT_STATUS:n");
  set_int_variable (name, table->Nhosts);

  char results[DVO_MAX_PATH];

  // load fields from file
  for (i = 0; i < table->Nhosts; i++) {

    // need to save the results filename with the uniquer
    snprintf (results, DVO_MAX_PATH, "%s/dvo.results.%s.fits", table->hosts[i].pathname, uniquer);

    snprintf (name, 256, "RESULT_FILE:%d", i);
    set_str_variable (name, results);

    // DATA : 0 (unread), 1 (read)
    snprintf (name, 256, "RESULT_DATA:%d", i);
    set_int_variable (name, 0);

    // STATUS : 0 (normal exit), -1 (crash), N (failure exit status)
    snprintf (name, 256, "RESULT_STATUS:%d", i);
    set_int_variable (name, table->hosts[i].status);
  }

  free (table);
  return TRUE;
}
