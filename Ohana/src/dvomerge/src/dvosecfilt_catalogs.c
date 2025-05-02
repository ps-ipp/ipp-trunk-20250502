# include "dvomerge.h"

int dvosecfilt_catalogs (int Nsecfilt) {

  off_t i, j, k, NsecInput, Nstart;
  Catalog catalog;
  SecFilt *insec, *outsec;
  char filename[DVO_MAX_PATH];
  SkyList *skylist;

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  if (PARALLEL && !HOST_ID) {
    int status = dvosecfilt_parallel (sky, Nsecfilt);
    return status;
  }

  // determine the populated SkyRegions overlapping the requested area (default depth)
  if (SINGLE_CPT) {
      skylist = SkyRegionByCPT (sky, SINGLE_CPT);
  } else {
      skylist = SkyListByPatch (sky, -1, &UserPatch);
  }
  myAssert (skylist, "ooops!");

  // loop over all input catalogs
  for (i = 0; i < skylist->Nregions; i++) {
    if (!skylist->regions[i]->table) continue;
    if (VERBOSE) fprintf (stderr, "table: %s\n", skylist->regions[i]->name);

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist->regions[i], HOST_ID)) continue;

    char hostfile[1024];
    snprintf_nowarn (hostfile, 1024, "%s/%s.cpt", HOSTDIR, skylist->regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = HOST_ID ? hostfile : skylist->filename[i];

    // always load all of the data (if any exists)
    catalog.Nsecfilt  = 0;
    catalog.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;  // XXX this will fail for MEF version
    catalog.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
    catalog.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data

    if (!dvo_catalog_open (&catalog, skylist->regions[i], VERBOSE, "w")) {
	fprintf (stderr, "ERROR: failure to open catalog file %s\n", filename);
	exit (2);
    }

    if (catalog.Naverage_disk == 0) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    if (Nsecfilt == catalog.Nsecfilt) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    NsecInput = catalog.Nsecfilt;
    Nstart = MIN(Nsecfilt, NsecInput);

    insec = catalog.secfilt;
    ALLOCATE (outsec, SecFilt, catalog.Naverage_disk * Nsecfilt);

    for (k = 0; k < catalog.Naverage_disk; k++) {
      for (j = 0; (j < catalog.Nsecfilt) && (j < Nsecfilt); j++) {
	outsec[k*Nsecfilt + j] = insec[k*NsecInput + j];
      }
      for (j = Nstart; j < Nsecfilt; j++) {
	dvo_secfilt_init (&outsec[k*Nsecfilt + j], SECFILT_RESET_ALL);
      }
    }
    free (catalog.secfilt);
    catalog.secfilt = outsec;
    catalog.Nsecfilt = Nsecfilt;
    catalog.Nsecfilt_mem = Nsecfilt * catalog.Naverage_disk;
    catalog.Nsecfilt_disk = Nsecfilt * catalog.Naverage_disk;

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);

    dvo_catalog_free (&catalog);
  }

  return TRUE;
}

// launch the dvosecfilt_client jobs to the parallel hosts
int dvosecfilt_parallel (SkyTable *sky, int Nsecfilt) {

  // ensure that the paths are absolute path names
  char *abscatdir = abspath (CATDIR, DVO_MAX_PATH);

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

    // options / arguments that can affect relastro_client -update-objects:
    char command[DVO_MAX_PATH];
    snprintf_nowarn (command, DVO_MAX_PATH, "dvosecfilt_client %s %d -hostID %d -hostdir %s -region %f %f %f %f", 
	      abscatdir, Nsecfilt, table->hosts[i].hostID, table->hosts[i].pathname, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax
      );

    char tmpline[DVO_MAX_PATH];
    if (VERBOSE)    { snprintf_nowarn (tmpline, DVO_MAX_PATH, "%s -v",      command);             strcpy (command, tmpline); }
    if (SINGLE_CPT) { snprintf_nowarn (tmpline, DVO_MAX_PATH, "%s -cpt %s", command, SINGLE_CPT); strcpy (command, tmpline); }

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
	fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
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
