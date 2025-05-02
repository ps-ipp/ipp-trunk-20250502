# include "relphot.h"
# define DEBUG 0

int launch_region_hosts (RegionHostTable *regionHosts) {

  int i;

  // do not remove the sync and fits files if we do a manual run -- user must clear if needed
  if (!PARALLEL_REGIONS_MANUAL) {
    // clear the I/O files
    for (i = 0; i < regionHosts->Nhosts; i++) {
      char *syncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanmags.sync");
      if (truncate (syncfile, 0)) fprintf (stderr, "trouble clearing syncfile %s\n", syncfile);
      free (syncfile);

      char *fitsfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "meanmags.fits");
      if (truncate (fitsfile, 0)) fprintf (stderr, "trouble clearing fitsfile %s\n", fitsfile );
      free (fitsfile);

      char *imsyncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagemags.sync");
      if (truncate (imsyncfile, 0)) fprintf (stderr, "trouble clearing imsyncfile %s\n", imsyncfile);
      free (imsyncfile);

      char *imfitsfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagemags.fits");
      if (truncate (imfitsfile, 0)) fprintf (stderr, "trouble clearing imfitsfile %s\n", imfitsfile);
      free (imfitsfile);

      char *loopsyncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "loop.sync");
      if (truncate (loopsyncfile, 0)) fprintf (stderr, "trouble clearing loopsyncfile %s\n", loopsyncfile);
      free (loopsyncfile);
    }
  }

  for (i = 0; i < regionHosts->Nhosts; i++) {

    RegionHostInfo *host = &regionHosts->hosts[i];

    // communication files:
    // subset images per host : CATDIR/Image.HOSTNAME.fits
    char filename[1024];
    snprintf (filename, 1024, "%s/Image.%d.fits", CATDIR, host->hostID);
    if (unlink (filename)) fprintf (stderr, "trouble clearing image %s\n", filename);

    // write the image subset for this host
    ImageTableSave (filename, host->image, host->Nimage);

    char *command = NULL;
    strextend (&command, "relphot %s", PhotcodeList);
    strextend (&command, "-parallel-images %s", filename);
    strextend (&command, "-region-hosts %s", REGION_FILE);
    strextend (&command, "-region-hostID %d", host->hostID);
    strextend (&command, "-D CATDIR %s", CATDIR);
    strextend (&command, "-region %f %f %f %f", host->RminCat, host->RmaxCat, host->DminCat, host->DmaxCat);
    strextend (&command, "-statmode %s", STATMODE);
    strextend (&command, "-D CAMERA %s", CAMERA);
    strextend (&command, "-D STAR_TOOFEW %d", STAR_TOOFEW);
    strextend (&command, "-minerror %f", MIN_ERROR);
    strextend (&command, "-cloud-limit %f", CLOUD_TOLERANCE);

    if (VERBOSE)       	     	    strextend (&command, "-v");
    if (VERBOSE2)      	     	    strextend (&command, "-vv");
    if (RESET)         	     	    strextend (&command, "-reset");
    if (RESET_ZEROPTS) 	     	    strextend (&command, "-reset-zpts");
    if (RESET_FLATCORR)	     	    strextend (&command, "-reset-flat");
    if (!KEEP_UBERCAL) 	     	    strextend (&command, "-reset-ubercal");
    if (DophotSelect)  	     	    strextend (&command, "-dophot %d", DophotValue);
    if (ImagSelect)    	     	    strextend (&command, "-instmag %f %f", ImagMin, ImagMax);
    if (MaxDensityUse) 	     	    strextend (&command, "-max-density %f", MaxDensityValue);
    if (SyntheticPhotometry) 	    strextend (&command, "-synthphot");
    if (USE_BASIC_CHECK)     	    strextend (&command, "-basic-image-search");

    if (UPDATE)        	     	    strextend (&command, "-update");
    if (MOSAIC_ZEROPT) 	     	    strextend (&command, "-mosaic");
    if (FREEZE_IMAGES) 	     	    strextend (&command, "-imfreeze");
    if (FREEZE_MOSAICS)	     	    strextend (&command, "-mosfreeze");
    if (CALIBRATE_STACKS_AND_WARPS) strextend (&command, "-only-stacks-and-warps");
    if (USE_MCAL_PSF_FOR_STACK_APER) { strextend (&command, "-use-mcal-psf-for-stack-aper"); }

    if (PARALLEL)      	     	    strextend (&command, "-parallel");
    if (PARALLEL_MANUAL)     	    strextend (&command, "-parallel-manual");
    if (PARALLEL_SERIAL)     	    strextend (&command, "-parallel-serial");

    // XXX deprecate this if we are happy with the new version
    // if (SET_MREL_VERSION != 1) strextend (command, "-set-mrel-version %d", SET_MREL_VERSION);

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

    // remote client is done, go ahead with next client
    check_sync_file (syncfile, 1);
    free (syncfile);
    free (command);
  }

  int status = TRUE;
  if (PARALLEL_REGIONS_MANUAL) {
    fprintf (stderr, "run the relphot_client commands above.  when these are done, hit return\n");
    getchar();
  } else {
    RegionHostTableWaitJobsGetIO (regionHosts, __FILE__, __LINE__, VERBOSE);

    for (i = 0; i < regionHosts->Nhosts; i++) {
      status = status && (regionHosts->hosts[i].status == 0);
    }
  }

  return status;
}
