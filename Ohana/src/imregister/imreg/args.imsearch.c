# include "imregister.h" 
# include "imreg.h"

/* criteria struct is global */
int args (int argc, char **argv) {

  double dt;
  int N, i;

  ConfigInit (&argc, argv); /* load elixir config data */
  ConfigCamera ();          /* load camera information */
  ConfigFilter ();          /* load filter information */

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

  /* image type (dark, flat, bias, etc) */
  criteria.TypeSelect = FALSE;
  criteria.Type = T_UNDEF;
  if ((N = get_argument (argc, argv, "-type"))) {
    remove_argument (N, &argc, argv);
    criteria.Type = get_image_type (argv[N]);
    if (criteria.Type == T_UNDEF) {
      fprintf (stderr, "ERROR: invalid image type %s\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
    criteria.TypeSelect = TRUE;
  }
 
  /* image mode (split, mef, single) */
  criteria.ModeSelect = FALSE;
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

  /* exposure time */
  criteria.ExptimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-etime"))) {
    remove_argument (N, &argc, argv);
    criteria.Exptime = atof (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.ExptimeSelect = TRUE;
  }
 
  /* ccd number */
  criteria.CCDSelect = FALSE;
  if ((N = get_argument (argc, argv, "-ccd"))) {
    remove_argument (N, &argc, argv);
    criteria.CCD = -1;
    for (i = 0; (i < Nccd) && (criteria.CCD == -1); i++) {
      if (strnumcmp (ccds[i], argv[N])) {
	criteria.CCD = i;
      }
    }
    if (criteria.CCD == -1) {
      fprintf (stderr, "ERROR: ccd %s choice not found in camera config\n", argv[N]);
      exit (1);
    }

    remove_argument (N, &argc, argv);
    criteria.CCDSelect = TRUE;
  }
  /* select CCD seq number */
  if ((N = get_argument (argc, argv, "-ccdn"))) {
    remove_argument (N, &argc, argv);
    criteria.CCD = atoi (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.CCDSelect = TRUE;
  }
 
  /* filter name */
  criteria.FilterSelect = FALSE;
  if ((N = get_argument (argc, argv, "-filter"))) {
    remove_argument (N, &argc, argv);
    criteria.Filter = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.FilterSelect = TRUE;
    if (!strcasecmp (criteria.Filter, "X")) {
      criteria.FilterSelect = FALSE;
    }
  }

  /* string in image name */
  criteria.NameSelect = FALSE;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    criteria.Name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    criteria.NameSelect = TRUE;
  }

  /* processed state */
  criteria.ProcSelect = FALSE;
  if ((N = get_argument (argc, argv, "-proc"))) {
    criteria.ProcSelect = TRUE;
    remove_argument (N, &argc, argv);

    criteria.Proc = -1;
    if (!strcasecmp (argv[N], "t")) criteria.Proc = TRUE;
    if (!strcasecmp (argv[N], "f")) criteria.Proc = FALSE;
    remove_argument (N, &argc, argv);
    if (criteria.Proc == -1) {
      fprintf (stderr, "ERROR: -proc (t/f)\n");
      exit (1);
    }
  }

  criteria.DistSelect = FALSE;
  if ((N = get_argument (argc, argv, "-dist"))) {
    criteria.DistSelect = TRUE;
    remove_argument (N, &argc, argv);

    criteria.Dist = -1;
    if (!strcasecmp (argv[N], "t")) criteria.Dist = TRUE;
    if (!strcasecmp (argv[N], "f")) criteria.Dist = FALSE;
    remove_argument (N, &argc, argv);
    if (criteria.Dist == -1) {
      fprintf (stderr, "ERROR: -dist (t/f)\n");
      exit (1);
    }
  }

  /*** command-line options which modify the output list */
  if ((N = get_argument (argc, argv, "-treg"))) {
    remove_argument (N, &argc, argv);
    SetOutputMode ("RegTimeMode");
  }
  if ((N = get_argument (argc, argv, "-seq"))) {
    remove_argument (N, &argc, argv);
    SetOutputMode ("CCDSeq");
  }
  if ((N = get_argument (argc, argv, "-pt"))) {
    remove_argument (N, &argc, argv);
    SetOutputMode ("PTstyle");
  }
  output.table = (char *) NULL;
  if ((N = get_argument (argc, argv, "-table"))) {
    remove_argument (N, &argc, argv);
    output.table = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  output.cadctable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-cadctable"))) {
    remove_argument (N, &argc, argv);
    output.cadctable = strcreate (argv[N]);
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
  output.modify_dist = FALSE;
  output.mef2split = FALSE; 
  output.split2mef = FALSE;
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

    /* modify the image mode (MEF <-> SPLIT) */
    if (!strcasecmp (argv[N], "mode")) {
      remove_argument (N, &argc, argv);
      if (!strcasecmp (argv[N], "mef")) {
	output.mef2split = TRUE;
	remove_argument (N, &argc, argv);
	goto valid_modify;
      }
      if (!strcasecmp (argv[N], "split")) {
	output.split2mef = TRUE;
	remove_argument (N, &argc, argv);
	goto valid_modify;
      }
    }

    /* modify distributed status */
    if (!strcasecmp (argv[N], "dist")) {
      output.modify_dist = TRUE;
      remove_argument (N, &argc, argv);
      if (!strcasecmp (argv[N], "t")) {
	output.dist = TRUE;
	remove_argument (N, &argc, argv);
	goto valid_modify;
      }
      if (!strcasecmp (argv[N], "f")) {
	output.dist = FALSE;
	remove_argument (N, &argc, argv);
	goto valid_modify;
      }
    }

    /* modify the type */
    if (!strcasecmp (argv[N], "type")) {
      output.modify_type = TRUE;
      remove_argument (N, &argc, argv);
      output.type = get_image_type (argv[N]);
      if (output.type == T_UNDEF) {
	fprintf (stderr, "ERROR: invalid image type %s\n", argv[N]);
	exit (1);
      }
      remove_argument (N, &argc, argv);
      goto valid_modify;
    }

    /* modify the filter */
    if (!strcasecmp (argv[N], "filter")) {
      output.modify_filter = TRUE;
      remove_argument (N, &argc, argv);
      output.filter = strcreate (argv[N]);
      remove_argument (N, &argc, argv);
      goto valid_modify;
    }

    if (!strcasecmp (argv[N], "help")) {
      fprintf (stderr, "-modify option: \n");
      fprintf (stderr, "  -modify path (oldpath) (newpath)\n"); 
      fprintf (stderr, "  -modify mode (mef | split)\n"); 
      fprintf (stderr, "  -modify dist (t | f)\n"); 
      fprintf (stderr, "  -modify type (flat, etc)\n"); 
      fprintf (stderr, "  -modify filter (name)\n\n"); 
      fprintf (stderr, "  mode mef : convert mef to split\n");
      fprintf (stderr, "  mode split : convert split to mef\n");
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
