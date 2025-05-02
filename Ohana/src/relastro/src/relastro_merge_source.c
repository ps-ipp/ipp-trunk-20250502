# include "relastro.h"

int relastro_merge_source (SkyTable *sky) {

  int i, m;
  Catalog catalog_src, catalog_dst;

  // relastro -merge-source (objID) (catID) into (objID) (catID) 
  // merge the detections of the first source with those of the second source
  
  // tell libdvo the CATDIR
  dvo_set_catdir(CATDIR);

  // we are merging the detections of the src into the dst : OBJ_ID_SRC,CAT_ID_SRC -> OBJ_ID_DST,CAT_ID_DST

  // find the catalog containing the src object:
  dvo_catalog_init (&catalog_src, TRUE);
  dvo_catalog_init (&catalog_dst, TRUE);
  SkyRegion *region_src = NULL;
  // SkyRegion *region_dst = NULL;

  // load data from each region file, only use bright stars
  for (i = 0; i < sky[0].Nregions; i++) {
    myAssert (sky[0].regions[i].index >= 0, "oops");
    if ((unsigned int) sky[0].regions[i].index == CAT_ID_SRC) {
      catalog_src.filename = sky[0].filename[i];
      region_src = &sky[0].regions[i];
    }
    // currently, we only accept dst == src...
    if ((unsigned int) sky[0].regions[i].index == CAT_ID_DST) {
      catalog_dst.filename = sky[0].filename[i];
      // region_dst = &sky[0].regions[i];
    }
  }    

  if (!catalog_src.filename) {
    fprintf (stderr, "ERROR: cannot find catalog file matching source catID %x\n", CAT_ID_SRC);
    exit (2);
  }
  if (!catalog_dst.filename) {
    fprintf (stderr, "ERROR: cannot find catalog file matching destination catID %x\n", CAT_ID_DST);
    exit (2);
  }

  // load the source object catalog:
  catalog_src.catformat = dvo_catalog_catformat (CATFORMAT);    // set the default catformat from config data
  catalog_src.catmode   = dvo_catalog_catmode (CATMODE);        // set the default catmode from config data
  catalog_src.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;
  catalog_src.Nsecfilt  = GetPhotcodeNsecfilt ();

  if (!dvo_catalog_open (&catalog_src, region_src, VERBOSE2, "w")) {
    fprintf (stderr, "ERROR: failure reading catalog %s\n", catalog_src.filename);
    exit (1);
  }
  if (!catalog_src.Naverage_disk) {
    fprintf (stderr, "no data in %s, error in cat ID?\n", catalog_src.filename);
    exit (1);
  }

  // find the source object (by objID)
  int index_src = -1;
  for (i = 0; i < catalog_src.Naverage; i++) {
    if (OBJ_ID_SRC != catalog_src.average[i].objID) continue;
    index_src = i;
    break;
  }
  if (index_src < 0) {
    fprintf (stderr, "cannot find source object ID %x\n", OBJ_ID_SRC);
    exit (1);
  }

  // save the measures from this object 
  // Measure *measures_src;
  // ALLOCATE (measures_src, Measure, catalog_src.average[index_src].Nmeasure);
  // m = catalog_src.average[index_src].measureOffset;
  // for (i = 0; i < catalog_src.average[index_src].Nmeasure; i++, m++) {
  //   memcpy (&measures_src[i], &catalog_src.measure[m], sizeof(Measure));
  // }
    
  if (CAT_ID_SRC == CAT_ID_DST) {

    // find the target objects 
    int index_dst = -1;
    for (i = 0; i < catalog_src.Naverage; i++) {
      if (OBJ_ID_DST != catalog_src.average[i].objID) continue;
      index_dst = i;
      break;
    }
    if (index_dst < 0) {
      fprintf (stderr, "cannot find source object ID %x\n", OBJ_ID_DST);
      exit (1);
    }

    // repoint the src measures at this object
    m = catalog_src.average[index_src].measureOffset;
    for (i = 0; i < catalog_src.average[index_src].Nmeasure; i++, m++) {

      // update objID & catID to match the new source
      catalog_src.measure[m].objID  = catalog_src.average[index_dst].objID;
      catalog_src.measure[m].catID  = catalog_src.average[index_dst].catID;
      catalog_src.measure[m].averef = index_dst;

      // OLD CODE: when measure.dR,dD were relative to average.R,D it was necessary to modify them
      // get the instantaneous positions:
      // DROP double R = catalog_src.average[index_src].R - catalog_src.measure[m].dR / 3600.0;
      // DROP double D = catalog_src.average[index_src].D - catalog_src.measure[m].dD / 3600.0;

      // update the offset coordinates to match the new source
      // DROP catalog_src.measure[m].dR = 3600.0*(catalog_src.average[index_dst].R - R);
      // DROP catalog_src.measure[m].dD = 3600.0*(catalog_src.average[index_dst].D - D);
    }

    // update the count? (updated by resort?)
    catalog_src.average[index_dst].Nmeasure += catalog_src.average[index_src].Nmeasure;

    // for the moment, don't delete this object...
    catalog_src.average[index_src].Nmeasure = 0;
    catalog_src.average[index_src].measureOffset = 0;

    catalog_src.sorted = FALSE;

    // resort the measure table and save
    resort_catalog (&catalog_src);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog_src, VERBOSE2)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog_src.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog_src)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog_src.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog_src);

    exit (0);
  }

  return TRUE;
}

