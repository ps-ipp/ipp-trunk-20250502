# include "dvosplit.h"
static void help (void);

int args (int argc, char **argv) {
  
  int i, N, CONFIRM;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  /*** check for command line options ***/

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? (REFCAT only) */
  UserPatch.Rmin = 0;
  UserPatch.Rmax = 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;
  CONFIRM = TRUE;
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
    CONFIRM = FALSE;
  }

  OUTDIR = NULL;
  if ((N = get_argument (argc, argv, "-outdir"))) {
    remove_argument (N, &argc, argv);
    OUTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  CATFORMAT = NULL;
  if ((N = get_argument (argc, argv, "-set-format"))) {
    remove_argument (N, &argc, argv);
    CATFORMAT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // override input catalog mode (raw, mef, split, mysql)
  CATMODE = NULL;
  if ((N = get_argument (argc, argv, "-set-mode"))) {
    remove_argument (N, &argc, argv);
    CATMODE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  FULL_TABLE = FALSE;
  if ((N = get_argument (argc, argv, "-full-table"))) {
    remove_argument (N, &argc, argv);
    FULL_TABLE = TRUE;
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* extra error messages */
  SKIP_EXIST = FALSE;
  if ((N = get_argument (argc, argv, "-skip-exist"))) {
    SKIP_EXIST = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc == 3) {
    if (CONFIRM) {
      fprintf (stderr, "you are splitting the entire sky in one pass\n");
      fprintf (stderr, "this could be a time consuming operation.  type Ctrl-C within 5 seconds to cancel\n");
      for (i = 5; i > 0; i--) {
	fprintf (stderr, "%d.. ", i);
	usleep (1000000);
      }
      fprintf (stderr, "\n");
    }    
    return (TRUE);
  }

  fprintf (stderr, "USAGE: dvosplit (catdir) (newlevel) [-outdir outdir] [-region (Rmin) (Rmax) (Dmin) (Dmax)]\n");
  exit (2);
}

// XXX need to free sky
void dvosplit_free (SkyTable *sky, SkyList *skylist) {
  FREE (OUTDIR);
  FREE (CATMODE);
  FREE (CATFORMAT);

  SkyTableFree (sky);
  SkyListFree(skylist);
  FreePhotcodeTable();
  
  ohana_memcheck (VERBOSE);
  ohana_memdump (VERBOSE);
}

static void help () {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvosplit (catdir) (newlevel)\n\n");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -outdir (outdir)    	  : copy to a new location (does not copy Images.dat)\n");
  fprintf (stderr, "  -region ra ra dec dec 	  : migrate catalogs in specified region\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -set-format (format)    	  : set format of output catalog (or inherit from input)\n");
  fprintf (stderr, "  -set-mode (mode)    	  : set mode of output catalog (or inherit from input)\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

/** addstar modes:
 
    addstar (image.smp)  - add cmp/smp image data to db
    addstar -ref (file.dat) (photcode) 
    addstar -cat (USNO/2MASS/GSC) -region (ra dec - ra dec)

    -replace : ref/cat - replace existing match (photcode/time)
    -match   : ref/cat - only add measures to existing averages

    ref types: 
    ASCII - RA,DEC,M,dM in a table

    addstar 

**/

