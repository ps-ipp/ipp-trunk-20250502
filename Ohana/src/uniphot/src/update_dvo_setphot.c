# include "setphot.h"
int update_dvo_setphot_parallel (SkyTable *sky, Image *image, off_t Nimage);

// XXX convert this to use the ImageSubset to save space
int update_dvo_setphot (Image *image, off_t Nimage, CamPhotomCorrection *camcorr) {

  SkyTable *sky = NULL;
  SkyList *skylist = NULL;
  Catalog catalog;
  off_t i, maxID, *index;

  // load the current sky table (layout of all SkyRegions) 
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  if (PARALLEL && !HOST_ID) {
    update_dvo_setphot_parallel (sky, image, Nimage);
    return TRUE;
  }

  if (KH_FILE)  if (!load_kh_correction  (KH_FILE))  { fprintf (stderr, "failed to load KH  correction %s\n", KH_FILE ); exit (1); }
  if (DCR_FILE) if (!load_dcr_correction (DCR_FILE)) { fprintf (stderr, "failed to load DCR correction %s\n", DCR_FILE); exit (1); }

  if (CAM_ASTROM_FILE) if (!CamAstromCorrectionLoad (CAM_ASTROM_FILE)) { fprintf (stderr, "failed to load CAM correction %s\n", CAM_ASTROM_FILE); exit (1); }

  // create an index for the image IDs
  maxID = 0;
  for (i = 0; i < Nimage; i++) {
    maxID = MAX(maxID, image[i].imageID);
  }
  ALLOCATE (index, off_t, maxID + 1);
  for (i = 0; i < maxID + 1; i++) {
    index[i] = -1;
  }
  for (i = 0; i < Nimage; i++) {
    if (!image[i].imageID) continue; // images with ID == 0 are virtual
    index[image[i].imageID] = i;
  }

  // determine the populated SkyRegions overlapping the requested area (default depth)
  if (SINGLE_CPT) {
      skylist = SkyRegionByCPT (sky, SINGLE_CPT);
  } else {
      skylist = SkyListByPatch (sky, -1, &UserPatch);
  }
  myAssert (skylist, "ooops!");

  // update measurements for each populated catalog
  for (i = 0; i < skylist[0].Nregions; i++) {

    // does this host ID match the desired location for the table?
    if (!HostTableTestHost(skylist[0].regions[i], HOST_ID)) continue;

    char localFilename[1024];
    snprintf (localFilename, 1024, "%s/%s.cpt", HOSTDIR, skylist->regions[i]->name);

    // set up the basic catalog info
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename    = HOST_ID ? localFilename : skylist[0].filename[i];
    catalog.catflags    = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE;
    if (KH_FILE || DCR_FILE || CAM_ASTROM_FILE) {
      catalog.catflags |= DVO_LOAD_SECFILT;
    }
    if (SET_GAL_MODEL) {
      catalog.catflags |= DVO_LOAD_STARPAR;
    }
    catalog.Nsecfilt    = GetPhotcodeNsecfilt ();

    if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "w")) {
      fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog.filename);
      exit (1);
    }
    if (!catalog.Naverage_disk) {
      if (VERBOSE) fprintf (stderr, "no data in %s, skipping\n", catalog.filename);
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    if (REPAIR_BY_OBJID) {
      // For gpc1 / PV3, I broke some catalogs with dvomerge -replace: the last object can
      // contaminate the first set of new measurements
      repair_catalog_by_objID (&catalog);
    }

    update_catalog_setphot (&catalog, image, index, Nimage, camcorr);

    if (KH_FILE || DCR_FILE || CAM_ASTROM_FILE) {
      update_catalog_setastrom (&catalog);
    }

    if (SET_GAL_MODEL) {
      update_catalog_setgalmodel (&catalog);
    }

    if (!UPDATE) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }
    
    if (VERBOSE) fprintf (stderr, "saving catalog %s\n", catalog.filename);
    
    struct timeval now;
    gettimeofday (&now, (void *) NULL);
    char *moddate = ohana_sec_to_date (now.tv_sec);
    gfits_modify (&catalog.header, "SETPHOT", "%s", 1, moddate); // replaces the single entry

    char history[128];
    snprintf (history, 128, "setphot applied: %s", moddate);
    gfits_modify_alt (&catalog.header, "HISTORY", "%S", 0, history); // adds a new entry

    snprintf (history, 128, "setphot options: reset: %1d, ubercal: %1d, camflat: %1d", RESET, UBERCAL, (CAM_PHOTOM_FILE != NULL));
    gfits_modify_alt (&catalog.header, "HISTORY", "%S", 0, history); // adds a new entry

    if (KH_FILE || DCR_FILE || CAM_ASTROM_FILE) {
      gfits_modify (&catalog.header, "SETASTR", "%s", 1, moddate); // replaces the single entry

      snprintf (history, 128, "setphot (setastrom) options: KH: %1d, DCR: %1d, CAM: %1d", (KH_FILE != NULL), (DCR_FILE != NULL), (CAM_ASTROM_FILE != NULL));
      gfits_modify_alt (&catalog.header, "HISTORY", "%S", 0, history); // adds a new entry
    }

    free (moddate);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }
  return (TRUE);
}      

# define DEBUG 1

int update_dvo_setphot_parallel (SkyTable *sky, Image *image, off_t Nimage) {

  // write out the subset table of image information
  char imageFile[512];
  snprintf (imageFile, 512, "%s/Images.subset.dat", CATDIR);
  if (!ImageSubsetSave (imageFile, image, Nimage)) {
    fprintf (stderr, "failed to write image subset\n");
    exit (1);
  }

  // now launch the setphot_client jobs to the parallel hosts

  // load the list of hosts
  HostTable *table = HostTableLoad (CATDIR, sky->hosts);

  int i;
  for (i = 0; i < table->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (table->hosts[i].pathname, DVO_MAX_PATH);
    free (table->hosts[i].pathname);
    table->hosts[i].pathname = tmppath;

    char *command = NULL;
    strextend (&command, "setphot_client -hostID %d -catdir %s -hostdir %s -images %s -region %f %f %f %f", 
	      table->hosts[i].hostID, CATDIR, table->hosts[i].pathname, imageFile, 
	      UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

    if (VERBOSE)       	 { strextend (&command, "-v"); }
    if (RESET)         	 { strextend (&command, "-reset"); }
    if (UPDATE)        	 { strextend (&command, "-update"); }
    if (REPAIR_BY_OBJID) { strextend (&command, "-repair-by-objid"); }

    if (UBERCAL)         { strextend (&command, "-ubercal"); }
    // if (CAM_PHOTOM_FILE) { strextend (&command, "-cam-flat %s", CAM_PHOTOM_FILE); }

    if (KH_FILE)      	  { strextend (&command, "-KH %s", KH_FILE); }
    if (DCR_FILE)      	  { strextend (&command, "-DCR %s", DCR_FILE); }
    if (CAM_ASTROM_FILE)  { strextend (&command, "-CAM %s", CAM_ASTROM_FILE); }

    if (SET_GAL_MODEL)    { strextend (&command, "-setgalmodel %s", SET_GAL_MODEL); }

    if (KH_RESET)      	  { strextend (&command, "-KH-reset"); }
    if (DCR_RESET)     	  { strextend (&command, "-DCR-reset"); }
    if (CAM_RESET)     	  { strextend (&command, "-CAM-reset"); }

    if (PHOTCODE_MAX) 	  { strextend (&command, "-photcode-range %d %d", PHOTCODE_MIN, PHOTCODE_MAX); }

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running setphot_client\n");
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

  // wait for the remote jobs to be completed
  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the setphot_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    int status = HostTableWaitJobsGetIO (table, __FILE__, __LINE__, VERBOSE);
    if (!status) return FALSE;
  }

  return (TRUE);
}      

