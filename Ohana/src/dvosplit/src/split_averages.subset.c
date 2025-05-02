# include "dvosplit.h"
# define NROWS 1000000 /* ~10MB per block for measures */
# define DNOUT 1000

/* used in find_matches, find_matches_refstars */
# define IN_REGION(REG,R,D) (			     \
((D) >= REG[0].Dmin) && ((D) < REG[0].Dmax) && \
((R) >= REG[0].Rmin) && ((R) < REG[0].Rmax))


AveLinks *split_averages (Catalog *incatalog, SkyList *outlist, Catalog *outcatalogs) {

  double inR, inD;
  int n, block, ave, cat, averef, Nblocks, Ncat, Nout, Nsecfilt;
  int *outref, *outcat, *outmem;
  AveLinks *avelinks;

  ALLOCATE (outref, int, incatalog[0].Naverage_disk);
  ALLOCATE (outcat, int, incatalog[0].Naverage_disk);
  ALLOCATE (outmem, int, outlist[0].Nregions);

  Nsecfilt = GetPhotcodeNsecfilt ();

  // allocate enough space for these output buffers: use Nsecfilt + 1 incase the file
  // contains primary photcodes, which will increase Nsecfilt by one.
  for (cat = 0; cat < outlist[0].Nregions; cat++) {
    outmem[cat] = DNOUT;
    REALLOCATE (outcatalogs[cat].average, Average, outmem[cat]);
    REALLOCATE (outcatalogs[cat].secfilt, SecFilt, outmem[cat]*(Nsecfilt + 1));
  }

  // split out the average & secfilt entries:
  incatalog[0].catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;

  if ((incatalog[0].catmode == DVO_MODE_SPLIT) && !FULL_TABLE) {
    Nblocks = incatalog[0].Naverage_disk / NROWS;
    if (incatalog[0].Naverage_disk % NROWS) Nblocks ++;
  } else {
    Nblocks = 1;
  }

  for (block = 0; block < Nblocks; block++) {

    if ((incatalog[0].catmode == DVO_MODE_SPLIT) && !FULL_TABLE) {
      // read up to NROWS at a time
      dvo_catalog_load_segment (incatalog, VERBOSE, block*NROWS, NROWS);
      fprintf (stderr, "splitting %s (averages) .. %d of %d\n", incatalog[0].filename, block, Nblocks);
      assert (block*NROWS == incatalog[0].Naverage_off);
    } else {
      dvo_catalog_load (incatalog, VERBOSE);
      fprintf (stderr, "splitting %s (averages)\n", incatalog[0].filename);
    }

    // distribute data to the output catalogs
    for (ave = 0; ave < incatalog[0].Naverage; ave++) {
      averef = ave + incatalog[0].Naverage_off;
	
      inR = incatalog[0].average[ave].R;
      inD = incatalog[0].average[ave].D;

      // XXX do not skip : galphot dvo has Nmeasure == 0
      if (incatalog[0].average[ave].Nmeasure == 0) {
	fprintf (stderr, "WARNING: object with no measurements, skipping %d (%f, %f)\n", averef, inR, inD);
      }

      // which of the outcatalogs contains this coordinate?
      Ncat = -1;
      for (cat = 0; cat < outlist[0].Nregions; cat++) {
	if (!IN_REGION(outlist[0].regions[cat], inR, inD)) continue;
	Ncat = cat;
	break;
      }

      // NO outcatalogs contains this coordinate?
      if (Ncat == -1) {
	fprintf (stderr, "WARNING: missed %d (%f, %f)\n", averef, inR, inD);
	continue;
      }

      Nout = outcatalogs[Ncat].Naverage;
      outref[averef] = Nout + outcatalogs[Ncat].Naverage_off;
      outcat[averef] = Ncat;

      // assign the value to the next element of the output catalog
      outcatalogs[Ncat].average[Nout] = incatalog[0].average[ave];
      outcatalogs[Ncat].Naverage ++;

      // update secfilt at the same time
      for (n = 0; n < Nsecfilt; n++) {
	outcatalogs[Ncat].secfilt[Nout*Nsecfilt + n] = incatalog[0].secfilt[ave*Nsecfilt + n];
	outcatalogs[Ncat].Nsecfilt_mem++;
      }

      if (outcatalogs[Ncat].Naverage >= outmem[Ncat]) {
	outmem[Ncat] += DNOUT;
	REALLOCATE (outcatalogs[Ncat].average, Average, outmem[Ncat]);
	REALLOCATE (outcatalogs[Ncat].secfilt, SecFilt, outmem[Ncat]*(Nsecfilt + 1));
      }
    }
    dvo_catalog_free_data (incatalog);

    // double check the values of Naverage, Nsecfilt_mem?

    // XXX for output.catformat == MEF, we probably need to skip this stuff and the free below

    // write out the new values
    if (!FULL_TABLE) {
      for (cat = 0; cat < outlist[0].Nregions; cat++) {
	outcatalogs[cat].catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;

	SetProtect (TRUE);
	dvo_catalog_save (&outcatalogs[cat], VERBOSE);
	SetProtect (FALSE);
	// fprintf (stderr, "secfilt: %d %d %d %d\n", outcatalogs[cat].Nsecfilt_mem, outcatalogs[cat].Nsecfilt_disk, outcatalogs[cat].Nsecfilt_off, outcatalogs[cat].Naverage, outcatalogs[cat].Nsecfilt);

	// advance the pointers and free the current data
	// XXX these should be done within save segment:
	outcatalogs[cat].Naverage_disk += outcatalogs[cat].Naverage;
	outcatalogs[cat].Naverage_off  += outcatalogs[cat].Naverage;
	outcatalogs[cat].Nsecfilt_disk += outcatalogs[cat].Nsecfilt * outcatalogs[cat].Naverage;
	outcatalogs[cat].Nsecfilt_off  += outcatalogs[cat].Nsecfilt * outcatalogs[cat].Naverage;
	outcatalogs[cat].Nsecfilt    = Nsecfilt;

	outcatalogs[cat].Naverage    = 0;
	outcatalogs[cat].Nsecfilt_mem   = 0;
      }
    }
  }

  if (!FULL_TABLE) {
    for (cat = 0; cat < outlist[0].Nregions; cat++) {
      dvo_catalog_free_data (&outcatalogs[cat]);
    }
  }

  free (outmem);

  ALLOCATE (avelinks, AveLinks, 1);
  avelinks[0].outref = outref;
  avelinks[0].outcat = outcat;


  return (avelinks);
}
