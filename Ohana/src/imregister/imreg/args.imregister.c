# include "imregister.h"
# include "imreg.h"

int args (int argc, char **argv) {

  int N;

  ConfigInit (&argc, argv); /* load elixir config data */
  ConfigCamera ();          /* load camera information */
  ConfigFilter ();          /* load filter information */

  SingleIsSplit = FALSE;
  if ((N = get_argument (argc, argv, "-split"))) {
    remove_argument (N, &argc, argv);
    SingleIsSplit = TRUE;
  }

  NoReg = FALSE;
  if ((N = get_argument (argc, argv, "-noreg"))) {
    remove_argument (N, &argc, argv);
    NoReg = TRUE;
  }

  output.verbose = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    output.verbose = TRUE;
  }

  NeedType = FALSE;
  if ((N = get_argument (argc, argv, "-needtype"))) {
    remove_argument (N, &argc, argv);
    NeedType = TRUE;
  }

  /* all imregister programs are implicitly modifying the db */
  output.modify = TRUE;
  IMSORT = FALSE;

  if (strstr (argv[0], "imregister") != (char *) NULL) {
    if (argc != 2) {
      fprintf (stderr, "ERROR: Usage: imregister (filename) [-split] [-noreg]\n");
      exit (1);
    }
    return (TRUE);
  }
  if (strstr (argv[0], "showiminfo") != (char *) NULL) {
    if (argc != 2) {
      fprintf (stderr, "ERROR: Usage: showiminfo (filename) [-split] [-noreg]\n");
      exit (1);
    }
    return (TRUE);
  }
  if (strstr (argv[0], "imsort") != (char *) NULL) {
    if (argc != 2) {
      fprintf (stderr, "ERROR: Usage: imsort filename(s) [-split] [-noreg]\n");
      exit (1);
    }
    IMSORT = TRUE;
    return (TRUE);
  }
  if (strstr (argv[0], "imstatreg") != (char *) NULL) {

    char *path, *file;

    CLIENT = TRUE;

    /* set up name to lockfile */
    path = pathname (ImageDB);
    file = filebasename (ImageDB);
    ALLOCATE (PIDFILE, char, strlen (path) + strlen (file) + 10);
    sprintf (PIDFILE, "%s/.%s.pid", path, file);

    /* create db.log and db.bfr */
    snprintf_nowarn (TempDB, MY_MAX_PATH, "%s.bfr", ImageDB);
    snprintf_nowarn (LogFile, MY_MAX_PATH, "%s.log", ImageDB);
    
    /* check for daemon mode */
    if ((N = get_argument (argc, argv, "-daemon"))) {
      remove_argument (N, &argc, argv);
      CLIENT = FALSE;

      /* special daemon options */
      if (get_argument (argc, argv, "-kill"))   KillProcess (PIDFILE);
      if (get_argument (argc, argv, "-status")) StatusProcess (PIDFILE);

      LOOP_DELAY = 60;
      if ((N = get_argument (argc, argv, "-delay"))) {
	remove_argument (N, &argc, argv);
	LOOP_DELAY = atof(argv[N]);
	remove_argument (N, &argc, argv);
      }	

      if (argc != 1) {
	fprintf (stderr, "ERROR: Usage: imstatreg -daemon [-delay N] [-status] [-kill]\n");
	exit (1);
      }
      return (TRUE);
    }

    if (argc != 4) {
      fprintf (stderr, "ERROR: Usage: imstatreg (fits) (stats) (sdat) [-split] [-noreg]\n");
      fprintf (stderr, "       or:    imstatreg -daemon\n");
      exit (1);
    }
    return (TRUE);
  }
  return (FALSE);
}
