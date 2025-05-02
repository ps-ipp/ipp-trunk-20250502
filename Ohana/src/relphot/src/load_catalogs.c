# include "relphot.h"
# include <sys/types.h>
# include <sys/wait.h>

// this function loops over the catalogs, loads the data, and extracts a bright subset
// the returned array (catalog, Ncatalog) has the same layout as the full database, but
// only a subset of the detetions & objects

// if this function is called in parallel mode, it in turn calls load_catalogs_parallel,
// which distributes the work to the remote hosts and loads their results

// if this function is called with a specified hostID, then only the fraction of the
// database hosted by that hostID is loaded
Catalog *load_catalogs (SkyList *skylist, int *Ncatalog, int hostID, char *hostpath, char *syncfile) {

  off_t i, Nmeas, Nstar, Nmeas_total, Nstar_total;
  Catalog *catalog, tcatalog;

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    catalog = load_catalogs_parallel (skylist, Ncatalog, syncfile);
    return catalog;
  }

  if (VERBOSE2) fprintf (stderr, "loading catalog data\n");

  // a bit of an over-alloc, since we don't load all catalogs for a region 
  ALLOCATE (catalog, Catalog, skylist[0].Nregions);

  Nmeas_total = Nstar_total = 0;

  // load data from each region file, only use bright stars
  for (i = 0; i < skylist[0].Nregions; i++) {
    // XXX keep in mind that not all catalogs are loaded
    dvo_catalog_init (&catalog[i], TRUE);
    dvo_catalog_init (&tcatalog, TRUE);

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // set up the basic catalog info
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);
    dvo_catalog_init (&tcatalog, TRUE);
    tcatalog.filename    = hostID ? hostfile : skylist[0].filename[i];
    tcatalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;    // don't need to load all data at this point
    tcatalog.Nsecfilt    = GetPhotcodeNsecfilt ();               // set the desired number in case we need to create the catalog

    if (!dvo_catalog_open (&tcatalog, skylist[0].regions[i], VERBOSE2, "r")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", tcatalog.filename);
      exit (1);
    }
    if (!tcatalog.Naverage_disk) {
	if (VERBOSE2) { fprintf (stderr, "no data in %s, skipping\n", tcatalog.filename); }
	dvo_catalog_unlock (&tcatalog);
	dvo_catalog_free (&tcatalog);
	continue;
    }

    if (!tcatalog.sorted) {
      fprintf (stderr, "this database is not sorted.  please sort using addstar -resort\n");
      exit (1);
    }

    Nstar_total += tcatalog.Naverage;
    Nmeas_total += tcatalog.Nmeasure;

    // select only the brighter stars
    bcatalog (&catalog[i], &tcatalog, i);
    dvo_catalog_unlock (&tcatalog);
    dvo_catalog_free (&tcatalog);
  }

  // XXX keep this test?
  Nstar = Nmeas = 0;
  for (i = 0; i < skylist[0].Nregions; i++) {
    Nstar += catalog[i].Naverage;
    Nmeas += catalog[i].Nmeasure;
    if ((sizeof(IDX_T) == 8) && (catalog[i].Nmeasure > 0xffffffff)) {
      fprintf (stderr, "ERROR: using small-sized IDX_T on data with more than 2G detections per table will cause errors\n");
      fprintf (stderr, "  If you need to do this, and you can afford the RAM, rebuild with IDX_T set to off_t (see relphot.h, ImagesOps.c)\n");
      exit (3);
    }
  }
  if (Nstar < 2) { 
    fprintf (stderr, "warning: insufficient stars "OFF_T_FMT"\n", Nstar);
  }

  fprintf (stderr, "using "OFF_T_FMT" of "OFF_T_FMT" stars ("OFF_T_FMT" of "OFF_T_FMT" measurements)\n", Nstar, Nstar_total, Nmeas, Nmeas_total);
  if (!hostID && !REGION_HOST_ID && (Nstar < 1)) Shutdown ("%s", "ERROR: no stars match the minimum requirements; exiting \n");
  // in regular relphot, we shutdown here; in relphot_client, we generate and return an empty table (for consistency)

  // if we are running with parallel_images but not a parallel database, we need to
  // release the lock so the next image host can proceed
  if (!hostID && syncfile) {
    update_sync_file (syncfile, 1);
  }

  // XXX consider only returning the populated catalogs
  *Ncatalog = skylist[0].Nregions;
  return (catalog);
}

/* this function loads all relevant catalog files for the first pass.  it currently loads the data
   read only (SOFT lock) since it assumes the image table has been locked. if we go to the new
   addstar locking paradigm, in which the images and catalogs are updated independently, then we may
   need to use an XCLD lock here.  
*/

// CATDIR is supplied globally
# define DEBUG 1
Catalog *load_catalogs_parallel (SkyList *sky, int *Ncatalog, char *syncfile) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();

  if (PARALLEL_MANUAL && MANUAL_UNIQUER) {
    snprintf_nowarn (uniquer, 12, "%11s", MANUAL_UNIQUER);
  } else {
    snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);
  }

  int Nsecfilt  = GetPhotcodeNsecfilt ();               // set the desired number in case we need to create the catalog

  // launch the setphot_client jobs to the parallel hosts

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

    ALLOCATE (table->hosts[i].results, char, 1024);
    snprintf (table->hosts[i].results, 1024, "%s/relphot.catalog.%s.dat", table->hosts[i].pathname, uniquer);

    // options / arguments that can affect relphot_client -load:
    // VERBOSE, VERBOSE2
    // KEEP_UBERCAL
    // RESET (-reset)
    // RESET_ZEROPTS (-reset-zpts)
    // TimeSelect -time
    // DophotSelect
    // (note that psfQF is applied rigidly at 0.85, as is the galaxy test)
    // MAG_LIM
    // SIGMA_LIM
    // ImagSelect, ImagMin, ImagMax
    // MaxDensityUse, MaxDensityValue

    char *command = NULL;
    strextend (&command, "relphot_client %s -load %s -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -D CAMERA %s -D MAG_LIM %f -D SIGMA_LIM %f", 
	      PhotcodeList, table->hosts[i].results, table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax, CAMERA, MAG_LIM, SIGMA_LIM);

    if (VERBOSE)       	     { strextend (&command, "-v"); }
    if (VERBOSE2)      	     { strextend (&command, "-vv"); }
    if (RESET)         	     { strextend (&command, "-reset"); }
    if (RESET_ZEROPTS) 	     { strextend (&command, "-reset-zpts"); }
    if (RESET_FLATCORR)	     { strextend (&command, "-reset-flat"); }
    if (!KEEP_UBERCAL) 	     { strextend (&command, "-reset-ubercal"); }
    if (DophotSelect)  	     { strextend (&command, "-dophot %d", DophotValue); }
    if (ImagSelect)    	     { strextend (&command, "-instmag %f %f", ImagMin, ImagMax); }
    if (MaxDensityUse) 	     { strextend (&command, "-max-density %f", MaxDensityValue); }
    if (SyntheticPhotometry) { strextend (&command, "-synthphot"); }

    if (TimeSelect) { 
      char *tstart = ohana_sec_to_date (TSTART);
      char *tstop  = ohana_sec_to_date (TSTOP);
      strextend (&command, "-time %s %s", tstart, tstop); 
      free (tstart);
      free (tstop);
    }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) {
      free (command);
      continue;
    }

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running relphot_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", table->hosts[i].hostname, command, table->hosts[i].stdio, &errorInfo, FALSE);
      if (!pid) {
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
	continue;
      }
      table->hosts[i].pid = pid; // save for future reference
    }
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the relphot_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    // if any one of the remote jobs fails, we should fail and exit
    int status = HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
    if (!status) {
      fprintf (stderr, "at least one remote client job failed to load data, exiting\n");
      exit (3);
    }
  }

  // update syncfile here (save lots of I/O time):

  // at this point, the remote relastro_client jobs are done loading their data.  in a
  // parallel_images mode, the next image host can be launched while this image host now
  // reads that

  // NOTE: if I let all hosts load blindly, I saturate the data clients with too many
  // relphot_client requests.  I need to have the master mediate this.  the master
  // will not launch the next remote job until this one says it is done
  if (syncfile) {
    update_sync_file (syncfile, 1);
  }

  // each host generates a BrightCatalog structure, with the measure, average, etc value
  // loaded into a single set of arrays (of MeasureTiny, AverageTiny, Secfilt).  I need to
  // split out the per-catalog measurements into separate catalog entries.

  // set up an initial array of catalogs
  CatalogSplitter *catalogs = BrightCatalogSplitInit (Nsecfilt);

  for (i = 0; i < table->Nhosts; i++) {

    BrightCatalog *bcatalog = NULL;
    while ((bcatalog = BrightCatalogLoad (table->hosts[i].results)) == NULL) {
      // failed to get the data from this host.  This can happen for various reasons.  Give the user a chance to try again...
      fprintf (stderr, "failed to read data from %s, stopping operations until this can be fixed\n", table->hosts[i].hostname);
      fprintf (stderr, "you may run the command manually and send this process the CONT signal\n");
      int pid = getpid();
      kill (pid, SIGSTOP);
      fprintf (stderr, "retrying %s\n", table->hosts[i].results);
    }
    free (table->hosts[i].results);
    table->hosts[i].results = NULL;
    
    BrightCatalogSplit (catalogs, bcatalog);

    free (bcatalog->average);
    free (bcatalog->measure);
    free (bcatalog->secfilt);
    free (bcatalog);
  }

  Catalog *catalog = catalogs->catalog;
  *Ncatalog = catalogs->Ncatalog;

  // need to free the place-holder catalogs:
  for (i = catalogs->Ncatalog; i < catalogs->NCATALOG; i++) {
    free (catalogs->catalog[i].averageT);
    free (catalogs->catalog[i].measureT);
    free (catalogs->catalog[i].secfilt);
  }

  int Nmeasure = 0;
  int Naverage = 0;
  for (i = 0; i < catalogs->Ncatalog; i++) {
    Nmeasure += catalogs->catalog[i].Nmeasure;
    Naverage += catalogs->catalog[i].Naverage;
  }

  fprintf (stderr, "loaded %d catalogs, using a total of %d stars (%d measures)\n", catalogs->Ncatalog, Naverage, Nmeasure);
  client_logger_message ("loaded %d catalogs, using a total of %d stars (%d measures)\n", catalogs->Ncatalog, Naverage, Nmeasure);

  BrightCatalogSplitFree (catalogs);
  FreeHostTable (table);

  return (catalog);
}      
