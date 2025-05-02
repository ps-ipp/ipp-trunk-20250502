# include "delstar.h"

void delete_times (FITS_DB *db) {

  int found, code;
  off_t i, j, k, n;
  off_t Nimage, Nimlist;
  off_t Nregions, NREGIONS;
  off_t *imlist;
  SkyList *skylist, *skyset;
  SkyTable *sky;
  Image *image;
  Catalog catalog;

  code = (PHOTCODE == NULL) ? -1 : PHOTCODE[0].code;

  /* load sky from correct table */
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  Nregions = 0;
  NREGIONS = 10;
  ALLOCATE (skylist, SkyList, 1);
  ALLOCATE (skylist[0].regions, SkyRegion *, NREGIONS);
  skylist[0].ownElements = FALSE; // free these elements when freeing the list

  image = gfits_table_get_Image (&db[0].ftable, &Nimage, &db[0].scaledValue, &db[0].nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }

  BuildChipMatch (image, Nimage);

  /* find images for time range, delete each image */ 
  imlist = find_images_time (db, START, END, PHOTCODE, &Nimlist);

  /* find all overlapping regions */
  for (n = 0; n < Nimlist; n++) {
    j = imlist[n];
    if (VERBOSE) fprintf (stderr, "finding regions for %s\n", image[j].name);

    skyset = SkyListByImage (sky, -1, &image[j]);

    // tregion = gregion_image (&image[j], &Ntregions);
    for (i = 0; i < skyset[0].Nregions; i++) {
      found = FALSE;
      for (k = 0; (k < skylist[0].Nregions) && !found; k++) {
	found = !strcmp (skylist[0].regions[k][0].name, skyset[0].regions[i][0].name);
      }
      if (found) continue;
      skylist[0].regions[Nregions] = skyset[0].regions[i];
      Nregions ++;
      CHECK_REALLOCATE (skylist[0].regions, SkyRegion *, NREGIONS, Nregions, 10);
    }
    SkyListFree (skyset);
  }

  /* delete from all identified regions */
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

    find_matches (&catalog, code, START, END);
    SetProtect (TRUE);
    if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
    if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
    SetProtect (FALSE);
    dvo_catalog_free (&catalog);
  }

  /* delete the identified images */
  gfits_vtable_from_ftable (&db[0].ftable, &db[0].vtable, imlist, Nimlist);
  dvo_image_update (db, VERBOSE);
  dvo_image_unlock (db);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
