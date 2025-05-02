# include "Ximage.h"

// in 3D we use channels 0,1,2 to choose the pixel from the cube
void ColorHistogram (KapaImageWidget *image, CCNode *cube) {

  int i, DX, DY, nPixels;
  float *rData, *bData, *gData, rValue, gValue, bValue;
  float redSlope, blueSlope, greenSlope;
  float redStart, blueStart, greenStart;
  CCNode *node;

  // Input images are scaled to 0.0 - 1.0 floating point range.  These values are accumulated in the
  // 3d histogram.  Use the histogram to allocate colors or subdivide the top-populated histogram
  // cells and re-evaluated.   

  // set start & slope for red (channel 0)
  if (image[0].channel[KAPA_CHANNEL_RED].range != 0.0) {
    redSlope = 1.0 / image[0].channel[KAPA_CHANNEL_RED].range;
    redStart = image[0].channel[KAPA_CHANNEL_RED].zero / image[0].channel[KAPA_CHANNEL_RED].range;
  } else {
    redSlope = 1.0;
    redStart = image[0].channel[KAPA_CHANNEL_RED].zero;
  }
  // set start & slope for blue (channel 0)
  if (image[0].channel[KAPA_CHANNEL_BLUE].range != 0.0) {
    blueSlope = 1.0 / image[0].channel[KAPA_CHANNEL_BLUE].range;
    blueStart = image[0].channel[KAPA_CHANNEL_BLUE].zero / image[0].channel[KAPA_CHANNEL_BLUE].range;
  } else {
    blueSlope = 1.0;
    blueStart = image[0].channel[KAPA_CHANNEL_BLUE].zero;
  }
  // set start & slope for green (channel 0)
  if (image[0].channel[KAPA_CHANNEL_GREEN].range != 0.0) {
    greenSlope = 1.0 / image[0].channel[KAPA_CHANNEL_GREEN].range;
    greenStart = image[0].channel[KAPA_CHANNEL_GREEN].zero / image[0].channel[KAPA_CHANNEL_GREEN].range;
  } else {
    greenSlope = 1.0;
    greenStart = image[0].channel[KAPA_CHANNEL_GREEN].zero;
  }

  DX = image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[0];
  DY = image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[1];
  // XXX check on equal size for all three channels

  nPixels = DX*DY;

  // loop over pixels, convert data values to range values (0.0 - 1.0), increment 3D histogram cell
  rData = (float *) image[0].channel[KAPA_CHANNEL_RED].matrix.buffer;
  bData = (float *) image[0].channel[KAPA_CHANNEL_BLUE].matrix.buffer;
  gData = (float *) image[0].channel[KAPA_CHANNEL_GREEN].matrix.buffer;

  // convert pixel data values to pixel index values (0 - Npixel)
  for (i = 0; i < nPixels; i++, rData++, bData++, gData++) {
    rValue = *rData * redSlope   - redStart;
    if (rValue < 0.0) rValue = 0.0;
    if (rValue > 1.0) rValue = 1.0;
    if (isnan(rValue)) rValue = 0.0;
    if (isinf(rValue)) rValue = 1.0;

    bValue = *bData * blueSlope  - blueStart;
    if (bValue < 0.0) bValue = 0.0;
    if (bValue > 1.0) bValue = 1.0;
    if (isnan(bValue)) bValue = 0.0;
    if (isinf(bValue)) bValue = 1.0;

    gValue = *gData * greenSlope - greenStart;
    if (gValue < 0.0) gValue = 0.0;
    if (gValue > 1.0) gValue = 1.0;
    if (isnan(gValue)) gValue = 0.0;
    if (isinf(gValue)) gValue = 1.0;

    // XXX at the moment, we are saturating before supplying to this function
    // NOTE : x,y,z = red,green,blue
    node = CCFindBottom (cube, rValue, gValue, bValue);
    node->count ++;
  }
}

// in 3D we use channels 0,1,2 to choose the pixel from the cube
void CCNodeSetColorPixels (KapaImageWidget *image, CCNode *cube) {

  int i, DX, DY, nPixels;
  float *rData, *bData, *gData, rValue, gValue, bValue;
  unsigned short *oData;
  float redSlope, blueSlope, greenSlope;
  float redStart, blueStart, greenStart;
  CCNode *node;

  // Input images are scaled to 0.0 - 1.0 floating point range.  These values are accumulated in the
  // 3d histogram.  Use the histogram to allocate colors or subdivide the top-populated histogram
  // cells and re-evaluated.   

  // set start & slope for red (channel 0)
  if (image[0].channel[KAPA_CHANNEL_RED].range != 0.0) {
    redSlope = 1.0 / image[0].channel[KAPA_CHANNEL_RED].range;
    redStart = image[0].channel[KAPA_CHANNEL_RED].zero / image[0].channel[KAPA_CHANNEL_RED].range;
  } else {
    redSlope = 1.0;
    redStart = image[0].channel[KAPA_CHANNEL_RED].zero;
  }
  // set start & slope for blue (channel 0)
  if (image[0].channel[KAPA_CHANNEL_BLUE].range != 0.0) {
    blueSlope = 1.0 / image[0].channel[KAPA_CHANNEL_BLUE].range;
    blueStart = image[0].channel[KAPA_CHANNEL_BLUE].zero / image[0].channel[KAPA_CHANNEL_BLUE].range;
  } else {
    blueSlope = 1.0;
    blueStart = image[0].channel[KAPA_CHANNEL_BLUE].zero;
  }
  // set start & slope for green (channel 0)
  if (image[0].channel[KAPA_CHANNEL_GREEN].range != 0.0) {
    greenSlope = 1.0 / image[0].channel[KAPA_CHANNEL_GREEN].range;
    greenStart = image[0].channel[KAPA_CHANNEL_GREEN].zero / image[0].channel[KAPA_CHANNEL_GREEN].range;
  } else {
    greenSlope = 1.0;
    greenStart = image[0].channel[KAPA_CHANNEL_GREEN].zero;
  }

  DX = image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[0];
  DY = image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[1];
  // XXX check on equal size for all three channels

  nPixels = DX*DY;
  if (image[0].nPixels != nPixels) {
    REALLOCATE (image[0].pixmap, unsigned short, nPixels);
    image[0].nPixels = nPixels;
  }

  // loop over pixels, convert data values to range values (0.0 - 1.0), increment 3D histogram cell
  rData = (float *) image[0].channel[KAPA_CHANNEL_RED].matrix.buffer;
  bData = (float *) image[0].channel[KAPA_CHANNEL_BLUE].matrix.buffer;
  gData = (float *) image[0].channel[KAPA_CHANNEL_GREEN].matrix.buffer;

  oData = image[0].pixmap;

  // convert pixel data values to pixel index values (0 - Npixel)
  for (i = 0; i < nPixels; i++, rData++, bData++, gData++, oData++) {
    rValue = *rData * redSlope   - redStart;
    if (rValue < 0.0) rValue = 0.0;
    if (rValue > 1.0) rValue = 1.0;
    if (isnan(rValue)) rValue = 0.0;
    if (isinf(rValue)) rValue = 1.0;

    bValue = *bData * blueSlope  - blueStart;
    if (bValue < 0.0) bValue = 0.0;
    if (bValue > 1.0) bValue = 1.0;
    if (isnan(bValue)) bValue = 0.0;
    if (isinf(bValue)) bValue = 1.0;

    gValue = *gData * greenSlope - greenStart;
    if (gValue < 0.0) gValue = 0.0;
    if (gValue > 1.0) gValue = 1.0;
    if (isnan(gValue)) gValue = 0.0;
    if (isinf(gValue)) gValue = 1.0;

    // XXX at the moment, we are saturating before supplying to this function
    // NOTE : x,y,z = red,green,blue 
    node = CCFindBottom (cube, rValue, gValue, bValue);
    *oData = node->pixel;
  }
}

// iterate over the CCNode tree and subdivide any bottom level nodes with value > minValue
// XXX this function is specific to the color histogram concept
// NOTE : x,y,z = red,green,blue 
int CCNodeSetColorMap (CCNode *node, XColor *cmap, int Npixels, int *current) {

  int ix, iy, iz;

  if (node->bottom) {
    cmap[current[0]].red   = node->mid[CC_X]*0xffff;
    cmap[current[0]].green = node->mid[CC_Y]*0xffff;
    cmap[current[0]].blue  = node->mid[CC_Z]*0xffff;
    cmap[current[0]].flags = DoRed | DoGreen | DoBlue;
    node->pixel = current[0];
    current[0] ++;
    assert (current[0] < Npixels);
    return TRUE;
  }
  
  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	CCNodeSetColorMap (node->sub[ix][iy][iz], cmap, Npixels, current);
      }
    }
  }
  return TRUE;
}
