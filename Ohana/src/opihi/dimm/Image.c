# include "dimm.h"

/*** this uses an Image structure from DIMM which 
     is different from the Image structure in DVO ***/

static Image *images = (Image *) NULL;
static int   Nimages = 0;

Image *createImage (int Nx, int Ny) {

  int N;

  if (Nx*Ny <= 0) return ((Image *) NULL);

  N = Nimages;
  Nimages ++;
  if (images == (Image *) NULL) {
    ALLOCATE (images, Image, MAX (1, Nimages));
  } else {
    REALLOCATE (images, Image, MAX (1, Nimages));
  }

  images[N].Nx = Nx;
  images[N].Ny = Ny;
  images[N].Nbytes = Nx*Ny*sizeof (short);
  ALLOCATE (images[N].buffer, char, images[N].Nbytes);

  return (&images[N]);
}

int freeImage (Image *entry) {

  int i, j, N;

  N = -1;
  for (i = 0; (i < Nimages) && (N == -1) ; i++) {
    if (&images[i] == entry) N = i;
  }
  if (N == -1) return (FALSE);

  free (images[N].buffer);

  for (j = N; j < Nimages - 1; j++) {
    images[j] = images[j+1];
  }

  Nimages --;
  REALLOCATE (images, Image, MAX (1, Nimages));
  return (TRUE);
}

int writeImage (char *filename, Image *image) {

  Header header;
  Matrix matrix;

  gfits_init_header (&header);

  header.Naxes = 2;
  header.Naxis[0] = image[0].Nx;
  header.Naxis[1] = image[0].Ny;
  header.bitpix = 16;

  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  free (matrix.buffer);

  matrix.buffer = image[0].buffer;
  
  /* write meta-data to header */
  gfits_print (&header, "RA", "%lf", 1, image[0].ra);
  gfits_print (&header, "DEC", "%lf", 1, image[0].dec);
  gfits_print (&header, "EQUINOX", "%lf", 1, 2000.0);

  gfits_print (&header, "AIRMASS", "%lf", 1, image[0].airmass);
  gfits_print (&header, "CCDTEMP", "%lf", 1, image[0].ccdtemp);
  gfits_print (&header, "AIRTEMP", "%lf", 1, image[0].airtemp);
  gfits_print (&header, "EXPTIME", "%lf", 1, image[0].exptime);

  gfits_write_header (filename, &header);
  gfits_write_matrix (filename, &matrix);
  
  return (TRUE);
}

