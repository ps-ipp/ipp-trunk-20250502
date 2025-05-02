# include "Ximage.h"

// this function is called with an initialized stream buffer
int PDF_Image (PDF_FILE *obj, KapaImageWidget *image, IOBuffer *buffer) {
  
  if (!USE_XWINDOW) {
    fprintf (stderr, "PDF_Image not working yet for no X mode\n");
    return (TRUE);
  }

  Graphic *graphic = GetGraphic();

  // for a PDF image, we need to (1) generate the image dictionary containing the image
  // data and (2) generate a content stream placing the image along with the overlay
  // elements.  We actually generate (2) first and then write (1)

  int Xoff = (int) image[0].picture.x;
  int Yoff = graphic->dy - (int) image[0].picture.y - image[0].picture.dy;
  int dX = image[0].picture.dx;
  int dY = image[0].picture.dy;

  PrintIOBuffer (buffer, "q %d 0 0 %d %d %d cm\n", dX, dY, Xoff, Yoff);
  PrintIOBuffer (buffer, "/Image%d Do\n", obj->Nimage);
  PrintIOBuffer (buffer, "Q\n");

  fprintf (stderr, "%d %d : %d %d : %d %d\n", image->picture.x, image->picture.y, image->picture.dx, image->picture.dy, graphic->dx, graphic->dy);

  PrintIOBuffer (buffer, "q 1 0 0 1 %d %d cm\n", Xoff, Yoff);
  // add the overlay to the stream with the image 
  for (int i = 0; i < NOVERLAYS; i++) {
    if (image[0].overlay[i].active) {
      PrintIOBuffer (buffer, "%% overlay %d\n", i);
      PDF_Overlay (image, i, buffer, 0);
    }
  }
  for (int i = 0; i < NOVERLAYS; i++) {
    if (image[0].overlay[i].active) {
      PrintIOBuffer (buffer, "%% overlay %d\n", i);
      PDF_Overlay (image, i, buffer, 1);
    }
  }
  PrintIOBuffer (buffer, "Q\n");

  // Write Content stream to file
  PDF_WriteStream (obj, buffer);

  PDF_Pixmap (graphic, image, buffer);
  PDF_WriteImage (obj, buffer, dX, dY);

  return (TRUE);
}
