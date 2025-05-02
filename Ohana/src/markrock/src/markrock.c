# include "markrock.h"

main (int argc, char **argv) {
  
  int i, Nrocks1, Nrocks2;
  Catalog catalog;
  CatStats catstats;
  Rocks *rocks1, *rocks2, *find_rocks(), *find_slow_rocks();
  struct timeval now, then;  
  
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

  /* if lockfile exists, program will complain and quit */
  if (!check_file_access (argv[1], TRUE, TRUE, TRUE)) exit (1);
  if (!check_file_access (RockCat, TRUE, TRUE, TRUE)) exit (1);

  dvo_catalog_init (&catalog, TRUE);
  catalog.filename = argv[1];
  catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
  catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_MISSING | DVO_LOAD_SECFILT;

  // an error exit status here is a significant error
  if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "a")) {
    fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
    exit (2);
  }
  if (catalog.Naverage_disk) {
    dvo_catalog_unlock (&catalog);
    dvo_catalog_free (&catalog);
    continue;
  }

  gcatstats (&catalog, &catstats);

  find_bright_stars (&catalog, &catstats); 

  rocks1 = find_rocks (&catalog, &catstats, &Nrocks1); 
  count_neighbors (rocks1, Nrocks1, &catalog, &catstats);

  /* rocks2 = find_slow_rocks (&catalog, &catstats, &Nrocks2);  */
  SetProtect (TRUE);
  if (!dvo_catalog_save (&catalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save %s\n", catalog.filename); exit (1); }
  if (!dvo_catalog_unlock (&catalog)) { fprintf (stderr, "ERROR: failed to unlock %s\n", catalog.filename); exit (1); }
  SetProtect (FALSE);
    
  dvo_catalog_free (&catalog);

  wrocks (rocks1, Nrocks1);
  /* wrocks (rocks2, Nrocks2); */

  if (VERBOSE) {
    gettimeofday (&now, (void *) NULL);
    fprintf (stderr, "%s: elapsed time = %.2f sec\n", argv[1], 
	     (now.tv_sec - then.tv_sec) + 1e-6*(now.tv_usec - then.tv_usec));
  }

  fprintf (stderr, "SUCCESS\n");
}

