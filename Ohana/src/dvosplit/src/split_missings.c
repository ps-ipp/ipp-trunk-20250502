# include "dvosplit.h"
# define NROWS 100000 /* ~10MB per row for missings */

int split_missings (Catalog *incatalog, SkyList *outlist, Catalog *outcatalogs, AveLinks *avelinks) {

  int *outref, *outcat;

  outref = avelinks->outref;
  outcat = avelinks->outcat;

  // allocate enough space for the output buffer
  for (cat = 0; cat < outlist[0].Nregions; cat++) {
    REALLOCATE (outcatalog[cat].missing, Missing, NROWS);
  }

  // split out the missing entries:
  incatalog[0].catflags = DVO_LOAD_MISSING;
  Nblocks = incatalog[0].Nmissing_disk / NROWS;
  if (incatalog[0].Nmissing_disk % NROWS) Nblocks ++;
  for (block = 0; block < Nblocks; block++) {

    // read up to NROWS at a time
    dvo_catalog_load_segment (incatalog, VERBOSE, block*NROWS, NROWS);

    for (miss = 0; miss < incatalog[0].Nmissing; miss++) {

      averef = incatalog[0].missing[miss].averef;
      Ncat = outcat[averef];

      Nout = outcatalog[Ncat].Nmissing;
      outcatalog[Ncat].missing[Nout] = incatalog[0].missing[miss];
      outcatalog[Ncat].missing[Nout].averef = outref[averef];
      outcatalog[Ncat].Nmissing++;
    }

    for (cat = 0; cat < outlist[0].Nregions; cat++) {
      outcatalogs[cat].catflags = DVO_LOAD_MISSING;
      dvo_catalog_save_segment (&outcatalog[cat], VERBOSE);

      outcatalog[cat].Nmissing_disk += outcatalog[cat].Nmissing;
      outcatalog[cat].Nmissing_off  += outcatalog[cat].Nmissing;
      outcatalog[cat].Nmissing    = 0;

    }
  }
  return (TRUE);
}
