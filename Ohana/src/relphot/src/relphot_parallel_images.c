# include "relphot.h"

/* This function is essentially identical to relphot_images, except:

 * load the subset images saved by the master node
 * distinguish detections we own (touch our images) and those we don't
 * distinguish objects we own (in region) and those we don't
 * update the unowned detections for owned objects from neighbor regions
 * update the unowned objects for owned detections 

 * TGROUP and GRID corrections are not calculated.
 */

int dumpObjects (char *filename, Catalog *catalog, int Ncatalog);

int relphot_parallel_images (SkyTable *sky) {

  int i, Ncatalog;
  Catalog *catalog = NULL;

  INITTIME;

  client_logger_init (CATDIR);

  // load the RegionTable (UserRegion should not be used at this level)
  RegionHostTable *regionHosts = RegionHostTableLoad (CATDIR, REGION_FILE);
  int myHost = regionHosts->index[REGION_HOST_ID];
  RegionHostFindNeighbors (regionHosts, myHost);
  client_logger_message ("started parallel images on %s\n", regionHosts->hosts[myHost].hostname);

  // load the subset images belonging to this host
  off_t Nimage;
  Image *image = ImageTableLoad (IMAGE_TABLE, &Nimage);
  if (!image) {
    fprintf (stderr, "ERROR loading image %s\n", IMAGE_TABLE);
    exit (2);
  }
  client_logger_message ("loaded images\n");

  makeMosaics (image, Nimage, TRUE);

  initImages (image, NULL, Nimage);

  // UserPatch.Rmin,Rmax may have range from a few degrees < 0.0 to few degrees > 360.0.
  // the following function correctly chooses the sky regions on the 0,360 boundary
  SkyList *skylist = SkyListByBounds (sky, -1, UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax);
  client_logger_message ("prepare to load catalogs\n");

  /* load catalog data from region files (hostID is 0 since we are not a client */
  char *syncfile = make_filename (CATDIR, regionHosts->hosts[myHost].hostname, REGION_HOST_ID, "loadcat.sync");
  catalog = load_catalogs (skylist, &Ncatalog, 0, NULL, syncfile);
  MARKTIME("-- load catalog data: %f sec\n", dtime);
  free (syncfile);

  client_logger_message ("loaded catalog data\n");

  // generate tables go from catID,objID -> catSeq,objSeq
  indexCatalogs (catalog, Ncatalog);
  client_logger_message ("indexed Catalogs\n");

  /* match measurements with images, mosaics */
  initImageBins  (catalog, Ncatalog, TRUE);
  MARKTIME("-- make image bins: %f sec\n", dtime);

  initMosaicBins (catalog, Ncatalog, TRUE);
  initMrel (catalog, Ncatalog);

  findImages (catalog, Ncatalog, TRUE);
  MARKTIME("-- set up image indexes: %f sec\n", dtime);

  findMosaics (catalog, Ncatalog, TRUE);  /* also sets Grid values */
  MARKTIME("-- set up mosaic indexes: %f sec\n", dtime);

  client_logger_message ("done setting up indexes\n");

  // dumpObjects ("test.obj.dat", catalog, Ncatalog);
  markObjects (catalog, Ncatalog);

  SAVEPLOT = FALSE;

  setExclusions (catalog, Ncatalog, TRUE);

  global_stats (catalog, Ncatalog, 0);

  if (PLOTSTUFF) {
    plot_star_coords (catalog, Ncatalog);
    // plot_mosaic_fields (catalog);
  }

  /* determine fit values */
  client_logger_message ("starting the loops : %d \n", NLOOP);
  for (i = 0; i < NLOOP; i++) {
    SetZptIteration (i);

    // set the mean stellar mags given the measurements and the image calibrations
    setMrel  (catalog, Ncatalog);

    // share mean mags for objects at the boundary (number of unowned meas > 0)
    share_mean_mags (catalog, Ncatalog, regionHosts, i);
    client_logger_message ("shared mean mag data : loop %d \n", i);

    // load mean mags from other region hosts
    slurp_mean_mags (catalog, Ncatalog, regionHosts, i);
    client_logger_message ("slurped mean mag data : loop %d \n", i);

    // set the image (Mcal) and mosaic (Mmos) zero point offsets given the mean mags and measurements
    setMcal  (catalog);
    setMmos  (catalog);
    MARKTIME("-- set Mrel, Mcal, Mmos, Mgrid : %f sec\n", dtime);
    
    // share image mags for images with non-zero unowned detections
    share_image_mags (regionHosts, i);
    client_logger_message ("shared image data : loop %d \n", i);

    slurp_image_mags (regionHosts, i);
    client_logger_message ("slurped image data : loop %d \n", i);

    global_stats (catalog, Ncatalog, i);

    SetZeroPointModes (catalog, Ncatalog);
    MARKTIME("-- finished loop %d: %f sec\n", i, dtime);
  }
  client_logger_message ("done with loops\n");

  // this is a checkpoint to make sure all hosts have finished the loop above
  char *loopsyncfile = make_filename (CATDIR, regionHosts->hosts[myHost].hostname, REGION_HOST_ID, "loop.sync");
  update_sync_file (loopsyncfile, 0);
  free (loopsyncfile);

  for (i = 0; i < regionHosts->Nhosts; i++) {
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    char *loopsync = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "loop.sync");
    check_sync_file (loopsync, 0);
    free (loopsync);
  }    
  client_logger_message ("all hosts are done the loops\n");

  MARKTIME("-- finalize Mcal values: %f sec\n", dtime);

  setMcalFromMosaics (); // copy per-mosaic calibrations to the images

  share_image_mags (regionHosts, -1);

  for (i = 0; i < Ncatalog; i++) {
    // these tiny values are set by BrightCatalogSplit from load_catalogs
    free_tiny_values (&catalog[i]);
    dvo_catalog_free (&catalog[i]);
  }
  free (catalog);
  
  freeCatalogIndexes (Ncatalog);
  freeImageBins(Ncatalog, TRUE);
  freeMosaicBins (Ncatalog, TRUE);
  freeImages((char *)image);
  free (image);

  SkyListFree(skylist);
  FreeRegionHostTable (regionHosts);

  client_logger_message ("done with parallel images\n");

  return TRUE;
}
