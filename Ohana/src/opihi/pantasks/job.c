# include "pantasks.h"

/* this will require a bit of care: to define a job, we need
   to specify:
   - the command 
     this is defined in the job command: job argv0 argv1 argv2...
   - timeout (an option to the command)
   - exit macros?
     do nothing by default?
   - stderr / stdout disposal?
     save in a specific buffer by default?
     send to a file?

   the command can look just like the controller equivalent one:
   job [-host host] [-timeout timeout] args...

*/

int job (int argc, char **argv) {

  char *Host, **targv;
  int i, N, Mode, targc, Timeout;
  IDtype JobID;

  Host = NULL;
  Mode = PCONTROL_JOB_ANYHOST;
  if ((N = get_argument (argc, argv, "-host"))) {
    remove_argument (N, &argc, argv);
    Host = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Mode = PCONTROL_JOB_WANTHOST;
  }
  if ((N = get_argument (argc, argv, "+host"))) {
    if (Mode == PCONTROL_JOB_WANTHOST) {
      gprint (GP_ERR, "ERROR: -host and +host are incompatible\n");
      FREE (Host);
      return (FALSE);
    }
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

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: job [options] (arg0) (arg1) ... (argN)\n");
    FREE (Host);
    return (FALSE);
  }
  
  targc = argc - 1;
  ALLOCATE (targv, char *, targc);
  for (i = 1; i < argc; i++) {
    targv[i-1] = strcreate (argv[i]);
  }

  JobID = AddJob (Host, Mode, Timeout, targc, targv);
  gprint (GP_LOG, "JobID: %d\n", JobID);
  return (TRUE);
}
