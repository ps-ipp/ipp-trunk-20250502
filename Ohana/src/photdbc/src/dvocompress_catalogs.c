# include "dvocompress.h"
# define DEBUG 1

int dvocompress_catalogs (char *catdir, SkyList *skylist, int hostID) {

  int i;
  Catalog catalog;

  if (PARALLEL && !hostID) {
    dvocompress_parallel (catdir, skylist);
    return FALSE;
  }

  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    // set the parameters which guide catalog open/load/create
    char hostfile[DVO_MAX_PATH];
    snprintf (hostfile, DVO_MAX_PATH, "%s/%s.cpt", HOSTDIR, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename  = hostID ? hostfile : skylist[0].filename[i];

    // read all catalog types
    catalog.catflags = 
      DVO_LOAD_AVERAGE | 
      DVO_LOAD_MEASURE | 
      DVO_LOAD_SECFILT | 
      DVO_LOAD_MISSING | 
      DVO_LOAD_LENSING | 
      DVO_LOAD_LENSOBJ | 
      DVO_LOAD_STARPAR | 
      DVO_LOAD_GALPHOT;

    // XXX this is fairly ad-hoc : I'd like to be able to check that the operation below
    // is a NOOP, but I don't want to put in all the full logic at the moment.  instead,
    // since I know I just want to compress previously uncompressed catalogs, I'm just
    // going to get the cpt header and check for ZTABLE.  Too bad my APIs force me to read
    // the header in full here and then again in dvo_catalog_open.

    if (SKIP_COMPRESSED) {
      FILE *f = fopen (catalog.filename, "r");
      if (!f) { fprintf (stderr, "cannot open %s, skipping\n", catalog.filename); continue; }

      Header header;
      if (!gfits_fread_Xheader (f, &header, 0)) { 
	fprintf (stderr, "cannot read header for %s, skipping\n", catalog.filename);
	fclose (f);
	continue;
      }

      int isZtable;
      int ztableStatus = gfits_scan_alt (&header, "ZTABLE", "%t", 1, &isZtable);

      fclose (f);
      gfits_free_header (&header);

      if (ztableStatus && isZtable) {
	fprintf (stderr, "%s is compressed, skipping\n", catalog.filename);
	continue;
      }
    }

    // XXX for a test, do nothing
    fprintf (stderr, "%s is not compressed, compressing\n", catalog.filename);

    // ohana_memcheck_func (TRUE);

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], (VERBOSE > 1), "w")) {
      fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
      exit (2);
    }

    // ohana_memcheck_func (TRUE);

    // skip empty input catalogs
    if (!catalog.Naverage_disk) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    if (VERBOSE) fprintf (stderr, "%s\n", skylist[0].regions[i][0].name);

    if (UPDATE_CATFORMAT) {
      catalog.catformat = dvo_catalog_catformat (UPDATE_CATFORMAT);
    } 
    if (UPDATE_CATCOMPRESS) {
      catalog.catcompress = dvo_catalog_catcompress (UPDATE_CATCOMPRESS);
    } 

    // ohana_memcheck_func (TRUE);

    if (!dvo_catalog_backup (&catalog, ".z", TRUE)) {
      fprintf (stderr, "ERROR: failed to make backup for catalog %s\n", catalog.filename);
      exit (1);
    }

    // ohana_memcheck_func (TRUE);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, (VERBOSE > 1))) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);

    // ohana_memcheck_func (TRUE);

    if (!dvo_catalog_unlink_backup (&catalog, ".z", TRUE)) {
      fprintf (stderr, "WARNING: failed to remove backup for catalog %s\n", catalog.filename);
    }

    dvo_catalog_free (&catalog);
  }

  return TRUE;
}

int dvocompress_parallel (char *catdir, SkyList *skylist) {

  // launch the photdbo_client jobs to the parallel hosts

  // ensure that the paths are absolute path names
  char *catdir_abs = abspath (catdir, DVO_MAX_PATH);

  // load the list of hosts
  HostTable *table = HostTableLoad (catdir, skylist->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", skylist->hosts, catdir);
    exit (1);
  }    

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    // options / arguments that can affect relastro_client -update-objects:
    char *command = NULL;
    strextend (&command, "dvocompress_client %s -hostID %d -hostdir %s -region %f %f %f %f", 
	       catdir_abs, table->hosts[i].hostID, table->hosts[i].pathname, 
	       REGION.Rmin, REGION.Rmax, REGION.Dmin, REGION.Dmax
      );

    if (VERBOSE)            { strextend (&command, "-v"); }
    if (VERBOSE > 1)        { strextend (&command, "-vv"); }
    if (UPDATE_CATCOMPRESS) { strextend (&command, "-set-compress %s", UPDATE_CATCOMPRESS); }
    if (UPDATE_CATFORMAT)   { strextend (&command, "-set-format %s", UPDATE_CATFORMAT); }
    if (SKIP_COMPRESSED)    { strextend (&command, "-skip-compressed"); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running dvocompress_client\n");
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
    fprintf (stderr, "run the dvocompress_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    HostTableWaitJobsGetIO (table, __FILE__, __LINE__, (VERBOSE > 1));
  }

  free (catdir_abs);

  return TRUE;
}      
