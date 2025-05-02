# include "relphot.h"

/** This code was previously used in relphot to fix problems coming from the dvo
    construction for PV3 / DR1.  The repair function is not relevant to relphot and was
    only included there to avoid multiple sweeps of the database (probably a mistake).

    The errors arose from the same image ID used for warps and chip images (and maybe some
    astrometry problems? I forget). 

    this bit of code was in relphot_objects.c:

  if (REPAIR_WARPS) { 
    FindWarpGroups ();
    MARKTIME("setup warp groups: %f sec\n", dtime);
    MakeStackIndex ();
    MARKTIME("setup stack index: %f sec\n", dtime);
    RepairWarpMeasuresOpenLogfile ();
  }

*/

FILE *logfile = NULL;

void RepairWarpMeasuresOpenLogfile () {

  char name[DVO_MAX_PATH];
  snprintf (name, DVO_MAX_PATH, "%s/repair.warp.log", HOSTDIR);
  logfile = fopen (name, "w");
  if (!logfile) fprintf (stderr, "failed to open repair warp logfile %s\n", name);
}

void RepairWarpMeasuresCloseLogfile () {
  if (!logfile) return;
  fclose (logfile);
}

// XXX need to load_images first and generate some lookup tables
int RepairWarpMeasures (Catalog *catalog) {

  off_t Nimage;
  Image *image = getimages (&Nimage, NULL);

  myAssert (!catalog->Nmeasure || catalog->measure, "programming error");

  int NfixChipID = 0, NfixStackID = 0, NfixWarpID = 0, NfixWarpImageID = 0, NmissWarp = 0, NmissStack = 0, NbadWarp = 0;

  int onePercent = catalog->Nmeasure / 100;

  for (off_t j = 0; j < catalog->Nmeasure; j++) {
    if (j % onePercent == 0) fprintf (stderr, ".");

    Measure *measure = &catalog->measure[j];

    // get the associated average value:
    int averef = measure->averef;
    Average *average = &catalog->average[averef];

    // double-check for consistency
    myAssert (average->objID == measure->objID, "objID mismatch: %f %f : %d %d\n", average->R, average->D, average->objID, measure->objID);

    // repair extID for non-warps:
    if (isGPC1chip(measure->photcode))  {
      double mjd = ohana_sec_to_mjd (measure->t);
      int ccdnum = measure->photcode % 100;
      uint64_t extID = CreatePSPSDetectionID (mjd, ccdnum, measure->detID);
      if (extID != measure->extID) {
	measure->extID = extID;
	NfixChipID ++;
      }
      continue;
    }

    // repair extID for non-warps:
    if (isGPC1stack(measure->photcode))  {
      int im = getImageByID (measure->imageID);
      if (im < 0) {
	// if we don't have a matching image, we are probably assigned to an image on the wrong side of the sky
	// reconstruct the imageID from the R,D,photcode & X,Y consistency
	im = GetStackSeq (image, average->R, average->D, measure->photcode, measure->Xccd, measure->Yccd);
	if (im < 0) {
	  // still failed, this is bad
	  if (NmissStack < 10) fprintf (stderr, "missing stack exposure: %f %f : %d %d\n", measure->R, measure->D, measure->imageID, measure->photcode);
	  NmissStack ++;
	  continue;
	}
	// the old imageID was wrong.  replace with the new one:
	if (logfile) fprintf (logfile, "fix stack %10.6f %10.6f %5d %6.1f %6.1f 0x%08x %d -> %d\n", average->R, average->D, measure->photcode, measure->Xccd, measure->Yccd, measure->detID, measure->imageID, image[im].imageID);
	measure->imageID = image[im].imageID;
      } else {
	// we have a matching image, but check that it is correct by re-projecting the pixel position
	double Xtst, Ytst;
	RD_to_XY (&Xtst, &Ytst, average->R, average->D, &image[im].coords);
	
	// find the pixel offset
	double dX = (Xtst - measure->Xccd);
	double dY = (Ytst - measure->Yccd);
	
	// if dPos is small, we have the right image
	double dPos = hypot(dX,dY);
	if (dPos > 10.0) {
	  // if dPos is large, we don't the right imageID, reconstruct the imageID from
	  // the R,D,photcode & X,Y consistency
	  im = GetStackSeq (image, average->R, average->D, measure->photcode, measure->Xccd, measure->Yccd);
	  if (im < 0) {
	    // still failed, this is bad
	    if (NmissStack < 10) fprintf (stderr, "missing stack exposure: %f %f : %d %d\n", measure->R, measure->D, measure->imageID, measure->photcode);
	    NmissStack ++;
	    continue;
	  }
	  // the old imageID was wrong.  replace with the new one:
	  if (logfile) fprintf (logfile, "fix stack %10.6f %10.6f %5d %6.1f %6.1f 0x%08x %d -> %d\n", average->R, average->D, measure->photcode, measure->Xccd, measure->Yccd, measure->detID, measure->imageID, image[im].imageID);
	  measure->imageID = image[im].imageID;
	}
      }
      uint64_t extID = CreatePSPSStackDetectionID (35, image[im].externID, measure->detID);
      if (extID != measure->extID) {
	measure->extID = extID;
	NfixStackID ++;
      }
      continue;
    }

    // we are only going to repair warp detections
    if (!isGPC1warp(measure->photcode)) continue;

    // warp coordinates to confirm warp
    double X = measure->Xccd;
    double Y = measure->Yccd;

    // check if this detection is far from its correct location
    double Rwrp = measure->R;
    double Dwrp = measure->D;

    double Rave = average->R;
    double Dave = average->D;
      
    float csdec = cos(Dave * RAD_DEG);

    // find the ra,dec displacement in arcsec:
    double dR = 3600.0*(Rwrp - Rave)*csdec;
    double dD = 3600.0*(Dwrp - Dave);

    // skip detections which are within a small distance of the expected location
    // NOTE: this actually works surprisingly well near the pole
    double dPos = hypot(dR,dD);
    if (dPos < 2.0) { 
      int im = getImageByID (measure->imageID);
      uint64_t extID = CreatePSPSStackDetectionID (34, image[im].externID, measure->detID);
      if (extID != measure->extID) {
	measure->extID = extID;
	NfixWarpID ++;
      }
      continue;
    }

    // find the corrected warp ID:
    int warpSeq = GetWarpSeq (image, measure->t, measure->photcode, Rave, Dave, X, Y);
    if (warpSeq < 0) {
      if (NmissWarp < 10) fprintf (stderr, "missing warp exposure: %f %f : %d %d\n", Rave, Dave, measure->t, measure->photcode);
      NmissWarp ++;
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
      NbadWarp ++;
    }

    measure->imageID = image[warpSeq].imageID;
    measure->R = Rnew;
    measure->D = Dnew;

    uint64_t extID = CreatePSPSStackDetectionID (34, image[warpSeq].externID, measure->detID);
    if (extID != measure->extID) {
      measure->extID = extID;
      NfixWarpID ++;
    }
    NfixWarpImageID ++;
  }

  fprintf (stderr, "\n");
  fprintf (stderr, "repaired %s : warp image ID %d : ext ID chip %d stack %d warp %d : missed warp %d stack %d : bad warp: %d\n", catalog->filename, NfixWarpImageID, NfixChipID, NfixStackID, NfixWarpID, NmissWarp, NmissStack, NbadWarp);

  return (TRUE);
}
