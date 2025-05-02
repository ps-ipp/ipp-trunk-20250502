# include "markstar.h"

main (argc, argv)
int argc;
char **argv;
{

  FILE *f;
  int i, Nstars, Nimage, Nregions, Nmissed;
  Image *image, *find_images();
  Catalog catalog;
  CatStats catstats;
  struct timeval now, then;  
  
  gettimeofday (&then, (void *) NULL);
  ConfigInit (argc, argv);

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

  /* if lockfile exists, program will complain and quit */
  check_lockfile (); 
  check_permissions (argv[1]);

  gcatalog (argv[1], &catalog);

  gcatstats (&catalog, &catstats);

  image = find_images (&catstats, &Nimage);

  match_images (&catalog, image, Nimage);
    
  find_bright_stars (&catalog, &catstats); 

  find_ghosts (&catalog, &catstats, argv[1], image, Nimage);  

  find_trails (&catalog, &catstats);  

  wcatalog (argv[1], &catalog);

  if (VERBOSE) {
    gettimeofday (&now, (void *) NULL);
    fprintf (stderr, "%s: elapsed time = %.2f sec\n", argv[1], 
	     (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec));
  }
  clear_lockfile (); 
  fprintf (stderr, "SUCCESS\n");
}

