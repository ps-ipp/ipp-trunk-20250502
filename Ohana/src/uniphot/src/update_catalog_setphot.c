# include "setphot.h"

// XXX I need to add a few things for HSC + MC + GPC1:
// * I can generate a zpt table for the MC exposures using the nominal zero points
// * for the HSC data, I need to add a function to define Mflat as a function of Xmos, Ymos

void update_catalog_setphot (Catalog *catalog, Image *image, off_t *index, off_t Nimage, CamPhotomCorrection *camcorr) {
  OHANA_UNUSED_PARAM(Nimage);

  off_t i, found;

  // if we are resetting, reset all flags
  DVOMeasureFlags photomFlags = ID_MEAS_POOR_PHOTOM | ID_MEAS_SKIP_PHOTOM | ID_MEAS_PHOTOM_UBERCAL;

  found = 0;    
  for (i = 0; i < catalog[0].Nmeasure; i++) {

    Measure *measure = &catalog[0].measure[i];

    // XXX deprecated 2016.09.22 : only do GPC1 data for now
    // if (measure[0].photcode < 10000) continue;
    // if (measure[0].photcode > 10600) continue;
      
    // only do DEP photcodes (skip REF, etc)
    PhotCode *code = GetPhotcodebyCode (measure[0].photcode);
    if (!code) continue; // invalid photcode
    if (code->type != PHOT_DEP) continue;

    // allow a restriction on the modified zpts:
    if (PHOTCODE_MAX) {
      if (measure[0].photcode < PHOTCODE_MIN) continue;
      if (measure[0].photcode > PHOTCODE_MAX) continue;
    }

    off_t idx = measure[0].imageID;
    if (idx <= 0) continue; // detections with imageID == 0 do not have a valid image (eg, ref photcode)

    off_t id = index[idx];
    if (id < 0) continue;

    float Mcal  = image[id].McalPSF;
    float dMcal = image[id].dMcal;
    float Mflat = 0.0;

    // if we know about a flat-field correction, then we need to apply the sub-chip correction
    int flat_id = image[id].photom_map_id;
    if (flat_id > 0) {
      Mflat = CamPhotomCorrectionValue (camcorr, flat_id, measure[0].Xccd, measure[0].Yccd);
    }

# if (0)
    // the mosaic lookup is broken : fix it then redo this block
    if (radialZP) {
      mosaic = MatchMosaicMetadata (measure[0].imageID);
      if (mosaic == NULL) break;
      double Rm = measure[0].R;
      double Dm = measure[0].D;
      RD_to_XY (&XMOS_MEAS, &YMOS_MEAS, Rm, Dm, mosaic);
      Mflat = RadialZPtrend (XMOS_MEAS, XMOS_MEAS);
    }
# endif

    myAssert(isfinite(Mcal), "oops: ubercal made a nan");

    measure[0].McalPSF  = Mcal;
    measure[0].McalAPER = Mcal;
    measure[0].Mflat = Mflat; // in the previous version, Mcal_offset (which is added to Mflat) had a negative sign here
    measure[0].dMcal = dMcal;

    if (RESET) {
      measure[0].dbFlags &= ~photomFlags;
    }

    // if we are setting the zero points from an UBERCAL database, and this detection is from one of those images,
    // then tag the measurement as well
    if (UBERCAL && (image[id].flags & ID_IMAGE_PHOTOM_UBERCAL)) {
      measure[0].dbFlags |=  ID_MEAS_PHOTOM_UBERCAL;
    }

    found ++;
  }

  if (found) {
    fprintf (stderr, "found "OFF_T_FMT" matches for setphot\n", found);
  }
}
