# include "dvoshell.h"

static int badim_int = FALSE;
void badim_escape () {
  badim_int = TRUE;
}

int badimages (int argc, char **argv) {
  
  off_t i, Nimage;
  int entry, First, Cross;
  float *ptr;
  double nominal, big, small, value;
  Image *image;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: badimages entry value\n");
    gprint (GP_ERR, "   OR: badimages -image N\n");
    return (FALSE);
  }
  
  image = LoadImagesDVO (&Nimage);

  Cross = FALSE;
  First = FALSE;
  nominal = 1;
  if (!strcmp (argv[1], "-image")) {
    First = TRUE;
    entry = atof(argv[2]);
  } else {
    entry = atof(argv[1]);
    nominal = atof(argv[2]);
    if (!strcasecmp (argv[1], "x")) {
      Cross = TRUE;
    }
  }
  
  if (First) {
    ptr = &image[entry].coords.crpix1;
    for (i = 0; i < 22; i++) {
      gprint (GP_LOG, "%2lld: %g\n", (long long) i, ptr[i]);
    }
    value = image[entry].coords.pc1_1*image[entry].coords.pc2_2 + image[entry].coords.pc1_2*image[entry].coords.pc2_1;
    gprint (GP_LOG, " x: %g\n", value);
    return (TRUE);
  }
  
  big = nominal * 1.05;
  small = nominal / 1.05;
  if (big < small) {
    double tmp;
    tmp = big; big = small; small = tmp;
  }
  
  badim_int = FALSE;
  if (Cross) {
    for (i = 0; (i < Nimage) && !badim_int; i++) {
      value = image[i].coords.pc1_1*image[i].coords.pc2_2 + image[i].coords.pc1_2*image[i].coords.pc2_1;
      if ((value > big) || (value < small)) {
	gprint (GP_LOG, "%5lld %s: %d %g\n", (long long) i, image[i].name, image[i].tzero, value);
      }
    }
  } else {
    for (i = 0; (i < Nimage) && !badim_int; i++) {
      ptr = &image[i].coords.crpix1;
      if ((ptr[entry] > big) || (ptr[entry] < small)) {
	gprint (GP_LOG, "%5lld %s: %d %g\n", (long long) i, image[i].name, image[i].tzero, ptr[entry]);
      }
    }
  }
  
  FreeImagesDVO(image);
  return (TRUE);
}
