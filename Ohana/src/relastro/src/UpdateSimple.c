# include "relastro.h"

int UpdateSimple (Catalog *catalog, int Ncatalog) {

  /* we can measure new image parameters for each non-mosaic chip independently */
  off_t i, Nimage, Nstars;
  Image *image;
  StarData *raw, *ref;

  image = getimages (&Nimage, NULL);

  for (i = 0; i < Nimage; i++) {

    /* skip WRP and DIS images */
    if (!strcmp(&image[i].coords.ctype[4], "-WRP")) continue;
    if (!strcmp(&image[i].coords.ctype[4], "-DIS")) continue;

    /* convert measure coordinates to raw entries */
    raw = getImageRaw (catalog, Ncatalog, i, &Nstars, MODE_SIMPLE);
    if (!raw) continue;

    /* convert average coordinates to ref entries */
    ref = getImageRef (catalog, Ncatalog, i, &Nstars, MODE_SIMPLE);
    if (!ref) continue;

    FitSimple (raw, ref, Nstars, &image[i].coords);

    free (raw);
    free (ref);
  }

  return (TRUE);
}

