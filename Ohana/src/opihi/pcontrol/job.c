# include "pcontrol.h"

int job (int argc, char **argv) {

  char *Host = NULL;
  char **targv = NULL;
  int i, N, Mode, targc, Timeout, nicelevel;
  IDtype JobID;
  char **xhosts = NULL;
  int Nxhosts = 0;
  int NXHOSTS = 0;

  if (get_argument (argc, argv, "-host") && get_argument (argc, argv, "+host")) {
      gprint (GP_ERR, "ERROR: -host and +host are incompatible\n");
      return (FALSE);
  }    

  if (get_argument (argc, argv, "-h")) goto usage;
  if (get_argument (argc, argv, "-help")) goto usage;
  if (get_argument (argc, argv, "--help")) goto usage;

  Host = NULL;
  Mode = PCONTROL_JOB_ANYHOST;
  if ((N = get_argument (argc, argv, "-host"))) {
    remove_argument (N, &argc, argv);
    Host = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Mode = PCONTROL_JOB_WANTHOST;
  }
  if ((N = get_argument (argc, argv, "+host"))) {
    remove_argument (N, &argc, argv);
    Host = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Mode = PCONTROL_JOB_NEEDHOST;
  }
  if (Host == NULL) Host = strcreate ("anyhost");
 
  Timeout = 100;
  if ((N = get_argument (argc, argv, "-timeout"))) {
    remove_argument (N, &argc, argv);
    Timeout = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  nicelevel = 0;
  if ((N = get_argument (argc, argv, "-nice"))) {
    remove_argument (N, &argc, argv);
    nicelevel = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  xhosts = NULL;
  Nxhosts = 0;
  NXHOSTS = 10;
  while ((N = get_argument (argc, argv, "-xhost"))) {
    if (xhosts == NULL) {
      ALLOCATE (xhosts, char *, NXHOSTS);
    }
    remove_argument (N, &argc, argv);
    xhosts[Nxhosts] = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Nxhosts ++;
    if (Nxhosts == NXHOSTS) {
      NXHOSTS += 10;
      REALLOCATE (xhosts, char *, NXHOSTS);
    }
  }

  if (argc < 2) goto usage;
  
  targc = argc - 1;
  ALLOCATE (targv, char *, targc);
  for (i = 1; i < argc; i++) {
    targv[i-1] = strcreate (argv[i]);
  }

  // a JobID < 0 mean the job was not accepted
  JobID = AddJob (Host, Mode, Timeout, nicelevel, targc, targv, Nxhosts, xhosts);
  gprint (GP_LOG, "JobID: %d\n", (int) JobID);
  return (TRUE);

 usage:
    gprint (GP_ERR, "USAGE: job [options] (arg0) (arg1) ... (argN)\n");
    gprint (GP_ERR, "  options: -host, +host, -timeout, -xhost (host) -nice (level)\n");
    gprint (GP_ERR, "  arguments of the form @MAX_THREADS@ will be replaced when the job is launched\n");

    FREE (Host);
    for (i = 0; i < Nxhosts; i++) {
      FREE (xhosts[i]);
    }
    FREE (xhosts);
    return (FALSE);
}
