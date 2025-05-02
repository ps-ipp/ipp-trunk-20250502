#include <pslib.h>
#include "tap.h"
#include "pstap.h"

# define DX 512
# define DY 512
# define ZMIN -50 
# define ZMAX 500

int main(int argc, char **argv)
{
  plan_tests(6);
  InitRotFonts();

  {
    bool status;
    psMemId id = psMemGetId();

    psImageJpegOptions *options = psImageJpegOptionsAlloc();
    ok (options, "allocate colormap");
    skip_start (!options, 4, "skipping psImageJpeg tests");

    status = psImageJpegColormapSet(options, "rainbow");
    ok (status, "set colormap");
	
    psImage *image = psImageAlloc(DX, DY, PS_TYPE_F32);
    ok (image, "allocate image");

    // define some random image
    for (int iy = 0; iy < DY; iy++) {
      for (int ix = 0; ix < DX; ix++) {
	image->data.F32[iy][ix] = hypot((ix - 0.5*DX), iy - 0.5*DY);
      }
    }
	
    options->min = ZMIN;
    options->max = ZMAX;
    options->xFlip = false;
    options->yFlip = false;

    options->showScale = PS_JPEG_SHOWSCALE_NONE;
    status = psImageJpeg(options, image, NULL, "test.00.jpg");
    ok (status, "wrote jpeg without scale");

    options->showScale = PS_JPEG_SHOWSCALE_TOP;
    status = psImageJpeg(options, image, NULL, "test.01.jpg");
    ok (status, "wrote jpeg with scale");

    options->showScale = PS_JPEG_SHOWSCALE_BOTTOM;
    status = psImageJpeg(options, image, NULL, "test.02.jpg");
    ok (status, "wrote jpeg with scale");

    psFree(image);
    psFree(options);

    skip_end();
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
  }

  {
    bool status;
    psMemId id = psMemGetId();

    psImageJpegOptions *options = psImageJpegOptionsAlloc();
    ok (options, "allocate colormap");
    skip_start (!options, 4, "skipping psImageJpeg tests");

    status = psImageJpegColormapSet(options, "rainbow");
    ok (status, "set colormap");
	
    psImage *image = psImageAlloc(DX, DY, PS_TYPE_F32);
    ok (image, "allocate image");

    // define some random image
    for (int iy = 0; iy < DY; iy++) {
      for (int ix = 0; ix < DX; ix++) {
	image->data.F32[iy][ix] = hypot((ix - 0.5*DX), iy - 0.5*DY);
      }
    }
	
    bDrawBuffer *bdbuf = psImageJpegOverlayInit (image);

    // add elements to bDraw buffer
    bDrawSetBuffer(bdbuf);
    bDrawColor red = KapaColorByName("red");
    bDrawSetStyle (red, 1, 0);
    bDrawCircle(40.0, 20.0, 3.0);

    options->min = ZMIN;
    options->max = ZMAX;
    options->xFlip = false;
    options->yFlip = false;

    options->showScale = PS_JPEG_SHOWSCALE_NONE;
    status = psImageJpeg(options, image, bdbuf, "test.03.jpg");
    ok (status, "wrote jpeg without scale");

    options->showScale = PS_JPEG_SHOWSCALE_TOP;
    status = psImageJpeg(options, image, bdbuf, "test.04.jpg");
    ok (status, "wrote jpeg with scale");

    options->showScale = PS_JPEG_SHOWSCALE_BOTTOM;
    status = psImageJpeg(options, image, bdbuf, "test.05.jpg");
    ok (status, "wrote jpeg with scale");

    bDrawBufferFree (bdbuf);
    psFree(image);
    psFree(options);

    skip_end();
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
  }
  {
    bool status;
    psMemId id = psMemGetId();

    psImageJpegOptions *options = psImageJpegOptionsAlloc();
    ok (options, "allocate colormap");
    skip_start (!options, 4, "skipping psImageJpeg tests");

    status = psImageJpegColormapSet(options, "greyscale");
    ok (status, "set colormap");
	
    psImage *image = psImageAlloc(DX, DY, PS_TYPE_F32);
    ok (image, "allocate image");

    // define some random image
    for (int iy = 0; iy < DY; iy++) {
      for (int ix = 0; ix < DX; ix++) {
	image->data.F32[iy][ix] = hypot((ix - 0.5*DX), iy - 0.5*DY);
      }
    }
	
    options->min = ZMIN;
    options->max = ZMAX;
    options->xFlip = false;
    options->yFlip = false;

    options->showScale = PS_JPEG_SHOWSCALE_NONE;
    status = psImageJpeg(options, image, NULL, "test.06.jpg");
    ok (status, "wrote jpeg without scale");

    options->showScale = PS_JPEG_SHOWSCALE_TOP;
    status = psImageJpeg(options, image, NULL, "test.07.jpg");
    ok (status, "wrote jpeg with scale");

    options->showScale = PS_JPEG_SHOWSCALE_BOTTOM;
    status = psImageJpeg(options, image, NULL, "test.08.jpg");
    ok (status, "wrote jpeg with scale");

    psFree(image);
    psFree(options);

    skip_end();
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
  }

  {
    bool status;
    psMemId id = psMemGetId();

    psImageJpegOptions *options = psImageJpegOptionsAlloc();
    ok (options, "allocate colormap");
    skip_start (!options, 4, "skipping psImageJpeg tests");

    status = psImageJpegColormapSet(options, "greyscale");
    ok (status, "set colormap");
	
    psImage *image = psImageAlloc(DX, DY, PS_TYPE_F32);
    ok (image, "allocate image");

    // define some random image
    for (int iy = 0; iy < DY; iy++) {
      for (int ix = 0; ix < DX; ix++) {
	image->data.F32[iy][ix] = ix + iy;
      }
    }
	
    options->min = ZMIN;
    options->max = 1000.0;
    options->showScale = PS_JPEG_SHOWSCALE_BOTTOM;

    options->xFlip = false;
    options->yFlip = false;
    status = psImageJpeg(options, image, NULL, "test.09.jpg");
    ok (status, "wrote jpeg without scale");

    options->xFlip = true;
    options->yFlip = false;
    status = psImageJpeg(options, image, NULL, "test.10.jpg");
    ok (status, "wrote jpeg without scale");

    options->xFlip = false;
    options->yFlip = true;
    status = psImageJpeg(options, image, NULL, "test.11.jpg");
    ok (status, "wrote jpeg without scale");

    options->xFlip = true;
    options->yFlip = true;
    status = psImageJpeg(options, image, NULL, "test.12.jpg");
    ok (status, "wrote jpeg without scale");

    psFree(image);
    psFree(options);

    skip_end();
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
  }
}
