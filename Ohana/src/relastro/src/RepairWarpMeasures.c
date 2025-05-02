# include "relastro.h"

// XXX need to load_images first and generate some lookup tables
WarpRepairResult RepairWarpMeasures (Catalog *catalog) {

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);

  myAssert (!catalog->Nmeasure || catalog->measure, "programming error");

  WarpRepairResult result;
  result.NfixChipID = 0;
  result.NfixStackID = 0;
  result.NfixWarpID = 0;
  result.NfixWarpImageID = 0;
  result.NfixWarpCoord = 0;
  result.NmissWarp = 0;
  result.NmissStack = 0;
  result.NbadWarp = 0;
  result.NbadWarpTime = 0;
  result.NwarpNoImage = 0;

  int onePercent = catalog->Nmeasure / 100;

  for (off_t j = 0; j < catalog->Nmeasure; j++) {
    if (j % onePercent == 0) fprintf (stderr, ".");

    Measure *measure = &catalog->measure[j];

    // repair extID for non-warps:
    if (isGPC1chip(measure->photcode))  {
      double mjd = ohana_sec_to_mjd (measure->t);
      int ccdnum = measure->photcode % 100;
      uint64_t extID = CreatePSPSDetectionID (mjd, ccdnum, measure->detID);
      if (extID != measure->extID) {
	measure->extID = extID;
	result.NfixChipID ++;
      }
      continue;
    }

    // repair extID for non-warps:
    if (isGPC1stack(measure->photcode))  {
      int im = getImageByID (measure->imageID);
      if (im < 0) {
	if (result.NmissStack < 10) fprintf (stderr, "missing stack exposure: %f %f : %d %d\n", measure->R, measure->D, measure->imageID, measure->photcode);
	result.NmissStack ++;
	continue;
      }
      uint64_t extID = CreatePSPSStackDetectionID (35, image[im].externID, measure->detID);
      if (extID != measure->extID) {
	measure->extID = extID;
	result.NfixStackID ++;
      }
      continue;
    }

    // we are only going to repair warp detections
    if (!isGPC1warp(measure->photcode)) continue;

    // get the associated average value:
    int averef = measure->averef;
    Average *average = &catalog->average[averef];

    // double-check for consistency
    myAssert (average->objID == measure->objID, "objID mismatch: %f %f : %d %d\n", average->R, average->D, average->objID, measure->objID);

    // warp coordinates to confirm warp
    double X = measure->Xccd;
    double Y = measure->Yccd;

    // check if this detection is far from its correct location
    double Rwrp = measure->R;
    double Dwrp = measure->D;

    /* 
       we have a warp detection.  X,Y are correct (never modified).  things which might be wrong:
       * imageID (thus the Mcal, R,D, extID may all be broken)
       */

    int coordsDiffer = FALSE;
    if (USE_IMAGE_COORDS_FOR_REPAIR) {
      off_t idx = getImageByID (measure->imageID);
      if (idx == -1) {
	if (VERBOSE2) fprintf (stderr, "can't match detection to image?\n");
	result.NwarpNoImage ++;
	continue;
      }

      Coords *imcoords = &image[idx].coords;

      // save the original warp coordinates for comparison
      double Rold = ohana_normalize_angle_to_midpoint(Rwrp, 180.0);
      double Dold = ohana_normalize_angle_to_midpoint(Dwrp,   0.0);

      XY_to_RD (&Rwrp, &Dwrp, X, Y, imcoords);
      
      Rwrp = ohana_normalize_angle_to_midpoint(Rwrp, 180.0);
      Dwrp = ohana_normalize_angle_to_midpoint(Dwrp,   0.0);

      // check if the new Rwrp, Dwrp is different from the current R,D:
      float csdec = cos(Dold * RAD_DEG);

      // find the ra,dec displacement in arcsec:
      double dR = 3600.0*(Rwrp - Rold)*csdec;
      double dD = 3600.0*(Dwrp - Dold);
      
      // skip detections which are within a small distance of the expected location
      // NOTE: this actually works surprisingly well near the pole
      double dPos = hypot(dR,dD);
      if (dPos > 0.5) { 
	// in this case, we should double-check measure.R,D values below
	coordsDiffer = TRUE;
      }
    }

    double Rave = average->R;
    double Dave = average->D;
      
    float csdec = cos(Dave * RAD_DEG);

    // find the ra,dec displacement in arcsec:
    double dR = 3600.0*(Rwrp - Rave)*csdec;
    double dD = 3600.0*(Dwrp - Dave);

    // skip detections which are within a small distance of the expected location
    // NOTE: this actually works surprisingly well near the pole
    double dPos = hypot(dR,dD);
    if (dPos < 3.0) { 
      // image coord agrees with average coord
      if (coordsDiffer) {
	// measure coord disagrees with image coord: replace with image coord
	measure->R = Rwrp;
	measure->D = Dwrp;
	result.NfixWarpCoord ++;
      }
      // check that the current extID value is good for warp detection:
      int im = getImageByID (measure->imageID);
      uint64_t extID = CreatePSPSStackDetectionID (34, image[im].externID, measure->detID);
      if (extID != measure->extID) {
	measure->extID = extID;
	result.NfixWarpID ++;
      }
      // check match of obstime:
      if ((measure->t != image[im].tzero) && (result.NbadWarpTime < 10)) {
	fprintf (stderr, "inconsistent warp measurement times: %d vs %d for %f, %f (%d, %ld)", measure->t, image[im].tzero, Rwrp, Dwrp, measure->photcode, extID); 
	result.NbadWarpTime ++;
      }
      continue;
    }

    // find the corrected warp ID:
    int warpSeq = GetWarpSeq (image, measure->t, measure->photcode, Rave, Dave, X, Y);
    if (warpSeq < 0) {
      if (result.NmissWarp < 10) fprintf (stderr, "missing warp exposure: %f %f : %d %d\n", Rave, Dave, measure->t, measure->photcode);
      result.NmissWarp ++;
      continue;
    }

    // assert on coords being TAN?
    Coords *imcoords = &image[warpSeq].coords;

    double Rnew, Dnew;
    XY_to_RD (&Rnew, &Dnew, X, Y, imcoords);

    Rnew = ohana_normalize_angle_to_midpoint(Rnew, 180.0);
    Dnew = ohana_normalize_angle_to_midpoint(Dnew,   0.0);

    // find the ra,dec displacement in arcsec:
    double dRnew = 3600.0*(Rnew - Rave)*csdec;
    double dDnew = 3600.0*(Dnew - Dave);

    // detections should now be repaired; fail if not
    dPos = hypot(dRnew,dDnew);
    if (dPos > 5.0) {
      fprintf (stderr, "measurement still far from average location: %f %f vs %f %f : %d %d\n", Rave, Dave, Rnew, Dnew, measure->t, measure->photcode);
      result.NbadWarp ++;
    }

    measure->imageID = image[warpSeq].imageID;
    measure->R = Rnew;
    measure->D = Dnew;

    uint64_t extID = CreatePSPSStackDetectionID (34, image[warpSeq].externID, measure->detID);
    if (extID != measure->extID) {
      measure->extID = extID;
      result.NfixWarpID ++;
    }
    result.NfixWarpImageID ++;
  }

  fprintf (stderr, "\n");
  fprintf (stderr, "repaired %s : warp image ID %d : ext ID chip %d stack %d warp %d : missed warp %d stack %d : bad warp: %d : warp no image: %d : badWarpTime: %d, fixWarpCoord: %d\n", 
	   catalog->filename, 
	   result.NfixWarpImageID, result.NfixChipID, result.NfixStackID, 
	   result.NfixWarpID, result.NmissWarp, result.NmissStack, 
	   result.NbadWarp, result.NwarpNoImage, result.NbadWarpTime, 
	   result.NfixWarpCoord);

  return (result);
}
