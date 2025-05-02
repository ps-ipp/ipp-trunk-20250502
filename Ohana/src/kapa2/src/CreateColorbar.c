# include "Ximage.h"

void CreateColorbar (KapaImageWidget *image, Graphic *graphic) {

  int i, j, dx, dy, extra, start;
  unsigned long  pixvalue;
  unsigned int  *out24;
  unsigned char *out8;

  dx = image[0].cmapbar.dx;
  dy = image[0].cmapbar.dy;

  /* create the cmap scale */
  switch (graphic[0].Nbits) {
  case 8:
    REALLOCATE (image[0].cmapbar.data, char, dx*dy);
    out8 = (unsigned char *) image[0].cmapbar.data;
    for (i = 0; i < dx; i++) {
      pixvalue = graphic[0].cmap[(int)(i*graphic[0].Npixels/dx)].pixel;
      for (j = 0; j < dy; j++) {
	out8[j*dx + i] = pixvalue;
      }
    }
    image[0].cmapbar.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					  image[0].cmapbar.data, dx, dy, 8, 0);
    break;

  case 16:
    REALLOCATE (image[0].cmapbar.data, char, 2*dy*dx);
    out8 = (unsigned char *) image[0].cmapbar.data;
    for (i = 0; i < dx; i++) {
      pixvalue = graphic[0].cmap[(int)(i*graphic[0].Npixels/dx)].pixel;
      for (j = 0; j < dy; j++) {
	start = 2*j*dx + 2*i;
	out8[start + 0] = 0x0000ff & pixvalue;
	out8[start + 1] = 0x0000ff & (pixvalue >> 8);
      }
    }
    image[0].cmapbar.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					  image[0].cmapbar.data, dx, dy, 16, 0);
    break;

  case 24:
    extra = 4 - (dx * 3) % 4;
    REALLOCATE (image[0].cmapbar.data, char, dy*(3*dx + extra));
    out8 = (unsigned char *) image[0].cmapbar.data;
    for (i = 0; i < dx; i++) {
      pixvalue = graphic[0].cmap[(int)(i*graphic[0].Npixels/dx)].pixel;
      for (j = 0; j < dy; j++) {
	start = j*(3*dx+extra) + 3*i;
	out8[start + 0] = 0x0000ff & pixvalue;
	out8[start + 1] = 0x0000ff & (pixvalue >> 8);
	out8[start + 2] = 0x0000ff & (pixvalue >> 16);
      }
    }
    image[0].cmapbar.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					  image[0].cmapbar.data, dx, dy, 32, 0);
    break;

  case 32:
    REALLOCATE (image[0].cmapbar.data, char, 4*dx*dy);
    out24 = (unsigned int *) image[0].cmapbar.data;
    for (i = 0; i < dx; i++) {
      pixvalue = graphic[0].cmap[(int)(i*graphic[0].Npixels/dx)].pixel;
      for (j = 0; j < dy; j++) {
	out24[j*dx + i] = pixvalue;
      }
    }
    image[0].cmapbar.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					 image[0].cmapbar.data, dx, dy, 32, 0);
    break;
  }
}
