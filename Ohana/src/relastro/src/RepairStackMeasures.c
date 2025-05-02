# include "relastro.h"

StackRepairResult RepairStackMeasures (Catalog *catalog) {

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);

  myAssert (!catalog->Nmeasure || catalog->measure, "programming error");

  // XXX define StackRepairResult
  StackRepairResult result;
  result.NStackFixExtID      = 0;
  result.NStackFixImageID    = 0;
  result.NstackBadCoords     = 0;
  result.NstackFailRepair_v1 = 0;
  result.NstackFailRepair_v2 = 0;
  result.NstackFixCoords     = 0;
  result.NstackMissImage     = 0;
  result.NstackMissTime      = 0;
  result.NstackNoImageID     = 0;

  int onePercent = catalog->Nmeasure / 100;

  for (off_t j = 0; j < catalog->Nmeasure; j++) {
    if (j % onePercent == 0) fprintf (stderr, ".");

    Measure     *measureB = &catalog->measure[j];
    MeasureTiny *measureT = catalog->measureT ? &catalog->measureT[j] : NULL;

    // only examine stack measurements
    if (!isGPC1stack(measureB->photcode)) continue;

    // get the associated average value:
    int averef = measureB->averef;
    Average *average = &catalog->average[averef];

    // double-check for consistency
    myAssert (average->objID == measureB->objID, "objID mismatch: %f %f : %d %d\n", average->R, average->D, average->objID, measureB->objID);

    // stack pixel coordinates
    double X = measureB->Xccd;
    double Y = measureB->Yccd;

    // use average position as a reference
    double Rave = average->R;
    double Dave = average->D;

    Rave = ohana_normalize_angle_to_midpoint(Rave, 180.0);
    Dave = ohana_normalize_angle_to_midpoint(Dave,   0.0);

    /* we have a stack detection.  X,Y are correct (never modified).  */

    off_t idx = getImageByID (measureB->imageID);
    if (idx == -1) {
      // ** this stack detection is not correctly associated with an image.  attempt to fix:
      result.NstackNoImageID ++;

      if (!RepairStackID (&result, image, Rave, Dave, measureB, measureT)) {
	result.NstackFailRepair_v1 ++;
      }
      continue;
    }

    // check if the image-derived coords and the average coords agree:
    Coords *imcoords = &image[idx].coords;

    double Rstk, Dstk;
    XY_to_RD (&Rstk, &Dstk, X, Y, imcoords);
    
    Rstk = ohana_normalize_angle_to_midpoint(Rstk, 180.0);
    Dstk = ohana_normalize_angle_to_midpoint(Dstk,   0.0);

    // check if the new Rstk, Dstk is different from the average R,D:
    float csdec = cos(Dave * RAD_DEG);

    // find the ra,dec displacement in arcsec:
    double dR = 3600.0*(Rstk - Rave)*csdec;
    double dD = 3600.0*(Dstk - Dave);
      
    // skip detections which are within a small distance of the expected location
    // NOTE: this actually works surprisingly well near the pole
    double dPos = hypot(dR,dD);
    if (dPos < 3.0) { 
      // stack image coords agree with average coords

      // check that the current extID value is good for stackdetection:
      uint64_t extID = CreatePSPSStackDetectionID (35, image[idx].externID, measureB->detID);
      if (extID != measureB->extID) {
	measureB->extID = extID; // only in measureB, not in measureT
	result.NStackFixExtID ++;
      }

      // check match of obstime:
      if ((measureB->t != image[idx].tzero) && (result.NstackMissTime < 10)) {
	fprintf (stderr, "inconsistent stack measurement times: %d vs %d for %f, %f (%d, %ld)\n", measureB->t, image[idx].tzero, Rstk, Dstk, measureB->photcode, extID); 
	result.NstackMissTime ++;
      }

      // check if the measure coords and image-based coords match
      // check if the new Rstk, Dstk is different from the average R,D:
      csdec = cos(Dstk * RAD_DEG);

      // find the ra,dec displacement in arcsec:
      dR = 3600.0*(Rstk - measureB->R)*csdec;
      dD = 3600.0*(Dstk - measureB->D);
      
      // skip detections which are within a small distance of the expected location
      // NOTE: this actually works surprisingly well near the pole
      double dPos = hypot(dR,dD);
      if (dPos > 0.05) { 
	measureB->R = Rstk;
	measureB->D = Dstk;
	if (measureT) {
	  measureT->R = measureB->R;
	  measureT->D = measureB->D;
	}
	result.NstackFixCoords ++;
      }
      continue;
    }

    if (!RepairStackID (&result, image, Rave, Dave, measureB, measureT)) {
      result.NstackFailRepair_v2 ++;
    }
  }

  fprintf (stderr, "\n");
  fprintf (stderr, "repaired stacks %s : imageID %d extID %d coords %d noImageID %d : bad coords %d : failures v1 %d v2 %d image %d time %d\n", 
	   catalog->filename, 
	   result.NStackFixImageID,
	   result.NStackFixExtID,
	   result.NstackFixCoords,
	   result.NstackNoImageID,
	   result.NstackBadCoords,
	   result.NstackFailRepair_v1,
	   result.NstackFailRepair_v2,
	   result.NstackMissImage,
	   result.NstackMissTime);

  return (result);
}

int RepairStackID (StackRepairResult *result, Image *image, double Rave, double Dave, Measure *measureB, MeasureTiny *measureT) {

    // get guess image based on R,D,photcode
    off_t idx = GetStackSeq (image, Rave, Dave, measureB->photcode, measureB->Xccd, measureB->Yccd);
    if (idx < 0) {
      // still failed, this is bad
      if (result->NstackMissImage < 10) fprintf (stderr, "missing stack image: %f %f : %d %d\n", Rave, Dave, measureB->imageID, measureB->photcode);
      result->NstackMissImage ++;
      return FALSE;
    }
      
    // check that this image and measure now agree
    if (image[idx].tzero != measureB->t) {
      // still failed, this is bad
      if (result->NstackMissTime < 10) fprintf (stderr, "new stack gives wrong time: %d vs %d : %f %f : %d %d\n", measureB->t, image[idx].tzero, Rave, Dave, measureB->imageID, measureB->photcode);
      result->NstackMissTime ++;
      return FALSE;
    }

    // XXX assert on coords being TAN?
    Coords *imcoords = &image[idx].coords;

    double Rnew, Dnew;
    XY_to_RD (&Rnew, &Dnew, measureB->Xccd, measureB->Yccd, imcoords);

    Rnew = ohana_normalize_angle_to_midpoint(Rnew, 180.0);
    Dnew = ohana_normalize_angle_to_midpoint(Dnew,   0.0);

    float csdec = cos(Dave * RAD_DEG);

    // find the ra,dec displacement in arcsec:
    double dRnew = 3600.0*(Rnew - Rave)*csdec;
    double dDnew = 3600.0*(Dnew - Dave);

    // detections should now be repaired; complain if not
    double dPos = hypot(dRnew,dDnew);
    if (dPos > 5.0) {
      fprintf (stderr, "new measurement position far from average location: %f %f vs %f %f : %d %d\n", Rave, Dave, Rnew, Dnew, measureB->t, measureB->photcode);
      result->NstackBadCoords ++;
    }

    // fix the image ID & coords:
    measureB->imageID = image[idx].imageID;
    measureB->R = Rnew;
    measureB->D = Dnew;

    if (measureT) {
      measureT->imageID = measureB->imageID;
      measureT->R       = measureB->R;
      measureT->D       = measureB->D;
    }

    // check (and fix) extID
    uint64_t extID = CreatePSPSStackDetectionID (35, image[idx].externID, measureB->detID);
    if (extID != measureB->extID) {
      measureB->extID = extID; // only in measureB, not in measureT
      result->NStackFixExtID ++;
    }

    // this detection is now fixed, right?
    result->NStackFixImageID ++;
    return TRUE;
}
