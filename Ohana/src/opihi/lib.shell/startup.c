# include "opihi.h"

/* program-independent initialization
 * these steps do not depend on the opihi implementation
 * (ie, the supplied commands or data structures)
 */

void general_init (int *argc, char **argv) {

  /* init srand for rnd numbers elsewhere */
  long A, B;
  A = time(NULL);
  for (B = 0; A == time(NULL); B++);
  srand48(B);
 
  /* set signals */
  signal (SIGINT, SIG_IGN);

  /* init for main (or only) thread */
  gprintInit ();

  init_error ();

  /* load config data (.ptolemyrc) */
  if (!ConfigInit (argc, argv)) {
    gprint (GP_ERR, "can't find config file. some functions will be unavailable\n");
  }

  return;
}

void startup (int *argc, char **argv) {
    
    int i, N, status, ONLY_INPUT, LOAD_RC;
    char *line, *home, *outline, *varname;
    char *rcfile, **list;
    int Nlist, NLIST;
   
    /* load in interesting environment variables */
    ALLOCATE (line, char, 1024);
    
    home = getenv ("HOME");
    if (home == NULL) 
      set_str_variable ("HOME", ".");
    else 
      set_str_variable ("HOME", home);
    set_str_variable ("KAPA", "kapa");
    set_variable ("PID", getpid());

    set_int_variable ("UNSIGN", 0);
    gfits_set_unsign_mode (FALSE);
    
    set_variable ("M_PI", M_PI);
    set_variable ("M_E",  M_E);
    set_variable ("M_c",  299792459.0); // meter / second
    set_variable ("M_c_cgs", 29979245900.0); // cm / second

    set_variable ("M_h",  6.62607004e-34); // meter^2 kg / second (J s)
    set_variable ("M_h_cgs",  6.62607004e-27); // erg s

    set_variable ("M_kB",  1.38064853e-23); // J / K
    set_variable ("M_kB_cgs",  1.38064853e-16); // erg / K

  /* check history file permission */
  {
    FILE *f;
    char *opihi_history;

    opihi_history = get_variable ("HISTORY");
    if (opihi_history && *opihi_history) {
      f = fopen (opihi_history, "a");
      if (f == NULL) /* no current history file here */
	gprint (GP_ERR, "can't save history.\n");
      else
	fclose (f);
      stifle_history (200);
      read_history (opihi_history);
    }
    if (opihi_history) free (opihi_history);
  }

  if (0) {
    /* fix the history list to remove the timestamp */
    char *c;
    int i;
    HIST_ENTRY **entry;
    
    entry = history_list ();
    if (entry != (HIST_ENTRY **) NULL) {
      for (i = 0; entry[i]; i++) {
	if ((strlen (entry[i][0].line) > 19) &&
	    (entry[i][0].line[2] == '/') && 
	    (entry[i][0].line[5] == '/') && 
	    (entry[i][0].line[11] == ':') && 
	    (entry[i][0].line[14] == ':') && 
	    (entry[i][0].line[17] == ':')) {
	  c = entry[i][0].line + 19;
	  memmove (entry[i][0].line, c, strlen(c) + 1);
	}
      }
    }
  }

    LOAD_RC = TRUE;
    if ((N = get_argument (*argc, argv, "--norc"))) {
      remove_argument (N, argc, argv);
      LOAD_RC = FALSE;
    }
    if ((N = get_argument (*argc, argv, "--no-rc"))) {
      remove_argument (N, argc, argv);
      LOAD_RC = FALSE;
    }
    if ((N = get_argument (*argc, argv, "-norc"))) {
      remove_argument (N, argc, argv);
      LOAD_RC = FALSE;
    }
    if ((N = get_argument (*argc, argv, "-no-rc"))) {
      remove_argument (N, argc, argv);
      LOAD_RC = FALSE;
    }
    
    ONLY_INPUT = FALSE;
    if ((N = get_argument (*argc, argv, "--only"))) {
      remove_argument (N, argc, argv);
      ONLY_INPUT = TRUE;
    }

    /* load cmdline files */
    Nlist = 0;
    NLIST = 10;
    ALLOCATE (list, char *, NLIST);
    while ((N = get_argument (*argc, argv, "--load"))) {
      remove_argument (N, argc, argv);
      list[Nlist] = strcreate (argv[N]);
      remove_argument (N, argc, argv);
      Nlist++;
      if (Nlist == NLIST) {
	NLIST += 10;
	REALLOCATE (list, char *, NLIST);
      }
    }

    is_script = TRUE;
    if (*argc == 1) is_script = FALSE;
    set_int_variable ("SCRIPT", is_script);

    if (LOAD_RC && !is_script) {
      rcfile = get_variable ("RCFILE");
      if (rcfile && check_file_access (rcfile, FALSE, FALSE, FALSE)) {
	sprintf (line, "input %s/%s", home, rcfile);
	status = command (line, &outline, TRUE);
	if (outline != (char *) NULL) free (outline);
	if (status) {
	  gprint (GP_LOG, "loaded file %s\n", rcfile); 
	}	  
      }
      if (rcfile) free (rcfile);
    }
      
    set_int_variable ("argv:n", 0);
    if (is_script) {
      /* first argument in input script, rest are argv */
      set_str_variable ("SCRIPT_NAME", argv[1]);
      list[Nlist] = strcreate (argv[1]);
      Nlist ++;
      /* generate list argv:0 - argv:n from arguments */
      ALLOCATE (varname, char, 256);
      for (i = 2; i < *argc; i++) {
	sprintf (varname, "argv:%d", i - 2);
	set_str_variable (varname, argv[i]);
      }
      set_int_variable ("argv:n", i - 2);
      free (varname);
    }

    /* execute command-line entries */
    for (i = 0; i < Nlist; i++) {
      ALLOCATE (line, char, 1024);
      sprintf (line, "input %s", list[i]);
      status = command (line, &outline, TRUE);
      if (outline != (char *) NULL) free (outline);
    }
    free (list);

    /* if this is not an interactive session, exit here */
    if (ONLY_INPUT) exit (40);
    if (is_script) exit (41);
    return;
}
