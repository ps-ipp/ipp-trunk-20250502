# include "relphot.h"

# define DEBUG 1

# define TIMESTAMP(TIME) \
    gettimeofday (&stop, (void *) NULL);	\
    dtime = DTIME (stop, start);		\
    TIME += dtime;				\
    gettimeofday (&start, (void *) NULL);

void reload_catalogs (SkyList *skylist, int hostID, char *hostpath) {

  int i;
  int status;
  struct stat filestat;
  Catalog catalog;

  struct timeval start, stop;
  double dtime = 0.0, time1 = 0.0, time2 = 0.0, time3 = 0.0;
  double time4 = 0.0, time5 = 0.0, time6 = 0.0, time7 = 0.0, time8 = 0.0;

  // XXX need to decide how to determine PARALLEL mode...
  if (PARALLEL && !hostID) {
    reload_catalogs_parallel (skylist);
    return;
  }

  // load the ZP corrections here
  if (SYNTH_ZERO_POINTS) SynthZeroPointsLoad (SYNTH_ZERO_POINTS);

  if (VERBOSE) fprintf (stderr, "re-loading catalog data\n");

  /* load data from each region file */
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], hostID)) continue;

    gettimeofday (&start, (void *) NULL);

    // set up the basic catalog info
    char hostfile[1024];
    snprintf (hostfile, 1024, "%s/%s.cpt", hostpath, skylist[0].regions[i]->name);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = hostID ? hostfile : skylist[0].filename[i];

    // only update existing db tables
    status = stat (catalog.filename, &filestat);
    if ((status == -1) && (errno == ENOENT)) {
      if (VERBOSE) fprintf (stderr, "no file %s, skipping\n", catalog.filename);
      continue;
    }
    TIMESTAMP(time1);

    catalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
    // if we want to update the average XRAD values in lensobj, need to load lensobj and lensing
    if (UPDATE_XRAD || UPDATE_CATFORMAT) {
      catalog.catflags |= DVO_LOAD_LENSING | DVO_LOAD_LENSOBJ;
    }
    // if we are going to update the format, we should update all tables
    if (UPDATE_CATFORMAT) {
      catalog.catflags |= DVO_LOAD_STARPAR | DVO_LOAD_GALPHOT;
    }
    catalog.Nsecfilt    = GetPhotcodeNsecfilt ();               // set the desired number in case we need to create the catalog

    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s, skipping\n", catalog.filename);
      continue;
    }
    if (VERBOSE && (catalog.Naverage_disk == 0)) {
	fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
	dvo_catalog_unlock (&catalog);
	dvo_catalog_free (&catalog);
	continue;
    }
    TIMESTAMP(time2);

    populate_tiny_values(&catalog, DVO_TV_MEASURE | DVO_TV_AVERAGE);
    TIMESTAMP(time3);

    // XXX need to worry about the image subset data
    initImageBins  (&catalog, 1, FALSE);
    TIMESTAMP(time4);

    findImages (&catalog, 1, FALSE);
    TIMESTAMP(time5);

    initMrel (&catalog, 1);

    setMrelFinal (&catalog, FALSE);
    TIMESTAMP(time6);

    if (UPDATE_XRAD) {
      setXradAverages (&catalog);
    }

    // modify the output format as desired (ignore current format on disk)
    if (UPDATE_CATFORMAT) {
      catalog.catformat = dvo_catalog_catformat (UPDATE_CATFORMAT);
    }

    struct timeval now;
    gettimeofday (&now, (void *) NULL);
    char *moddate = ohana_sec_to_date (now.tv_sec);
    gfits_modify (&catalog.header, "RELPHOT", "%s", 1, moddate);      
    free (moddate);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);

    free_tiny_values(&catalog);
    dvo_catalog_free (&catalog);
    TIMESTAMP(time7);

    freeImageBins (1, FALSE);
    TIMESTAMP(time8);

    if (hostID) {
      client_logger_message ("updated catalog file %s\n", catalog.filename);
    }
  }

  fprintf (stderr, "time step 1  %10.3f sec : find catalog\n",     time1);
  fprintf (stderr, "time step 2  %10.3f sec : load catalog\n",     time2);
  fprintf (stderr, "time step 3  %10.3f sec : make tiny values\n", time3);
  fprintf (stderr, "time step 4  %10.3f sec : init imbins\n",      time4);
  fprintf (stderr, "time step 5  %10.3f sec : find images\n",      time5);
  fprintf (stderr, "time step 6  %10.3f sec : set Mrel\n",         time6);
  fprintf (stderr, "time step 7  %10.3f sec : save catalog\n",     time7);
  fprintf (stderr, "time step 8  %10.3f sec : free catalog\n",     time8);
}

int reload_catalog_parallel_group (HostTableGroup *group, SkyList *sky, char *imageFile);

// XXX Image to Image Subset
int reload_catalogs_parallel (SkyList *sky) {

  // name of image subset file:
  char imageFile[512];
  snprintf (imageFile, 512, "%s/Images.subset.dat", CATDIR);

  // the ApplyOffsets option re-uses an existing Images.subset.dat file
  if (!ApplyOffsets) {
    off_t Nimage;
    ImageSubset *image = getimages_subset (&Nimage);
    if (!ImageSubsetSave (imageFile, image, Nimage)) {
      fprintf (stderr, "failed to write image subset\n");
      exit (1);
    }
    free (image);
  }

  // now launch the relphot_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: problem with parallel host table\n");
    exit (2);
  }

  if (BOUNDARY_TREE) {
    char *tmppath = abspath(BOUNDARY_TREE, DVO_MAX_PATH);
    free (BOUNDARY_TREE);
    BOUNDARY_TREE = tmppath;
  }
  if (SYNTH_ZERO_POINTS) {
    char *tmppath = abspath(SYNTH_ZERO_POINTS, DVO_MAX_PATH);
    free (SYNTH_ZERO_POINTS);
    SYNTH_ZERO_POINTS = tmppath;
  }

  int Ngroups;
  HostTableGroup *groups = HostTableGroupsUniqueMachines (table, &Ngroups);
  // split the table into Ngroups, each with a unique set of machines (this avoids overloading the remote host machines)

  int i;
  for (i = 0; i < Ngroups; i++) {
    // update only a group of unique machines at a time
    if (i < SKIP_PARALLEL_GROUPS) continue;
    reload_catalog_parallel_group (&groups[i], sky, imageFile);
  }

  for (i = 0; i < Ngroups; i++) {
    free (groups[i].hosts);
  }
  free (groups);
  FreeHostTable (table);

  return TRUE;
}

int reload_catalog_parallel_group (HostTableGroup *group, SkyList *sky, char *imageFile) {

  int i, j;
  for (i = 0; i < group->Nhosts; i++) {

    if (sky->Nregions < group->Nhosts) {
      // do any of the regions want this host?
      int wantThisHost = FALSE;
      for (j = 0; j < sky->Nregions; j++) {
	if (HostTableTestHost (sky->regions[j], group->hosts[i][0].hostID)) {
	  wantThisHost = TRUE;
	  break;
	}
      }
      if (!wantThisHost) {
	// fprintf (stderr, "skip host %s\n", group->hosts[i][0].hostname);
	continue;
      }
    }

    // ensure that the paths are absolute path names
    char *tmppath = abspath (group->hosts[i][0].pathname, DVO_MAX_PATH);
    free (group->hosts[i][0].pathname);
    group->hosts[i][0].pathname = tmppath;

    char *command = NULL;
    strextend (&command, "relphot_client %s -update-catalogs %s", PhotcodeList, imageFile);
    strextend (&command, "-hostID %d", group->hosts[i][0].hostID);
    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-hostdir %s", group->hosts[i][0].pathname);
    strextend (&command, "-region %f %f %f %f", UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);
    strextend (&command, "-statmode %s", STATMODE);
    strextend (&command, "-D CAMERA %s", CAMERA);
    strextend (&command, "-D STAR_TOOFEW %d", STAR_TOOFEW);
    strextend (&command, "-minerror %f", MIN_ERROR);

    if (VERBOSE)           { strextend (&command, "-v"); }
    if (VERBOSE2)          { strextend (&command, "-vv"); }
    if (RESET)             { strextend (&command, "-reset"); }
    if (RESET_ZEROPTS)     { strextend (&command, "-reset-zpts"); }
    if (RESET_FLATCORR)    { strextend (&command, "-reset-flat"); }
    if (!KEEP_UBERCAL)     { strextend (&command, "-reset-ubercal"); }
    if (PRESERVE_PS1)      { strextend (&command, "-preserve-ps1"); }
    if (IS_DIFF_DB)        { strextend (&command, "-is-diff-db"); }
    if (UPDATE)            { strextend (&command, "-update"); }
    if (UPDATE_CATFORMAT)  { strextend (&command, "-update-catformat %s", UPDATE_CATFORMAT); }
    if (BOUNDARY_TREE)     { strextend (&command, "-boundary-tree %s", BOUNDARY_TREE); }
    if (SYNTH_ZERO_POINTS) { strextend (&command, "-synthphot-zpts %s", SYNTH_ZERO_POINTS); }
    if (GRID_ZEROPT)       { strextend (&command, "-grid %s", GRID_MEANFILE); } 
    if (USE_BASIC_CHECK)   {  strextend (&command, "-basic-image-search"); }
    // if (USE_ALL_IMAGES)    { strextend (&command, "-use-all-images"); }
    if (VARIABILITY_STATS) { strextend (&command, "-varstats"); }
    if (USE_MCAL_PSF_FOR_STACK_APER) { strextend (&command, "-use-mcal-psf-for-stack-aper"); }

    if (!(STAGES & STAGE_CHIP))  { strextend (&command, "-skip-chip"); }
    if (!(STAGES & STAGE_WARP))  { strextend (&command, "-skip-warp"); }
    if (!(STAGES & STAGE_STACK)) { strextend (&command, "-skip-stack"); }

    // deprecate
    // if (SET_MREL_VERSION != 1) { snprintf (tmpline, 1024, "%s -set-mrel-version %d", SET_MREL_VERSION); } // XXXX deprecate this...

    if (AreaSelect)       { strextend (&command, "-area %f %f %f %f", AreaXmin, AreaXmax, AreaYmin, AreaYmax); }
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
      int pid = rconnect ("ssh", group->hosts[i][0].hostname, command, group->hosts[i][0].stdio, &errorInfo, FALSE);
      if (!pid) {
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", group->hosts[i][0].hostname, errorInfo);
	exit (1);
      }
      group->hosts[i][0].pid = pid; // save for future reference
    }
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the relphot_client commands above.  when these are done, hit return\n");
    getchar();
  } 
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    int status = HostTableGroupWaitJobsGetIO (group, __FILE__, __LINE__, VERBOSE);
    if (!status) {
      fprintf (stderr, "at least one remote client job failed to load data, exiting\n");
      exit (3);
    }
  }
  return (TRUE);
}      
