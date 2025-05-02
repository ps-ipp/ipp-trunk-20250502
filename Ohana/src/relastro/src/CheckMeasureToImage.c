# include "relastro.h"

// here are the tests I am making (for chip, warp, stack):
// imageID: measure->imageID matches a valid image
// time : measure->t matches image->tzero
// coords : measure->R,D within 3 arcsec of average->R,D
// imcoords : measure->R,D within 0.1 arcsec of [r,d](image[x,y])

int CheckMeasureToImage (Catalog *catalog) {

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);

  myAssert (!catalog->Nmeasure || catalog->measure, "programming error");

  // XXX define StackRepairResult
  CheckMeasureResult result;
  result.NstackNoImageID = 0;
  result.NstackBadCoords = 0;
  result.NstackBadTime = 0;
  result.NstackBadImageCoords = 0;
  result.NwarpNoImageID = 0;
  result.NwarpBadCoords = 0;
  result.NwarpBadTime = 0;
  result.NwarpBadImageCoords = 0;
  result.NchipNoImageID = 0;
  result.NchipBadCoords = 0;
  result.NchipBadTime = 0;
  result.NchipBadImageCoords = 0;

  int onePercent = catalog->Nmeasure / 100;

  // I want to check the measure->imageID for each photcode type:
  for (off_t j = 0; j < catalog->Nmeasure; j++) {
    if (j % onePercent == 0) fprintf (stderr, ".");

    Measure *measureB = &catalog->measure[j];

    if ((measureB->averef == 0x01a) && (measureB->detID == 0x000d469a)) {
      // fprintf (stderr, "example object\n");
    }
    if ((measureB->averef == 0x01e) && (measureB->detID == 0x000d487f)) {
      // fprintf (stderr, "example object\n");
    }

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

    // check if the new Rstk, Dstk is different from the average R,D:
    float csdec = cos(Dave * RAD_DEG);

    // things to check: time, photcode, coordinates

    // do them separately to track where the failures are:
    if (isGPC1stack(measureB->photcode)) {
      off_t idx = getImageByID (measureB->imageID);
      if (idx == -1) {
	if (result.NstackNoImageID < 10) fprintf (stderr, "missing stack imageID: %f %f : %d %d %d\n", measureB->R, measureB->D, measureB->t, measureB->photcode, measureB->imageID); 
	result.NstackNoImageID ++;
	continue;
      }

      // check if the image-derived coords and the average coords agree:
      Coords *imcoords = &image[idx].coords;
      
      double Rstk, Dstk;
      XY_to_RD (&Rstk, &Dstk, X, Y, imcoords);
      
      Rstk = ohana_normalize_angle_to_midpoint(Rstk, 180.0);
      Dstk = ohana_normalize_angle_to_midpoint(Dstk,   0.0);
      
      // find the ra,dec displacement in arcsec:
      double dR = 3600.0*(Rstk - Rave)*csdec;
      double dD = 3600.0*(Dstk - Dave);
      
      // skip detections which are within a small distance of the expected location
      // NOTE: this actually works surprisingly well near the pole
      double dPos = hypot(dR,dD);
      if (dPos > 3.0) { 
	if (result.NstackBadCoords < 100) fprintf (stderr, "inconsistent stack avecoords: %f %f : %f %f (%d %d) 0x%08x 0x%08x\n", Rave, Dave, dR, dD, measureB->t, measureB->photcode, average->objID, average->catID); 
	result.NstackBadCoords ++;
      }

      // check match of obstime:
      if ((measureB->t < image[idx].tzero) && (measureB->t > image[idx].tzero + image[idx].NX*image[idx].trate/5000.0)) {
	if (result.NstackBadTime < 10) fprintf (stderr, "inconsistent stack times: %d vs %d for %f, %f (%d)\n", measureB->t, image[idx].tzero, Rstk, Dstk, measureB->photcode); 
	result.NstackBadTime ++;
      }

      // check if the measure coords and image-based coords match
      // check if the new Rstk, Dstk is different from the average R,D:
      csdec = cos(Dstk * RAD_DEG);

      // find the ra,dec displacement in arcsec:
      dR = 3600.0*(Rstk - measureB->R)*csdec;
      dD = 3600.0*(Dstk - measureB->D);

      dPos = hypot(dR,dD);
      if (dPos > 0.1) { 
	if (result.NstackBadImageCoords < 10) fprintf (stderr, "inconsistent stack coords: %f %f : %f %f (%d %d)\n", Rstk, Dstk, dR, dD, measureB->t, measureB->photcode); 
	result.NstackBadImageCoords ++;
      }
      continue;
    }

    // do them separately to track where the failures are:
    if (isGPC1warp(measureB->photcode)) {
      off_t idx = getImageByID (measureB->imageID);
      if (idx == -1) {
	if (result.NwarpNoImageID < 10) fprintf (stderr, "missing warp imageID: %f %f : %d %d %d\n", measureB->R, measureB->D, measureB->t, measureB->photcode, measureB->imageID); 
	result.NwarpNoImageID ++;
	continue;
      }

      // check if the image-derived coords and the average coords agree:
      Coords *imcoords = &image[idx].coords;
      
      double Rwrp, Dwrp;
      XY_to_RD (&Rwrp, &Dwrp, X, Y, imcoords);
      
      Rwrp = ohana_normalize_angle_to_midpoint(Rwrp, 180.0);
      Dwrp = ohana_normalize_angle_to_midpoint(Dwrp,   0.0);
      
      // find the ra,dec displacement in arcsec:
      double dR = 3600.0*(Rwrp - Rave)*csdec;
      double dD = 3600.0*(Dwrp - Dave);
      
      // skip detections which are within a small distance of the expected location
      // NOTE: this actually works surprisingly well near the pole
      double dPos = hypot(dR,dD);
      if (dPos > 3.0) { 
	if (result.NwarpBadCoords < 100) fprintf (stderr, "inconsistent warp avecoords: %f %f : %f %f (%d %d) 0x%08x 0x%08x\n", Rave, Dave, dR, dD, measureB->t, measureB->photcode, average->objID, average->catID); 
	result.NwarpBadCoords ++;
      }

      // check match of obstime:
      if ((measureB->t < image[idx].tzero) && (measureB->t > image[idx].tzero + image[idx].NX*image[idx].trate/5000.0)) {
	if (result.NwarpBadTime < 10) fprintf (stderr, "inconsistent warp times: %d vs %d for %f, %f (%d)", measureB->t, image[idx].tzero, Rwrp, Dwrp, measureB->photcode); 
	result.NwarpBadTime ++;
      }

      // check if the measure coords and image-based coords match
      // check if the new Rwrp, Dwrp is different from the average R,D:
      csdec = cos(Dwrp * RAD_DEG);

      // find the ra,dec displacement in arcsec:
      dR = 3600.0*(Rwrp - measureB->R)*csdec;
      dD = 3600.0*(Dwrp - measureB->D);

      dPos = hypot(dR,dD);
      if (dPos > 0.1) { 
	if (result.NwarpBadImageCoords < 10) fprintf (stderr, "inconsistent warp coords: %f %f : %f %f (%d %d)\n", Rwrp, Dwrp, dR, dD, measureB->t, measureB->photcode); 
	result.NwarpBadImageCoords ++;
      }
      continue;
    }

    // do them separately to track where the failures are:
    if (isGPC1chip(measureB->photcode)) {
      off_t idx = getImageByID (measureB->imageID);
      if (idx == -1) {
	if (result.NchipNoImageID < 10) fprintf (stderr, "missing chip imageID: %f %f : %d %d %d\n", measureB->R, measureB->D, measureB->t, measureB->photcode, measureB->imageID); 
	result.NchipNoImageID ++;
	continue;
      }

      // check if the image-derived coords and the average coords agree:
      Coords *imcoords = &image[idx].coords;
      
      // chip pixel coordinates
      double Xchp = measureB->Xccd;
      double Ychp = measureB->Yccd;
      if (isfinite(measureB->Xfix) && isfinite(measureB->Yfix)) {
	float dX = measureB->Xfix - measureB->Xccd;
	float dY = measureB->Yfix - measureB->Yccd;
	if (hypot(dX,dY) < 2.0) {
	  Xchp = measureB->Xfix;
	  Ychp = measureB->Yfix;
	}
      }

      double Rchp, Dchp;
      XY_to_RD (&Rchp, &Dchp, Xchp, Ychp, imcoords);
      
      Rchp = ohana_normalize_angle_to_midpoint(Rchp, 180.0);
      Dchp = ohana_normalize_angle_to_midpoint(Dchp,   0.0);
      
      // find the ra,dec displacement in arcsec:
      double dR = 3600.0*(Rchp - Rave)*csdec;
      double dD = 3600.0*(Dchp - Dave);
      
      // skip detections which are within a small distance of the expected location
      // NOTE: this actually works surprisingly well near the pole
      double dPos = hypot(dR,dD);
      if (dPos > 3.0) { 
	if (result.NchipBadCoords < 100) fprintf (stderr, "inconsistent chip avecoords: %f %f : %f %f (%d %d) 0x%08x 0x%08x\n", Rave, Dave, dR, dD, measureB->t, measureB->photcode, average->objID, average->catID); 
	result.NchipBadCoords ++;
      }

      // check match of obstime:
      if ((measureB->t < image[idx].tzero) && (measureB->t > image[idx].tzero + image[idx].NX*image[idx].trate/5000.0)) {
	if (result.NchipBadTime < 10) fprintf (stderr, "inconsistent chip times: %d vs %d for %f, %f (%d)\n", measureB->t, image[idx].tzero, Rchp, Dchp, measureB->photcode); 
	result.NchipBadTime ++;
      }

      // check if the measure coords and image-based coords match
      // check if the new Rchp, Dchp is different from the average R,D:
      csdec = cos(Dchp * RAD_DEG);

      // find the ra,dec displacement in arcsec:
      dR = 3600.0*(Rchp - measureB->R)*csdec;
      dD = 3600.0*(Dchp - measureB->D);

      dPos = hypot(dR,dD);
      if (dPos > 0.1) { 
	if (result.NchipBadImageCoords < 10) fprintf (stderr, "inconsistent chip coords: %f %f : %f %f (%d %d) %d 0x%08x 0x%08x | 0x%08x 0x%08x\n", Rchp, Dchp, dR, dD, measureB->t, measureB->photcode, measureB->averef, measureB->detID, measureB->imageID, average->objID, average->catID); 
	result.NchipBadImageCoords ++;
      }
      continue;
    }
  }

  int Nproblem = 0;
  Nproblem += result.NstackNoImageID;
  Nproblem += result.NstackBadCoords;
  Nproblem += result.NstackBadTime;
  Nproblem += result.NstackBadImageCoords;
  Nproblem += result.NwarpNoImageID;
  Nproblem += result.NwarpBadCoords;
  Nproblem += result.NwarpBadTime;
  Nproblem += result.NwarpBadImageCoords;
  Nproblem += result.NchipNoImageID;
  Nproblem += result.NchipBadCoords;
  Nproblem += result.NchipBadTime;
  Nproblem += result.NchipBadImageCoords;

  fprintf (stderr, "\n");
  fprintf (stderr, "check measure %s : %d problems : stack ImageID %d Coords %d Time %d ImCoords %d : warp ImageID %d Coords %d Time %d ImCoords %d : chip ImageID %d Coords %d Time %d ImCoords %d\n", 
	   catalog->filename, 
	   Nproblem,
	   result.NstackNoImageID,
	   result.NstackBadCoords,
	   result.NstackBadTime,
	   result.NstackBadImageCoords,
	   result.NwarpNoImageID,
	   result.NwarpBadCoords,
	   result.NwarpBadTime,
	   result.NwarpBadImageCoords,
	   result.NchipNoImageID,
	   result.NchipBadCoords,
	   result.NchipBadTime,
	   result.NchipBadImageCoords
    );

  return (TRUE);
}
