# include "addstar.h"
# include "supercos.h"

/* This is the DVO program to upload Supercosmos detections into a DVO database.  It is modeled
   on the loadwise program.  Like that case, it is expected to be run only rarely (once?).  The
   supercosmos data are delivered as *.bin files.  The format of the file is described by the
   SuperCosDetection structure.  It does not allow a subset of the sky to be uploaded; entire
   Supercosmos files are loaded if supplied on the command line.

   USAGE: loadsupercos -D CATDIR (catdir) (surveys.csv) (plates.csv) (scfile) [...more files]
 */

int main (int argc, char **argv) {

  int i, status;
  SkyTable *sky;
  AddstarClientOptions options;
  FITS_DB db;

  // need to construct these options with args_loadWISE...
  SetSignals ();
  options = ConfigInit (&argc, argv);
  options = args_loadsupercos (&argc, argv, options);

  // load the full sky description table:
  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  
  // load the images table (needed regardless)
  int Nsurvey = 0;
  Survey *survey = loadsupercos_survey(argv[1], &Nsurvey);

  // load the images table (needed regardless)
  int Nimage = 0;
  Image *image = loadsupercos_plates(survey, Nsurvey, argv[2], &Nimage);

  // attempt to open the existing images table.  if it does not exist, we will save the loaded images
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, ImageCat, 3600.0, LCK_XCLD);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) {
    if (VERBOSE) fprintf (stderr, "can't find %s, creating a new one\n", ImageCat);
    dvo_image_create (&db, GetZeroPoint());
    dvo_image_addrows (&db, image, Nimage);
    SetProtect (TRUE);
    dvo_image_update (&db, VERBOSE);
    SetProtect (FALSE);
    if (VERBOSE) fprintf (stderr, "created image table for Supercosmos\n");
  }

  int *imlist = loadsupercos_image_index (image, Nimage);

  for (i = 3; i < argc; i++) {
      fprintf (stderr, "loading %s\n", argv[i]);
      loadsupercos_rawdata (image, imlist, Nimage, sky, argv[i], options);
  }
  exit (0);
}  

