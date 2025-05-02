# include "checkastro.h"
void usage (void);
void usage_client (void);
void usage_merge_source (void);
void usage_merge_source_id (char *name);

int args (int argc, char **argv) {

  int N;
  double trange;

  /* specify portion of the sky : allow default of all sky? */
  UserPatch.Rmin = 0;
  UserPatch.Rmax = 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Rmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
  } 
  if ((N = get_argument (argc, argv, "-catalog"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    UserPatch.Rmax = UserPatch.Rmin + 0.001;
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    UserPatch.Dmax = UserPatch.Dmin + 0.001;
    remove_argument (N, &argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // checkastro will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the checkastro_client jobs remotely, they are 
  // run in serial via 'system'
  PARALLEL_SERIAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-serial"))) {
    if (PARALLEL_MANUAL) {
      fprintf (stderr, "ERROR: cannot mix -parallel-manual and -parallel-serial\n");
      exit (1);
    }
    PARALLEL = TRUE; // -parallel-serial implies -parallel
    PARALLEL_SERIAL = TRUE;
    remove_argument (N, &argc, argv);
  }

  MaxDensityUse = FALSE;
  if ((N = get_argument (argc, argv, "-max-density"))) {
    remove_argument (N, &argc, argv);
    MaxDensityValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
    MaxDensityUse = TRUE;
  }

  /* define time */
  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &TSTART)) { 
      fprintf (stderr, "ERROR: syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      if (!ohana_str_to_time (argv[N], &TSTOP)) { 
	fprintf (stderr, "ERROR: syntax error\n");
	return (FALSE);
      }
    } else {
      if (trange < 0) {
	trange = fabs (trange);
	TSTOP = TSTART;
	TSTART -= trange;
      } else {
	TSTOP = TSTART + trange;
      }
    }
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;
  }

  PHOTCODE_RESET_LIST = NULL;
  if ((N = get_argument (argc, argv, "-reset-to-photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_RESET_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_KEEP_LIST = NULL;
  if ((N = get_argument (argc, argv, "+photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_KEEP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_SKIP_LIST = strcreate("SCOS.103a.E,SCOS.4414.OG590,SCOS.4415.OG590,SCOS.IIIaF.OG590,SCOS.IIIaF.RG610,SCOS.IIIaF.RG630,SCOS.IIIaJ.GG385,SCOS.IIIaJ.GG395,SCOS.IVN.RG715,SCOS.IVN.RG9");
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    char *tmp1 = strcreate(argv[N]);

    int Ntotal = strlen(tmp1) + strlen(PHOTCODE_SKIP_LIST) + 5;

    char *tmp2 = NULL;
    ALLOCATE (tmp2, char, Ntotal);
    snprintf (tmp2, Ntotal, "%s,%s", PHOTCODE_SKIP_LIST, tmp1);

    free (tmp1);
    free (PHOTCODE_SKIP_LIST);

    PHOTCODE_SKIP_LIST = tmp2;
    remove_argument (N, &argc, argv);
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE = VERBOSE2 = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* XXX update this */
  MIN_ERROR = 0.001;
  if ((N = get_argument (argc, argv, "-minerror"))) {
    remove_argument (N, &argc, argv);
    MIN_ERROR = atof (argv[N]);
    remove_argument (N, &argc, argv);
    /* require MIN_ERROR > 0 */
  }  

  ImagSelect = FALSE;
  if ((N = get_argument (argc, argv, "-instmag"))) {
    remove_argument (N, &argc, argv);
    ImagMin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagMax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagSelect = TRUE;
  }
  
  // for now, make the default to ignore the photflags
  // XXX make it true by default instead?
  PhotFlagSelect = FALSE;
  if ((N = get_argument (argc, argv, "+photflags"))) {
    remove_argument (N, &argc, argv);
    PhotFlagSelect = TRUE;
  }

  PhotFlagBad = 0;
  if ((N = get_argument (argc, argv, "-photflagbad"))) {
    remove_argument (N, &argc, argv);
    PhotFlagBad = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PhotFlagPoor = 0;
  if ((N = get_argument (argc, argv, "-photflagpoor"))) {
    remove_argument (N, &argc, argv);
    PhotFlagPoor = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MinBadQF = 0.0;
  if ((N = get_argument (argc, argv, "-min-bad-psfqf"))) {
    remove_argument (N, &argc, argv);
    MinBadQF = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) usage ();
  return TRUE;
}

int args_client (int argc, char **argv) {

  int N;
  double trange;

  /* possible operations */
  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  BCATALOG = NULL;

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) usage_client();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) usage_client();

  if ((N = get_argument (argc, argv, "-load-objects"))) {
    remove_argument (N, &argc, argv);
    BCATALOG = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* specify portion of the sky : allow default of all sky? */
  UserPatch.Rmin = 0;
  UserPatch.Rmax = 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Rmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
  } 
  if ((N = get_argument (argc, argv, "-catalog"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    UserPatch.Rmax = UserPatch.Rmin + 0.001;
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    UserPatch.Dmax = UserPatch.Dmin + 0.001;
    remove_argument (N, &argc, argv);
  }

  MaxDensityUse = FALSE;
  if ((N = get_argument (argc, argv, "-max-density"))) {
    remove_argument (N, &argc, argv);
    MaxDensityValue = atof(argv[N]);
    remove_argument (N, &argc, argv);
    MaxDensityUse = TRUE;
  }

  /* define time */
  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &TSTART)) { 
      fprintf (stderr, "ERROR: syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_dtime (argv[N], &trange)) { 
      if (!ohana_str_to_time (argv[N], &TSTOP)) { 
	fprintf (stderr, "ERROR: syntax error\n");
	return (FALSE);
      }
    } else {
      if (trange < 0) {
	trange = fabs (trange);
	TSTOP = TSTART;
	TSTART -= trange;
      } else {
	TSTOP = TSTART + trange;
      }
    }
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;
  }

  PHOTCODE_RESET_LIST = NULL;
  if ((N = get_argument (argc, argv, "-reset-to-photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_RESET_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_KEEP_LIST = NULL;
  if ((N = get_argument (argc, argv, "+photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_KEEP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_SKIP_LIST = NULL;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_SKIP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE = VERBOSE2 = TRUE;
    remove_argument (N, &argc, argv);
  }

  ImagSelect = FALSE;
  if ((N = get_argument (argc, argv, "-instmag"))) {
    remove_argument (N, &argc, argv);
    ImagMin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagMax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    ImagSelect = TRUE;
  }
  
  // for now, make the default to ignore the photflags
  // XXX make it true by default instead?
  PhotFlagSelect = FALSE;
  if ((N = get_argument (argc, argv, "+photflags"))) {
    remove_argument (N, &argc, argv);
    PhotFlagSelect = TRUE;
  }

  PhotFlagBad = 0;
  if ((N = get_argument (argc, argv, "-photflagbad"))) {
    remove_argument (N, &argc, argv);
    PhotFlagBad = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  PhotFlagPoor = 0;
  if ((N = get_argument (argc, argv, "-photflagpoor"))) {
    remove_argument (N, &argc, argv);
    PhotFlagPoor = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MinBadQF = 0.0;
  if ((N = get_argument (argc, argv, "-min-bad-psfqf"))) {
    remove_argument (N, &argc, argv);
    MinBadQF = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) usage_client ();
  return TRUE;
}

void usage () {
  fprintf (stderr, "ERROR: USAGE: checkastro [options]\n");
  fprintf (stderr, " options: \n");
  fprintf (stderr, "  -region RA RA DEC DEC\n");
  fprintf (stderr, "  -catalog (ra) (dec)\n\n");
  fprintf (stderr, "  -time (start)(stop)\n");
  fprintf (stderr, "  +photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -reset-to-photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -max-density (rho)\n");
  fprintf (stderr, "  -minerror\n");
  fprintf (stderr, "  -minerror\n");
  fprintf (stderr, "  -instmag min max\n\n");
  fprintf (stderr, "  +photflags\n");
  fprintf (stderr, "  -photflagbad\n");
  fprintf (stderr, "  -photflagpoor\n");
  fprintf (stderr, "  -v\n");
  fprintf (stderr, "  -vv\n");
  fprintf (stderr, "  -parallel\n");
  fprintf (stderr, "  -parallel-manual\n");
  fprintf (stderr, "  -parallel-serial\n");
  fprintf (stderr, "  \n");
  exit (2);
} 

void usage_client () {
  fprintf (stderr, "ERROR: USAGE: checkastro_client -load\n");
  fprintf (stderr, "       OR:    checkastro_client -update-offsets\n");
  fprintf (stderr, "       OR:    checkastro_client -update-objects\n");
  fprintf (stderr, "       OR:    checkastro_client -high-speed\n\n");

  fprintf (stderr, "  specify one of the following modes: \n");
  fprintf (stderr, "  -load : load the bright source detections for analysis\n");
  fprintf (stderr, "  -update-offsets : apply the updated image parameters\n");
  fprintf (stderr, "  -update-objects : calculate average astrometric properties of objects\n");
  fprintf (stderr, "  -high-speed (code[,code,code]) (code[,code,code]) (radius) (output catdir)\n");
  fprintf (stderr, "    support for the special high-speed mode\n");

  fprintf (stderr, " additional options: \n");
  fprintf (stderr, "  -pm : calculate proper motions\n");
  fprintf (stderr, "  -par : calculate parallaxes\n");
  fprintf (stderr, "  -pmpar : calculate motions and parallaxes\n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax");
  fprintf (stderr, "  -catalog RA DEC");
  fprintf (stderr, "  -time (start)(stop)\n");
  fprintf (stderr, "  +photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -photcode (code)[,code,code...]\n");
  fprintf (stderr, "  -statmode (mode)\n");
  fprintf (stderr, "  -reset\n");
  fprintf (stderr, "  -minerror\n");
  fprintf (stderr, "  -instmag min max\n\n");
  fprintf (stderr, "  +photflags\n");
  fprintf (stderr, "  -photflagbad\n");
  fprintf (stderr, "  -photflagpoor\n");
  fprintf (stderr, "  -v\n");
  fprintf (stderr, "  \n");
  exit (2);
} 

void usage_merge_source_id (char *name) {

  fprintf (stderr, "ERROR: invalid ID %s (remember to prefix 0x to hex IDs)\n", name);
  exit (2);
}

void usage_merge_source () {
  fprintf (stderr, "ERROR: USAGE: checkastro -merge-source (objID) (catID) into (objID) (catID)\n");
  exit (2);
}
