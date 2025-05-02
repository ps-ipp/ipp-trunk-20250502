# include "relastro.h"

// we are sharing image astrometry calibrations for all images which (a) I own and (b) which have unowned detections

# define D_NIMAGEPOS 1000
int share_image_pos (RegionHostTable *regionHosts, int nloop) {

  off_t i, Nimages;
  Image *images = getimages (&Nimages, NULL);

  off_t Nimage_pos = 0;
  off_t NIMAGE_POS = D_NIMAGEPOS;
  
  ImagePos *image_pos = NULL;
  ALLOCATE (image_pos, ImagePos, NIMAGE_POS);

  for (i = 0; i < Nimages; i++) {
    // XXX does this image have missing detections (does someone else need it?)
    // XXX : NOTE NEED TO FIX THIS: if (imageExtra[i].Nmiss == 0) continue;
    
    set_image_pos (&image_pos[Nimage_pos], &images[i]);
    Nimage_pos ++;

    CHECK_REALLOCATE (image_pos, ImagePos, NIMAGE_POS, Nimage_pos, D_NIMAGEPOS);
  }

  // write out the image_mag fits table AND write state in some file
  int myHost = regionHosts->index[REGION_HOST_ID];
  char *hostname = regionHosts->hosts[myHost].hostname;

  char *iposfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "imagepos.fits");
  ImagePosSave (iposfile, image_pos, Nimage_pos);
  free (image_pos);
  free (iposfile);

  // save the per-chip residual table
  AstromOffsetTable *table = get_astrom_table ();
  if (table) {
    char mapname[1024];
    snprintf_nowarn (mapname, 1024, "%s/AstroMapUpdate.%d.fits", CATDIR, REGION_HOST_ID);
    
    // write the image subset for this host
    AstromOffsetMapSave (table, mapname);
  }

  char *syncfile = make_filename (CATDIR, hostname, REGION_HOST_ID, "imagepos.sync");
  update_sync_file (syncfile, nloop);
  free (syncfile);

  return TRUE;
}

static char *masterHost = "master";
int slurp_image_pos (Catalog *catalog, int Ncatalog, RegionHostTable *regionHosts, int nloop) {

  off_t Nimage, i, j;
  Image *images = getimages (&Nimage, NULL);

  int Nimage_pos = 0;
  ImagePos *image_pos = NULL;
  ALLOCATE (image_pos, ImagePos, 1);

  INITTIME;
  int myHost = regionHosts->index[REGION_HOST_ID];
  char *myHostName = (myHost == -1) ? masterHost : regionHosts->hosts[myHost].hostname;

  fprintf (stderr, "grabbing image mags from other hosts...\n");

  LOGRTIME("image_load_start loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);
  for (i = 0; i < regionHosts->Nhosts; i++) {
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    if (REGION_HOST_ID && !regionHosts->hosts[i].isNeighbor) continue;

    char *syncfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagepos.sync");
    check_sync_file (syncfile, nloop);
    free (syncfile);
    LOGRTIME("image_load_sync host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);
    
    off_t Nsubset;
    char *iposfile = make_filename (CATDIR, regionHosts->hosts[i].hostname, regionHosts->hosts[i].hostID, "imagepos.fits");
    ImagePos *image_pos_subset = ImagePosLoad (iposfile, &Nsubset);
    free (iposfile);
    LOGRTIME("image_load_read host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);

    image_pos = merge_image_pos (image_pos, &Nimage_pos, image_pos_subset, Nsubset);
    LOGRTIME("image_load_merge host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);
  }

  for (i = 0; i < Nimage_pos; i++) {
    off_t seq = getImageByID (image_pos[i].imageID);
    if (seq < 0) {
      // XXX is this a problem? (no, other hosts don't know which images I own)
      continue;
    }
    Coords *moscoords = images[seq].coords.mosaic;
    images[seq].coords     = image_pos[i].coords    ;
    images[seq].coords.mosaic = moscoords;

    images[seq].dXpixSys     = image_pos[i].dXpixSys  ;
    images[seq].dYpixSys     = image_pos[i].dYpixSys  ;
    images[seq].refColorBlue = image_pos[i].refColorBlue;
    images[seq].refColorRed  = image_pos[i].refColorRed;
    images[seq].imageID      = image_pos[i].imageID   ;
    images[seq].nFitAstrom   = image_pos[i].nFitAstrom;
    images[seq].flags        = image_pos[i].flags     ;
  }
  LOGRTIME("image_load_convert loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  // load the astrometry offset maps, if they exist, and apply 
  for (i = 0; i < regionHosts->Nhosts; i++) {
    if (regionHosts->hosts[i].hostID == REGION_HOST_ID) continue;
    if (REGION_HOST_ID && !regionHosts->hosts[i].isNeighbor) continue;

    char mapname[1024];
    snprintf_nowarn (mapname, 1024, "%s/AstroMapUpdate.%d.fits", CATDIR, regionHosts->hosts[i].hostID);
    
    AstromOffsetTable *table = AstromOffsetMapLoad (mapname, 100000, VERBOSE);
    LOGRTIME("image_maps_load host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);
  
    // apply table entries here to existing images
    for (j = 0; j < table->Nmap; j++) {
      off_t seq = getImageByID (table->map[j][0].imageID);
      // I do not necessarily own all images listed in the table.  skip if not found
      if (seq < 0) continue;

      AstromOffsetMap *oldMap = images[seq].coords.offsetMap;
      AstromOffsetMap *newMap = table->map[j];

      if (oldMap) {
	AstromOffsetMapSetOrder (oldMap, newMap->Nx, newMap->Ny, &images[seq]);
	AstromOffsetMapCopyData (oldMap, newMap);
      } else {
	lockUpdateChips ();
	AstromOffsetTable *FullTable = get_astrom_table ();
	AstromOffsetTableNewMap(FullTable, newMap->Nx, newMap->Ny, &images[seq]);  // registers the map with the image
	unlockUpdateChips ();
	AstromOffsetMapCopyData (images[seq].coords.offsetMap, newMap); // oldMap is now not valid
      }
    }
    LOGRTIME("image_maps_copy host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);

    AstromOffsetTableFree(table);
    free (table);
    LOGRTIME("image_maps_free host %d loop %d on %s, host %d: %f sec\n", i, nloop, myHostName, REGION_HOST_ID, dtime);
  }

  // apply the modified image parameters to my detections
  // This loop is skipped because the updated image parameters are applied to 
  // all measurements in UpdateChips (and those modified measurements are saved 
  // at the same time)
  for (i = 0; FALSE && (Ncatalog > 0) && (i < Nimage_pos); i++) {
    off_t seq = getImageByID (image_pos[i].imageID);
    if (seq < 0) continue;
    updateImageRaw (catalog, Ncatalog, seq);
  }
  free (image_pos);
  LOGRTIME("image_load_apply loop %d on %s, host %d: %f sec\n", nloop, myHostName, REGION_HOST_ID, dtime);

  fprintf (stderr, "DONE grabbing image mags from other hosts\n");

  return TRUE;
}

int set_image_pos (ImagePos *image_pos, Image *image) {

  image_pos->coords       = image->coords;
  image_pos->dXpixSys     = image->dXpixSys;
  image_pos->dYpixSys     = image->dYpixSys;
  image_pos->refColorBlue = image->refColorBlue;
  image_pos->refColorRed  = image->refColorRed;
  image_pos->imageID      = image->imageID;
  image_pos->nFitAstrom   = image->nFitAstrom;
  image_pos->flags        = image->flags;

  return TRUE;
}

ImagePos *merge_image_pos (ImagePos *target, int *ntarget, ImagePos *source, int Nsource) {

  off_t i;

  REALLOCATE (target, ImagePos, *ntarget + Nsource);
  for (i = 0; i < Nsource; i++) {
    off_t n = i + *ntarget;
    target[n] = source[i];
  }
  
  free (source);

  *ntarget += Nsource;
  return (target);
}

