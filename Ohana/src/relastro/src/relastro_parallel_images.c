# include "relastro.h"

/* This function is essentially identical to relastro_images, except:

 * load the subset images saved by the master node
 * distinguish detections we own (touch our images) and those we don't
 * distinguish objects we own (in region) and those we don't
 * update the unowned detections for owned objects from neighbor regions
 * update the unowned objects for owned detections 
 */

int relastro_parallel_images () {

  int i, Ncatalog;
  Catalog *catalog = NULL;

  INITTIME;

  // over-ride user selection here: this program does not set the final object 
  // astrometry.  since it only sets the image parameters it should not use pm or par
  FIT_MODE = FIT_AVERAGE;

  client_logger_init (CATDIR);

  // load the RegionTable (UserRegion should not be used at this level)
  RegionHostTable *regionHosts = RegionHostTableLoad (CATDIR, REGION_FILE);
  int myHost = regionHosts->index[REGION_HOST_ID];
  char *myHostName = regionHosts->hosts[myHost].hostname;

  RegionHostFindNeighbors (regionHosts, myHost);
  client_logger_message ("started parallel images on %s\n", myHostName);

  // load the subset images belonging to this host
  off_t Nimage;
  Image *image = ImageTableLoad (IMAGE_TABLE, &Nimage);
  if (!image) {
    fprintf (stderr, "ERROR loading image %s\n", IMAGE_TABLE);
    exit (2);
  }

  // assign image->parent and image->coords.mosaic 
  BuildChipMatch (image, Nimage);

  char mapfile[DVO_MAX_PATH];
  snprintf_nowarn (mapfile, DVO_MAX_PATH, "%s/AstroMap.%d.fits", CATDIR, REGION_HOST_ID);
  AstromOffsetTable *table = AstromOffsetMapLoad (mapfile, 100000, VERBOSE);

  // assign images.coords.offsetMap -> table->map[i]
  if (table) {
    AstromOffsetTableMatchChips (image, Nimage, table);
  } else {
    table = AstromOffsetTableInit ();
  }
  put_astrom_table (table);
  client_logger_message ("loaded images on %s\n", myHostName);

  // once we have read this table, we should remove it for repeat runs
  // unlink (IMAGE_TABLE); // XXX a bit risky, add some protection?

  // XXX need to deal with mosaic vs image...
  initMosaics (image, Nimage);

  initImages (image, NULL, Nimage, FALSE);

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  SkyList *skylist = SkyListByBounds (sky, -1, UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);

  /* load catalog data from region files (hostID is 0 since we are not a client */
  char *syncfile = make_filename (CATDIR, myHostName, REGION_HOST_ID, "loadcat.sync");
  catalog = load_catalogs (skylist, &Ncatalog, TRUE, 0, NULL, syncfile);
  MARKTIME("-- load catalog data, host %d: %f sec\n", REGION_HOST_ID, dtime);
  free (syncfile);
  
  client_logger_message ("loaded catalog data %s\n", myHostName);

  // find ICRF QSOs for reference downstream (only if USE_ICRF_CORRECT)
  select_catalog_ICRF (catalog, Ncatalog);

  if (photcodesReset) {
    photcodesKeep  = photcodesReset;
    NphotcodesKeep = NphotcodesReset;
  }

  // generate tables go from catID,objID -> catSeq,objSeq
  indexCatalogs (catalog, Ncatalog);

  /* match measurements with images, mosaics */
  initImageBins  (catalog, Ncatalog, TRUE);
  MARKTIME("-- make image bins, host %d: %f sec\n", REGION_HOST_ID, dtime);

  findImages (catalog, Ncatalog, TRUE);
  MARKTIME("-- set up image indexes, host %d: %f sec\n", REGION_HOST_ID, dtime);

  // set test points based on the starmap
  createStarMap (catalog, Ncatalog);

  client_logger_message ("set up image indexes on %s\n", myHostName);

  markObjects (catalog, Ncatalog);

  SAVEPLOT = FALSE;

  client_logger_message ("starting the loops: %s\n", myHostName);

  RESET = TRUE; // we need to reset when we load the bright catalog subset 
  FIT_MODE = FIT_AVERAGE; // we need to only fit the average
  USE_IRLS = FALSE;  // do not use IRLS yet -- leads to excessive outlier rejections in the loops

  /* major modes */
  switch (FIT_TARGET) {
    case TARGET_SIMPLE:
      for (i = 0; i < NLOOP; i++) {
	UpdateObjects (catalog, Ncatalog, i);
	share_mean_pos (catalog, Ncatalog, regionHosts, i);
	slurp_mean_pos (catalog, Ncatalog, regionHosts, i);
	UpdateSimple (catalog, Ncatalog);
	share_image_pos (regionHosts, i);
	slurp_image_pos (catalog, Ncatalog, regionHosts, i);
      }
      break;

    case TARGET_CHIPS:
      if (RESET_IMAGES) {
	UpdateMeasures (catalog, Ncatalog);
	MARKTIME("UpdateMeasures on %s, host %d: %f sec\n", myHostName, REGION_HOST_ID, dtime);
	LOGRTIME("UpdateMeasures on %s, host %d: %f sec\n", myHostName, REGION_HOST_ID, dtime);
      }
      for (i = 0; i < NLOOP; i++) {
	UpdateObjects (catalog, Ncatalog, i);
	LOGRTIME("UpdateObjects loop %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);

	share_mean_pos (catalog, Ncatalog, regionHosts, i);
	LOGRTIME("share_mean_pos loop %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);
	slurp_mean_pos (catalog, Ncatalog, regionHosts, i);
	LOGRTIME("slurp_mean_pos loop %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);

	UpdateChips (catalog, Ncatalog, i);
	LOGRTIME("UpdateChips loop %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);

	share_image_pos (regionHosts, i);
	LOGRTIME("share_image_pos loop on %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);
	slurp_image_pos (catalog, Ncatalog, regionHosts, i);
	LOGRTIME("slurp_image_pos loop on %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);

	share_meas_pos (catalog, Ncatalog, regionHosts, i);
	LOGRTIME("share_meas_pos loop %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);
	slurp_meas_pos (catalog, Ncatalog, regionHosts, i);
	LOGRTIME("slurp_meas_pos loop %d on %s, host %d: %f sec\n", i, myHostName, REGION_HOST_ID, dtime);
      }

      // measure astrometry or scatter [default] for stacks
      UpdateStacks (catalog, Ncatalog);
      LOGRTIME("UpdateStacks on %s, host %d: %f sec\n", myHostName, REGION_HOST_ID, dtime);

      // create summary plots of the process
      // relastroVisualSummaryChips();
      break;

    case SET_STACKS:
      // we just want to fit the selected stacks to the mean positions
      share_mean_pos (catalog, Ncatalog, regionHosts, 0);
      slurp_mean_pos (catalog, Ncatalog, regionHosts, 0);
      UpdateStacks (catalog, Ncatalog);
      share_image_pos (regionHosts, 0);
      slurp_image_pos (catalog, Ncatalog, regionHosts, 0);
      MARKTIME("update stacks : %f sec\n", dtime);
      break;

    case TARGET_MOSAICS:
      for (i = 0; i < NLOOP; i++) {
	UpdateObjects (catalog, Ncatalog, i);
	share_mean_pos (catalog, Ncatalog, regionHosts, i);
	slurp_mean_pos (catalog, Ncatalog, regionHosts, i);
	UpdateMosaic (catalog, Ncatalog);
	share_image_pos (regionHosts, i);
	slurp_image_pos (catalog, Ncatalog, regionHosts, i);
      }
      break;

    default:
      fprintf (stderr, "programming error at %s:%d", __FILE__, __LINE__);
      exit (2);
  }

  // this is a checkpoint to make sure all hosts have finished the loop above
  char *loopsyncfile = make_filename (CATDIR, myHostName, REGION_HOST_ID, "loop.sync");
  update_sync_file (loopsyncfile, 0);
  free (loopsyncfile);

  for (i = 0; i < regionHosts->Nhosts; i++) {
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    char *loopsync = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "loop.sync");
    check_sync_file (loopsync, 0);
    free (loopsync);
  }    

  share_image_pos (regionHosts, -1);
  LOGRTIME("share image pos loop %d on %s, host %d: %f sec\n", -1, myHostName, REGION_HOST_ID, dtime);

  // free the image / measurement pointers
  freeImageBins (Ncatalog);
  for (i = 0; i < Ncatalog; i++) {
    dvo_catalog_free (&catalog[i]);
  }
  free (catalog);
  freeCatalogIndexes(Ncatalog);

  freeMosaics ();

  freeStarMaps();

  freeImages((char *) image);
  free (image);

  FreeRegionHostTable (regionHosts);
  relastro_free (sky, skylist);

  exit (0);
}
