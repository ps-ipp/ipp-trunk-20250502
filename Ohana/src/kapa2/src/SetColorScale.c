# include "Ximage.h"

void SetColorScale (Graphic *graphic, KapaImageWidget *image) {

  switch (graphic[0].ColorScaleMode) {
    case KAPA_SCALE_1D:
      SetColorScale1D (graphic, image);
      break;
    case KAPA_SCALE_3D_RUFF:
      // fall-back on 1D colors (ie, if images are mis-matched)
      if (!SetColorScale3D (graphic, image)) {
	graphic[0].ColorScaleMode = KAPA_SCALE_1D;
	SetColorScale1D (graphic, image);
      }
      break;
    case KAPA_SCALE_3D_FULL:
      // fall-back on 1D colors (ie, if images are mis-matched)
      if (!SetColorScale3D_CC (graphic, image)) {
	graphic[0].ColorScaleMode = KAPA_SCALE_1D;
	SetColorScale1D (graphic, image);
      }
      break;
    default:
      fprintf (stderr, "programming error in kapa: unknown color scale mode\n");
      return;
  }
  return;
}

void SetColorScale1D (Graphic *graphic, KapaImageWidget *image) {
  int i, DX, DY, value, nPixels;
  float *iData;
  unsigned short *oData;
  float slope;
  float start;
  unsigned short MaxValue, NANValue;
  Matrix *matrix;

  // define the color transform parameters
  NANValue = graphic[0].Npixels - 1;
  MaxValue = graphic[0].Npixels - 2;
  if (image[0].image[0].range != 0.0) {
    slope = (graphic[0].Npixels - 1) / image[0].image[0].range;
    start = slope * image[0].image[0].zero;
  } else {
    slope = 1.0;
    start = image[0].image[0].zero;
  }

  matrix = &image->image->matrix;

  DX = matrix[0].Naxis[0];
  DY = matrix[0].Naxis[1];

  nPixels = DX*DY;
  if (image[0].nPixels != nPixels) {
    REALLOCATE (image[0].pixmap, unsigned short, nPixels);
    image[0].nPixels = nPixels;
  }

  oData = image[0].pixmap;
  iData = (float *) matrix[0].buffer;

  // convert pixel data values to pixel index values (0 - Npixel)
  for (i = 0; i < nPixels; i++, iData++, oData++) {
    if (isnan(*iData) || isinf(*iData)) {
      *oData = NANValue;
      continue;
    }
    value = *iData * slope - start;
    if (value < 0) {
      *oData = 0;
      continue;
    }
    if (value > MaxValue) {
      *oData = MaxValue;
      continue;
    }
    *oData = value;
  }
}

// in 3D we use channels 0,1,2 to choose the pixel from the cube
// XXX this uses a crude, uniform-spacing cube
int SetColorScale3D (Graphic *graphic, KapaImageWidget *image) {

  int i, DX, DY, nPixels, rValue, gValue, bValue;
  float *rData, *bData, *gData;
  unsigned short *oData;
  float redSlope, blueSlope, greenSlope;
  float redStart, blueStart, greenStart;

  DX = image[0].channel[KAPA_CHANNEL_RED].matrix.Naxis[0];
  DY = image[0].channel[KAPA_CHANNEL_RED].matrix.Naxis[1];

  if (DX != image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[0]) return FALSE;
  if (DY != image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[1]) return FALSE;
  if (DX != image[0].channel[KAPA_CHANNEL_BLUE].matrix.Naxis[0]) return FALSE;
  if (DY != image[0].channel[KAPA_CHANNEL_BLUE].matrix.Naxis[1]) return FALSE;

  // define the color transform parameters
  unsigned short maxRed = graphic[0].nRed - 1;
  unsigned short maxBlue = graphic[0].nBlue - 1;
  unsigned short maxGreen = graphic[0].nGreen - 1;

  unsigned short NANValue = graphic[0].Npixels - 1;
  
  // set start & slope for red (channel 0)
  if (image[0].channel[KAPA_CHANNEL_RED].range != 0.0) {
    redSlope = graphic[0].nRed / image[0].channel[KAPA_CHANNEL_RED].range;
    redStart = graphic[0].nRed * image[0].channel[KAPA_CHANNEL_RED].zero / image[0].channel[KAPA_CHANNEL_RED].range;
  } else {
    redSlope = 1.0;
    redStart = image[0].channel[KAPA_CHANNEL_RED].zero;
  }
  // set start & slope for blue (channel 0)
  if (image[0].channel[KAPA_CHANNEL_BLUE].range != 0.0) {
    blueSlope = graphic[0].nBlue / image[0].channel[KAPA_CHANNEL_BLUE].range;
    blueStart = graphic[0].nBlue * image[0].channel[KAPA_CHANNEL_BLUE].zero / image[0].channel[KAPA_CHANNEL_BLUE].range;
  } else {
    blueSlope = 1.0;
    blueStart = image[0].channel[KAPA_CHANNEL_BLUE].zero;
  }
  // set start & slope for green (channel 0)
  if (image[0].channel[KAPA_CHANNEL_GREEN].range != 0.0) {
    greenSlope = graphic[0].nGreen / image[0].channel[KAPA_CHANNEL_GREEN].range;
    greenStart = graphic[0].nGreen * image[0].channel[KAPA_CHANNEL_GREEN].zero / image[0].channel[KAPA_CHANNEL_GREEN].range;
  } else {
    greenSlope = 1.0;
    greenStart = image[0].channel[KAPA_CHANNEL_GREEN].zero;
  }

  nPixels = DX*DY;
  if (image[0].nPixels != nPixels) {
    REALLOCATE (image[0].pixmap, unsigned short, nPixels);
    image[0].nPixels = nPixels;
  }

  oData = image[0].pixmap;
  rData = (float *) image[0].channel[KAPA_CHANNEL_RED].matrix.buffer;
  bData = (float *) image[0].channel[KAPA_CHANNEL_BLUE].matrix.buffer;
  gData = (float *) image[0].channel[KAPA_CHANNEL_GREEN].matrix.buffer;

  // convert pixel data values to pixel index values (0 - Npixel)
  // NOTE: a nan value in any of the 3 channels will result in a nan value for output
  for (i = 0; i < nPixels; i++, rData++, bData++, gData++, oData++) {

    // RED channel
    if (isnan(*rData) || isinf(*rData)) {
      *oData = NANValue;
      continue;
    }
    rValue = *rData * redSlope   - redStart;
    if (rValue < 0) rValue = 0;
    if (rValue > maxRed) rValue = maxRed;

    // BLUE channel
    if (isnan(*bData) || isinf(*bData)) {
      *oData = NANValue;
      continue;
    }
    bValue = *bData * blueSlope  - blueStart;
    if (bValue < 0) bValue = 0;
    if (bValue > maxBlue) bValue = maxBlue;

    // GREEN channel
    if (isnan(*gData) || isinf(*gData)) {
      *oData = NANValue;
      continue;
    }
    gValue = *gData * greenSlope - greenStart;
    if (gValue < 0) gValue = 0;
    if (gValue > maxGreen) gValue = maxGreen;

    *oData = gValue + bValue * (maxGreen + 1) + rValue * (maxGreen + 1) * (maxBlue + 1);
  }
  return TRUE;
}

// in 3D we use channels 0,1,2 to choose the pixel from the cube
int SetColorCubeHistogram () {

  int i, DX, DY, Nvalues, NVALUES, Npixels, Nmin;
  float *values;
  CCNode *cube;
  Graphic *graphic;
  Section *section;
  KapaImageWidget *image;

  graphic = GetGraphic ();
  section = GetActiveSection();
  if (section->image == NULL) return (FALSE);
  image = section->image;

  DX = image[0].channel[KAPA_CHANNEL_RED].matrix.Naxis[0];
  DY = image[0].channel[KAPA_CHANNEL_RED].matrix.Naxis[1];
  if (DX != image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[0]) return FALSE;
  if (DY != image[0].channel[KAPA_CHANNEL_GREEN].matrix.Naxis[1]) return FALSE;
  if (DX != image[0].channel[KAPA_CHANNEL_BLUE].matrix.Naxis[0]) return FALSE;
  if (DY != image[0].channel[KAPA_CHANNEL_BLUE].matrix.Naxis[1]) return FALSE;

  // XXX a bit bogus: we are going to store a special color at Npixels-1 for 
  // marking the NAN pixels.  
  // XXX : isn't this set now by the user ?
  if (0) { 
      // XXX set pure green for now
      graphic[0].cmap[graphic[0].Npixels-1].red = 0;
      graphic[0].cmap[graphic[0].Npixels-1].green = 0xffff;
      graphic[0].cmap[graphic[0].Npixels-1].blue = 0;
  }  

  // create the top-level cube
  if (graphic[0].cube != NULL) {
    CCNodeFree (graphic[0].cube);
  }
  graphic[0].cube = CCNodeAlloc();

  cube = graphic[0].cube;
  cube->min[CC_X] = cube->min[CC_Y] = cube->min[CC_Z] = 0.0;
  cube->max[CC_X] = cube->max[CC_Y] = cube->max[CC_Z] = 1.0;
  cube->mid[CC_X] = cube->mid[CC_Y] = cube->mid[CC_Z] = 0.5;

  // subdivide cube into first, uniform division (4x4x4)
  CCSplitNodeIterate (cube, 0, 2);

  // need to allocate at least 64 + NPASS*7*NSPLIT pixels (overestimate if 64 < NSPLIT)
# define NPASS 5
# define NSPLIT 128
// # define NPASS 4
// # define NSPLIT 32

  for (i = 0; i < NPASS; i++) {
    // generate histogram
    ColorHistogram (image, cube);

    // extract histogram values
    values  = NULL;
    Nvalues = 0;
    NVALUES = 0;
    CCNodeExtractCounts (cube, &values, &Nvalues, &NVALUES);
  
    // select top N cells
    fsort (values, Nvalues);

    // split nodes with more than value[n]
    Nmin = MAX (0, Nvalues - NSPLIT);
    CCNodeDivideLimit (cube, values[Nmin]);
    free (values);

    CCNodeInitCounts (cube, 0.0);
  }

  // generate histogram
  ColorHistogram (image, cube);
  
  // convert histogram 3D values to cmap colors
  Npixels = 0;
  CCNodeSetColorMap (cube, graphic[0].cmap, graphic[0].Npixels - 1, &Npixels);

  // store the colors
  if (USE_XWINDOW) {
    if (graphic[0].dynamicColors) {
      XStoreColors(graphic[0].display, graphic[0].colormap, graphic[0].cmap, Npixels);
    } else {
      for (i = 0; i < Npixels; i++) {
	if (XAllocColor (graphic[0].display, graphic[0].colormap, &graphic[0].cmap[i]) == 0) {
	  fprintf (stderr, "error on %d\n", i);
	}
      }
    }
  }
  return TRUE;
}

// in 3D we use channels 0,1,2 to choose the pixel from the cube
int SetColorScale3D_CC (Graphic *graphic, KapaImageWidget *image) {

  CCNode *cube;

  // create the top-level cube
  cube = graphic[0].cube;
  CCNodeSetColorPixels (image, cube);
  return TRUE;
}

