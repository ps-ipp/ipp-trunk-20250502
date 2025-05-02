# include "imregister.h"
# include "detrend.h"

/********* decipher all of the possible command-line options ***********/
int args (int argc, char **argv) {

  int i, N;
  int MosaicSelect, ImageSelect, bad, Recipe, Ncrit;
  char *ImageFile, *ImageMode, *ImageExtend;
  Criteria base, *crit;
  int *filt;
  time_t *tstart, *tstop;

  if ((N = get_argument (argc, argv, "-h"))) { usage (); }
  if ((N = get_argument (argc, argv, "--help"))) { usage (); }
  bzero (&base, sizeof (Criteria));

  /* check config & command-line args */
  ConfigInit (&argc, argv);
  ConfigCamera ();
  ConfigFilter ();

  ImageFile = ImageMode = ImageExtend = NULL;

  /* mosaic-based selection */
  MosaicSelect = FALSE;
  if ((N = get_argument (argc, argv, "-mosaic"))) {
    remove_argument (N, &argc, argv);
    ImageFile = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    MosaicSelect = TRUE;
    /* check for incompatible arguments: -exptime, -filter, -time, -ccd */
    bad = get_argument (argc, argv, "-exptime");
    bad &= get_argument (argc, argv, "-image");
    bad &= get_argument (argc, argv, "-filter");
    bad &= get_argument (argc, argv, "-trange");
    bad &= get_argument (argc, argv, "-time");
    bad &= get_argument (argc, argv, "-ccd");
    if (bad) { 
      fprintf (stderr, "ERROR: syntax error: conflict with -mosaic\n");
      exit (1);
    }
  }

  /* image-based selection */
  ImageSelect = FALSE;
  if ((N = get_argument (argc, argv, "-image"))) {
    remove_argument (N, &argc, argv);
    ImageFile = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    ImageExtend = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    ImageMode = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    ImageSelect = TRUE;
    /* check for incompatible arguments: -exptime, -filter, -time, -ccd */
    bad = get_argument (argc, argv, "-exptime");
    bad &= get_argument (argc, argv, "-filter");
    bad &= get_argument (argc, argv, "-trange");
    bad &= get_argument (argc, argv, "-time");
    bad &= get_argument (argc, argv, "-ccd");
    if (bad) { 
      fprintf (stderr, "ERROR: syntax error: conflict with -image\n");
      exit (1);
    }
  }

  /* define image type */
  base.TypeSelect = FALSE;
  base.Type = T_UNDEF;
  if ((N = get_argument (argc, argv, "-type"))) {
    remove_argument (N, &argc, argv);
    base.Type = get_image_type (argv[N]);
    if (base.Type == T_UNDEF) {
      fprintf (stderr, "ERROR: invalid image type %s\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
    base.TypeSelect = TRUE;
    if (base.Type == T_ANY) base.TypeSelect = FALSE;
  }
 
  /* image mode (mef, splt, etc) */
  base.ModeSelect = FALSE;
  base.Mode = M_UNDEF;
  if ((N = get_argument (argc, argv, "-mode"))) {
    remove_argument (N, &argc, argv);
    base.Mode = get_image_mode (argv[N]);
    if (base.Mode == M_UNDEF) {
      fprintf (stderr, "ERROR: invalid image mode %s\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
    base.ModeSelect = TRUE;
  }
 
  /* define time / ranges */
  tstart = tstop = NULL;
  base.TimeSelect = base.tstart = base.tstop = 0;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    bad  = get_argument (argc, argv, "-trange");
    bad &= ohana_str_to_time (argv[N], &base.tstart);
    if (bad) {
      fprintf (stderr, "ERROR: syntax error in -time\n");
      exit (1);
    }
    base.tstop = base.tstart;  /* no need for dtime > 0 ? */
    remove_argument (N, &argc, argv);
    base.TimeSelect = 1;
  } else {
    if (!get_trange_arguments (&argc, argv, &tstart, &tstop, &base.TimeSelect)) {
      fprintf (stderr, "ERROR: syntax error in -trange\n");
      exit (1);
    }
  }

  /* define filters */
  base.FilterSelect = FALSE;
  if (!get_filter_arguments (&argc, argv, &filt, &base.FilterSelect)) {
    fprintf (stderr, "ERROR: syntax error in -filter\n");
    exit (1);
  }

  /* define ccd number */
  base.CCDSelect = FALSE;
  if ((N = get_argument (argc, argv, "-ccd"))) {
    base.CCD = -1;
    remove_argument (N, &argc, argv);
    for (i = 0; (i < Nccd) && (base.CCD == -1); i++) {
      if (strnumcmp (ccds[i], argv[N])) {
	base.CCD = i;
      }
    }
    if (base.CCD == -1) {
      fprintf (stderr, "ERROR: ccd %s choice out not found in camera config\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
    base.CCDSelect = TRUE;
  }
 
  /* define exposure time */
  base.ExptimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-exptime"))) {
    remove_argument (N, &argc, argv);
    base.Exptime = atof (argv[N]);
    remove_argument (N, &argc, argv);
    base.ExptimeSelect = TRUE;
  }
 
  /* varients on the standard selection */
  base.EntrySelect = FALSE;
  if ((N = get_argument (argc, argv, "-entry"))) {
    remove_argument (N, &argc, argv);
    base.Entry = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    base.EntrySelect = TRUE;
  }
  base.MatchNumber = -1;
  if ((N = get_argument (argc, argv, "-match"))) {
    remove_argument (N, &argc, argv);
    base.MatchNumber = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  base.LabelSelect = FALSE;
  if ((N = get_argument (argc, argv, "-label"))) {
    remove_argument (N, &argc, argv);
    base.Label = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    base.LabelSelect = TRUE;
  }
  base.NameSelect = FALSE;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    base.Name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    base.NameSelect = TRUE;
  }

  /* these options affect the output mode */
  output.Close = FALSE;
  if ((N = get_argument (argc, argv, "-close"))) {
    if (!base.TimeSelect && !ImageSelect && !MosaicSelect) {       
      fprintf (stderr, "ERROR: syntax error in -close requires one of: -time -trange -image -mosaic\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    output.Close = TRUE;
  }
  output.TimeMode = START;
  if ((N = get_argument (argc, argv, "-tstop"))) {
    remove_argument (N, &argc, argv);
    output.TimeMode = STOP;
  }
  if ((N = get_argument (argc, argv, "-treg"))) {
    remove_argument (N, &argc, argv);
    output.TimeMode = REG;
  }
  output.ElixirSmart = FALSE;
  if ((N = get_argument (argc, argv, "-ve"))) {
    remove_argument (N, &argc, argv);
    output.ElixirSmart = TRUE;
  }
  output.verbose = TRUE;
  if ((N = get_argument (argc, argv, "-quiet"))) {
    remove_argument (N, &argc, argv);
    output.verbose = FALSE;
  }
  output.table = (char *) NULL;
  if ((N = get_argument (argc, argv, "-fits"))) {
    remove_argument (N, &argc, argv);
    output.table = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  output.bintable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-binfits"))) {
    remove_argument (N, &argc, argv);
    output.bintable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  output.Criteria = FALSE;
  if ((N = get_argument (argc, argv, "-criteria"))) {
    output.Criteria = TRUE;
    remove_argument (N, &argc, argv);
  }
  output.Chipname = FALSE;
  if ((N = get_argument (argc, argv, "-chipname"))) {
    output.Chipname = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* these options modify behavior */
  output.Select = FALSE;
  if ((N = get_argument (argc, argv, "-select"))) {
    remove_argument (N, &argc, argv);
    output.Select = TRUE;
  }
 
  /* these options modify behavior */
  Recipe = FALSE;
  if ((N = get_argument (argc, argv, "-recipe"))) {
    remove_argument (N, &argc, argv);
    Recipe = TRUE;
    if (base.Type != T_UNDEF) {
      fprintf (stderr, "-recipe and -type cannot be combined\n");
      exit (1);
    }
  }
 
  /* options which alter the database */
  output.Delete = FALSE;
  if ((N = get_argument (argc, argv, "-del"))) {
    remove_argument (N, &argc, argv);
    output.Delete = TRUE;
  }
  if ((N = get_argument (argc, argv, "-delete"))) {
    remove_argument (N, &argc, argv);
    output.Delete = TRUE;
  }
 
  output.Altpath = NONE;
  if ((N = get_argument (argc, argv, "-altpath"))) {
    if (output.Delete) {
      fprintf (stderr, "can't combine -delete and -altpath flags\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    if (!strcasecmp (argv[N], "add")) output.Altpath = ADD;
    if (!strcasecmp (argv[N], "delete")) output.Altpath = DELETE;
    if (!strcasecmp (argv[N], "update")) output.Altpath = UPDATE;
    if (!output.Altpath) { 
      fprintf (stderr, "invalid -altpath option\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
  }

  output.Modify = FALSE;
  if ((N = get_argument (argc, argv, "-modify"))) {
    if (output.Delete) {
      fprintf (stderr, "can't combine -delete and -modify flags\n");
      exit (1);
    }
    if (output.Altpath) {
      fprintf (stderr, "can't combine -altpath and -modify flags\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    output.Modify = TRUE;
    output.ModifyEntry = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    output.ModifyValue = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    if (!strcasecmp (output.ModifyEntry, "label")) goto valid_entry;
    if (!strcasecmp (output.ModifyEntry, "order")) goto valid_entry;
    if (!strcasecmp (output.ModifyEntry, "mode"))  goto valid_entry;
    if (!strcasecmp (output.ModifyEntry, "tstop")) goto valid_entry;
    if (!strcasecmp (output.ModifyEntry, "tstart")) goto valid_entry;
    fprintf (stderr, "invalid entry for -modify %s\n", output.ModifyEntry);
    exit (1);
  valid_entry:
    if (!strcasecmp (output.ModifyEntry, "tstart") || !strcasecmp (output.ModifyEntry, "tstop")) {
      if (!ohana_str_to_time (output.ModifyValue, &output.TimeValue)) { 
	fprintf (stderr, "ERROR: invalid time %s\n", output.ModifyValue);
	exit (1);
      }
    }
  }

  /* consistency checks */
  if ((base.Type == T_DARK) || (base.Type == T_BIAS) || (base.Type == T_MASK)) {
    if (base.FilterSelect) {
      fprintf (stderr, "ERROR: filter invalid with type %s\n", get_type_name(base.Type));
      exit (1);
    }
  }
  /*
  if ((base.Type != T_DARK) && base.ExptimeSelect) {
    fprintf (stderr, "ERROR: exptime invalid with type %s\n", get_type_name(base.Type));
    exit (1);
  }
  */
  if (output.Select && !base.TimeSelect) {
    fprintf (stderr, "ERROR: selection missing time\n");
    exit (1);
  }

  /* arguments have all been read */
  if (argc != 1) {
    fprintf (stderr, "USAGE: detsearch [config ops] [-mosaic name] [-image name Nx Mode] [-time start] [-type type] [-ccd N] [-filter name]\n");
    exit (1);
  }

  /* set up criteria based on image, mosaic, or base (all mutually exclusive) */
  crit = NULL;
  if (ImageSelect)  
    crit = ImageCriteria (base, ImageFile, ImageExtend, ImageMode, &Ncrit);
  if (MosaicSelect) 
    crit = MosaicCriteria (base, ImageFile, &Ncrit);
  if (!MosaicSelect && !ImageSelect)
    crit = ExpandBase (base, &Ncrit, tstart, tstop, filt);

  if (Recipe) {
    crit = ExpandRecipe (crit, &Ncrit);
  }

  Ncriteria = Ncrit;
  criteria = crit;

  return (TRUE);
}

Criteria *ExpandBase (Criteria base, int *ncrit, time_t *tstart, time_t *tstop, int *filt) {

  Criteria *crit;
  int i, j, Nc, Ncrit, Ntimes, Nfilt; 

  /* expand multiple -filter, -trange options */
  Nfilt  = base.FilterSelect;
  Ntimes = base.TimeSelect;
  if (Ntimes == 0) Ntimes = 1;
  if (Nfilt  == 0) Nfilt  = 1;

  Ncrit = Nfilt*Ntimes;
  ALLOCATE (crit, Criteria, Ncrit);

  Nc = 0;
  for (i = 0; i < Nfilt; i++) {
    for (j = 0; j < Ntimes; j++) {
      crit[Nc] = base;
      if (tstart != NULL) {
	crit[Nc].tstart       = tstart[j];
	crit[Nc].tstop        = tstop[j];
      } 
      if (base.FilterSelect) {
	crit[Nc].Filter       = filt[i];
      } 
      Nc ++;
    }
  }
  *ncrit = Ncrit;
  return (crit);
}
