# include "delstar.h"

// delstar_client is run on a remote host and is responsible for deleting measures from the
// catalogs owned by that host.

int main (int argc, char **argv) {

  char imageFile[512];

  SetSignals ();
  ConfigInit (&argc, argv);
  args_client (argc, argv);

  switch (MODE) {
    case MODE_DUP_IMAGES:
      // read the subset table of image information
      snprintf (imageFile, 512, "%s/ImageIDs.tmp.fits", CATDIR);
      
      IndexArray *imageID = ImageIDLoad (imageFile);
      if (!imageID) {
	fprintf (stderr, "failed to read image ID table\n");
	exit (1);
      }
      if (!delete_duplicate_image_measures (imageID)) exit (1);
      delstar_client_args_free ();
      ohana_memcheck (TRUE);
      ohana_memdump (TRUE);
      exit (0);
      break;
    case MODE_DUP_MEASURES:
      if (!delete_duplicate_measures ()) exit (1);
      delstar_client_args_free ();
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
      ImageSubset *image = ImageSubsetLoad (IMAGES, &Nimage);
      if (!image) {
	fprintf (stderr, "failed to read image table\n");
	exit (1);
      }

      if (!delete_fix_LAP (image, Nimage)) exit (1);
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
      if (!MeasureEdgeSave (MEASURE_EDGE_FILE, measure_edge, Nmeasure_edge)) {
	fprintf (stderr, "problem saving edge measures\n"); 
	exit (1);
      }
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

    case MODE_IMAGEFILE:
      break;
    case MODE_IMAGENAME:
      break;
    case MODE_TIME:
      break;
    case MODE_PHOTCODES:
      if (!delete_photcodes ()) {
	fprintf (stderr, "failure deleting measurements from %s\n", HOSTDIR);
	exit (1);
      }
      exit (0);
    case MODE_ORPHAN:
      break;
    case MODE_MISSED:
      break;
    default:
      usage ();
  }
  fprintf (stderr, "this mode is not supported by delstar_client\n");
  exit (1);
}
