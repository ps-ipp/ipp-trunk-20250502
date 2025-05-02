# include "Ximage.h"

int SetImageData (int sock) {
  
  Section *section;
  KapaImageWidget *image;

  // XXX needed for XClearWindow below 
  // Graphic *graphic = GetGraphic();

  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;
  
  // get image data from client 
  KiiScanMessage (sock, "%lf %lf %s %s", 
		  &image[0].image[0].zero, 
		  &image[0].image[0].range, 
		  image[0].image[0].name, 
		  image[0].image[0].file);

  // XXX when we go to 32bit, this should remap the image
  // if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
  // Refresh (1);

  return (TRUE);  
}

int GetImageData (int sock) {
  
  Section *section;
  KapaImageWidget *image;

  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;

  KiiSendMessage (sock, "%g %g %s %s", 
		  image[0].image[0].zero,
		  image[0].image[0].range,
		  image[0].image[0].name,
		  image[0].image[0].file);

  return (TRUE);
}

int SetImageCoords (int sock) {
  
  Section *section;
  KapaImageWidget *image;

  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;
  
  KiiScanMessage (sock, "%f %f %f %f", 
		  &image[0].image[0].coords.pc1_1, &image[0].image[0].coords.pc2_2,
		  &image[0].image[0].coords.pc1_2, &image[0].image[0].coords.pc2_1);

  KiiScanMessage (sock, "%s", image[0].image[0].coords.ctype);

  KiiScanMessage (sock, "%lf %lf %f %f %f %f", 
		  &image[0].image[0].coords.crval1,
		  &image[0].image[0].coords.crval2,
		  &image[0].image[0].coords.crpix1,
		  &image[0].image[0].coords.crpix2,
		  &image[0].image[0].coords.cdelt1,
		  &image[0].image[0].coords.cdelt2);

  return (TRUE);  
}

int GetImageCoords (int sock) {
  
  Section *section;
  KapaImageWidget *image;

  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;

  KiiSendMessage (sock, "%g %g %g %g", 
		  image[0].image[0].coords.pc1_1, image[0].image[0].coords.pc2_2,
		  image[0].image[0].coords.pc1_2, image[0].image[0].coords.pc2_1);

  KiiSendMessage (sock, "%s", image[0].image[0].coords.ctype);

  KiiSendMessage (sock, "%g %g %g %g %g %g", 
		  image[0].image[0].coords.crval1,
		  image[0].image[0].coords.crval2,
		  image[0].image[0].coords.crpix1,
		  image[0].image[0].coords.crpix2,
		  image[0].image[0].coords.cdelt1,
		  image[0].image[0].coords.cdelt2);

  return (TRUE);
}

int GetImageRange (int sock) {
  
  Section *section;
  KapaImageWidget *image;
  double Xmin, Xmax, Ymin, Ymax;

  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;

  Picture_to_Image (&Xmin, &Ymin, 0.0, 0.0, &image[0].picture);
  Picture_to_Image (&Xmax, &Ymax, image[0].picture.dx, image[0].picture.dy, &image[0].picture);

  KiiSendMessage (sock, "%g %g %g %g %d %d", Xmin, Xmax, Ymin, Ymax, image[0].picture.dx, image[0].picture.dy);

  return (TRUE);
}

