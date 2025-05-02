#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <strings.h>  // for strcasecmp
#include <string.h>

#include <kapa.h>
#undef free
// note: when using OHANA_MEMORY, kapa.h includes ohana.h, etc re-define free to use the
// internal function this file does not use ohana memory management directly (ALLOCATE/free),
// so we can undef it and allow psMemory.h to poison it

#include "psMemory.h"
#include "psImage.h"
#include "psVector.h"
#include "psError.h"
#include "psAssert.h"
#include "psImageJpeg.h"

/* XXX this to do to make this reasonably complete
 * update bDraw APIs to accept a bDrawBuffer structure as an input operand
 */

#ifdef HAVE_STDLIB_H
// jpeglib.h includes jconfig.h which is full of autoconf generated HAVE_*
// defines. This is a hack to work around CPP redefinition errors.  Arrrrrgh!!!
// -JH

// XXX specifically, jconfig.h has defines like the following.  these
// could be individually tested here and specifically undefed. EAM.
// #define HAVE_PROTOTYPES 
// #define HAVE_UNSIGNED_CHAR 
// #define HAVE_UNSIGNED_SHORT 

# undef HAVE_STDLIB_H
# include <jpeglib.h>
#endif // ifdef HAVE_STDLIB_H

static void imageJpegOptionsFree(psImageJpegOptions *options)
{

  if (!options) {
    return;
  }

  psFree(options->red);
  psFree(options->green);
  psFree(options->blue);
  return;
}

psImageJpegOptions *psImageJpegOptionsAlloc(void)
{

  psImageJpegOptions *options = psAlloc(sizeof(psImageJpegOptions));
  psMemSetDeallocator(options, (psFreeFunc)imageJpegOptionsFree);

  options->red   = psVectorAlloc(256, PS_TYPE_U8);
  options->blue  = psVectorAlloc(256, PS_TYPE_U8);
  options->green = psVectorAlloc(256, PS_TYPE_U8);

  options->min = 0.0;
  options->max = 1000.0;

  options->xFlip = false;
  options->yFlip = false;
  options->showScale = PS_JPEG_SHOWSCALE_BOTTOM;

  psImageJpegColormapSet(options, "greyscale");

  return options;
}

bool psImageJpegColormapSet(psImageJpegOptions *options, const char *name)
{
  PS_ASSERT_PTR_NON_NULL(options, false);

  /* grayscale */
  if ((!strcasecmp (name, "grayscale")) || (!strcasecmp (name, "greyscale"))) {
    for (int i = 0; i < options->red->n; i++) {
      options->red->data.U8[i]   = PS_JPEG_RANGELIM(i);
      options->green->data.U8[i] = PS_JPEG_RANGELIM(i);
      options->blue->data.U8[i]  = PS_JPEG_RANGELIM(i);
    }
    options->white = 255;
    options->black = 0;
    return options;
  }

  /* -grayscale */
  if ((!strcasecmp (name, "-grayscale")) || (!strcasecmp (name, "-greyscale"))) {
    for (int i = 0; i < options->red->n; i++) {
      options->red->data.U8[i]   = PS_JPEG_RANGELIM(256 - i);
      options->green->data.U8[i] = PS_JPEG_RANGELIM(256 - i);
      options->blue->data.U8[i]  = PS_JPEG_RANGELIM(256 - i);
    }
    options->white = 0;
    options->black = 255;
    return options;
  }

  /* rainbow */
  if (!strcasecmp (name, "rainbow")) {
    int I1 = 0.25*options->red->n;
    int I2 = 0.50*options->red->n;
    int I3 = 0.75*options->red->n;
    for (int i = 0; i < I1; i++) {
      options->red->data.U8[i]   = 0;
      options->green->data.U8[i] = 0;
      options->blue->data.U8[i]  = PS_JPEG_RANGELIM(4*i);
    }
    for (int i = I1; i < I2; i++) {
      options->red->data.U8[i]   = PS_JPEG_RANGELIM(4*(i - I1));
      options->green->data.U8[i] = 0;
      options->blue->data.U8[i]  = PS_JPEG_RANGELIM(4*(I2 - i));
    }
    for (int i = I2; i < I3; i++) {
      options->red->data.U8[i]   = 255;
      options->green->data.U8[i] = 4*(i - I2);
      options->blue->data.U8[i]  = 0;
    }
    for (int i = I3; i < options->red->n; i++) {
      options->red->data.U8[i]   = 255;
      options->green->data.U8[i] = 255;
      options->blue->data.U8[i]  = PS_JPEG_RANGELIM(4*(i - I3));
    }
    options->white = 255;
    options->black = 0;
    return options;
  }

  /* heat */
  if (!strcasecmp (name, "heat")) {
    int I1 = 0.25*options->red->n;
    int I2 = 0.50*options->red->n;
    int I3 = 0.75*options->red->n;
    for (int i = 0; i < I1; i++) {
      options->red->data.U8[i]   = PS_JPEG_RANGELIM(2*i);
      options->green->data.U8[i] = 0;
      options->blue->data.U8[i]  = 0;
    }
    for (int i = I1; i < I2; i++) {
      options->red->data.U8[i]   = PS_JPEG_RANGELIM(2*i);
      options->green->data.U8[i] = PS_JPEG_RANGELIM(2*(i - I1));
      options->blue->data.U8[i]  = 0;
    }
    for (int i = I2; i < I3; i++) {
      options->red->data.U8[i]   = 255;
      options->green->data.U8[i] = PS_JPEG_RANGELIM(2*(i - I1));
      options->blue->data.U8[i]  = PS_JPEG_RANGELIM(2*(i - I2));
    }
    for (int i = I3; i < options->red->n; i++) {
      options->red->data.U8[i]   = 255;
      options->green->data.U8[i] = 255;
      options->blue->data.U8[i]  = PS_JPEG_RANGELIM(2*(i - I2));
    }
    options->white = 255;
    options->black = 0;
    return options;
  }

  // invalid colormap: warn user 
  psWarning("Invalid colormap name: %s --- using greyscale\n", name);
  return psImageJpegColormapSet (options, "greyscale");
}

bDrawBuffer *psImageJpegOverlayInit (const psImage *image) {

  int dx = image->numCols;
  int dy = image->numRows;
  
  int Npalette;
  png_color *palette = KapaPNGPalette (&Npalette);

  bDrawBuffer *bdbuf = bDrawBufferCreate(dx, dy, 1, palette, Npalette);

  return bdbuf;
}

// copy the buffer pixels which are not white (probably should be "not blank")
bool psImageJpegOverlayDraw (JSAMPLE *jpegImage, bDrawBuffer *bdbuf, int offX, int offY) {

  // XXX check valid limits

  int dx = bdbuf->Nx;
  int dy = bdbuf->Ny;

  png_color *palette = bdbuf->palette;
  bDrawColor white = KapaColorByName ("white");
  for (int j = 0; j < dy; j++) {
    for (int i = 0; i < dx; i++) {
      bDrawColor color = bdbuf->pixels[j][i];
      if (color == white) continue;
      jpegImage[(j + offY)*3*dx + 3*(i + offX) + 0] = palette[color].red;
      jpegImage[(j + offY)*3*dx + 3*(i + offX) + 1] = palette[color].green;
      jpegImage[(j + offY)*3*dx + 3*(i + offX) + 2] = palette[color].blue;
    }
  }
  return true;
}

bool sprint_double (char *string, double value) {

  int Nexp = fabs(log10(fabs(value)));
  
  if (Nexp > 3) {
    sprintf (string, "%.1e", value);
  } else {
    sprintf (string, "%.1f", value);
  }
  return true;
}

// XXX need to update bDraw APIs to pass in/out structure and avoid the local static 
bool psImageJpeg(const psImageJpegOptions *options, const psImage *image, bDrawBuffer *bdbuf, const char *filename)
{
  PS_ASSERT_PTR_NON_NULL(options, false);
  PS_ASSERT_VECTOR_NON_NULL(options->red, false);
  PS_ASSERT_VECTOR_NON_NULL(options->green, false);
  PS_ASSERT_VECTOR_NON_NULL(options->blue, false);
  PS_ASSERT_FLOAT_REAL(options->min, false);
  PS_ASSERT_FLOAT_REAL(options->max, false);
  PS_ASSERT_IMAGE_NON_NULL(image, false);
  PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, false);
  PS_ASSERT_PTR_NON_NULL(filename, false);
  PS_ASSERT_INT_POSITIVE(strlen(filename), false);

  float zero, scale;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  long pixel;
  JSAMPLE *jpegLine;   // Points to data for current line
  JSAMPROW jpegLineList[1];  // pointer to JSAMPLE row[s]
  JSAMPLE *jpegImage;
  JSAMPLE *outPix;

  /* JPEG init calls */
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  /* open file, prep for jpeg */
  FILE *f = fopen(filename, "w");
  if (!f) {
    psError(PS_ERR_IO, true, "failed to open %s for output\n", filename);
    return false;
  }
  jpeg_stdio_dest(&cinfo, f);

  /* set up color jpeg buffers */
  int quality = 75;
  cinfo.image_width = image->numCols; // image width and height, in pixels
  cinfo.image_height = image->numRows;

  if (options->showScale != PS_JPEG_SHOWSCALE_NONE) {
    cinfo.image_height += PS_JPEG_COLORPAD + PS_JPEG_LABELPAD;
  }

  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, true); // limit to baseline-JPEG values
  jpeg_start_compress (&cinfo, true);

  psU8 *Rpix = options->red->data.U8;
  psU8 *Gpix = options->green->data.U8;
  psU8 *Bpix = options->blue->data.U8;

  if (options->max == options->min) {
    zero = options->min - 0.1;
    scale = 256.0/0.2;
  } else {
    zero = options->min;
    scale = 256.0/(options->max - options->min);
  }

  // dx,dy is the size of the image itself.  the drawing window may be larger
  // by the size of the scalebar (depending on the location)

  int dx = image->numCols;
  int dy = image->numRows;
  int Nx = cinfo.image_width;
  int Ny = cinfo.image_height;

  // output image buffer and line buffer
  jpegLine = psAlloc (3*Nx*sizeof(JSAMPLE));
  jpegImage = psAlloc (3*Nx*Ny*sizeof(JSAMPLE));

  // first copy the image data into the output buffer 
  // output image ranges from offset to offset + dy
  int offset = (options->showScale == PS_JPEG_SHOWSCALE_TOP) ? PS_JPEG_COLORPAD + PS_JPEG_LABELPAD : 0;
  for (int j = 0; j < dy; j++) {

    psF32 *row = options->yFlip ? image->data.F32[j] : image->data.F32[dy - j - 1];

    outPix = options->xFlip ?  jpegLine + 3*(dx - 1) : jpegLine;
    int delta = options->xFlip ? -3 : 3;
    for (int i = 0; i < dx; i++, outPix += delta) {
      if (isfinite(row[i])) {
	pixel = PS_JPEG_SCALEVALUE(row[i],zero,scale);
	outPix[0] = Rpix[pixel];
	outPix[1] = Gpix[pixel];
	outPix[2] = Bpix[pixel];
      } else {
	// XXX NAN value should be set per-color map
	outPix[0] = 0xe6;
	outPix[1] = 0xe6;
	outPix[2] = 0xff;
      }
    }
    memcpy (&jpegImage[(j + offset)*3*dx], jpegLine, 3*dx);
  }

  if (options->showScale != PS_JPEG_SHOWSCALE_NONE) {
    offset = (options->showScale == PS_JPEG_SHOWSCALE_TOP) ? 0 : dy;
    zero = 0;
    scale = 256.0 / dx;
    for (int j = 0; j < PS_JPEG_COLORPAD; j++) {
      outPix = jpegLine;
      for (int i = 0; i < dx; i++, outPix += 3) {
	pixel = PS_JPEG_SCALEVALUE(i, zero, scale);
	outPix[0] = Rpix[pixel];
	outPix[1] = Gpix[pixel];
	outPix[2] = Bpix[pixel];
      }
      memcpy (&jpegImage[(j + offset)*3*dx], jpegLine, 3*dx);
    }

    // set the LABEL region to white
    psU8 white = options->white;
    offset = (options->showScale == PS_JPEG_SHOWSCALE_TOP) ? PS_JPEG_COLORPAD : PS_JPEG_COLORPAD + dy; 
    for (int j = 0; j < PS_JPEG_LABELPAD; j++) {
      outPix = jpegLine;
      for (int i = 0; i < dx; i++, outPix += 3) {
	outPix[0] = Rpix[white];
	outPix[1] = Gpix[white];
	outPix[2] = Bpix[white];
      }
      memcpy (&jpegImage[(j + offset)*3*dx], jpegLine, 3*dx);
    }

    // set the scalebar labels
    int Npalette;
    png_color *palette = KapaPNGPalette (&Npalette);

    char string[64];
    bDrawBuffer *labels = bDrawBufferCreate(dx, PS_JPEG_LABELPAD, 1, palette, Npalette);
    SetRotFont ("helvetica", 8);
    sprint_double (string, options->min);
    bDrawRotText(labels, 2, 2, string, 2, 0.0);
    sprint_double (string, options->max);
    bDrawRotText(labels, dx - 2, 2, string, 0, 0.0);
    sprint_double (string, 0.5*(options->min + options->max));
    bDrawRotText(labels, 0.5*dx, 2, string, 1, 0.0);
    psImageJpegOverlayDraw(jpegImage, labels, 0, offset);
    bDrawBufferFree(labels);
  }
    
  if (bdbuf) {
    offset = (options->showScale == PS_JPEG_SHOWSCALE_TOP) ? PS_JPEG_COLORPAD + PS_JPEG_LABELPAD : 0;
    psImageJpegOverlayDraw(jpegImage, bdbuf, 0, offset);
  }

  // write out the image buffer
  for (int j = 0; j < Ny; j++) {
    jpegLineList[0] = &jpegImage[j*3*dx];
    if (jpeg_write_scanlines(&cinfo, jpegLineList, 1) == 0) {
      psError(PS_ERR_IO, true, "Unable to write line %d to JPEG", j);
      psFree(jpegLine);
      psFree(jpegImage);
      fclose(f);
      return false;
    }
  }

  jpeg_finish_compress(&cinfo);
  if (fclose(f) == EOF) {
    psError(PS_ERR_IO, true, "Failed to close %s", filename);
    psFree(jpegLine);
    psFree(jpegImage);
    return false;
  }
  jpeg_destroy_compress(&cinfo);

  psFree(jpegLine);
  psFree(jpegImage);
  return true;
}
