#include <dvo_util.h>

// dvo_util.c
// This file contains a set of functions that provide simple read-only access to a dvo database
//

void dvoFree(void *ptr) {
  if (ptr) {
    FREE(ptr);
  }
}

dvoConfig *dvoConfigAlloc() {
  dvoConfig *config;

  ALLOCATE(config, dvoConfig, 1);

  memset(config, 0, sizeof(dvoConfig));
  return config;
}

void dvoConfigFree(dvoConfig *dvoConfig) {
  if (dvoConfig) {
    if (dvoConfig->skyTable) {
      SkyTableFree(dvoConfig->skyTable);
    }
    if (dvoConfig->images) {
      gfits_db_free(&dvoConfig->imageDB);
    }
    FREE(dvoConfig);
  }
}

dvoConfig *dvoConfigRead(int *argc, char **argv) {
  dvoConfig *dvoConfig = dvoConfigAlloc();
  char MasterPhotcodeFile[256];

  char *file = SelectConfigFile(argc, argv, "ptolemy");
  char *configData = LoadConfigFile(file);
  if (!configData) {
    fprintf(stderr, "ERROR: can't find configuration file %s\n", file);
    if (file) {
      FREE(file);
    }
    exit(3);
  }
  FREE (file);
  file = NULL;

  ScanConfig (configData, "GSCFILE",         "%s", 0, dvoConfig->gscfile);
  ScanConfig (configData, "CATDIR",          "%s", 0, dvoConfig->catdir);
  ScanConfig (configData, "CATMODE",         "%s",  0, dvoConfig->catmode);
  ScanConfig (configData, "CATFORMAT",       "%s",  0, dvoConfig->catformat);
  ScanConfig (configData, "PHOTCODE_FILE",   "%s",  0, MasterPhotcodeFile);
  if (!ScanConfig (configData, "SKY_DEPTH",    "%d",  0, &dvoConfig->skyDepth)) {
    dvoConfig->skyDepth = 2;
  }
  if (!ScanConfig (configData, "SKY_TABLE",    "%s",  0, dvoConfig->skyTableFile)) {
    dvoConfig->skyTableFile[0] = 0;
  }

  // photcodeFile[] must be > catdir[]
  snprintf (dvoConfig->photcodeFile, PHOTCODE_FILE_SIZE, "%s/Photcodes.dat", dvoConfig->catdir);
  if (!LoadPhotcodes (dvoConfig->photcodeFile, MasterPhotcodeFile, FALSE)) {
    fprintf (stderr, "error loading photcode table %s or master file %s\n",
	     dvoConfig->photcodeFile, MasterPhotcodeFile);
    exit (1);
  }

  double zero_point;
  ScanConfig (configData, "ZERO_PT",                "%lf", 0, &zero_point);
  SetZeroPoint (zero_point);

#if (DVO_UTIL_READ_CAMERA_CONFIG)
  // at one point I thought reading the camera configuration might be useful
  // but we didn't need it
  if (!ScanConfig (configData, "CAMERA_CONFIG", "%s", 0, dvoConfig->cameraConfig)) {
    fprintf (stderr, "can't find CAMERA_CONFIG in configuration\n");
    exit (3);
  }
  char * cameraConfigData = LoadConfigFile(dvoConfig->cameraConfig);
  if (!cameraConfigData) {
    fprintf (stderr, "failed to load %s\n", dvoConfig->cameraConfig);
    exit (3);
  }

  ScanConfig (cameraConfigData, "NCCD", "%d", 1, &dvoConfig->nCCD);
#endif

  return (dvoConfig);
}

int dvoLoadImages(dvoConfig *dvoConfig) {
  if (dvoConfig->images) {
    return TRUE;
  }

  char filename[280];
  
  snprintf (filename, 280, "%s/Images.dat", dvoConfig->catdir);

  gfits_db_init (&dvoConfig->imageDB);
  dvoConfig->imageDB.lockstate = LCK_SOFT;
  dvoConfig->imageDB.timeout   = 120.0;

  if (!gfits_db_lock (&dvoConfig->imageDB, filename)) {
    fprintf (stderr, "error opening image catalog %s (1)\n", filename);
    return (FALSE);
  }

  if (dvoConfig->imageDB.dbstate == LCK_EMPTY) {
    fprintf (stderr, "note: image catalog is empty\n");
    ALLOCATE (dvoConfig->images, Image, 1);
    dvoConfig->nImages = 1;
    return (TRUE);
  }

  int status = dvo_image_load (&dvoConfig->imageDB, TRUE, FALSE);
  gfits_db_close (&dvoConfig->imageDB);

  if (!status) {
    fprintf (stderr, "problem loading image database table\n");
    return (FALSE);
  }

  dvoConfig->images = gfits_table_get_Image (&dvoConfig->imageDB.ftable, &dvoConfig->nImages, &dvoConfig->imageDB.scaledValue, &dvoConfig->imageDB.nativeOrder);
  if (!dvoConfig->images) {
    fprintf (stderr, "problem loading images\n");
    return (FALSE);
  }

  return (TRUE);
}

Image *dvoImageByExternID(dvoConfig *dvoConfig, unsigned short sourceID, unsigned int externID) {

  unsigned int i;
    
  if (!dvoLoadImages(dvoConfig)) return NULL;

  for (i = 0; i < dvoConfig->nImages; i++) {
    Image *image = dvoConfig->images + i;
    if ((image->externID == externID) && (image->sourceID == sourceID)) {
      BuildChipMatch(dvoConfig->images, dvoConfig->nImages);
      return image;
    }
  }
  fprintf(stderr, "can't find image for %d %d\n", sourceID, externID);
  return NULL;
}

SkyTable *dvoLoadSkyTable(dvoConfig *dvoConfig) {
  if (!dvoConfig->skyTable) {
    char *skyfile = "";
    dvoConfig->skyTable = SkyTableLoadOptimal(dvoConfig->catdir, skyfile, dvoConfig->gscfile,
					      FALSE, dvoConfig->skyDepth, 0);
  }

  if (dvoConfig->skyTable == NULL) {
    fprintf(stderr, "failed to load SkyTable\n");
    return NULL;
  }

  SkyTableSetFilenames(dvoConfig->skyTable, dvoConfig->catdir, "cpt");

  return dvoConfig->skyTable;
}

SkyList *dvoSkyListByExternID(dvoConfig *dvoConfig, int sourceID, int externID, Image **ppImage) {
  Image *image = dvoImageByExternID(dvoConfig, sourceID, externID);
  if (image == NULL) {
    // fprintf(stderr, "can't find image for %d %d\n", sourceID, externID);
    return NULL;
  }

  if (dvoLoadSkyTable(dvoConfig) == NULL) {
    fprintf(stderr, "failed to load Sky table\n");
    return NULL;
  }
  SkyList *skylist = SkyListByImage(dvoConfig->skyTable, -1, image);
  if (!skylist->Nregions) {
    fprintf(stderr, "failed to find SkyList for image  %d %d\n", sourceID, externID);
    return NULL;
  }

  *ppImage = image;

  return skylist;
}

off_t dvoGetDetections(SkyList *skylist, unsigned int imageID, dvoDetection **results, unsigned int *pMaxDetID) {
  int GetMeasures = 1;
  int reg;
  off_t Ndetect = 0;

  int detectionsArrayLength = 1000;
  dvoDetection *detections;
  ALLOCATE (detections, dvoDetection, detectionsArrayLength);
  *pMaxDetID = -1;
  for (reg=0; reg< skylist->Nregions; reg++) {
    /* lock, load, unlock catalog */
    Catalog catalog;

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist->filename[reg];
    catalog.catflags = GetMeasures ? DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT : DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    off_t Nmeasure = catalog.Nmeasure;

    off_t i;
    for (i=0; i< Nmeasure; i++) {
      Measure *m = catalog.measure + i;
      if (m->imageID != imageID) {
	continue;
      }
      Average *a = catalog.average + m->averef;
        
      dvoDetection *det = detections + Ndetect;

      det->valid     = 1;
      det->ave      = *a;
      det->meas     = *m;
#ifdef notdfe
      det->objID     = a->objID;
      det->catID     = m->catID;
      det->detID     = m->detID;
      det->pspsObjID = a->extID;
      det->pspsDetID = m->extID;
#endif
        
      if (det->meas.detID > *pMaxDetID) {
	*pMaxDetID = det->meas.detID;
      }

      Ndetect++;
      CHECK_REALLOCATE(detections, dvoDetection, detectionsArrayLength, Ndetect, 1000);
    }

    dvo_catalog_free (&catalog);
  }
  size_t NInvalidDetID = 0;
  if (Ndetect) {
    dvoDetection *sorted;
    ALLOCATE(sorted, dvoDetection, *pMaxDetID + 1);

    memset(sorted, 0, sizeof(dvoDetection) * *pMaxDetID);
    off_t i;
    for (i = 0; i < Ndetect; i++) {

      // if (detections[i].meas.detID < 0 || detections[i].meas.detID > (*pMaxDetID + 1))

      if (detections[i].meas.detID > (*pMaxDetID + 1))
	NInvalidDetID++;
      else
	sorted[detections[i].meas.detID] = detections[i];
    }
    *results = sorted;
  } else {
    *results = NULL;
  }

  FREE(detections);

  if (NInvalidDetID) fprintf (stderr, "ERROR: Encountered %zx invalid detection IDs\n", NInvalidDetID);

  return (Ndetect);

}
