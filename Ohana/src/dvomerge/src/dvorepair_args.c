# include "dvorepair.h"

/*** check for command line options ***/
int dvorepair_args (int *argc, char **argv) {
  
  int N;

  HOST_ID = 0;
  HOSTDIR = NULL;
  MODE = DVOREPAIR_MODE_NONE;

  if ((N = get_argument (*argc, argv, "-fix-warp-ids" 	    	      ))) { MODE = DVOREPAIR_MODE_FixWarpIDs                ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-fix-stack-ids"	    	      ))) { MODE = DVOREPAIR_MODE_FixStackIDs               ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-fix-cpt"      	    	      ))) { MODE = DVOREPAIR_MODE_FixCPT                    ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-fix-cpt-by-objID"  	      ))) { MODE = DVOREPAIR_MODE_BY_OBJ_ID                 ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-images-vs-measures"	      ))) { MODE = DVOREPAIR_MODE_ImagesVsMeasures          ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-delete-image-list" 	      ))) { MODE = DVOREPAIR_MODE_DeleteImageList           ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-delete-images-by-extern-id"   ))) { MODE = DVOREPAIR_MODE_DeleteImagesByExternID    ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-delete-images-by-extern-id-v2"))) { MODE = DVOREPAIR_MODE_DeleteImagesByExternID_v2 ; remove_argument (N, argc, argv); }
  if ((N = get_argument (*argc, argv, "-fix-images"                   ))) { MODE = DVOREPAIR_MODE_FixImages                 ; remove_argument (N, argc, argv); }

  if (MODE == DVOREPAIR_MODE_NONE) dvorepair_help(0, NULL);

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  PARALLEL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, argc, argv);
  }

  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // relphot will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, argc, argv);
  }
  // this is a test mode : rather than launching the relphot_client jobs remotely, they are 
  // run in serial via 'system'
  PARALLEL_SERIAL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel-serial"))) {
    if (PARALLEL_MANUAL) {
      fprintf (stderr, "ERROR: cannot mix -parallel-manual and -parallel-serial\n");
      exit (1);
    }
    PARALLEL = TRUE; // -parallel-serial implies -parallel
    PARALLEL_SERIAL = TRUE;
    remove_argument (N, argc, argv);
  }

  dvorepair_help (*argc, argv);
  return TRUE;
}

/*** check for command line options ***/
int dvorepair_client_args (int *argc, char **argv, SkyRegion *UserPatch) {
  
  int N;

  MODE = DVOREPAIR_MODE_NONE;

  if ((N = get_argument (*argc, argv, "-delete-images-by-extern-id"   ))) { MODE = DVOREPAIR_MODE_DeleteImagesByExternID    ; remove_argument (N, argc, argv); }

  if (MODE == DVOREPAIR_MODE_NONE) dvorepair_client_help (0, NULL);

  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  HOST_ID = 0;
  if ((N = get_argument (*argc, argv, "-hostID"))) {
    remove_argument (N, argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOST_ID) dvorepair_client_help(0, NULL);

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) dvorepair_client_help(0, NULL);

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? */
  int i;
  int nUserPatch = 0;
  for (i = 0; i < 2; i++) {
    UserPatch[i].Rmin = 0;
    UserPatch[i].Rmax= 360;
    UserPatch[i].Dmin = -90;
    UserPatch[i].Dmax = +90;
    if ((N = get_argument (*argc, argv, "-region"))) {
      remove_argument (N, argc, argv);
      UserPatch[i].Rmin = atof (argv[N]);
      remove_argument (N, argc, argv);
      UserPatch[i].Rmax = atof (argv[N]);
      remove_argument (N, argc, argv);
      UserPatch[i].Dmin = atof (argv[N]);
      remove_argument (N, argc, argv);
      UserPatch[i].Dmax = atof (argv[N]);
      remove_argument (N, argc, argv);
      nUserPatch ++;
    }
  }

  dvorepair_client_help (*argc, argv);
  return nUserPatch;
}

void dvorepair_help (int argc, char **argv) {

  /* check for help request */
  if (!argv) goto show_help;
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  if (argc < 2) goto show_help;
  return;

show_help:

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvorepair -fix-warp-ids (catdir.list) (idfile) - regenerate images.dat warp_skyfile_ids (EXTERN_ID) values\n");
  fprintf (stderr, "  dvorepair -fix-stack-ids (catdir.list) - regenerate images.dat stackID (EXTERN_ID) values\n");
  fprintf (stderr, "  dvorepair -fix-cpt (images) (rootlist) - regenerate cpt & cps files from the cpm files\n");
  fprintf (stderr, "  dvorepair -images-vs-measures (catdir) (Ntol) - find images with too many missing detections\n");
  fprintf (stderr, "  dvorepair -delete-image-list (catdir) (deleteList) - delete a set of images based on image IDs (output from -images-vs-measures)\n");
  fprintf (stderr, "  dvorepair -delete-images-by-extern-id (catdir) (deleteList) - delete a set of images based on externIDs\n");
  fprintf (stderr, "     ** deletes the detections associated with the image\n");
  fprintf (stderr, "     ** deleteList has 9 words per line, 2nd word is the externID\n");
  fprintf (stderr, "  dvorepair -delete-images-by-extern-id-v2 (catdir) (deleteList) - delete a set of images based on extern image IDs\n");
  fprintf (stderr, "     ** does NOT delete the detections associated with the image\n");
  fprintf (stderr, "     ** deleteList has 3 words per line, 2nd word is the externID\n");
  fprintf (stderr, "  dvorepair -fix-images (catdir) (deleteList) - delete a set of images based on image IDs (output from -images-vs-measures)\n");
  fprintf (stderr, "\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

void dvorepair_client_help (int argc, char **argv) {

  /* check for help request */
  if (!argv) goto show_help;
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  if (argc < 2) goto show_help;
  return;

show_help:

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvorepair_client -delete-images-by-extern-id (catdir) (deleteList) - delete a set of images based on externIDs\n");
  fprintf (stderr, "     ** deletes the detections associated with the image\n");
  fprintf (stderr, "     ** deleteList has 9 words per line, 2nd word is the externID\n");
  fprintf (stderr, "\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}
