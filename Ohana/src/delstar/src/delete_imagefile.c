# include "delstar.h"

void delete_imagefile (FITS_DB *db) {

  off_t i, Nimlist;
  off_t *imlist;
  double trange;
  e_time start, stop;
  Image *image;
  Catalog catalog;
  SkyTable *sky;

  /* load sky from correct table */
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  /* load information about file - time/photcode */
  image = gimages (IMAGENAME);
  
  /* need to define method to get the mosaic (look up from table) */
  if (VERBOSE) fprintf (stderr, "deleting %s\n", image[0].name);

  for (i = 0; i < sky[0].Nregions; i++) {

    if (VERBOSE) fprintf (stderr, "deleting from %s\n", sky[0].filename[i]);

    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = sky[0].filename[i];  /* don't free region before catalog! */
    catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, &sky[0].regions[i], VERBOSE, "a")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    if (!catalog.Naverage_disk) {
      dvo_catalog_unlock (&catalog);
      dvo_catalog_free (&catalog);
      continue;
    }

    /* pad exposure time by 1 sec to require a valid time */
    /* trate is in 0.1 msec / row  - stop is the latest exposure end time */
    trange = 1e-4*image[0].NY*image[0].trate + image[0].exptime + 1;  
    start = image[0].tzero;
    stop  = image[0].tzero + trange;
    find_matches (&catalog, image[0].photcode, start, stop);

    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }

  /* find and delete matching images */
  // XXX EAM : load image data above, find mosaic?
  imlist = find_images_data (db, image, &Nimlist);
  if (!Nimlist) Shutdown ("image %s not found in db", IMAGENAME);

  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, imlist, Nimlist);
  dvo_image_update (db, VERBOSE);
  dvo_image_unlock (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
