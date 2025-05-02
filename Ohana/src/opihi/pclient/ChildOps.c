# include "pclient.h"
// # include <sys/ioctl.h>
// # include <sys/types.h>
// # include <unistd.h>
// #include <stropts.h>

static int Nbad = 0;
static struct timeval last = {0, 0};

int InitChild () {

  ChildPID         = 0;                /** current child PID **/
  ChildStatus      = PCLIENT_NONE;     /** current status of child **/
  ChildExitStatus  = 0;                /** recent exit status of child **/
  
  child_stdin_fd[0]  = 0;
  child_stdin_fd[1]  = 0;
  child_stdout_fd[0] = 0;
  child_stdout_fd[1] = 0;
  child_stderr_fd[0] = 0;
  child_stderr_fd[1] = 0;

  InitIOBuffer (&child_stdout, 1024);
  InitIOBuffer (&child_stderr, 1024);
  return (TRUE);
}

int FreeChild () {

  FreeIOBuffer (&child_stdout);
  FreeIOBuffer (&child_stderr);
  return (TRUE);
}

int CheckChild () {

  int Nread;
  double dtime;
  struct timeval now;

  /* runaway test - if pcontrol is killed, pclient starts running away.  this test is a bit
     dangerous: the choice of dtime probably depends on the processor and the value provided to
     pclient.c:rl_set_keyboard_input_timeout (1000); note that we cannot use getppid == 1 as a test
     because the parent of pclient is the ssh process on the pclient host, not pcontrol.  in any
     case, the opihi shell catches if the ssh dies using getppid
   */
  gettimeofday (&now, (void *) NULL);
  dtime = 1e6*DTIME (now, last);
  if (dtime < 50) {
    Nbad ++;
    if (Nbad > 100) {
      abort ();
    }
  }
  if (dtime > 950) Nbad = 0;
  last = now;

  CheckChildStatus ();
  
  /* read stdout buffer */
  Nread = ReadtoIOBuffer (&child_stdout, child_stdout_fd[0]);
  switch (Nread) {
    case -2:  /* error in read (programming error?  system level error?) */
      abort ();
    case -1:  /* no data in pipe */
      break;
    case 0:   /* pipe is closed */
      /** change child state? **/
      pipe_signal_clear();
      break;
    default:  /* data in pipe */
      break;
  }
  
  /* read stderr buffer */
  Nread = ReadtoIOBuffer (&child_stderr, child_stderr_fd[0]);
  switch (Nread) {
    case -2:  /* error in read (programming error?  system level error?) */
      abort ();
    case -1:  /* no data in pipe */
      break;
    case 0:   /* pipe is closed */
      /** change child state? **/
      pipe_signal_clear();
      break;
    default:  /* data in pipe */
      break;
  }
  return (TRUE);
}

void CheckChildStatus () {

  int result, waitstatus;

  if (ChildStatus != PCLIENT_BUSY) return;

  /* check current child status */
  result = waitpid (ChildPID, &waitstatus, WNOHANG);
  switch (result) {
    case -1:  /* error with waitpid */
      switch (errno) {
	case ECHILD:
	  gprint (GP_ERR, "unknown PID, not a child proc\n");
	  gprint (GP_ERR, "did process already exit?  programming error?\n");
	  ChildStatus = PCLIENT_NONE;
	  break;
	case EINVAL:
	  gprint (GP_ERR, "error EINVAL (waitpid): programming error\n");
	  exit (1);
	case EINTR:
	  gprint (GP_ERR, "error EINTR (waitpid): programming error\n");
	  exit (1);
	default:
	  gprint (GP_ERR, "unknown error for waitpid (%d): programming error\n", errno);
	  exit (1);
      }
      break;
      
    case 0:  /* process not exited */
      ChildStatus = PCLIENT_BUSY;
      break;

    default:
      if (result != ChildPID) {
	gprint (GP_ERR, "waitpid error: mis-matched PID (%d vs %d).  programming error\n", result, ChildPID);
	exit (1);
      }
      
      if (WIFEXITED(waitstatus)) {
	ChildStatus = PCLIENT_EXIT;
	ChildExitStatus = WEXITSTATUS(waitstatus);
      }
      if (WIFSIGNALED(waitstatus)) {
	ChildStatus = PCLIENT_CRASH;
	ChildExitStatus = WTERMSIG(waitstatus);
      }
      if (WIFSTOPPED(waitstatus)) {
	gprint (GP_ERR, "waitpid returns 'stopped': programming error\n");
	exit (1);
      }
  }
  return;
}
