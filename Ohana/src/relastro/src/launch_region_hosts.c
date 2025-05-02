# include "relastro.h"
# define DEBUG 0

int launch_region_hosts (RegionHostTable *regionHosts) {

  int i;

  // do not remove the sync and fits files if we do a manual run -- user must clear if needed
  if (!PARALLEL_REGIONS_MANUAL) {
    // clear the I/O files
    for (i = 0; i < regionHosts->Nhosts; i++) {
      char *meansync = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanpos.sync");
      if (truncate (meansync, 0)) fprintf (stderr, "trouble clearing meansync %s\n", meansync);
      free (meansync);

      char *meanfits = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanpos.fits");
      if (truncate (meanfits, 0)) fprintf (stderr, "trouble clearing meanfits %s\n", meanfits);
      free (meanfits);

      char *meassync = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "measpos.sync");
      if (truncate (meassync, 0)) fprintf (stderr, "trouble clearing meassync %s\n", meassync);
      free (meassync);

      char *measfits = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "measpos.fits");
      if (truncate (measfits, 0)) fprintf (stderr, "trouble clearing measfits %s\n", measfits);
      free (measfits);

      char *icrfsync = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "icrfobj.sync");
      if (truncate (icrfsync, 0)) fprintf (stderr, "trouble clearing icrfsync %s\n", icrfsync);
      free (icrfsync);

      char *icrffits = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "icrfobj.fits");
      if (truncate (icrffits, 0)) fprintf (stderr, "trouble clearing icrffits %s\n", icrffits);
      free (icrffits);

      char *imsyncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagepos.sync");
      if (truncate (imsyncfile, 0)) fprintf (stderr, "trouble clearing imsyncfile %s\n", imsyncfile);
      free (imsyncfile);

      char *imfitsfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagepos.fits");
      if (truncate (imfitsfile, 0)) fprintf (stderr, "trouble clearing imfitsfile %s\n", imfitsfile);
      free (imfitsfile);

      char *loopsyncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "loop.sync");
      if (truncate (loopsyncfile, 0)) fprintf (stderr, "trouble clearing loopsyncfile %s\n", loopsyncfile);
      free (loopsyncfile);
    }

    char *framesync = make_filename (CATDIR, "master", 0, "frame.corr.sync");
    if (truncate (framesync, 0)) fprintf (stderr, "trouble clearing framesync %s\n", framesync);
    free (framesync);
  
    char *framefits = make_filename (CATDIR, "master", 0, "frame.corr.fits");
    if (truncate (framefits, 0)) fprintf (stderr, "trouble clearing framefits %s\n", framefits);
    free (framefits);
  }

  for (i = 0; i < regionHosts->Nhosts; i++) {

    RegionHostInfo *host = &regionHosts->hosts[i];

    // communication files:
    // subset images per host : CATDIR/Image.HOSTNAME.fits
    char filename[1024];
    snprintf_nowarn (filename, 1024, "%s/Image.%d.fits", CATDIR, host->hostID);
    if (unlink (filename)) fprintf (stderr, "trouble clearing image %s\n", filename);

    // write the image subset for this host
    ImageTableSave (filename, host->image, host->Nimage);

    if (host->astromTable) {
      char mapname[1024];
      snprintf_nowarn (mapname, 1024, "%s/AstroMap.%d.fits", CATDIR, host->hostID);

      // write the image subset for this host
      AstromOffsetMapSave (host->astromTable, mapname);
    }

    char *command = NULL;
    strextend (&command, "relastro -parallel-images %s", filename);
    strextend (&command, "-region-hosts %s", REGION_FILE);
    strextend (&command, "-region-hostID %d", host->hostID);

    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-region %f %f %f %f", host->RminCat, host->RmaxCat, host->DminCat, host->DmaxCat);
    strextend (&command, "-statmode %s", STATMODE);
    strextend (&command, "-minerror %f", MIN_ERROR);

    strextend (&command, "-D RELASTRO_SIGMA_LIM %f", SIGMA_LIM);
    strextend (&command, "-D RELASTRO_SRC_MEAS_TOOFEW %d", SRC_MEAS_TOOFEW);

    strextend (&command, " -D RELASTRO_MIN_DISTANCE_MOD %f",     MIN_DISTANCE_MOD);
    strextend (&command, " -D RELASTRO_MAX_DISTANCE_MOD %f",     MAX_DISTANCE_MOD);
    strextend (&command, " -D RELASTRO_MAX_DISTANCE_MOD_ERR %f", MAX_DISTANCE_MOD_ERR);

    strextend (&command, "-D USE_GALAXY_MODEL %d", USE_GALAXY_MODEL);
    strextend (&command, "-D USE_ICRF_CORRECT %d", USE_ICRF_CORRECT);

    strextend (&command, "-D RELASTRO_DPOS_MAX %f", DPOS_MAX);
    strextend (&command, "-D ADDSTAR_RADIUS %f", ADDSTAR_RADIUS);

    strextend (&command, "-D USE_ICRF_LOCAL %d",   USE_ICRF_LOCAL);
    strextend (&command, "-D USE_ICRF_SHFIT %d",   USE_ICRF_SHFIT);
    strextend (&command, "-D USE_ICRF_POLE %d",    USE_ICRF_POLE);

    switch (FIT_TARGET) {
      case TARGET_SIMPLE:
	strextend (&command, "-update-simple");
	break;
      case TARGET_CHIPS:
	strextend (&command, "-update-chips");
	break;
      case TARGET_MOSAICS:
	strextend (&command, "-update-mosaics");
	break;
      case SET_CHIPS:
	strextend (&command, "-set-chips");
	break;
      case SET_STACKS:
	strextend (&command, "-set-stacks");
	break;
      case TARGET_NONE:
	abort();
    }

    if (VERBOSE)       	    strextend (&command, "-v");
    if (VERBOSE2)      	    strextend (&command, "-vv");
    if (RESET)         	    strextend (&command, "-reset");

    if (ImagSelect)         strextend (&command, "-instmag %f %f", ImagMin, ImagMax);
    if (MaxDensityUse) 	    strextend (&command, "-max-density %f", MaxDensityValue);
    if (FlagOutlier)        strextend (&command, "-clip %d", CLIP_THRESH);
    if (ExcludeBogus)       strextend (&command, "-exclude-bogus %f", ExcludeBogusRadius);

    if (USE_FIXED_PIXCOORDS) strextend (&command, "-D USE_FIXED_PIXCOORDS 1");
    if (PHOTCODE_KEEP_LIST) strextend (&command, "+photcode %s", PHOTCODE_KEEP_LIST); 
    if (PHOTCODE_SKIP_LIST) strextend (&command, "-photcode %s", PHOTCODE_SKIP_LIST);
    if (PhotFlagSelect)     strextend (&command, "+photflags"); 
    if (PhotFlagBad)        strextend (&command, "+photflagbad %d", PhotFlagBad); 
    if (PhotFlagPoor)       strextend (&command, "+photflagpoor %d", PhotFlagPoor); 

    if (DCR_BLUE_COLOR_POS && DCR_BLUE_COLOR_NEG) {
      strextend (&command, "-dcr-blue-color %s %s", DCR_BLUE_COLOR_POS, DCR_BLUE_COLOR_NEG); 
    }
    if (DCR_RED_COLOR_POS && DCR_RED_COLOR_NEG) {
      strextend (&command, "-dcr-red-color %s %s", DCR_RED_COLOR_POS, DCR_RED_COLOR_NEG); 
    }

    if (TEST_SCALE != 1.0)   strextend (&command, "-testing %f", TEST_SCALE);

    if (SKIP_PS1_CHIP)       strextend (&command, "-skip-ps1-chip");
    if (SKIP_PS1_STACK)      strextend (&command, "-skip-ps1-stack");
    if (SKIP_HSC)            strextend (&command, "-skip-hsc");
    if (SKIP_CFH)            strextend (&command, "-skip-cfh");

    if (FIT_STACKS)          strextend (&command, "-fit-stacks");
    if (IMSTATS_ONLY)        strextend (&command, "-imstats-only");

    strextend (&command, "-nloop %d", NLOOP);
    strextend (&command, "-threads %d", NTHREADS);
    
    if (PHOTCODE_RESET_LIST) strextend (&command, "-reset-to-photcode %s", PHOTCODE_RESET_LIST);

    if (UPDATE)        	    strextend (&command, "-update");
    if (PARALLEL)      	    strextend (&command, "-parallel");
    if (PARALLEL_MANUAL)    strextend (&command, "-parallel-manual");
    if (PARALLEL_SERIAL)    strextend (&command, "-parallel-serial");

    strextend (&command, "-chiporder %d", CHIPORDER); 
    if (CHIPMAP)            strextend (&command, "-chipmap %d", CHIPMAP); 
    if (ChipMapLoop)        strextend (&command, "-chipmaploop %s", ChipMapLoopStr); 
    if (ChipOrderLoop)      strextend (&command, "-chiporderloop %s", ChipOrderLoopStr); 

    if (RESET_IMAGES)       strextend (&command, "-reset-images"); 

    if (MinBadQF > 0.0)        strextend (&command, "-min-bad-psfqf %f", MinBadQF);
    if (MaxMeanOffset != 10.0) strextend (&command, "-max-mean-offset  %f", MaxMeanOffset);

    if (LoopWeight2MASS) {  strextend (&command, "-loop-weights-2mass %s", LoopWeight2MASSstr); }
    if (LoopWeightTycho) {  strextend (&command, "-loop-weights-tycho %s", LoopWeightTychostr); }
    if (LoopWeightGAIA)  {  strextend (&command, "-loop-weights-gaia %s", LoopWeightGAIAstr); }
    if (APPLY_PROPER_MOTION) strextend (&command, "-apply-proper-motion");

    if (TimeSelect) { 
      char *tstart = ohana_sec_to_date (TSTART);
      char *tstop  = ohana_sec_to_date (TSTOP);
      strextend (&command, "-time %s %s", tstart, tstop); 
      free (tstart);
      free (tstop);
    }

    fprintf (stderr, "command: %s\n", command);
    
    if (PARALLEL_REGIONS_MANUAL) { 
      free (command);
      continue;
    }

    // launch the job, then wait for it to be done loading catalogs.  force the remote
    // client to generate the file
    char *syncfile = make_filename (CATDIR, host->hostname, host->hostID, "loadcat.sync");
    clear_sync_file (syncfile);

    // launch the job on the remote machine (no handshake)
    int errorInfo = 0;
    int pid = rconnect ("ssh", host->hostname, command, host->stdio, &errorInfo, FALSE);
    if (!pid) {
      if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", host->hostname, errorInfo);
      exit (1);
    }
    host->pid = pid; // save for future reference

    // remove client is done, go ahead with next client
    check_sync_file (syncfile, 1);
    free (syncfile);

    free (command);
  }

  if (USE_ICRF_CORRECT) return TRUE;

  int status = HarvestRegionHosts (regionHosts);
  return status;
}

int HarvestRegionHosts (RegionHostTable *regionHosts) {

  int i;
  int status = TRUE;

  if (PARALLEL_REGIONS_MANUAL) {
    fprintf (stderr, "run the relastro_client commands above.  when these are done, hit return\n");
    getchar();
  } else {
    status = RegionHostTableWaitJobsGetIO (regionHosts, __FILE__, __LINE__, VERBOSE);

    for (i = 0; i < regionHosts->Nhosts; i++) {
      status = status && (regionHosts->hosts[i].status == 0);
    }
  }
  return status;
}
