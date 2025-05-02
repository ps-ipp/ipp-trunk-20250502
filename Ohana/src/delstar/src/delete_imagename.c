# include "delstar.h"

void delete_imagename (FITS_DB *db) {

  off_t i, j, k;
  off_t Nimlist, Nimage, Noutimage;
  off_t *imlist;
  double trange;
  time_t start, stop;
  Image *image;
  Image *outimage;
  Catalog catalog;
  SkyList *skylist;
  SkyTable *sky;

  /* load sky from correct table */
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  BuildChipMatch (image, Nimage);

  /* find image in db by name */
  imlist = find_images_name (db, IMAGENAME, &Nimlist);
  if (!Nimlist) Shutdown ("image %s not found in db", IMAGENAME);
  
  for (k = 0; k < Nimlist; k++) {

    j = imlist[k];
    if (VERBOSE) fprintf (stderr, "deleting %s\n", image[j].name);
    
    skylist = SkyListByImage (sky, -1, &image[j]);

    for (i = 0; i < skylist[0].Nregions; i++) {
      if (VERBOSE) fprintf (stderr, "deleting from %s\n", skylist[0].filename[i]);
      dvo_catalog_init (&catalog, TRUE);
      catalog.filename = skylist[0].filename[i];  /* don't free region before catalog! */
      catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
      catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

      // an error exit status here is a significant error
      if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "a")) {
	fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
	exit (2);
      }
      if (!catalog.Naverage_disk) {
	dvo_catalog_unlock (&catalog);
	dvo_catalog_free (&catalog);
	continue;
      }

      /* trate is in 0.1 msec / row  - stop is the latest exposure end time */
      /* pad exposure time by 1 sec to require a valid time */
      trange = 1e-4*image[j].NY*image[j].trate + image[j].exptime + 1;  
      start = image[j].tzero;
      stop  = image[j].tzero + trange;
      find_matches (&catalog, image[j].photcode, start, stop);

      SetProtect (TRUE);
      dvo_catalog_save_complete (&catalog, VERBOSE);
      dvo_catalog_unlock (&catalog);
      SetProtect (FALSE);
      dvo_catalog_free (&catalog);
    }
  }

  /* delete the identified images */
  ALLOCATE (outimage, Image, Nimage - Nimlist);
  for (i = 0, k = 0; i < Nimage; i++) {
      for (j = 0; j < Nimlist; j++) {
	  if (imlist[j] == i) goto skip;
      }
      outimage[k] = image[i];
      k++;
  skip:
      continue;
  }
  free (image);
  Noutimage = Nimage - Nimlist;
  
  if (VERBOSE) fprintf (stderr, "removing "OFF_T_FMT" images (leaving "OFF_T_FMT" of "OFF_T_FMT")\n",  Nimlist,  Noutimage,  Nimage);
  // gfits_table_set_Image (&db[0].ftable, outimage, Noutimage, TRUE);

  gfits_modify (&db[0].theader, "NAXIS2", OFF_T_FMT, 1,  Noutimage);
  gfits_modify (&db[0].header, "NIMAGES", OFF_T_FMT, 1,  Noutimage);
  db[0].theader.Naxis[1] = Noutimage;
  db[0].ftable.buffer = (char *) outimage;

  dvo_image_save (db, VERBOSE);
  dvo_image_unlock (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
