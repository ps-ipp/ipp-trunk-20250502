# include "dvosplit.h"
# define NROWS 1000000 /* ~10MB per row for measures */
# define DNOUT 1000

int split_measures (Catalog *incatalog, SkyList *outlist, Catalog *outcatalogs, AveLinks *avelinks) {

  int block, meas, cat, Nblocks, Ncat, Nout, averef;
  int *outref, *outcat, *outmem;

  outref = avelinks->outref;
  outcat = avelinks->outcat;
  ALLOCATE (outmem, int, outlist[0].Nregions);

  // allocate enough space for the output buffer
  for (cat = 0; cat < outlist[0].Nregions; cat++) {
    outmem[cat] = DNOUT;
    REALLOCATE (outcatalogs[cat].measure, Measure, outmem[cat]);
  }

  // split out the measure entries:
  incatalog[0].catflags = DVO_LOAD_MEASURE | DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;

  // if ((incatalog[0].catformat == DVO_FORMAT_ELIXIR) || (incatalog[0].catformat == DVO_FORMAT_LONEOS)) {
  //   // for these two formats, we need the average and secfilt values around until we do the measures...
  //   // XXX I am loading these 2x -- perhaps I can make the API smarter about reloading?
  //   incatalog[0].catflags |= DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT; 
  // }

  if ((incatalog[0].catmode == DVO_MODE_SPLIT) && !FULL_TABLE) {
    Nblocks = incatalog[0].Nmeasure_disk / NROWS;
    if (incatalog[0].Nmeasure_disk % NROWS) Nblocks ++;
  } else {
    Nblocks = 1;
  }

  for (block = 0; block < Nblocks; block++) {

    // read up to NROWS at a time
    if ((incatalog[0].catmode == DVO_MODE_SPLIT) && !FULL_TABLE) {
      dvo_catalog_load_segment (incatalog, VERBOSE, block*NROWS, NROWS);
      fprintf (stderr, "splitting %s (measures) .. %d of %d\n", incatalog[0].filename, block, Nblocks);
      assert (block*NROWS == incatalog[0].Nmeasure_off);
    } else {
      dvo_catalog_load (incatalog, VERBOSE);
      fprintf (stderr, "splitting %s (measures)\n", incatalog[0].filename);
    }

    for (meas = 0; meas < incatalog[0].Nmeasure; meas++) {

      averef = incatalog[0].measure[meas].averef;
      Ncat = outcat[averef];

      int averef_out = outref[averef];
      if (averef_out >= outcatalogs[Ncat].Naverage) {
	fprintf (stderr, "mismatch 1\n");
	abort();
      }

      Nout = outcatalogs[Ncat].Nmeasure;
      outcatalogs[Ncat].measure[Nout] = incatalog[0].measure[meas];
      outcatalogs[Ncat].measure[Nout].averef = outref[averef];

      outcatalogs[Ncat].Nmeasure++;

      if (outcatalogs[Ncat].Nmeasure >= outmem[Ncat]) {
	outmem[Ncat] += DNOUT;
	REALLOCATE (outcatalogs[Ncat].measure, Measure, outmem[Ncat]);
      }
    }
    dvo_catalog_free_data (incatalog);

    if (!FULL_TABLE) {
      for (cat = 0; cat < outlist[0].Nregions; cat++) {
	outcatalogs[cat].catflags = DVO_LOAD_MEASURE;

	SetProtect (TRUE);
	dvo_catalog_save (&outcatalogs[cat], VERBOSE);
	SetProtect (FALSE);

	outcatalogs[cat].Nmeasure_disk += outcatalogs[cat].Nmeasure;
	outcatalogs[cat].Nmeasure_off  += outcatalogs[cat].Nmeasure;
	outcatalogs[cat].Nmeasure    = 0;
      }
    }
  }

  if (FULL_TABLE) {
    for (cat = 0; cat < outlist[0].Nregions; cat++) {
      outcatalogs[cat].catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT | DVO_LOAD_MEASURE;
      dvo_catalog_save (&outcatalogs[cat], VERBOSE);
    }
  }

  for (cat = 0; cat < outlist[0].Nregions; cat++) {
    dvo_catalog_free_data (&outcatalogs[cat]);
  }

  free (outmem);

  return (TRUE);
}
