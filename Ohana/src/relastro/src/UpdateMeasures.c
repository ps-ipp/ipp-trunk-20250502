# include "relastro.h"
void dump_measures (Average *average, Measure *measure); // in ImageOps.c

// this function is called by UpdateObjectOffsets or relastro_images.c for reset
int UpdateMeasures (Catalog *catalog, int Ncatalog) {

  off_t i, j, Nimage;
  Image *image;
  double DPOS_MAX_ASEC;

  image = getimages (&Nimage, NULL);

  for (i = 0; i < Ncatalog; i++) {

    // track measurements which are far from their original position (poor at pole)
    int NoffRAori = 0; 
    int NoffDECori = 0;

    int NoffRAchip = 0; 
    int NoffRAstack = 0; 
    int NoffRAwarp = 0; 

    int NoffDECchip = 0; 
    int NoffDECstack = 0; 
    int NoffDECwarp = 0; 

    myAssert (!catalog[i].Nmeasure || catalog[i].measureT, "programming error");
    for (j = 0; j < catalog[i].Naverage; j++) {
      // normalize R,D to 0,360 & -90,90
      Average *average = &catalog[i].average[j];
      average[0].R = ohana_normalize_angle_to_midpoint(average[0].R, 180.0);
      average[0].D = ohana_normalize_angle_to_midpoint(average[0].D,   0.0);
    }

    for (j = 0; j < catalog[i].Nmeasure; j++) {
      MeasureTiny *measureT = &catalog[i].measureT[j];
      Measure     *measureB = catalog[i].measure ? &catalog[i].measure[j] : NULL;

      if (measureB->averef == 0x01a) {
	// fprintf (stderr, "test object\n");
      }
      if ((measureB->averef == 0x01a) && (measureB->detID == 0x000d469a)) {
	// fprintf (stderr, "example object\n");
      }
      if ((measureB->averef == 0x01e) && (measureB->detID == 0x000d487f)) {
	// fprintf (stderr, "example object\n");
      }

      Average *average = &catalog[i].average[measureB->averef];
      int FOUND_TEST = FALSE;
      FOUND_TEST |= (average[0].objID == OBJ_ID_SRC) && (average[0].catID == CAT_ID_SRC);
      FOUND_TEST |= (average[0].objID == OBJ_ID_DST) && (average[0].catID == CAT_ID_DST);
      if (FOUND_TEST) {
	fprintf (stderr, "found test object\n");
      }

      // we need this below but cannot declare it after the label update_this_measure:
      off_t im;

      // we have a few options to include/exclude specific types of measurements
      if (UPDATE_ALL_MEASURE) goto update_this_measure;
      if (isGPC1chip (measureT->photcode)  && UPDATE_PS1_CHIP_MEASURE) goto update_this_measure;
      if (isGPC1stack (measureT->photcode) && UPDATE_PS1_STACK_MEASURE) goto update_this_measure;
      if (isHSCchip (measureT->photcode)   && UPDATE_HSC_MEASURE) goto update_this_measure;
      if (isCFHchip (measureT->photcode)   && UPDATE_CFH_MEASURE) goto update_this_measure;
      continue; // skip all others

      update_this_measure:
      im = getImageByID (measureT->imageID);
      if (im < 0) continue; // detections without imageIDs are not associated with a known image (eg, refs)

      // check that we have the right image:
# if (0)
      myAssert (measureT->photcode == image[im].photcode, "image photcode mismatch");
      myAssert (measureT->t >= image[im].tzero, "image time mismatch (1)");
      myAssert (measureT->t <= image[im].tzero + image[im].NX*image[im].trate/5000.0, "image time mismatch (2)");
# endif
      // I'm allowing the trate*NX to be a factor of 2x too small

      Coords *moscoords = image[im].coords.mosaic;
      Coords *imcoords = &image[im].coords;

      if (moscoords) {
	DPOS_MAX_ASEC = 3600.0*DPOS_MAX*hypot(moscoords->cdelt1, moscoords->cdelt2);
      } else {
	DPOS_MAX_ASEC = 3600.0*DPOS_MAX*hypot(imcoords->cdelt1, imcoords->cdelt2);
      }

      double X = measureT->Xccd;
      double Y = measureT->Yccd;
      if (USE_FIXED_PIXCOORDS) {
	if (isfinite(measureT->Xfix) && isfinite(measureT->Yfix)) {
	  float dX = measureT->Xfix - measureT->Xccd;
	  float dY = measureT->Yfix - measureT->Yccd;
	  if (hypot(dX,dY) < 2.0) {
	    X = measureT->Xfix;
	    Y = measureT->Yfix;
	  } 
	}
      }

      double R, D;
      XY_to_RD (&R, &D, X, Y, imcoords);

      R = ohana_normalize_angle_to_midpoint(R, 180.0);
      D = ohana_normalize_angle_to_midpoint(D,   0.0);

      double oldR = ohana_normalize_angle_to_midpoint(measureT[0].R, 180.0);
      double oldD = ohana_normalize_angle_to_midpoint(measureT[0].D,   0.0);

      float csdec = cos(oldD * RAD_DEG);

      double dR = 3600.0*fabs(csdec*(oldR - R));
      double dD = 3600.0*fabs(oldD - D);

      // complain if the new location is far from the old location
      if (dR > DPOS_MAX_ASEC) {
	NoffRAori ++;
	if (isGPC1chip  (measureT->photcode)) NoffRAchip ++;
	if (isGPC1stack (measureT->photcode)) NoffRAstack ++;
	if (isGPC1warp  (measureT->photcode)) NoffRAwarp ++;
	if (VERBOSE2 && catalog[i].measure && (NoffRAori < 100)) {
	  fprintf (stderr, "measurement moves far from original location (R): %f %f : %f %f (%f) : %d\n", oldR, oldD, R, D, dR, measureT->photcode);
	}
      }
      if (dD > DPOS_MAX_ASEC) {
	NoffDECori ++;
	if (isGPC1chip  (measureT->photcode)) NoffDECchip ++;
	if (isGPC1stack (measureT->photcode)) NoffDECstack ++;
	if (isGPC1warp  (measureT->photcode)) NoffDECwarp ++;
	if (VERBOSE2 && catalog[i].measure && (NoffDECori < 100)) {
	  fprintf (stderr, "measurement moves far from original location (D): %f %f : %f %f (%f) : %d\n", oldR, oldD, R, D, dD, measureT->photcode);
	}
      }

      // modify the measure coordinates
      measureT->R = R;
      measureT->D = D;
      if (measureB) {
	measureB->R = R;
	measureB->D = D;
      }
    }
    if (VERBOSE) fprintf (stderr, "%s : Noff ori RA %d ( %d %d %d ), Noff ori DEC %d ( %d %d %d )\n", catalog[i].filename, NoffRAori, NoffRAchip, NoffRAstack, NoffRAwarp, NoffDECori, NoffDECchip, NoffDECstack, NoffDECwarp);
  }

  return (TRUE);
}

// this function operates on Measure, not MeasureTiny
int UpdateMeasuresOld (Catalog *catalog, int Ncatalog) {

  off_t i, Nimage;
  Image *image;

  int badImage = 
    ID_IMAGE_ASTROM_NOCAL | 
    ID_IMAGE_ASTROM_POOR | 
    ID_IMAGE_ASTROM_FAIL | 
    ID_IMAGE_ASTROM_SKIP | 
    ID_IMAGE_ASTROM_FEW;

  if (RESET_BAD_IMAGES) badImage = 0;

  image = getimages (&Nimage, NULL);

  for (i = 0; i < Nimage; i++) {

    /* skip DIS images (since they have no associated detections) */
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) continue;

    // skip images that have failed solutions (divergent or otherwise)
    if (image[i].flags & badImage) continue;

    /* convert measure coordinates to raw entries */
    fixImageRaw (catalog, Ncatalog, i);
  }

  // printNcatTotal ();

  return (TRUE);
}

