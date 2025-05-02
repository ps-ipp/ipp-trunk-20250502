# include "setphot.h"

int main (int argc, char **argv) {

  off_t Nimage;
  int status, Nzpts;
  FITS_DB db;
  ZptTable *zpts;

  CamPhotomCorrection *camcorr = NULL;
  FlatCorrectionTable flatcorrTable;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_setphot (argc, argv);

  set_db (&db);
  gfits_db_init (&db);
  status = dvo_image_lock (&db, ImageCat, 60.0, (UPDATE ? LCK_XCLD : LCK_SOFT));
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
  if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);

  fprintf (stderr, "this program needs to be fixed to allow a zpt lookup table\n");
  exit (3);

  if (UBERCAL) {
    zpts = load_zpt_ubercal (argv[1], &Nzpts, &flatcorrTable);
    if (!zpts) Shutdown ("failed to load zero points from ubercal table");

    char flatcorrfile[DVO_MAX_PATH];
    int size = snprintf (flatcorrfile, DVO_MAX_PATH, "%s/flatcorr.fits", CATDIR);
    assert (size < DVO_MAX_PATH);
    FlatCorrectionSave(&flatcorrTable, flatcorrfile);
    // XXX should this program save any old copy of this file?

    CamPhotomCorrection *rawcorr = CamPhotomCorrectionLoad (CAM_PHOTOM_FILE);
    if (!rawcorr) {
      fprintf (stderr, "failed to load camera-static flat-field correction\n");
      exit (2);
    }
    camcorr = merge_flatcorr_and_camcorr (&flatcorrTable, rawcorr);
    
    char newflatfile[DVO_MAX_PATH];
    size = snprintf (newflatfile, DVO_MAX_PATH, "%s/flatfield.fits", CATDIR);
    assert (size < DVO_MAX_PATH);
    CamPhotomCorrectionSave (camcorr, newflatfile);
  } else {
    //    zpts = load_zpt_table (argv[1], &Nzpts);
  }

  if ((CAM_PHOTOM_FILE)&&(!camcorr)) {
    CamPhotomCorrection *rawcorr = CamPhotomCorrectionLoad(CAM_PHOTOM_FILE);
    if (!rawcorr) {
      fprintf (stderr, "failed to load camera-static flat-field correction\n");
      exit (2);
    }
    camcorr = rawcorr;
    char newflatfile[DVO_MAX_PATH];
    int size = snprintf (newflatfile, DVO_MAX_PATH, "%s/flatfield.fits", CATDIR);
    assert (size < DVO_MAX_PATH);
    CamPhotomCorrectionSave (camcorr, newflatfile);

    //    zpts = load_zpt_table (argv[1], &Nzpts);
  }    

  
  //  if (!zpts) Shutdown ("failed to load zero points, or empty table");

  // load images 
  Image *image  = load_images_setphot (&db, &Nimage);
  if (!UPDATE) dvo_image_unlock (&db); 
  
  //  match_zpts_to_images (image, Nimage, zpts, Nzpts);

  //  if (UBERCAL) {
    // we are going to deprecate the flatcorr imaage lookup
    // match_flatcorr_to_images (image, Nimage, &flatcorrTable);
  match_camcorr_to_images (image, Nimage, camcorr);
    //  } 

  if (!IMAGES_ONLY) {
    status = update_dvo_setphot (image, Nimage, camcorr);
  }

  // write image table (even if some remote clients failed)
  if (UPDATE) {
    SetProtect (TRUE);
    dvo_image_save (&db, VERBOSE);
    SetProtect (FALSE);
  }

  if (!status) exit (1);
  exit (0);
}
  

/* setphot : set the zero points for images in the db (perhaps based on external information)
   setphot (zpt_table)

   setphot has 2 modes : basic & ubercal

   * outline of basic mode:

   ** load text table of zpts, time (photcode?)
   ** load images
   ** match images to zpts
   ** set zpts
   ** load catalogs
   ** update detection (& averages?)

   * outline of ubercal mode:

   ** load fits table of zpts & time + table of flat-field corrections
   ** load images
   ** match images to zpts
   ** set zpts
   ** load catalogs
   ** update detection (& averages?)

 */

