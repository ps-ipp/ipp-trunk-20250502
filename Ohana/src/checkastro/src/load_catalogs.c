# include "checkastro.h"

Catalog *load_catalogs (SkyList *skylist, int *Ncatalog, int subselect, int hostID, char *hostpath) {

  int i, Nstar;
  Catalog *catalog, *pcatalog, tcatalog;

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    catalog = load_catalogs_parallel (skylist, Ncatalog);
    return catalog;
  }

  if (VERBOSE) fprintf (stderr, "loading catalog data\n");

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

    // select only the brighter stars
    if (subselect) {
      // results are in Average, Secfilt, and MeasureTiny
      // Ncat tracks the actually used catalogs
      bcatalog (&catalog[Ncat], &tcatalog);
      dvo_catalog_unlock (&tcatalog);
      dvo_catalog_free (&tcatalog);
    }
    Ncat ++;
  }

  // XXX TEST : bcatalog_show_skips();

  Nstar = 0;
  for (i = 0; i < Ncat; i++) {
    Nstar += catalog[i].Naverage;
    if ((sizeof(IDX_T) == 8) && (catalog[i].Nmeasure > 0xffffffff)) {
      fprintf (stderr, "ERROR: using small-sized IDX_T on data with more than 2G detections per table will cause errors\n");
      fprintf (stderr, "  If you need to do this, and you can afford the RAM, rebuild with IDX_T set to off_t (see checkastro.h, ImagesOps.c)\n");
      exit (3);
    }
  }
  if (Nstar < 2) { 
    fprintf (stderr, "warning: insufficient stars %d\n", Nstar);
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
Catalog *load_catalogs_parallel (SkyList *sky, int *Ncatalog) {

  char uniquer[12];
  int TIME = time(NULL);
  int PID = getpid();
  snprintf_nowarn (uniquer, 12, "%05d.%05d", PID, TIME % 100000);

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
    snprintf (table->hosts[i].results, 1024, "%s/checkastro.catalog.%s.dat", table->hosts[i].pathname, uniquer);

    char command[1024];
    snprintf (command, 1024, "checkastro_client -load-objects %s -hostID %d -D CATDIR %s -hostdir %s -region %f %f %f %f -D RELASTRO_SIGMA_LIM %f", 
	      table->hosts[i].results, table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax, SIGMA_LIM);

    char tmpline[1024];

    if (VERBOSE)       { snprintf (tmpline, 1024, "%s -v",              command);                    strcpy (command, tmpline); }
    if (VERBOSE2)      { snprintf (tmpline, 1024, "%s -vv",             command); 		     strcpy (command, tmpline); }
    if (MaxDensityUse) { snprintf (tmpline, 1024, "%s -max-density %f", command, MaxDensityValue);   strcpy (command, tmpline); }
    if (ImagSelect)    { snprintf (tmpline, 1024, "%s -instmag %f %f",  command, ImagMin, ImagMax);  strcpy (command, tmpline); }

    if (PHOTCODE_KEEP_LIST) { snprintf (tmpline, 1024, "%s +photcode %s", command, PHOTCODE_KEEP_LIST); strcpy (command, tmpline); }
    if (PHOTCODE_SKIP_LIST) { snprintf (tmpline, 1024, "%s -photcode %s", command, PHOTCODE_SKIP_LIST); strcpy (command, tmpline); }
    if (PHOTCODE_RESET_LIST) { snprintf (tmpline, 1024, "%s -reset-to-photcode %s", command, PHOTCODE_RESET_LIST); strcpy (command, tmpline); }
    if (PhotFlagSelect)    { snprintf (tmpline, 1024, "%s +photflags",   command);                     strcpy (command, tmpline); }
    if (PhotFlagBad)       { snprintf (tmpline, 1024, "%s +photflagbad %d", command, PhotFlagBad);     strcpy (command, tmpline); }
    if (PhotFlagPoor)      { snprintf (tmpline, 1024, "%s +photflagpoor %d", command, PhotFlagPoor);   strcpy (command, tmpline); }
    // XXX note that the above pass in the flag as decimal -- also note that args.c cannot handle 0xHEX values

    if (TimeSelect) { 
      char *tstart = ohana_sec_to_date (TSTART);
      char *tstop  = ohana_sec_to_date (TSTOP);
      snprintf (tmpline, 1024, "%s -time %s %s", command, tstart, tstop); 
      free (tstart);
      free (tstop);
      strcpy (command, tmpline); 
    }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running checkastro_client\n");
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
    fprintf (stderr, "run the checkastro_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  // each host generates a BrightCatalog structure, with the measure, average, etc value
  // loaded into a single set of arrays (of MeasureTiny, AverageTiny, Secfilt).  I need to
  // split out the per-catalog measurements into separate catalog entries.

  // set up an initial array of catalogs
  int Nsecfilt  = GetPhotcodeNsecfilt (); 
  CatalogSplitter *catalogs = BrightCatalogSplitInit (Nsecfilt);

  for (i = 0; i < table->Nhosts; i++) {

    BrightCatalog *bcatalog = BrightCatalogLoad (table->hosts[i].results);
    assert (bcatalog);
    
    BrightCatalogSplit (catalogs, bcatalog);

    free (bcatalog->average);
    free (bcatalog->measure);
    free (bcatalog->secfilt);
    free (bcatalog);
  }

  Catalog *catalog = catalogs->catalog;
  *Ncatalog = catalogs->Ncatalog;
  BrightCatalogSplitFree (catalogs);

  int Nmeasure = 0;
  int Naverage = 0;
  for (i = 0; i < catalogs->Ncatalog; i++) {
    Nmeasure += catalogs->catalog[i].Nmeasure;
    Naverage += catalogs->catalog[i].Naverage;
  }

  fprintf (stderr, "loaded %d catalogs, using a total of %d stars (%d measures)\n", catalogs->Ncatalog, Naverage, Nmeasure);

  return (catalog);
}      
