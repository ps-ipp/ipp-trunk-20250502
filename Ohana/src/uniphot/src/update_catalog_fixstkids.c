# include "fixstkids.h"

void update_catalog_fixstkids (Catalog *catalog) {

  off_t i;

  off_t found = 0;    
  off_t Nvalid = 0;    
  off_t Ninvalid = 0;    

  int sourceID, externID, detID;
  int Nmissed = 0;

  for (i = 0; i < catalog[0].Nmeasure; i++) {
    
    // only do PS1 stack photcodes
    if (catalog[0].measure[i].photcode < 11000) continue;
    if (catalog[0].measure[i].photcode > 11500) continue;

    // reverse engineer the detID, imageID, sourceID from the psps det ID (extID)
    UnpackPSPSStackDetectionID (&sourceID, &externID, &detID, catalog[0].measure[i].extID);
    myAssert (sourceID == 35, "problem with source ID %d (%0x08 %0x08)", sourceID, catalog[0].measure[i].objID, catalog[0].measure[i].catID);
    myAssert (detID == catalog[0].measure[i].detID, "problem with det ID %d (%0x08 %0x08)", detID, catalog[0].measure[i].objID, catalog[0].measure[i].catID);

    // we now have the externID of the given detection -- find the real image in the image table
    off_t imageID, Seq;
    short Photcode;
    if (!FindImageID (&imageID, &Seq, &Photcode, externID)) {
      Nmissed ++;
      continue;
    }

    // if only one matched (or none), the we got the right ID the first time around
    if (imageID <= 0) {
	fprintf (stderr, "?");
    }

    // validate the photcode and time?
    myAssert (catalog[0].measure[i].photcode == Photcode, "bad photcode match? objID, catID: (%0x08 %0x08), imageID, externID: "OFF_T_FMT", %d", catalog[0].measure[i].objID, catalog[0].measure[i].catID, imageID, externID)

    if (imageID == catalog[0].measure[i].imageID) {
      // existing image ID is valid
      Nvalid ++;
      BumpValidImage (Seq);
    } else {
      // existing image ID is NOT valid
      Ninvalid ++;
      catalog[0].measure[i].imageID = imageID;
      BumpInvalidImage (Seq);
    }

    found ++;
  }

  if (found) {
    fprintf (stderr, "found "OFF_T_FMT" matches: "OFF_T_FMT" valid, "OFF_T_FMT" invalid\n", found, Nvalid, Ninvalid);
  }
  if (Nmissed) {
    fprintf (stderr, "missed %d stack entries\n", Nmissed);
  }
}

int UnpackPSPSStackDetectionID(int *sourceID, int *imageID, int *detID, uint64_t pspsStackID)
{
  // sourceID : ID of database + table that tracked the image (< 0x7f = 127)
  // imageID : external ID of the image which provided the detections (< 0x1000.0000 ~ 2.7e8)
  // detID : detection sequence in image (< 0x1000.0000 ~ 2.7e8)

  // 0x0000.0000.0000.0000

  // bits  0 - 27 : 0x0000.0000.0fff.ffff
  // bits 28 - 55 : 0x00ff.ffff.f000.0000
  // bits 56 - 63 : 0xff00.0000.0000.0000

  *sourceID = (pspsStackID & (uint64_t) 0xff00000000000000) >> 56;
  *imageID  = (pspsStackID & (uint64_t) 0x00fffffff0000000) >> 28;
  *detID    = (pspsStackID & (uint64_t) 0x000000000fffffff);

  return TRUE;
}
    
