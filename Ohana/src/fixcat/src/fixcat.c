# include "markstar.h"

main (argc, argv)
int argc;
char **argv;
{

  FILE *f;
  int i, Nimages;
  Image *image, *find_images();
  Catalog catalog;
  CatStats catstats;
  struct timeval now, then;  
  
  fprintf (stderr, "this function is not well-defined: review and recode\n");
  exit (1);

  gettimeofday (&then, (void *) NULL);
  ConfigInit (argc, argv);

  VERBOSE = FALSE;
  if ((i = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (i, &argc, argv);
  }
  if (argc < 2) {
    fprintf (stderr, "ERROR: Usage: fixcat (catalog)\n");
    exit (0);
  }

  /* replace with addstar/gcatalog as example */
  gcatalog (argv[1], &catalog);

  gcatstats (&catalog, &catstats);

  image = find_images (&catstats, &Nimages);

  match_images (&catalog, image, Nimages);
    
  find_funnymags (&catalog, image, Nimages);  

  if (VERBOSE) {
    gettimeofday (&now, (void *) NULL);
    fprintf (stderr, "%s: elapsed time = %.2f sec\n", argv[1], 
	     (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec));
  }
  clear_lockfile (); 
  fprintf (stderr, "SUCCESS\n");
}

