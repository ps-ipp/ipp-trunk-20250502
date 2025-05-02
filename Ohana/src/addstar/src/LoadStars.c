# include "addstar.h"

Catalog *LoadStars (char *filename, Image **images, off_t *Nimages, AddstarClientOptions *options) {

  int i, Nfile, mode;

  AddstarFile *file = LoadFilenames (&Nfile, filename, options);

  *Nimages = 0;
  *images = NULL;

  Catalog *catalog = NULL;
  ALLOCATE (catalog, Catalog, 1);
  dvo_catalog_init (catalog, TRUE);
  ALLOCATE (catalog->average, Average, 1);
  ALLOCATE (catalog->measure, Measure, 1);
  ALLOCATE (catalog->lensing, Lensing, 1);

  SkyRegion region;
  region.Rmin =  720.0;
  region.Rmax = -720.0;
  region.Dmin =  180.0;
  region.Dmax = -180.0;

  for (i = 0; i < Nfile; i++) {
    FILE *f = fopen (file[i].filename, "r");
    if (f == NULL) {
      fprintf (stderr, "can't read file %s, skipping\n", file[i].filename);
      continue;
    }

    int Nheaders = 0;
    Header **headers = NULL;
    off_t NheaderSets = 0;
    HeaderSet *headerSets = NULL;

    off_t NimagesStart = *Nimages;
    Catalog *newcat = NULL;
    off_t *extsize = NULL;

    // load PMM data if specified (these are not stored as FITS-tables)
    if (PMM_CCD_TABLE != NULL) {
      newcat = LoadDataPMM (f, file[i].imagename, images, Nimages);
      goto next_file;
    }

    // otherwise, we have FITS-table files: parse their headers to determine the contents
    headers = LoadHeaders (f, &mode, &Nheaders);
    if (Nheaders == 0) {
      fprintf (stderr, "ERROR: no FITS headers in %s, is it a FITS file?\n", file[i].filename);
      goto next_file;
    }

    headerSets = MatchHeaders (&extsize, &NheaderSets, mode, headers, Nheaders);
    if (headerSets == NULL) {
      fprintf (stderr, "ERROR: can't read headers for %s\n", file[i].filename);
      goto next_file;
    }
    if (NheaderSets == 0) {
      fprintf (stderr, "no object data in file %s, skipping\n", file[i].filename);
      goto next_file;
    }
    if (VERBOSE) fprintf (stderr, "file %s has %d headers, including "OFF_T_FMT" images\n", file[i].filename, Nheaders,  NheaderSets);

    /* supplied photcode is incompatible with multi-chip images */
    if ((NheaderSets > 1) && options[0].photcode) {
      fprintf (stderr, "ERROR: photcode cannot be supplied to multi-chip images -- manually adjust the headers\n");
      exit (1);
    }
    /* supplied photcode is incompatible with multi-chip images */
    if ((NheaderSets > 1) && options[0].mosaic) {
      fprintf (stderr, "ERROR: -mosaic cannot be supplied to multiple images\n");
      exit (1);
    }

    // if these are SDSS data, load with SDSS-specific wrapper
    if (headerSets[0].exttype && !strcmp (headerSets[0].exttype, "SDSS_OBJ")) {
      newcat = LoadDataSDSS (f, file[i].imagename, images, Nimages, headers, extsize, headerSets, NheaderSets);
      goto next_file;
    }

    // if these are SDSS data, load with SDSS-specific wrapper
    if (headerSets[0].exttype && !strcmp (headerSets[0].exttype, "UKIRT_OBJ")) {
      newcat = LoadDataUKIRT (f, file[i].imagename, images, Nimages, headers, extsize, headerSets, NheaderSets, &region);
      goto next_file;
    }

    newcat = LoadData (f, &file[i], images, Nimages, headers, extsize, headerSets, NheaderSets, &region, options);

  next_file:

    // if we added data from an image, then we can merge it in
    if (*Nimages > NimagesStart) {
      AddstarClientOptions matchOptions = *options;
      matchOptions.radius = 0.4; // tight radius at this stage
      matchOptions.calibrate = FALSE;
      matchOptions.only_match = FALSE;
      matchOptions.nosort = FALSE;
      matchOptions.photcode = 0; // use an invalid photcode to avoid touching secfilt
      
      SkyRegion matchRegion;
      float dR = region.Rmax - region.Rmin;
      float dD = region.Dmax - region.Dmin;

      // define matchRegion a bit generously
      matchRegion.Rmin = region.Rmin - 0.1*dR;
      matchRegion.Rmax = region.Rmax + 0.1*dR;
      matchRegion.Dmin = region.Dmin - 0.1*dD;
      matchRegion.Dmax = region.Dmax + 0.1*dD;

      find_matches_closest (&matchRegion, newcat, catalog, matchOptions);
    }

    if (newcat) {
      dvo_catalog_free (newcat);
      free (newcat);
    }

    HeaderSetFree (headerSets, NheaderSets);
    int j;
    for (j = 0; j < Nheaders; j++) {
      gfits_free_header (headers[j]);
      FREE (headers[j]);
    }
    FREE (headers);
    FREE (extsize);

    fclose (f);
  }

  // only keep the even numbered images
  if (DIFF_WITH_INV) {
    if (*Nimages % 2) {
      fprintf (stderr, "-diff-inv only makes sense if an even number of images are supplied, non-inv first\n");
      exit (3);
    }

    // detections from the odd images get bumped by the number of stars:
    for (i = 0; i < catalog->Nmeasure; i++) {
      int imageID = catalog->measure[i].imageID;
      if (imageID % 2) {
	catalog->measure[i].detID += images[0][imageID-1].nstar;
	catalog->measure[i].extID += images[0][imageID-1].nstar; // the PSPS ID uses the detID in definition
      }
      catalog->measure[i].imageID = imageID / 2;
    }
    for (i = 0; i < catalog->Nlensing; i++) {
      int imageID = catalog->lensing[i].imageID;
      if (imageID % 2) {
	catalog->lensing[i].detID += images[0][imageID-1].nstar;
      }
      catalog->lensing[i].imageID = imageID / 2;
    }

    int NoutImages = *Nimages / 2;
    Image *outImages = NULL;
    ALLOCATE (outImages, Image, NoutImages);
    for (i = 0; i < NoutImages; i++) {
      outImages[i] = images[0][2*i];
      outImages[i].imageID = i;
    }
    free (*images);

    *Nimages = NoutImages;
    *images = outImages;
  }

  if (*Nimages == 0) {
    if (Nfile == 1) 
      fprintf (stderr, "no valid image data in any of these files, giving up\n");
    else 
      fprintf (stderr, "no valid image data in this file, giving up\n");
    exit (0);
  }

  AddstarFileFree (file, Nfile);
  return catalog;
}

