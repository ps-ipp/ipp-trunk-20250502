# include "addstar.h"

// load photometry data from psphot, sextractor, and a few other formats
// examine the header sets and set the Image entries for the the valid images
Catalog *LoadData (FILE *f, AddstarFile *file, Image **images, off_t *nvalid, Header **headers, off_t *extsize, HeaderSet *headerSets, int Nimages, SkyRegion *region, AddstarClientOptions *options) {

  off_t Nskip, Nvalid, NVALID;
  int i, j, Nhead, Ndata;
  uint32_t parentID = UINT32_MAX;

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, 1);
  ALLOCATE (catalog->measure, Measure, 1);
  ALLOCATE (catalog->lensing, Lensing, 1);

  if (images[0] == NULL) {
    Nvalid = 0;
    NVALID = 10;
    ALLOCATE (images[0], Image, NVALID);
  } else {
    Nvalid = *nvalid;
    NVALID = Nvalid + 10;
    REALLOCATE (images[0], Image, NVALID);
  }    

  // if zero points are calculated for the full exposure using more than just the matched chip header,
  // we need to perform that analysis here
  GetZeroPointExposure (headers, headerSets, Nimages);

  if (!options[0].mosaic) {
    // we are requiring a single set of N x WRPs + 1 DIS per file set.
    // NOTE: if the -mosaic was supplied, do not reset the mosaic coords here 
    initMosaicCoords ();
  }

  // now run through the images, interpret the headers and read the stars
  for (i = 0; i < Nimages; i++) {
    Nhead = headerSets[i].extnum_head;

    if (VERBOSE) fprintf (stderr, "reading header for %s (%s)\n", headerSets[i].exthead, headerSets[i].extdata);
    if (!ReadImageHeader (headers[Nhead], &images[0][Nvalid], options[0].photcode)) {
      fprintf (stderr, "skipping %s\n", headerSets[i].exthead);
      continue;
    }
    images[0][Nvalid].imageID = Nvalid;

    if (FORCE_SINGLE_TIME && (i > 0)) {
      if (images[0][Nvalid].tzero != images[0][0].tzero) {
	fprintf (stderr, "WARNING: mismatched header times, setting all to PHU value\n");
	images[0][0].tzero = images[0][Nvalid].tzero;
      }
    }

    // XXX EAM : I seemed to have dropped the ability to support TEXT (old-style cmp format files).
    // I need to detect them here and load them with ReadStarsTEXT instead of calling the code
    // below.
    // inStars = ReadStarsFITS (f, headers[Nhead], headers[Ndata], &images[0][Nvalid].nstar);

    // XXX use something to set the chip name? EXTNAME?
    if (!strcmp(headerSets[i].exthead, "PHU") && (Nimages == 1)) {
      snprintf (images[0][Nvalid].name, DVO_IMAGE_NAME_LEN, "%s", file->imagename);
    } else {
      snprintf (images[0][Nvalid].name, DVO_IMAGE_NAME_LEN, "%s[%s]", file->imagename, headerSets[i].exthead);
    }

    // skip the table if there is no data segment (eg, mosaic WRP image)
    if (!strcmp(headerSets[i].extdata, "NONE")) {
      if (!strcmp(headerSets[i].exthead, "PHU") && (Nimages > 1)) {
        // This image is the parent of subsequent images
        parentID = Nvalid;
        images[0][Nvalid].parentID = UINT32_MAX;
      }
      Nvalid++;
      CHECK_REALLOCATE (images[0], Image, NVALID, Nvalid, 10);
      continue;
    }
    images[0][Nvalid].parentID = parentID;

    // advance the pointer to the start of the corresponding table block
    Ndata = headerSets[i].extnum_data;
    Nskip = 0;
    for (j = 0; j < Ndata; j++) {
      Nskip += extsize[j];
    }
    fseeko (f, Nskip, SEEK_SET); 
	 
    // ReadStarsFITS populates catalog->measure,Nmeasure 
    Catalog *newcat = ReadStarsFITS (f, headers[Nhead], headers[Ndata]);
    if (!newcat) continue;

    images[0][Nvalid].nstar = newcat->Nmeasure;

    // XRAD : if we want to read the xrad table, skip to that table here:
    if (headerSets[i].extnum_xrad != -1) {
      int Nxrad = headerSets[i].extnum_xrad;
      Nskip = 0;
      for (j = 0; j < Nxrad; j++) {
	Nskip += extsize[j];
      }
      fseeko (f, Nskip, SEEK_SET); 
      
      if (!ReadXradFITS (f, headers[Nxrad], newcat)) {
	fprintf (stderr, "problem reading the radial flux data for %s\n", headerSets[i].extdata);
      }
    }

    // replace full input catalog newcat with subset version
    newcat = FilterStars (newcat, &images[0][Nvalid], Nvalid, region, options);

    AddstarClientOptions matchOptions = *options;
    matchOptions.radius = 0.4; // tight radius at this stage
    matchOptions.calibrate = FALSE;
    matchOptions.only_match = FALSE;
    matchOptions.nosort = FALSE;
    matchOptions.photcode = 0; // use an invalid photcode to avoid touching secfilt

    SkyRegion matchRegion;
    float dR = region->Rmax - region->Rmin;
    float dD = region->Dmax - region->Dmin;

    // define matchRegion a bit generously
    matchRegion.Rmin = region->Rmin - 0.1*dR;
    matchRegion.Rmax = region->Rmax + 0.1*dR;
    matchRegion.Dmin = region->Dmin - 0.1*dD;
    matchRegion.Dmax = region->Dmax + 0.1*dD;

    find_matches_closest (&matchRegion, newcat, catalog, matchOptions);
    dvo_catalog_free (newcat);
    free (newcat);

    Nvalid++;
    CHECK_REALLOCATE (images[0], Image, NVALID, Nvalid, 10);
  }

  if (isfinite(OFFSET_ZPT)) {
    for (i = 0; i < catalog->Nmeasure; i++) {
      catalog->measure[i].M   += OFFSET_ZPT;
      catalog->measure[i].Map += OFFSET_ZPT;
    }
  }

  *nvalid = Nvalid;
  return catalog;
}

// thoughts:
// 1) I read the data from a single image file into a catalog structure (instead of Stars)
// 2) merge these into a single catalog here or in LoadData above?
