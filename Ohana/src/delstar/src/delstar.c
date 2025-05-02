# include "delstar.h"

int main (int argc, char **argv) {

  FITS_DB db;

  int status;
  SetSignals ();
  ConfigInit (&argc, argv);
  args (argc, argv);

  if (!SKIP_IMAGES) {
    set_db (&db);
    gfits_db_init (&db);
    status = dvo_image_lock (&db, ImageCat, 60.0, LCK_XCLD); /* XCLD */
    if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
    if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
    status = dvo_image_load (&db, VERBOSE, FALSE);
    if (!status) Shutdown ("can't read image catalog %s", db.filename);
  }

  switch (MODE) {
    case MODE_DUP_IMAGES:
      if (!delete_duplicate_images (&db)) exit (1);
      exit (0);
      break;
    case MODE_DUP_MEASURES:
      if (!delete_duplicate_measures ()) exit (1);
      delstar_args_free ();
      ohana_memcheck (TRUE);
      ohana_memdump (TRUE);
      exit (0);
      break;
    case MODE_DELETE_MEASURES_BY_MATCH:
      if (!delete_measures_by_match ()) exit (1);
      delstar_args_free ();
      ohana_memcheck (TRUE);
      ohana_memdump (TRUE);
      exit (0);
      break;
    case MODE_DELETE_MEASURES_BY_DETID:
      if (!delete_measures_by_detID ()) exit (1);
      delstar_args_free ();
      ohana_memcheck (TRUE);
      ohana_memdump (TRUE);
      exit (0);
      break;
    case MODE_FIX_LAP: {
	off_t Nimage;
	Image *image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
	if (!image) {
	  fprintf (stderr, "ERROR: failed to read images\n");
	  exit (2);
	}
	dvo_image_unlock (&db); 

	// the imageSubset here is a reduced set of fields, not a reduced set of images
	ImageSubset *subset = ImagesToSubset (image, Nimage);

	if (!delete_fix_LAP (subset, Nimage)) exit (1);
	SummaryImageStats (subset, Nimage);

	exit (0);
	break;
      }
    case MODE_FIX_LAP_EDGES: {
      off_t Nmeasure_edge = 0;
      MeasureEdge *measure_edge = delete_fix_LAP_edges(&Nmeasure_edge);
      if (!measure_edge) {
	fprintf (stderr, "problem loading edge measures\n"); 
	exit (1);
      }
      delete_fix_LAP_edges_find_dups (measure_edge, Nmeasure_edge);
      exit (0);
      break;
    }
    case MODE_FIX_LAP_EDGES_DELETE: {
      if (!delete_fix_LAP_edges_delete()) {
	fprintf (stderr, "problem loading edge measures\n"); 
	exit (1);
      }
      exit (0);
      break;
    }
    case MODE_FIX_LAP_STATS: {
	off_t Nimage;
	Image *image = gfits_table_get_Image (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
	if (!image) {
	  fprintf (stderr, "ERROR: failed to read images\n");
	  exit (2);
	}
	// dvo_image_unlock (&db); 
	fprintf (stderr, "read images\n");

	// the imageSubset here is a reduced set of fields, not a reduced set of images
	ImageSubset *subset = ImagesToSubset (image, Nimage);
	fprintf (stderr, "make subset\n");

	if (!delete_fix_LAP_setstats (subset, Nimage)) exit (1);
	SummaryImageStats_Full (image, Nimage);

	exit (0);
	break;
      }
    case MODE_IMAGEFILE:
      delete_imagefile (&db);
      break;
    case MODE_IMAGENAME:
      delete_imagename (&db);
      break;
    case MODE_TIME:
      delete_times (&db);
      break;
    case MODE_PHOTCODES: {
      if (!SKIP_IMAGES) {
	if (!delete_image_photcodes (&db)) exit (1);
      }

      if (!delete_photcodes ()) {
	fprintf (stderr, "failure deleting measurements\n");
	exit (1);
      }
      exit (0);
    }
    case MODE_ORPHAN:
      fprintf (stderr, "delete orphans not available\n");
      // delete_orphans (argv[1]);
      break;
    case MODE_MISSED:
      fprintf (stderr, "delete missed not available\n");
      // delete_missed (argv[1]);
      break;
    default:
      usage ();
  }
  exit (1);
}
