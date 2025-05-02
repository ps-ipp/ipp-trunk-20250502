# include "fiximids.h"

// test image: 2013/06/15,13:25:51, GPC1.r.XY50
static int CHECK_TEST_IMAGE = FALSE;
static unsigned int Tref = 1378812312; 
static short Cref = 10001;

void update_catalog_fiximids (Catalog *catalog) {

  off_t i;

  off_t found = 0;    
  off_t Nvalid = 0;    
  off_t Ninvalid = 0;    

  for (i = 0; i < catalog[0].Nmeasure; i++) {
    
    // only do PS1 / GPC1 photcodes?
    // XXX if (catalog[0].measure[i].photcode < 10000) continue;
    // XXX if (catalog[0].measure[i].photcode > 10600) continue;

    // we have a measure with a given photcode and time:
    short photcode = catalog[0].measure[i].photcode;
    e_time time = catalog[0].measure[i].t;

    if (CHECK_TEST_IMAGE && (abs(time - Tref) < 10) && (photcode == Cref)) {
      fprintf (stderr, ".");
    }

    // skip detections with no valid imageID (eg, ref photcode)

    off_t imageID, Seq;
    if (!FindImageID (&imageID, &Seq, time, photcode)) continue;
    if (imageID <= 0) continue; 

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
}

