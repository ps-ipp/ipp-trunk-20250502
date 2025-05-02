/* @file  psImageJpeg.h
 * @brief functions to generate JPEG images from psImage
 *
 * @author EAM
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-08-09 01:40:07 $
 */

#ifndef PS_IMAGE_JPEG_H
#define PS_IMAGE_JPEG_H

/// @addtogroup FileIO Input/Output
/// @{

#include "psImage.h"

typedef enum {
  PS_JPEG_SHOWSCALE_NONE,
  PS_JPEG_SHOWSCALE_TOP,
  PS_JPEG_SHOWSCALE_BOTTOM
} psImageJpegShowScaleOption;  

typedef struct {
  psVector *red;                      // Red colormap
  psVector *green;                    // Green colormap
  psVector *blue;                     // Blue colormap
  psU8 white;			      // colormap-independent values
  psU8 black;
  float min;
  float max;
  bool xFlip;
  bool yFlip;
  psImageJpegShowScaleOption showScale; 
  // XXX include bDrawBuffer in here?
} psImageJpegOptions;

#define PS_JPEG_RANGELIM(A)(PS_MAX(0,PS_MIN(255,(A))))
#define PS_JPEG_SCALEVALUE(VALUE,ZERO,SCALE)(PS_MAX(0,PS_MIN(255,(SCALE*(VALUE-ZERO)))))

#define PS_JPEG_COLORPAD 10
#define PS_JPEG_LABELPAD 12

// allocate a colormap (does not define the map values)
psImageJpegOptions *psImageJpegOptionsAlloc(void) PS_ATTR_MALLOC;

// set the colormap values using the supplied name
bool psImageJpegColormapSet(psImageJpegOptions *options, // Colormap to set
			    const char *name // Name of colormap
			    );

// write out a JPEG file using the supplied image and colormap
// output goes to the specified filename
bool psImageJpeg(const psImageJpegOptions *options, // Color map
                 const psImage *image,  // Image to write
		 bDrawBuffer *bdbuf, 
                 const char *filename  // Filename of JPEG
		 );

bDrawBuffer *psImageJpegOverlayInit (const psImage *image);

/// @}
#endif /* PS_IMAGE_JPEG_H */
