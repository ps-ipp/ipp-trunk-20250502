# include "Ximage.h"

void CreatePicture (KapaImageWidget *image, Graphic *graphic) {

  int i, j, extra;
  unsigned char *c;
  unsigned int *l;
  unsigned int start, start1, start2, start3;

  start = graphic[0].back;

  switch (graphic[0].Nbits) {
  case 8:
    REALLOCATE (image[0].wide.data,    char, image[0].wide.dx*image[0].wide.dy);
    REALLOCATE (image[0].zoom.data,    char, image[0].zoom.dx*image[0].zoom.dy);
    REALLOCATE (image[0].picture.data, char, image[0].picture.dx*image[0].picture.dy);
    c = (unsigned char *) image[0].picture.data;
    for (i = 0; i < (image[0].picture.dx*image[0].picture.dy); i++, c++)
      *c = start;
    image[0].picture.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					  image[0].picture.data, image[0].picture.dx, image[0].picture.dy, 8, 0);
    break;

  case 16:
    REALLOCATE (image[0].wide.data,    char, 2*image[0].wide.dx*image[0].wide.dy);
    REALLOCATE (image[0].zoom.data,    char, 2*image[0].zoom.dx*image[0].zoom.dy);
    REALLOCATE (image[0].picture.data, char, 2*image[0].picture.dy*image[0].picture.dx);
    c = (unsigned char *) image[0].picture.data;
    start1 = 0x0000ff & (start);
    start2 = 0x0000ff & (start >> 8);
    for (i = 0; i < image[0].picture.dy; i++) {
      for (j = 0; j < image[0].picture.dx; j++, c+=2) {
	c[0] = start1;
	c[1] = start2;
      }
    }
    image[0].picture.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					  image[0].picture.data, image[0].picture.dx, image[0].picture.dy, 16, 0);
    break;

  case 24:
    
    extra = 4 - (image[0].wide.dx * 3) % 4;
    REALLOCATE (image[0].wide.data, char, image[0].wide.dy*(3*image[0].wide.dx+extra));

    extra = 4 - (image[0].zoom.dx * 3) % 4;
    REALLOCATE (image[0].zoom.data, char, image[0].zoom.dy*(3*image[0].zoom.dx+extra));

    extra = 4 - (image[0].picture.dx * 3) % 4;
    REALLOCATE (image[0].picture.data, char, image[0].picture.dy*(3*image[0].picture.dx+extra));
    c = (unsigned char *) image[0].picture.data;
    start1 = 0x0000ff & (start);
    start2 = 0x0000ff & (start >> 8);
    start3 = 0x0000ff & (start >> 16);
    for (i = 0; i < image[0].picture.dy; i++) {
      for (j = 0; j < image[0].picture.dx; j++, c+=3) {
	c[0] = start1;
	c[1] = start2;
	c[2] = start3;
      }
      c+=extra;
    }
    image[0].picture.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					  image[0].picture.data, image[0].picture.dx, image[0].picture.dy, 24, 0);
    break;

  case 32:
    REALLOCATE (image[0].wide.data,    char, 4*image[0].wide.dx*image[0].wide.dy);
    REALLOCATE (image[0].zoom.data,    char, 4*image[0].zoom.dx*image[0].zoom.dy);
    REALLOCATE (image[0].picture.data, char, 4*image[0].picture.dx*image[0].picture.dy);
    l = (unsigned int *) image[0].picture.data;
    for (i = 0; i < (image[0].picture.dx*image[0].picture.dy); i++, l++)
      *l = start;
    image[0].picture.pix = XCreateImage (graphic[0].display, graphic[0].visual, graphic[0].depth, ZPixmap, 0, 
					  image[0].picture.data, image[0].picture.dx, image[0].picture.dy, 32, 0);
    break;
  }
    
}
