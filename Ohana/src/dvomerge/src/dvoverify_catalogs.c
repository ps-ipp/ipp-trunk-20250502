# include "dvoverify.h"

int dvoverify_catalogs (SkyList *skylist, int *nbad) {

  int i;
  char filename[DVO_MAX_PATH];

  InitFailures ();

  if (PARALLEL && !HOST_ID) {
    int status = dvoverify_parallel (skylist, nbad);
    return status;
  }

  int Nbad = *nbad;

  char *mycatdir = HOST_ID ? HOSTDIR : CATDIR;

  // loop over all catalogs, save to output catalogs
  for (i = 0; i < skylist[0].Nregions; i++) {
    if (!skylist[0].regions[i][0].table) continue;
    if (i % 1000 == 0) fprintf (stderr, ".");

    int skipIndexCheck = FALSE;

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/%s.cpt", mycatdir, skylist[0].regions[i][0].name) < DVO_MAX_PATH, "overflow");
    if (!VerifyTableFile (filename)) {
      Nbad ++;
      skipIndexCheck = TRUE;
      AddFailures (filename);
    }

    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/%s.cps", mycatdir, skylist[0].regions[i][0].name) < DVO_MAX_PATH, "overflow");
    if (!VerifyTableFile (filename)) {
      Nbad ++;
      AddFailures (filename);
    }

    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/%s.cpm", mycatdir, skylist[0].regions[i][0].name) < DVO_MAX_PATH, "overflow");
    if (!VerifyTableFile (filename)) {
      Nbad ++;
      skipIndexCheck = TRUE;
      AddFailures (filename);
    }

    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/%s.cpx", mycatdir, skylist[0].regions[i][0].name) < DVO_MAX_PATH, "overflow");
    if (!VerifyTableFile (filename)) {
      Nbad ++;
      skipIndexCheck = TRUE;
      AddFailures (filename);
    }

    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/%s.cpy", mycatdir, skylist[0].regions[i][0].name) < DVO_MAX_PATH, "overflow");
    if (!VerifyTableFile (filename)) {
      Nbad ++;
      skipIndexCheck = TRUE;
      AddFailures (filename);
    }

    myAssert (snprintf (filename, DVO_MAX_PATH, "%s/%s.cpt", mycatdir, skylist[0].regions[i][0].name) < DVO_MAX_PATH, "overflow");
    if (!skipIndexCheck) {
      if (!CheckCatalogIndexes(filename, skylist[0].regions[i])){
	Nbad ++;
	AddFailures (filename);
      }
    }

    // exit immediately if any file are unsorted and we require sorted tables
    // this probably goes elsewhere...
    if (CHECKSORTED && NNotSorted) {
      fprintf (stderr, "ERROR: files are not sorted\n");
      return FALSE;
    }
  }

  *nbad = Nbad;

  return TRUE;
}

// launch the dvoverify_client jobs to the parallel hosts
int dvoverify_parallel (SkyList *skylist, int *Nbad) {

  // ensure that the paths are absolute path names
  char *abscatdir = abspath (CATDIR, DVO_MAX_PATH);

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, skylist->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", skylist->hosts, CATDIR);
    exit (1);
  }    

  // save the IDlist as a smaller FITS table
  if (CHECK_IMAGE_ID) {
    char filename[DVO_MAX_PATH];
    snprintf (filename, DVO_MAX_PATH, "%s/ImageIDs.fits", CATDIR);
    if (!SaveImageIDsSmall (filename)) {
      fprintf (stderr, "ERROR: failure to save image IDs\n");
      exit (2);
    }
  }

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    ALLOCATE (table->hosts[i].results, char, 1024);
    snprintf (table->hosts[i].results, 1024, "%s/dvoverify.dat", table->hosts[i].pathname);

    // options / arguments that can affect relastro_client -update-objects:
    char *command = NULL;
    strextend (&command, "dvoverify_client %s -results %s -hostID %d -hostdir %s -region %f %f %f %f", 
	       abscatdir, table->hosts[i].results, table->hosts[i].hostID, table->hosts[i].pathname, 
	       UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)             { strextend (&command, "-v"); }
    if (CHECKSORTED)         { strextend (&command, "-s"); }
    if (IGNORE_SORTED_STATE) { strextend (&command, "-ignore-sorted-state"); }
    if (!CHECK_IMAGE_ID)     { strextend (&command, "-skip-image-ids"); }
    if (LIST_MISSING)        { strextend (&command, "-list-missing"); }

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
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the photdbc_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
  }

  int NbadHost = 0;
  int NNotSortedHost = 0;
  for (i = 0; i < table->Nhosts; i++) {
    FILE *results = fopen (table->hosts[i].results, "r");
    if (!results) {
      fprintf (stderr, "cannot read results for %d: %s\n", i, table->hosts[i].results);
      continue;
    }
    int Nresults = fscanf (results, "%*s %d %*s %d", &NbadHost, &NNotSortedHost);
    if (Nresults != 2) {
      fprintf (stderr, "warning: failure scanning results\n");
      fclose (results);
      continue;
    }
    *Nbad += NbadHost;
    NNotSorted += NNotSortedHost;
    fclose (results);
  }

  return TRUE;
}      
