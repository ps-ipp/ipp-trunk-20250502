# include "relastro.h"

Catalog *load_catalogs_parallel (SkyList *sky, int *Ncatalog, char *syncfile);
void bcatalog_show_skips ();

Catalog *load_catalogs (SkyList *skylist, int *Ncatalog, int subselect, int hostID, char *hostpath, char *syncfile) {

  int i, j;
  // int k, m;
  Catalog *catalog, *pcatalog, tcatalog;

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    catalog = load_catalogs_parallel (skylist, Ncatalog, syncfile);
    return catalog;
  }

  if (VERBOSE) fprintf (stderr, "loading catalog data\n");
  client_logger_message ("loading catalog data\n");

  ALLOCATE (catalog, Catalog, skylist[0].Nregions);

  // load data from each region file, only use bright stars
  int Ncat = 0;
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // we only allow output if we do not use a subset.  in this case,
    // the output parameters are correctly set for catalog[i] via pcatalog
    pcatalog = subselect ? &tcatalog : &catalog[i];

    // define the catalog file name
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);

    dvo_catalog_init (pcatalog, TRUE);
    pcatalog->filename = hostID ? hostfile : skylist[0].filename[i];

    // set up the basic catalog info
    pcatalog->catformat = dvo_catalog_catformat (CATFORMAT);    // set the default catformat from config data
    pcatalog->catmode   = dvo_catalog_catmode (CATMODE);        // set the default catmode from config data
    pcatalog->catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT; // don't need to load all data at this point
    pcatalog->Nsecfilt  = GetPhotcodeNsecfilt ();

    if (USE_GALAXY_MODEL) {
      pcatalog->catflags = pcatalog->catflags | DVO_LOAD_STARPAR;
    }

    // loads Average, Measure, SecFilt
    if (!dvo_catalog_open (pcatalog, skylist[0].regions[i], VERBOSE2, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", pcatalog[0].filename);
      exit (1);
    }
    if (!pcatalog[0].Naverage_disk) {
      if (VERBOSE2) fprintf (stderr, "no data in %s, skipping\n", pcatalog[0].filename);
      dvo_catalog_unlock (pcatalog);
      dvo_catalog_free (pcatalog);
      continue;
    }

    if (!pcatalog->sorted) {
      fprintf (stderr, "this database is not sorted.  please sort using addstar -resort\n");
      exit (1);
    }

    // outlier rejection (operates on full tables (Average, Measure, Secfilt))
    if (FlagOutlier) {
      FlagOutliers(&tcatalog);
    }

    // select only the brighter stars
    if (subselect) {
      // results are in Average, Secfilt, and MeasureTiny
      // Ncat tracks the actually used catalogs
      bcatalog (&catalog[Ncat], &tcatalog);
      dvo_catalog_unlock (&tcatalog);
      dvo_catalog_free (&tcatalog);
    } else {
      if (RESET) {
	for (j = 0; j < catalog[Ncat].Naverage; j++) {
# if (0)	  
	  catalog[Ncat].average[j].flags = 0;
	  m = catalog[Ncat].average[j].measureOffset;
	  for (k = 0; k < catalog[Ncat].average[j].Nmeasure; k++) {
	    catalog[Ncat].measure[m+k].dbFlags = 0;
	  }
# endif
	}
      }
    }
    Ncat ++;

    char message[1024];
    snprintf (message, 1024, "loaded catalog: %d", Ncat);
    my_memdump(message);
  }

  bcatalog_show_skips();
  fprintf (stderr, "included %d ICRF QSOs\n", ICRFmax()); 

  int Nstar = 0;
  int Nmeas = 0;
  for (i = 0; i < Ncat; i++) {
    Nstar += catalog[i].Naverage;
    Nmeas += catalog[i].Nmeasure;
    if ((sizeof(IDX_T) == 8) && (catalog[i].Nmeasure > 0xffffffff)) {
      fprintf (stderr, "ERROR: using small-sized IDX_T on data with more than 2G detections per table will cause errors\n");
      fprintf (stderr, "  If you need to do this, and you can afford the RAM, rebuild with IDX_T set to off_t (see relastro.h, ImagesOps.c)\n");
      exit (3);
    }
  }
  if (Nstar < 2) { 
    fprintf (stderr, "warning: insufficient stars %d\n", Nstar);
  }
  fprintf (stderr, "using %d stars, %d measurements to calibrate\n", Nstar, Nmeas);

  // if we are running with parallel_images but not a parallel database, we need to
  // release the lock so the next image host can proceed
  if (!hostID && syncfile) {
    update_sync_file (syncfile, 1);
  }

  // only return the populated catalogs
  REALLOCATE (catalog, Catalog, Ncat);
  *Ncatalog = Ncat;
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
    snprintf (table->hosts[i].results, 1024, "%s/relastro.catalog.%s.dat", table->hosts[i].pathname, uniquer);

    // options / arguments that can affect relastro_client -load:
    // VERBOSE, VERBOSE2
    // RESET (-reset)
    // TimeSelect -time
    // (note that psfQF is applied rigidly at 0.85, as is the galaxy test)
    // ImagSelect, ImagMin, ImagMax
    // MaxDensityUse, MaxDensityValue

    char *command = NULL;
    strextend (&command, "relastro_client -load-objects %s", table->hosts[i].results);
    strextend (&command, " -hostID %d", table->hosts[i].hostID);
    strextend (&command, " -hostdir %s", table->hosts[i].pathname);

    strextend (&command, " -D CATDIR %s", CATDIR);
    strextend (&command, " -region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);
    strextend (&command, " -statmode %s", STATMODE);
    strextend (&command, " -minerror %f", MIN_ERROR);

    strextend (&command, " -D RELASTRO_SIGMA_LIM %f", SIGMA_LIM);
    strextend (&command, " -D RELASTRO_SRC_MEAS_TOOFEW %d", SRC_MEAS_TOOFEW);

    strextend (&command, " -D RELASTRO_MIN_DISTANCE_MOD %f",     MIN_DISTANCE_MOD);
    strextend (&command, " -D RELASTRO_MAX_DISTANCE_MOD %f",     MAX_DISTANCE_MOD);
    strextend (&command, " -D RELASTRO_MAX_DISTANCE_MOD_ERR %f", MAX_DISTANCE_MOD_ERR);

    strextend (&command, "-D USE_GALAXY_MODEL %d", USE_GALAXY_MODEL);
    strextend (&command, "-D USE_ICRF_CORRECT %d", USE_ICRF_CORRECT);

    if (FIT_MODE == FIT_PM_ONLY)  	 strextend (&command, "-pm");
    if (FIT_MODE == FIT_PAR_ONLY) 	 strextend (&command, "-par");
    if (FIT_MODE == FIT_PM_AND_PAR)      strextend (&command, "-pmpar");

    if (VERBOSE)             strextend (&command, "-v");
    if (VERBOSE2)            strextend (&command, "-vv");
    if (RESET)               strextend (&command, "-reset");
		             
    if (ImagSelect)          strextend (&command, "-instmag %f %f", ImagMin, ImagMax);
    if (MaxDensityUse)       strextend (&command, "-max-density %f", MaxDensityValue);
    if (FlagOutlier)         strextend (&command, "-clip %d", CLIP_THRESH);
    if (ExcludeBogus)        strextend (&command, "-exclude-bogus %f", ExcludeBogusRadius);

    if (USE_FIXED_PIXCOORDS) strextend (&command, "-D USE_FIXED_PIXCOORDS 1");
    if (PHOTCODE_KEEP_LIST)  strextend (&command, "+photcode %s", PHOTCODE_KEEP_LIST);
    if (PHOTCODE_SKIP_LIST)  strextend (&command, "-photcode %s", PHOTCODE_SKIP_LIST);
    if (PhotFlagSelect)      strextend (&command, "+photflags");
    if (PhotFlagBad)         strextend (&command, "+photflagbad %d", PhotFlagBad); 
    if (PhotFlagPoor)        strextend (&command, "+photflagpoor %d", PhotFlagPoor);

    if (DCR_BLUE_COLOR_POS && DCR_BLUE_COLOR_NEG) {
      strextend (&command, "-dcr-blue-color %s %s", DCR_BLUE_COLOR_POS, DCR_BLUE_COLOR_NEG); 
    }
    if (DCR_RED_COLOR_POS && DCR_RED_COLOR_NEG) {
      strextend (&command, "-dcr-red-color %s %s", DCR_RED_COLOR_POS, DCR_RED_COLOR_NEG); 
    }

    if (SKIP_PS1_CHIP)       strextend (&command, "-skip-ps1-chip");
    if (SKIP_PS1_STACK)      strextend (&command, "-skip-ps1-stack");
    if (SKIP_HSC)            strextend (&command, "-skip-hsc");
    if (SKIP_CFH)            strextend (&command, "-skip-cfh");

    if (USE_ALL_IMAGES)      strextend (&command, "-use-all-images");

    // XXX note that the above pass in the flag as decimal -- also note that args.c cannot handle 0xHEX values

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

  if (PARALLEL_MANUAL && !PARALLEL_MANUAL_NO_WAIT) {
    fprintf (stderr, "run the relastro_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  // update syncfile here (save lots of I/O time):

  // at this point, the remote relastro_client jobs are done loading their data.  in a
  // parallel_images mode, the next image host can be launched while this image host now
  // reads that

  // NOTE: if I let all hosts load blindly, I saturate the data clients with too many
  // relastro_client requests.  I need to have the master mediate this.  the master
  // will not launch the next remote job until this one says it is done
  if (syncfile) {
    update_sync_file (syncfile, 1);
  }

  // each host generates a BrightCatalog structure, with the measure, average, etc value
  // loaded into a single set of arrays (of MeasureTiny, AverageTiny, Secfilt).  I need to
  // split out the per-catalog measurements into separate catalog entries.

  // set up an initial array of catalogs
  int Nsecfilt  = GetPhotcodeNsecfilt (); 
  CatalogSplitter *catalogs = BrightCatalogSplitInit (Nsecfilt);

  ohana_memstats (TRUE);

  for (i = 0; i < table->Nhosts; i++) {

    BrightCatalog *bcatalog = BrightCatalogLoad (table->hosts[i].results);
    if (!bcatalog) {
      client_logger_message ("problem loading table from %s\n", table->hosts[i].hostname);
      exit (2);
    }
    
    BrightCatalogSplit (catalogs, bcatalog);

    free (bcatalog->average);
    free (bcatalog->measure);
    free (bcatalog->secfilt);
    free (bcatalog);

    ohana_memstats (TRUE);
  }

  FreeHostTable (table);

  Catalog *catalog = catalogs->catalog;
  *Ncatalog = catalogs->Ncatalog;

  // need to free the place-holder catalogs:
  for (i = catalogs->Ncatalog; i < catalogs->NCATALOG; i++) {
    free (catalogs->catalog[i].average);
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
  ohana_memstats (TRUE);

  return (catalog);
}      
