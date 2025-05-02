# include "imregister.h" 
# include "detrend.h"

/* criteria struct is global */
int regargs (int argc, char **argv, Descriptor *descriptor) {

  int N, i;

  ConfigInit (&argc, argv);
  ConfigCamera ();
  ConfigFilter ();

  output.Modify = TRUE;

  /* these command line arguments will override image header info */
  /* define time range */
  descriptor[0].TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &descriptor[0].tstart)) { 
      fprintf (stderr, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &descriptor[0].tstop)) { 
      fprintf (stderr, "syntax error\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    descriptor[0].TimeSelect = TRUE;
  }

  /* set optional label */
  descriptor[0].label = strcreate ("detrend");
  if ((N = get_argument (argc, argv, "-label"))) {
    remove_argument (N, &argc, argv);
    descriptor[0].label = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
 
  /* set optional label */
  descriptor[0].imageID = NULL;
  if ((N = get_argument (argc, argv, "-ID"))) {
    remove_argument (N, &argc, argv);
    descriptor[0].imageID = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
 
  /* set optional order */
  descriptor[0].order = 0;
  if ((N = get_argument (argc, argv, "-order"))) {
    remove_argument (N, &argc, argv);
    descriptor[0].order = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
 
  /* define image type */
  descriptor[0].type = T_UNDEF;
  if ((N = get_argument (argc, argv, "-type"))) {
    remove_argument (N, &argc, argv);
    descriptor[0].type = get_image_type (argv[N]);
    if (descriptor[0].type == T_UNDEF) {
      fprintf (stderr, "ERROR: invalid image type %s\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
  }
 
  /* define ccd number */
  descriptor[0].CCDSelect = FALSE;
  if ((N = get_argument (argc, argv, "-ccd"))) {
    remove_argument (N, &argc, argv);
    descriptor[0].CCD = -1;
    for (i = 0; (i < Nccd) && (descriptor[0].CCD == -1); i++) {
      if (strnumcmp (ccds[i], argv[N])) {
	descriptor[0].CCD = i;
      }
    }
    if (descriptor[0].CCD == -1) {
      fprintf (stderr, "ERROR: ccd %s choice out not found in camera config\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
    descriptor[0].CCDSelect = TRUE;
  }
 
  /* define exposure time */
  descriptor[0].Exptime = 0.0;
  descriptor[0].ExptimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-exptime"))) {
    remove_argument (N, &argc, argv);
    descriptor[0].Exptime = atof (argv[N]);
    remove_argument (N, &argc, argv);
    descriptor[0].ExptimeSelect = TRUE;
  }
 
  /* define filter */
  descriptor[0].filter = FILTER_NONE;
  if ((N = get_argument (argc, argv, "-filter"))) {
    remove_argument (N, &argc, argv);
    for (i = 0; (i < NFILTER) && (descriptor[0].filter == FILTER_NONE); i++) {
      if (!strcasecmp (argv[N], filtername[i])) {
	descriptor[0].filter = filternum[i];
      }
    }      
    if (descriptor[0].filter == FILTER_NONE) {
      fprintf (stderr, "ERROR: invalid filter %s\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
  }

  /*** this program will only register SPLIT images ***/

  if (argc != 2) {
    fprintf (stderr, "USAGE: detregister (filename) [config ops] -time start stop] [-type type] [-ccd N] [-filter name]\n");
    fprintf (stderr, "ERROR - detregister not run\n");
    exit (1);
  }
  return (TRUE);
}
