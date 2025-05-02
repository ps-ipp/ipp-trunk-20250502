# include "relastro.h"

int UpdateMosaic (Catalog *catalog, int Ncatalog) {

  /* we can measure new image parameters for each mosaic independently */
  off_t i, Nmosaic, Nstars;
  Mosaic *mosaic;
  StarData *raw, *ref;

  mosaic = getmosaics (&Nmosaic);

  for (i = 0; i < Nmosaic; i++) {

    /* convert measure coordinates to raw entries */
    raw = getMosaicRaw (catalog, Ncatalog, i, &Nstars);

    /* convert average coordinates to ref entries */
    ref = getMosaicRef (catalog, Ncatalog, i, &Nstars);

    // XXX : I'll need to supply these back to the image[] entry
    FitMosaic (raw, ref, Nstars, &mosaic[i].coords);

    free (raw);
    free (ref);
  }

  return (TRUE);
}

