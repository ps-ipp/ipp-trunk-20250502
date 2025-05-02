# include "imregister.h" 
# include "spreg.h"

/* criteria struct is global */
int args (int argc, char **argv) {

  double dt;
  int N;

  ConfigInitSpec (&argc, argv); /* load elixir config data */

  /* set timezone (set static in misc.c) */
  if ((N = get_argument (argc, argv, "-tz"))) {
    remove_argument (N, &argc, argv);
    dt = atof (argv[N]);
    set_timezone (dt);
    remove_argument (N, &argc, argv);
  }

  /* define time range */
  criteria.Ntimes = 0;
  if (!get_trange_arguments (&argc, argv, &criteria.tstart, &criteria.tstop, &criteria.Ntimes)) {
    fprintf (stderr, "ERROR: syntax error\n");
    exit (1);
  }

  /* exposure time */
  criteria.ExptimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-etime"))) {
    remove_argument (N, &argc, argv);
    criteria.Exptime = atof (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.ExptimeSelect = TRUE;
  }
 
  /* image type (dark, flat, bias, etc) */
  criteria.StateSelect = FALSE;

 
  /* image mode (split, mef, single) */
  criteria.ModeSelect = FALSE;
  /*
  criteria.Mode = M_UNDEF;
  if ((N = get_argument (argc, argv, "-mode"))) {
    remove_argument (N, &argc, argv);
    criteria.Mode = get_image_mode (argv[N]);
    if (criteria.Mode == M_UNDEF) {
      fprintf (stderr, "ERROR: invalid image mode %s\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
    criteria.ModeSelect = TRUE;
  }
  */

  /* string in image name */
  criteria.FilenameSelect = FALSE;
  if ((N = get_argument (argc, argv, "-filename"))) {
    remove_argument (N, &argc, argv);
    criteria.Filename = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.FilenameSelect = TRUE;
  }

  /* string in image name */
  criteria.ObjectSelect = FALSE;
  if ((N = get_argument (argc, argv, "-object"))) {
    remove_argument (N, &argc, argv);
    criteria.Object = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.ObjectSelect = TRUE;
  }

  /* string in image name */
  criteria.TelescopeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-tel"))) {
    remove_argument (N, &argc, argv);
    criteria.Telescope = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.TelescopeSelect = TRUE;
  }

  /* string in image name */
  criteria.InstrumentSelect = FALSE;
  if ((N = get_argument (argc, argv, "-inst"))) {
    remove_argument (N, &argc, argv);
    criteria.Instrument = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.InstrumentSelect = TRUE;
  }

  /*** command-line options which modify the output list */
  if ((N = get_argument (argc, argv, "-treg"))) {
    remove_argument (N, &argc, argv);
    SetOutputMode ("RegTimeMode");
  }
  output.table = (char *) NULL;
  if ((N = get_argument (argc, argv, "-table"))) {
    remove_argument (N, &argc, argv);
    output.table = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  output.bintable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-bintable"))) {
    remove_argument (N, &argc, argv);
    output.bintable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /*** command-line options which modify behavior (delete, modify, newpath, mef2split split2mef */
  output.delete = FALSE;
  output.modify = FALSE;
  output.modify_path = FALSE; 
  output.unique = FALSE;

  if ((N = get_argument (argc, argv, "-delete"))) {
    remove_argument (N, &argc, argv);
    output.delete = TRUE;
  }

  if ((N = get_argument (argc, argv, "-unique"))) {
    remove_argument (N, &argc, argv);
    output.unique = TRUE;
  }

  if ((N = get_argument (argc, argv, "-modify"))) {
    if (output.delete) { 
      fprintf (stderr, "can't specify more than one modifier at a time\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    output.modify = TRUE;
    
    /* modify part of the path */
    if (!strcasecmp (argv[N], "path")) {
      output.modify_path = TRUE;
      remove_argument (N, &argc, argv);
      output.oldpath = strcreate (argv[N]);
      remove_argument (N, &argc, argv);
      output.newpath = strcreate (argv[N]);
      remove_argument (N, &argc, argv);
      goto valid_modify;
    }

    /* modify distributed status */
    if (!strcasecmp (argv[N], "mode")) {
      output.modify_mode = TRUE;
      remove_argument (N, &argc, argv);
      output.mode = atoi(argv[N]);
      remove_argument (N, &argc, argv);
      goto valid_modify;
    }

    if (!strcasecmp (argv[N], "help")) {
      fprintf (stderr, "-modify option: \n");
      fprintf (stderr, "  -modify path (oldpath) (newpath)\n"); 
      fprintf (stderr, "  -modify mode (mef | split)\n"); 
      exit (2);
    }

    fprintf (stderr, "invalid -modify option, try -modify help\n");
    exit (1);
  }
 valid_modify:

  if (argc != 1) {
    fprintf (stderr, "USAGE: imsearch [config ops] \n");
    fprintf (stderr, "  [-type type] [-mode mode] [-trange start range] [-ccd N]\n");
    fprintf (stderr, "  [-etime exptime] [-filter name] [-name string] [-proc t/f]\n");
    fprintf (stderr, "  [-treg] [-seq] [-pt] [-table] [-cadctable] [-bintable]\n");
    fprintf (stderr, "  [-delete] [-modify (options)]\n");
    exit (1);
  }

  return (TRUE);
}
