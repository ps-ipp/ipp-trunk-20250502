# include "pclient.h"

int reset (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  int i, result, waitstatus;
  struct timespec request, remain;

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: reset\n");
    gprint (GP_LOG, "RESET_RESULT: -1\n");
    return (FALSE);
  }
  
  if (ChildStatus == PCLIENT_NONE) {
    gprint (GP_ERR, "no child process, cannot reset\n");
    gprint (GP_LOG, "RESET_RESULT: 2\n");
    return (TRUE);
  }

  /* avoid blocking on waitpid, test every 100 usec, up to 50 msec */
  request.tv_sec = 0;
  request.tv_nsec = 100000;

  if (ChildStatus == PCLIENT_BUSY) {
    /* job is still running, send SIGTERM signal to job */
    kill (ChildPID, SIGTERM);
    result = waitpid (ChildPID, &waitstatus, WNOHANG);
    for (i = 0; (i < 500) && (result == 0); i++) {
      nanosleep (&request, &remain);
      result = waitpid (ChildPID, &waitstatus, WNOHANG);
    }
    if (result) goto reset_job;

    /* job did not exit, send SIGKILL signal to job */
    kill (ChildPID, SIGKILL);
    result = waitpid (ChildPID, &waitstatus, WNOHANG);
    for (i = 0; (i < 500) && (result == 0); i++) {
      nanosleep (&request, &remain);
      result = waitpid (ChildPID, &waitstatus, WNOHANG);
    }
    if (result) goto reset_job;

    /* total failure, don't reset */
    gprint (GP_ERR, "child process %d is hung, cannot reset\n", ChildPID);
    gprint (GP_LOG, "RESET_RESULT: 0\n");
    return (FALSE);
  }

reset_job:
  /* close out ends of the pipes */
  close (child_stdin_fd[1]);
  close (child_stdout_fd[0]);
  close (child_stderr_fd[0]);
  child_stdin_fd[1] = 0;
  child_stdout_fd[0] = 0;
  child_stderr_fd[0] = 0;

  FlushIOBuffer (&child_stdout);
  FlushIOBuffer (&child_stderr);
  ChildStatus = PCLIENT_NONE;
  ChildPID = 0;
  ChildExitStatus = 0;

  gprint (GP_LOG, "RESET_RESULT: 1\n");
  return (TRUE);
}

/* exit conditions:
   -1 - usage message / syntax error
    2 - unknown job
    0 - process hung
    1 - successful resetn
*/

/* the linux kernel timer sticks in a 10ms lag between kill and the harvest */
