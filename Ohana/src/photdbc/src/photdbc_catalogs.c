# include "photdbc.h"
# define DEBUG 1

int photdbc_catalogs (char *outroot, SkyList *skylist, int hostID) {

  int i;
  Catalog incatalog;
  Catalog outcatalog;

  if (PARALLEL && !hostID) {
    photdbc_parallel (outroot, skylist);
    return FALSE;
  }

  for (i = 0; i < skylist[0].Nregions; i++) {
    if (VERBOSE) fprintf (stderr, "%s\n", skylist[0].regions[i][0].name);

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // set the parameters which guide catalog open/load/create
    char hostfile[DVO_MAX_PATH];
    snprintf (hostfile, DVO_MAX_PATH, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&incatalog, TRUE);
    incatalog.filename  = hostID ? hostfile : skylist[0].filename[i];
    incatalog.Nsecfilt = GetPhotcodeNsecfilt ();

    incatalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
    incatalog.catflags   |= DVO_LOAD_MEASURE | DVO_LOAD_MISSING;
    incatalog.catflags   |= DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ;
    incatalog.catflags   |= DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&incatalog, skylist[0].regions[i], VERBOSE, "r")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", incatalog.filename);
      exit (2);
    }
    // skip empty input catalogs
    if (!incatalog.Naverage_disk) {
      dvo_catalog_unlock (&incatalog);
      dvo_catalog_free (&incatalog);
      continue;
    }

    // create output catalog filename
    char outfile[DVO_MAX_PATH];
    snprintf (outfile, DVO_MAX_PATH, "%s/%s.cpt", outroot, skylist[0].regions[i]->name);
    outcatalog.filename = outfile;

    if (HOSTDIR_OUTPUT) {
      // parallel-ouput puts the output in a per-host directory (keeps the source sky partition)
      snprintf (outfile, DVO_MAX_PATH, "%s/%s.cpt", HOSTDIR_OUTPUT, skylist[0].regions[i]->name);
    }

    if (outcatalog.filename == NULL) Shutdown ("error with input catalog name");

    // define outcatalog open parameters
    outcatalog.catformat   = CATFORMAT   ? dvo_catalog_catformat   (CATFORMAT)   : incatalog.catformat;
    outcatalog.catmode     = CATMODE     ? dvo_catalog_catmode     (CATMODE)     : incatalog.catmode;
    outcatalog.catcompress = CATCOMPRESS ? dvo_catalog_catcompress (CATCOMPRESS) : incatalog.catcompress; // set the default catcompress from config data
    outcatalog.Nsecfilt    = incatalog.Nsecfilt;                 // inherit from the incatalog

    outcatalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
    outcatalog.catflags   |= DVO_LOAD_MEASURE | DVO_LOAD_MISSING;
    outcatalog.catflags   |= DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ;
    outcatalog.catflags   |= DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT;

    // output catalogs always represent the same skyregions as the input catalogs
    if (!dvo_catalog_open (&outcatalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", outcatalog.filename);
      exit (2);
    }

    // the output catalog needs to have the same values for 'objID' and 'sorted' as the input
    outcatalog.objID = incatalog.objID;
    outcatalog.sorted = incatalog.sorted;
    if (!incatalog.sorted) {
      fprintf (stderr, "ERROR: input db must be sorted: %s\n", incatalog.filename);
      exit (2);
    }

    /* limit number of measures based on selections */
    make_subcatalog (&outcatalog, &incatalog, skylist[0].regions[i]);
	
    // XXX add other filters here:
    // join_stars (&outcatalog);
    // unique_measures (catalog);
    // flag_measures (&db, catalog);
    // get_mags (catalog);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&outcatalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", outcatalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&outcatalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", outcatalog.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_free (&outcatalog);

    dvo_catalog_unlock (&incatalog);
    dvo_catalog_free (&incatalog);
  }

  return TRUE;
}

int photdbc_parallel (char *outroot, SkyList *skylist) {

  // launch the photdbo_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, skylist->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", skylist->hosts, CATDIR);
    exit (1);
  }    

  HostTable *table_output = NULL;
  if (PARALLEL_OUTHOSTS) {
    table_output = HostTableLoad (outroot, PARALLEL_OUTHOSTS);
    if (!table_output) {
      fprintf (stderr, "%s not found in %s, please create\n", PARALLEL_OUTHOSTS, outroot);
    }
  }

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // ensure that the paths are absolute path names
    char *tmproot = abspath (outroot, DVO_MAX_PATH);

    // options / arguments that can affect relastro_client -update-objects:
    char *command = NULL;
    strextend (&command, "photdbc_client %s -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -D NMEAS_MIN %d -D NMEAS_MIN_FILTERED %d -D AVE_SIGMA_LIM %f -D SIGMA_MAX %f", 
	      tmproot, table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, 
	      REGION.Rmin, REGION.Rmax, REGION.Dmin, REGION.Dmax,
	      NMEAS_MIN, NMEAS_MIN_FILTERED, AVE_SIGMA_LIM, SIGMA_MAX
      );

    if (VERBOSE)            { strextend (&command, "-v"); }
    if (ExcludeByInstMag)   { strextend (&command, "-instmag %f %f", INST_MAG_MIN, INST_MAG_MAX); }
    if (ExcludeByMinSigma)  { strextend (&command, "-min-sigma %f", SIGMA_MIN_KEEP); }
    if (ExcludeByMaxMinMag) { strextend (&command, "-maxminmag %f", MAX_MIN_MAG); }
    if (PHOTCODE_DROP_LIST) { strextend (&command, "-photcode-drop %s", PHOTCODE_DROP_LIST); }
    if (PHOTCODE_KEEP_LIST) { strextend (&command, "-photcode-keep %s", PHOTCODE_KEEP_LIST); }
    if (CATCOMPRESS)        { strextend (&command, "-set-compress %s", CATCOMPRESS); }
    if (CATFORMAT)          { strextend (&command, "-set-format %s", CATFORMAT); }
    if (CATMODE)            { strextend (&command, "-set-mode %s", CATMODE); }

    if (SKIP_MEASURE)       { strextend (&command, "-skip-measure"); }
    if (SKIP_MISSING)       { strextend (&command, "-skip-missing"); }
    if (SKIP_LENSING)       { strextend (&command, "-skip-lensing"); }
    if (SKIP_LENSOBJ)       { strextend (&command, "-skip-lensobj"); }
    if (SKIP_GALPHOT)       { strextend (&command, "-skip-galphot"); }
    if (SKIP_STARPAR)       { strextend (&command, "-skip-starpar"); }

    if (PARALLEL_OUTHOSTS) {
      tmppath = abspath (table_output->hosts[i].pathname, DVO_MAX_PATH);
      free  (table_output->hosts[i].pathname);
      table_output->hosts[i].pathname = tmppath;
      strextend (&command, "-hostdir-output %s", table_output->hosts[i].pathname);
    }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running photdbc_client\n");
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
    fprintf (stderr, "run the photdbc_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  return TRUE;
}      
