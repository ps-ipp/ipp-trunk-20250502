# include "setposangle.h"

void update_catalog_setposangle (Catalog *catalog, ImageSubset *image, off_t *index, off_t Nimage) {
  OHANA_UNUSED_PARAM(Nimage);

  float posAngle, pltScale;
  off_t i, j, found;

  found = 0;    
  for (i = 0; i < catalog[0].Naverage; i++) {

    off_t m = catalog[0].average[i].measureOffset;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++, m++) {
      off_t ID = catalog[0].measure[m].imageID;
      if (ID <= 0) continue; // detections with imageID == 0 do not have a valid image (eg, ref photcode)

      // XXX only do GPC1 data for now
      catalog[0].measure[m].pltscale = pltScale;
      if (catalog[0].measure[m].photcode < 10000) continue;
      if (catalog[0].measure[m].photcode > 10600) continue;
      
      // index[ID] = seqN

      off_t seq = index[ID];

      Mosaic *mosaic = getMosaicForImage(seq);
      Coords *coords = &image[seq].coords;
      Coords *mosaicCoords = mosaic ? &mosaic->coords : NULL;

      double Xccd = catalog[0].measure[m].Xccd;
      double Yccd = catalog[0].measure[m].Yccd;
      
      setposangle_local_astrometry (&posAngle, &pltScale, Xccd, Yccd, mosaicCoords, coords);

      catalog[0].measure[m].posangle = ToShortDegrees(posAngle);
      catalog[0].measure[m].pltscale = pltScale;
      // myAssert(isfinite(catalog[0].measure[m].posangle), "oops: setposangle made a nan");
      myAssert(isfinite(catalog[0].measure[m].pltscale), "oops: setposangle made a nan");
      found ++;
    }
  }

  if (found) {
    fprintf (stderr, "found "OFF_T_FMT" matches\n", found);
  }
}

// this is basically a re-write / adaptation of pmSourceLocalAstrometry
// posangle in degrees, plate scale in arcseconds/pixel
int setposangle_local_astrometry (float *posAngle, float *pltScale, double x, double y, Coords *mosaic, Coords *coords) {

  double Lx, Mx, Po, Qo, Px, Qx, Py, Qy;

  // calculate the astrometry for the coordinate of interest
  XY_to_LM (&Lx, &Mx, x,       y,       coords);
  if (mosaic) {
    XY_to_LM (&Po, &Qo, Lx,      Mx,      mosaic);
  } else {
    Po = Lx;
    Qo = Mx;
  }

  XY_to_LM (&Lx, &Mx, x + 1.0, y,       coords);
  if (mosaic) {
    XY_to_LM (&Px, &Qx, Lx,      Mx,      mosaic);
  } else {
    Px = Lx;
    Qx = Mx;
  }

  XY_to_LM (&Lx, &Mx, x,       y + 1.0, coords);
  if (mosaic) {
    XY_to_LM (&Py, &Qy, Lx,      Mx,      mosaic);
  } else {
    Py = Lx;
    Qy = Mx;
  }

  // XXX units for the resulting Tangent Plane coordinates??

  double dPdX = Px - Po;
  double dPdY = Py - Po;

  double dQdX = Qx - Qo;
  double dQdY = Qy - Qo;

  double pltScale_x = hypot(dPdX, dQdX);
  double pltScale_y = hypot(dPdY, dQdY);
  *pltScale = 3600.0*0.5*(pltScale_x + pltScale_y);

  double posAngle_x, posAngle_y;
  double crossProduct = dPdX * dQdY - dPdY * dQdX;
  if  (crossProduct > 0.) {
    *pltScale *= -1.0;
    posAngle_x = atan2 (dQdX, dPdX);
    posAngle_y = atan2 (dQdY, dPdY) - M_PI_2;
  } else {
    posAngle_x = atan2 (dQdX, -dPdX);
    posAngle_y = atan2 (dQdY,  dPdY) - M_PI_2;
  }

  // with errors, these may end up on opposite sides of the M_PI boundary.  
  if (posAngle_x - posAngle_y > M_PI) {
    posAngle_y += 2.0 * M_PI;
  }
  if (posAngle_y - posAngle_x > M_PI) {
    posAngle_x += 2.0 * M_PI;
  }
  *posAngle = 0.5*(posAngle_x + posAngle_y)*DEG_RAD;

  return TRUE;
}
