# include "markstar.h"

/*** this needs to be cleaned to match current db I/O model ***/

int main (int argc, char **argv) {

  FILE *f;
  int i, Nstars, Nimage, Nregions, Nmissed;
  Image *image, *find_images();
  Catalog catalog;
  CatStats catstats;
  struct timeval now, then;  
  FITS_DB db;
  
  gettimeofday (&then, (void *) NULL);
  ConfigInit (&argc, argv);

  VERBOSE = FALSE;
  if ((i = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (i, &argc, argv);
  }
  FORCE_RUN = FALSE;
  if ((i = get_argument (argc, argv, "-f"))) {
    FORCE_RUN = TRUE;
    remove_argument (i, &argc, argv);
  }
  RESET = FALSE;
  if ((i = get_argument (argc, argv, "-reset"))) {
    RESET = TRUE;
    remove_argument (i, &argc, argv);
  }
  if (argc < 2) {
    fprintf (stderr, "ERROR: Usage: markstar (catalog)\n");
    exit (0);
  }

  set_db (&db);
  gfits_db_init (&db);
  dvo_image_lock (&db, ImageCat, 3600.0, LCK_XCLD);

  dvo_catalog_init (&catalog, TRUE);
  catalog.filename = argv[1];
  catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, NULL, VERBOSE, "a")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
    exit (2);
  }
  if (!catalog.Naverage_disk) {
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
    exit (0);
  }

  gcatstats (&catalog, &catstats);

  image = find_images (&catstats, &Nimage);

  match_images (&catalog, image, Nimage);
    
  find_trails (&catalog, &catstats, i);  

  /* find_bright_stars (&catalog, &catstats);  */

  /* find_ghosts (&catalog, &catstats, argv[1], image, Nimage); */

  SetProtect (TRUE);
  if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
  if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
  SetProtect (FALSE);

  dvo_catalog_free (&catalog);

  if (VERBOSE) {
    gettimeofday (&now, (void *) NULL);
    fprintf (stderr, "%s: elapsed time = %.2f sec\n", argv[1], 
	     (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec));
  }
  fprintf (stderr, "SUCCESS\n");
}

