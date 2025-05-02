# include "Ximage.h"

# define SETVALUE(VAR,VALUE,MINVAL,MAXVAL) { \
  float tmp = (VALUE); \
  if (tmp < MINVAL) { \
    VAR = MINVAL; \
  } else if (tmp > MAXVAL) { \
    VAR = MAXVAL; \
  } else { \
    VAR = tmp; \
  } }   

// the image arrives from the client program as an array of floats, stored in image->matrix.
// whenever we load a new image or change tv channels, we need to map the currently active
// image from float to pixel index (image->pixmap).  When we actually display (or change the
// display of) an image, we simply remap the lit pixels (in Remap32.c, etc).

int SetColormap (char *inName) {

  int i, red, blue, green;
  float scale, blueRef, redRef, greenRef;
  Graphic *graphic;

  graphic = GetGraphic();

  if (!inName && !graphic->colormapName) abort();

  if (inName) {
    if (graphic->colormapName) free (graphic->colormapName);
    graphic->colormapName = strcreate (inName);
  }

  int NANValue = graphic[0].Npixels - 1;
  int MaxValue = graphic[0].Npixels - 1;

  // the "fullcolor" colormap is uniquely defined for each image;
  // defer to the 'SetColorScale' step
  if (!strcasecmp (graphic->colormapName, "fullcolor")) {
    graphic[0].ColorScaleMode = KAPA_SCALE_3D_FULL;
    if (SetColorCubeHistogram ()) return TRUE;
  }

  // XXX a bit bogus: we are going to store a special color at Npixels-1 for 
  // marking the NAN pixels.  
  { 
      // XXX set pure green for now
      graphic[0].cmap[NANValue].red = NAN_RED;
      graphic[0].cmap[NANValue].green = NAN_GREEN;
      graphic[0].cmap[NANValue].blue = NAN_BLUE;
  }  

  // very simple color model: evenly spaced cube 
  if (!strcasecmp (graphic->colormapName, "ruffcolor")) {
      graphic[0].nRed  = pow (graphic[0].Npixels, 0.333);
      graphic[0].nBlue = pow (graphic[0].Npixels, 0.333);
      graphic[0].nGreen = (graphic[0].Npixels - 1) / (graphic[0].nRed * graphic[0].nBlue);

      // red,green,blue are values in range 0x0000 to 0xffff
      float redScale = 0xffff / (graphic[0].nRed - 1);
      float blueScale = 0xffff / (graphic[0].nBlue - 1);
      float greenScale = 0xffff / (graphic[0].nGreen - 1);

      i = 0;
      for (red = 0; red < graphic[0].nRed; red++) {  
	  for (blue = 0; blue < graphic[0].nBlue; blue++) {  
	      for (green = 0; green < graphic[0].nGreen; green++, i++) {  
		  SETVALUE (graphic[0].cmap[i].red,   red*redScale, 0, 0xffff);
		  SETVALUE (graphic[0].cmap[i].blue,  blue*blueScale, 0, 0xffff);
		  SETVALUE (graphic[0].cmap[i].green, green*greenScale, 0, 0xffff);
		  graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
	      }
	  }
      }
      // fprintf (stderr, "ruff Npix: %d vs %d\n", i, graphic[0].Npixels);

      // all other modes are 1D: set the flag:
      graphic[0].ColorScaleMode = KAPA_SCALE_3D_RUFF;
      goto store_colors;
  }

  // all other modes are 1D: set the flag:
  graphic[0].ColorScaleMode = KAPA_SCALE_1D;

  // red,green,blue are values in range 0x0000 to 0xffff
  scale = 0xffff / (float) (MaxValue - 1);

  /* greyscale */
  if ((!strcasecmp (graphic->colormapName, "grayscale")) || (!strcasecmp (graphic->colormapName, "greyscale"))) {
    for (i = 0; i < MaxValue; i++) {  
      SETVALUE (graphic[0].cmap[i].red,   0xffff - i*scale, 0, 0xffff);
      SETVALUE (graphic[0].cmap[i].green, 0xffff - i*scale, 0, 0xffff);
      SETVALUE (graphic[0].cmap[i].blue,  0xffff - i*scale, 0, 0xffff);
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    goto store_colors;
  }
  /* -grayscale */
  if ((!strcasecmp (graphic->colormapName, "-grayscale")) || (!strcasecmp (graphic->colormapName, "-greyscale"))) {
    for (i = 0; i < MaxValue; i++) {  
      SETVALUE (graphic[0].cmap[i].red,   i*scale, 0, 0xffff);
      SETVALUE (graphic[0].cmap[i].green, i*scale, 0, 0xffff);
      SETVALUE (graphic[0].cmap[i].blue,  i*scale, 0, 0xffff);
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    goto store_colors;
  }
  /* heat */
  if (!strcasecmp (graphic->colormapName, "Heat")) {
    greenRef = 0.25*MaxValue*scale*2.0;
    blueRef  = 0.50*MaxValue*scale*2.0;
    for (i = 0; i < (int)(0.25*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].red,   2*i*scale, 0, 0xffff);
      graphic[0].cmap[i].green = 0;
      graphic[0].cmap[i].blue  = 0;
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    for (i = 0.25*MaxValue; i < 0.50*MaxValue; i++) {  
      SETVALUE (graphic[0].cmap[i].red,   2*i*scale,            0, 0xffff);
      SETVALUE (graphic[0].cmap[i].green, 2*i*scale - greenRef, 0, 0xffff);
      graphic[0].cmap[i].blue = 0;
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    for (i = (int)(0.50*MaxValue); i < (int)(0.75*MaxValue); i++) {  
      graphic[0].cmap[i].red = 0xffff;
      SETVALUE (graphic[0].cmap[i].green, 2*i*scale - greenRef, 0, 0xffff);
      SETVALUE (graphic[0].cmap[i].blue,  2*i*scale - blueRef,  0, 0xffff);
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    for (i = (int)(0.75*MaxValue); i < MaxValue; i++) {  
      graphic[0].cmap[i].red   = 0xffff;
      graphic[0].cmap[i].green = 0xffff;
      SETVALUE (graphic[0].cmap[i].blue,  2*i*scale - blueRef,  0, 0xffff);
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    goto store_colors;
  }
  /* rainbow */
  if (!strcasecmp (graphic->colormapName, "Rainbow")) {
    redRef   = 0.25*MaxValue*scale*4.0;
    greenRef = 0.50*MaxValue*scale*4.0;
    blueRef  = 0.50*MaxValue*scale*4.0;
    for (i = 0; i < (int)(0.25*MaxValue); i++) {  
      graphic[0].cmap[i].red   = 0;
      graphic[0].cmap[i].green = 0;
      SETVALUE (graphic[0].cmap[i].blue,  4*i*scale,           0, 0xffff);
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    for (i = (int)(0.25*MaxValue); i < (int)(0.50*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].red,   4*i*scale - redRef,  0, 0xffff);
      graphic[0].cmap[i].green = 0;
      SETVALUE (graphic[0].cmap[i].blue,  blueRef - 4*i*scale, 0, 0xffff);
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    for (i = (int)(0.50*MaxValue); i < (int)(0.75*MaxValue); i++) {  
      graphic[0].cmap[i].red  = 0xffff;
      SETVALUE (graphic[0].cmap[i].green,  4*i*scale - greenRef, 0, 0xffff);
      graphic[0].cmap[i].blue = 0;
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    // blue = blueScale * i + blueRef
    // blue = (0xffff - 0.0)*(i - MinValue) / (MaxValue - MinValue) + 0.0
    // blue = i * 0xffff / (MaxValue - MinValue) - 0xffff * MinValue / (MaxValue - MinValue)
    float iMin = (int)(0.75*MaxValue);
    float iMax = MaxValue - 1;
    float dB = 0xffff / (iMax - iMin);
    float oB = -0xffff * iMin / (iMax - iMin);
    for (i = iMin; i < MaxValue; i++) {  
      graphic[0].cmap[i].red   = 0xffff;
      graphic[0].cmap[i].green = 0xffff;
      SETVALUE (graphic[0].cmap[i].blue,  i*dB + oB, 0, 0xffff);
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    goto store_colors;
  }
  /* anuenue */
  if (!strcasecmp (graphic->colormapName, "anuenue")) {
    redRef   = 0.25*MaxValue*scale*4.0; // value at 0.0
    greenRef = 0.50*MaxValue*scale*4.0;
    blueRef  = 1.33*MaxValue*scale;
    for (i = 0; i < (int)(0.50*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].green, 0.0, 0, 0xffff); // red is a ramp from 0 to 0xffff
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    for (i = (int)(0.50*MaxValue); i < (int)(0.75*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].green, 4.0*i*scale - 0x20000, 0, 0xffff); // red is a ramp from 0 to 0xffff
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }
    for (i = (int)(0.75*MaxValue); i < MaxValue; i++) {  
      SETVALUE (graphic[0].cmap[i].green, 0xffff, 0, 0xffff); // red is a ramp from 0 to 0xffff
      graphic[0].cmap[i].flags = DoRed | DoGreen | DoBlue;
    }

    for (i = 0; i < (int)(0.25*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].red,  4*i*scale, 0, 0xffff);
    }
    for (i = (int)(0.25*MaxValue); i < (int)(0.50*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].red,  0x20000 - 4*i*scale, 0, 0xffff);
    }
    for (i = (int)(0.50*MaxValue); i < MaxValue; i++) {  
      SETVALUE (graphic[0].cmap[i].red,  2*i*scale - 0xffff, 0, 0xffff);
    }

    for (i = 0; i < (int)(0.50*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].blue,  2*i*scale, 0, 0xffff);
    }
    for (i = (int)(0.50*MaxValue); i < (int)(0.75*MaxValue); i++) {  
      SETVALUE (graphic[0].cmap[i].blue,  0x30000 - 4*i*scale, 0, 0xffff);
    }
    for (i = (int)(0.75*MaxValue); i < MaxValue; i++) {  
      SETVALUE (graphic[0].cmap[i].blue,  4*i*scale - 0x30000, 0, 0xffff);
    }
    goto store_colors;
  }

  /* anuenue */
  if (!strncmp (graphic->colormapName, "file:", 5) || 
      !strncmp (graphic->colormapName, "lgcy:", 5) || 
      !strncmp (graphic->colormapName, "cetf:", 5) || 
      !strncmp (graphic->colormapName, "cetr:", 5) || 
      !strncmp (graphic->colormapName, "csvf:", 5)) {

    FILE *f = fopen (&graphic->colormapName[5], "r");
    if (!f) {
      fprintf (stderr, "failed to open colormap file %s\n", &graphic->colormapName[5]);
      return FALSE;
    }
    char line[1024];

    int lastIndex = 0;
    float lastRed = 0.0;
    float lastBlue = 0.0;
    float lastGreen = 0.0;

    int isCSV = !strncmp (graphic->colormapName, "csvf:", 5);
    int isCET = !strncmp (graphic->colormapName, "cetf:", 5);
    int isCETRev = !strncmp (graphic->colormapName, "cetr:", 5);
    int isLegacy = !strncmp (graphic->colormapName, "lgcy:", 5);

    float fracIndex, fracRed, fracBlue, fracGreen;

    if (isCET || isCETRev) fracIndex = 0.0;

    while (scan_line_maxlen (f, line, 1024) != EOF) {
      // file contains f R G B : f = 0.0 - 1.0, R,G,B = 0.0 - 1.0

      int Nscan;
      if (isCSV) {
	Nscan = sscanf (line, "%f,%f,%f,%f", &fracIndex, &fracRed, &fracGreen, &fracBlue);
	if (Nscan != 4) continue;
      }
      if (isCET || isCETRev) {
	Nscan = sscanf (line, "%f,%f,%f", &fracRed, &fracGreen, &fracBlue);
	fracIndex += 1.0 / 256.0;
	if (Nscan != 3) continue;
      }
      if (isLegacy) {
	Nscan = sscanf (line, "%f %f %f %f", &fracIndex, &fracRed, &fracBlue, &fracGreen);
	if (Nscan != 4) continue;
      }
      if (!isCSV && !isLegacy && !isCET && !isCETRev) {
	Nscan = sscanf (line, "%f %f %f %f", &fracIndex, &fracRed, &fracGreen, &fracBlue);
	if (Nscan != 4) continue;
      }

      int nextIndex = fracIndex * MaxValue;
      if (nextIndex <= lastIndex) {
	lastRed = fracRed;
	lastBlue = fracBlue;
	lastGreen = fracGreen;
	continue;
      }
      float dRdI = (fracRed   - lastRed)   / (float) (nextIndex - lastIndex);
      float dBdI = (fracBlue  - lastBlue)  / (float) (nextIndex - lastIndex);
      float dGdI = (fracGreen - lastGreen) / (float) (nextIndex - lastIndex);
      for (i = lastIndex; i < nextIndex; i++) {
	float redValue   = 0xffff * (dRdI * (i - lastIndex) + lastRed);
	float blueValue  = 0xffff * (dBdI * (i - lastIndex) + lastBlue);
	float greenValue = 0xffff * (dGdI * (i - lastIndex) + lastGreen);
	SETVALUE (graphic[0].cmap[i].red,   redValue,   0, 0xffff);
	SETVALUE (graphic[0].cmap[i].blue,  blueValue,  0, 0xffff);
	SETVALUE (graphic[0].cmap[i].green, greenValue, 0, 0xffff);
      }
      lastRed = fracRed;
      lastBlue = fracBlue;
      lastGreen = fracGreen;
      lastIndex = nextIndex;
    }
    fclose (f);

    if ((int) (MaxValue*fracIndex) < MaxValue) {
      float dRdI = (1.0 - lastRed)   / (float) (MaxValue - lastIndex);
      float dBdI = (1.0 - lastBlue)  / (float) (MaxValue - lastIndex);
      float dGdI = (1.0 - lastGreen) / (float) (MaxValue - lastIndex);
      for (i = lastIndex; i < MaxValue; i++) {
	float redValue   = 0xffff * (dRdI * (i - lastIndex) + lastRed);
	float blueValue  = 0xffff * (dBdI * (i - lastIndex) + lastBlue);
	float greenValue = 0xffff * (dGdI * (i - lastIndex) + lastGreen);
	SETVALUE (graphic[0].cmap[i].red,   redValue,   0, 0xffff);
	SETVALUE (graphic[0].cmap[i].blue,  blueValue,  0, 0xffff);
	SETVALUE (graphic[0].cmap[i].green, greenValue, 0, 0xffff);
      }
    }

    // reverse the color sequence:
    if (isCETRev) {
      for (i = 0; i < MaxValue / 2; i++) {  
	unsigned short tmp;
	// fprintf (stderr, "swap: %d %d\n", graphic[0].cmap[i].red, graphic[0].cmap[MaxValue - 1 - i].red);
	tmp = graphic[0].cmap[i].red;   graphic[0].cmap[i].red   = graphic[0].cmap[MaxValue - 1 - i].red;   graphic[0].cmap[MaxValue - 1 - i].red   = tmp;
	tmp = graphic[0].cmap[i].blue;  graphic[0].cmap[i].blue  = graphic[0].cmap[MaxValue - 1 - i].blue;  graphic[0].cmap[MaxValue - 1 - i].blue  = tmp;
	tmp = graphic[0].cmap[i].green; graphic[0].cmap[i].green = graphic[0].cmap[MaxValue - 1 - i].green; graphic[0].cmap[MaxValue - 1 - i].green = tmp;
      }
    }

    goto store_colors;
  }

  return (FALSE);

 store_colors:
  if (!USE_XWINDOW) return (TRUE);
  if (graphic[0].dynamicColors) {
    if (!XStoreColors(graphic[0].display, graphic[0].colormap, graphic[0].cmap, graphic[0].Npixels)) {
	fprintf (stderr, "error storing colors\n");
    }
  } else {
    for (i = 0; i < graphic[0].Npixels; i++) {
      if (!XAllocColor (graphic[0].display, graphic[0].colormap, &graphic[0].cmap[i])) {
	fprintf (stderr, "error on %d\n", i);
      }
    }
  }
  return (TRUE);
}

