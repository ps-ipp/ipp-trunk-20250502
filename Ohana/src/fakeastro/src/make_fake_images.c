# include "fakeastro.h"

// fields I need to inherit from (one of) the WRP images:
// secz, Mcal (ensure measure.M is set right), exptime (if not hardwired), sidtime (or calculate from jd, longitude),
// extern_id, source_id?, name (is this used in relastro?, NO: stack names are used in relphot to set the tess_id)

Image *make_fake_images (Image *image, int *nfakeImage) {

  // given an image (really, the WCS) which defines an exposure, generate the image
  // parameters for a full set of chips in an exposure
  
  int NfakeImage = 65;
  Image *fakeImage = NULL;

  ALLOCATE (fakeImage, Image, NfakeImage);

  // first image is the PHU, copy basic data:

  fakeImage[0] = image[0];
  fakeImage[0].imageID = IMAGE_ID;
  IMAGE_ID ++;
  
  int N = 1;

  if (ONE_BIG_CHIP) {
    fakeImage[N] = fakeImage[0];

    // coords map relative to image center:
    fakeImage[N].coords.crval1 = 0.0;
    fakeImage[N].coords.crval2 = 0.0;

    fakeImage[N].coords.crpix1 = 0.5*8*5000;
    fakeImage[N].coords.crpix2 = 0.5*8*5000;
    fakeImage[N].coords.pc1_1 = 1.0;
    fakeImage[N].coords.pc2_2 = 1.0;

    fakeImage[N].coords.cdelt1 = 1.0;
    fakeImage[N].coords.cdelt2 = 1.0;

    fakeImage[N].coords.pc1_2 = 0.0;
    fakeImage[N].coords.pc2_1 = 0.0;

    strcpy (fakeImage[N].coords.ctype, "DEC--WRP");
    fakeImage[N].coords.Npolyterms = 1;

    fakeImage[N].coords.mosaic = &fakeImage[0].coords;

    fakeImage[N].NX = 8*5000;
    fakeImage[N].NY = 8*5000;

    fakeImage[N].imageID = IMAGE_ID;
    IMAGE_ID ++;

    // gpc1-specific (assumes grizy map to 10101, etc)
    fakeImage[N].photcode = fakeImage[0].photcode + 01;

    char *ptr = strstr (fakeImage[N].name, "[PHU]");
    if (ptr) {
      snprintf (ptr, 7, "[XY01]");
    }
    N++;
  } else {
    int ix, iy;
    for (ix = 0; ix < 8; ix++) {
      for (iy = 0; iy < 8; iy++) {
      
	if ((ix == 0) && (iy == 0)) continue;
	if ((ix == 7) && (iy == 0)) continue;
	if ((ix == 0) && (iy == 7)) continue;
	if ((ix == 7) && (iy == 7)) continue;

	fakeImage[N] = fakeImage[0];

	// coords map relative to image center:
	fakeImage[N].coords.crval1 = 0.0;
	fakeImage[N].coords.crval2 = 0.0;

	if (ix < 4) {
	  fakeImage[N].coords.crpix1 = (ix - 3)*5000;
	  fakeImage[N].coords.crpix2 = (iy - 3)*5100;
	  fakeImage[N].coords.pc1_1 = 1.0;
	  fakeImage[N].coords.pc2_2 = 1.0;
	} else {
	  fakeImage[N].coords.crpix1 = (4 - ix)*5000;
	  fakeImage[N].coords.crpix2 = (4 - iy)*5100;
	  fakeImage[N].coords.pc1_1 = -1.0;
	  fakeImage[N].coords.pc2_2 = -1.0;
	} 

	fakeImage[N].coords.cdelt1 = 1.0;
	fakeImage[N].coords.cdelt2 = 1.0;

	fakeImage[N].coords.pc1_2 = 0.0;
	fakeImage[N].coords.pc2_1 = 0.0;

	strcpy (fakeImage[N].coords.ctype, "DEC--WRP");
	fakeImage[N].coords.Npolyterms = 1;

	// XXX add higher order polynomials or imagemaps here

	fakeImage[N].coords.mosaic = &fakeImage[0].coords;

	fakeImage[N].NX = 4850;
	fakeImage[N].NY = 4850;

	fakeImage[N].imageID = IMAGE_ID;
	IMAGE_ID ++;

	// gpc1-specific (assumes grizy map to 10101, etc)
	fakeImage[N].photcode = fakeImage[0].photcode + iy*10 + ix;

	char *ptr = strstr (fakeImage[N].name, "[PHU]");
	if (ptr) {
	  snprintf (ptr, 7, "[XY%d%d]", ix, iy);
	}
	N++;
      }
    }
  }

  // the PHU needs to have photcode of 0 for relphot
  fakeImage[0].photcode = 0;

  *nfakeImage = N;
  return fakeImage;
}

