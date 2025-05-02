# include "pantasks.h"
/* adding a new host can delay controller up to a second or so */
# define CONTROLLER_TIMEOUT 5000
# define CONNECT_TIMEOUT 1000

/* local static variables to hold the connection to the controller */
static int ControllerStatus = FALSE;
static int stdin_cntl, stdout_cntl, stderr_cntl;
static IOBuffer stdout_buffer;
static IOBuffer stderr_buffer;
static int ControllerPID = 0;

/* local static variables to track the controller host properties */
/* these are used by AddHost and DeleteHost, and are only called by controller_host.c (clientThread) */
/* or by RestartController : lock between these two? */
static Host *hosts = NULL;
static int Nhosts = 0;
static int NHOSTS = 0;

int AddHost (char *hostname, int max_threads) {

  int N;

  N = Nhosts;
  Nhosts ++;

  CHECK_REALLOCATE (hosts, Host, NHOSTS, Nhosts, 16);

  hosts[N].hostname = strcreate (hostname);
  hosts[N].max_threads = max_threads;
  
  return (TRUE);
}

// XXX possible race condition problem: if we delete a host while the controller
// is being restarted.  to fix this, we need to keep the deletion in controller_thread,
// but perhaps mark it in the client_thread?
int DeleteHost (char *hostname) {

  int i, j;

  for (i = 0; i < Nhosts; i++) {
    if (strcmp (hosts[i].hostname, hostname)) continue;
    
    // delete this one
    free (hosts[i].hostname);
    for (j = i; j < Nhosts - 1; j++) {
      hosts[j].hostname = hosts[j+1].hostname;
      hosts[j].max_threads = hosts[j+1].max_threads;
    }
    Nhosts --;
    return (TRUE);
  }
  return (FALSE);
}

/* test if the controller is running */
int CheckControllerStatus () {
  return (ControllerStatus);
}

/* check job / get output if done */
int CheckControllerJob (Job *job) {
  struct timeval start, stop;

  gettimeofday (&start, (void *) NULL);
  CheckControllerJobStatus (job);
  gettimeofday (&stop, (void *) NULL);
  // float dtime = DTIME (stop, start);
  // if (VerboseMode()) gprint (GP_ERR, "check job status %f\n", dtime);

  if ((job[0].state == JOB_EXIT) || (job[0].state == JOB_CRASH)) {
    gettimeofday (&start, (void *) NULL);
    GetJobOutput ("stdout", job[0].pid, &job[0].stdout_buff, job[0].stdout_size);
    gettimeofday (&stop, (void *) NULL);
    // float dtime = DTIME (stop, start);
    /* if (VerboseMode()) gprint (GP_ERR, "get stdout %f\n", dtime); */

    gettimeofday (&start, (void *) NULL);
    GetJobOutput ("stderr", job[0].pid, &job[0].stderr_buff, job[0].stderr_size);
    gettimeofday (&stop, (void *) NULL);
    // float dtime = DTIME (stop, start);
    /* if (VerboseMode()) gprint (GP_ERR, "get stderr %f\n", dtime); */

    gettimeofday (&start, (void *) NULL);
    DeleteControllerJob (job);
    gettimeofday (&stop, (void *) NULL);
    // float dtime = DTIME (stop, start);
    /* if (VerboseMode()) gprint (GP_ERR, "delete job %f\n", dtime); */
  }  
  return (TRUE);
}

int DeleteControllerJob (Job *job) {

  int status;
  char cmd[128]; 
  IOBuffer buffer;

  sprintf (cmd, "delete %d", job[0].pid);
  InitIOBuffer (&buffer, 0x100);
  status = ControllerCommand (cmd, CONTROLLER_PROMPT, &buffer);
  FreeIOBuffer (&buffer);
  return (status);
}
  
/* ask controller about job status */
int CheckControllerJobStatus (Job *job) {

  int outstate, status;
  char cmd[128], status_string[64], string[128];
  char *p;
  IOBuffer buffer;

  sprintf (cmd, "check job %d", job[0].pid);
  InitIOBuffer (&buffer, 0x100);
  status = ControllerCommand (cmd, CONTROLLER_PROMPT, &buffer);
  if (!status) {
    FreeIOBuffer (&buffer);
    return (FALSE);
  }

  /** was this a valid job? **/
  p = memstr (buffer.buffer, "job not found", buffer.Nbuffer);
  if (p != NULL) {
    gprint (GP_ERR, "unknown job %d\n", job[0].pid);
    FreeIOBuffer (&buffer);
    return (FALSE);
  }

  /** parse status message **/
  p = memstr (buffer.buffer, "STATUS",   buffer.Nbuffer);
  if (p == NULL) goto escape;
  sscanf (p, "%*s %s", status_string);
  p = memstr (buffer.buffer, "EXITST",   buffer.Nbuffer);
  if (p == NULL) goto escape;
  sscanf (p, "%*s %d", &job[0].exit_status);
  p = memstr (buffer.buffer, "STDOUT",   buffer.Nbuffer);
  if (p == NULL) goto escape;
  sscanf (p, "%*s %d", &job[0].stdout_size);
  p = memstr (buffer.buffer, "STDERR",   buffer.Nbuffer);
  if (p == NULL) goto escape;
  sscanf (p, "%*s %d", &job[0].stderr_size);
  p = memstr (buffer.buffer, "DTIME",    buffer.Nbuffer);
  if (p == NULL) goto escape;
  sscanf (p, "%*s %lf", &job[0].dtime);
  p = memstr (buffer.buffer, "HOSTNAME", buffer.Nbuffer);
  if (p == NULL) goto escape;
  sscanf (p, "%*s %s", string);
  job[0].realhost = strcreate (string);
  FreeIOBuffer (&buffer);

  /* possible exit status values */
  outstate = -1;
  if (!strcmp(status_string, "BUSY"))    outstate = JOB_BUSY;
  if (!strcmp(status_string, "DONE"))    outstate = JOB_BUSY;
  if (!strcmp(status_string, "PENDING")) outstate = JOB_PENDING;
  if (!strcmp(status_string, "EXIT"))    outstate = JOB_EXIT;
  if (!strcmp(status_string, "CRASH"))   outstate = JOB_CRASH;
  if (outstate == -1) goto escape;

  job[0].state = outstate;
  return (TRUE);

escape:
  gprint (GP_ERR, "garbage in pcontrol reponse\n");
  FreeIOBuffer (&buffer);
  return (FALSE);
}

/* we read Nbytes from the host, then watch for the prompt */ 
int GetJobOutput (char *cmd, int pid, IOBuffer *buffer, int Nbytes) {
  
  int i, status, Nstart;
  char *line;
  struct timespec request, remain;

  /* avoid blocking on waitpid, test every 100 usec, up to 50 msec */
  request.tv_sec = 0;
  request.tv_nsec = 100000;

  /* flush any earlier messages */
  ReadtoIOBuffer (buffer, stdout_cntl);
  FlushIOBuffer (buffer);
  Nstart = buffer[0].Nbuffer;

  /* send command to get appropriate channel */
  status = write_fmt (stdin_cntl, "%s %d\n", cmd, pid);

  /* is pipe still open? */
  // XXX call StopController() here?
  if ((status == -1) && (errno == EPIPE)) {
    StopController ();
    return (CONTROLLER_DOWN);
  }

  /* read at least Nbytes, then watch for CONTROLLER_PROMPT */
  line = NULL;
  status = -1;
  for (i = 0; (i < CONTROLLER_TIMEOUT) && (status != 0) && (line == NULL); i++) {
    status = ReadtoIOBuffer (buffer, stdout_cntl);
    if ((buffer[0].Nbuffer - Nstart) >= Nbytes) {
      line = memstr (buffer[0].buffer, CONTROLLER_PROMPT, buffer[0].Nbuffer);
    }
    if (status == -1) nanosleep (&request, &remain);
  }
  if (status ==  0) {
    StopController ();
    return (CONTROLLER_DOWN);
  }
  if (status == -1) return (CONTROLLER_HUNG);

  /* if (VerboseMode()) gprint (GP_ERR, "message received (GetJobOutput : %s)\n", cmd);   */
  /* drop extra bytes from pcontrol (not pclient:job) */
  buffer[0].Nbuffer = Nstart + Nbytes;
  if (buffer[0].Nalloc > buffer[0].Nbuffer) {
    bzero (buffer[0].buffer + buffer[0].Nbuffer, buffer[0].Nalloc - buffer[0].Nbuffer);
  }
  return (CONTROLLER_GOOD);
}

/* submitting a job to the controller automatically starts controller */
int SubmitControllerJob (Job *job) {

  int i, Nchar, status;
  char *cmd, *p, string[64];
  IOBuffer buffer;

  if (job[0].task[0].host == NULL) return (FALSE); 

  if (!StartController ()) {
    gprint (GP_ERR, "failure to start pcontrol\n");
    return (FALSE);
  }

  /** construct the line to be sent to the controller **/

  /* determine the total line length */
  Nchar = 0;
  for (i = 0; i < job[0].argc; i++) {
    Nchar += strlen (job[0].argv[i]) + 1;
  }
  if (job[0].task[0].host) {
    Nchar += strlen (job[0].task[0].host) + 1;
  }
  Nchar += 10;
  ALLOCATE (cmd, char, Nchar);
  bzero (cmd, Nchar);

  /* construct the controller command portion */
  if (!strcasecmp (job[0].task[0].host, "ANYHOST")) {
    sprintf (cmd, "job");
  } else {
    if (job[0].task[0].host_required) {
      sprintf (cmd, "job +host %s", job[0].task[0].host);
    } else {
      sprintf (cmd, "job -host %s", job[0].task[0].host);
    }
  }
  
  if (job[0].nicelevel) {
    char tmp[64];
    snprintf (tmp, 64, " -nice %d", job[0].nicelevel);
    strcat (cmd, tmp);
  }

  /* add the command arguments */
  for (i = 0; i < job[0].task[0].argc; i++) {
    strcat (cmd, " ");
    strcat (cmd, job[0].task[0].argv[i]);
  }

  // This function is called by the JobTaskThread via SubmitJob.  We need to unlock the
  // JobTaskLock to avoid a dead lock with the JobTaskLock called in CheckController
  JobTaskUnlock();
  ControlLock(__func__);
  InitIOBuffer (&buffer, 0x100);
  status = ControllerCommand (cmd, CONTROLLER_PROMPT, &buffer);
  free (cmd);
  ControlUnlock(__func__);
  JobTaskLock();


  /* extract the job PID from the controller response */
  p = memstr (buffer.buffer, "JobID", buffer.Nbuffer);
  if (p == NULL) {
    gprint (GP_ERR, "missing PID in pcontrol message : programming error\n");
    gprint (GP_ERR, "ControllerCommand returns: %d\n", status);
    gprint (GP_ERR, "ControllerCommand response: %s\n", buffer.buffer);
    exit (1);
  }
  sscanf (p, "%*s %s", string);
  FreeIOBuffer (&buffer);

  job[0].pid = atoi (string);
  if (job[0].pid < 0) {
    return (FALSE);
  }
  return (TRUE);
}

int StartController () {

  char *p;
  char **argv, cmd[128];
  int i, pid, status;
  int stdin_fd[2], stdout_fd[2], stderr_fd[2];
  IOBuffer buffer;

  if (ControllerStatus) return (TRUE);

  if (VarConfig ("CONTROLLER", "%s", cmd) == NULL) strcpy (cmd, "pcontrol");

  if (hosts == NULL) {
    NHOSTS = 16;
    ALLOCATE (hosts, Host, NHOSTS);
  }

  bzero (stdin_fd,  2*sizeof(int));
  bzero (stdout_fd, 2*sizeof(int));
  bzero (stderr_fd, 2*sizeof(int));

  if (pipe (stdin_fd)  < 0) goto pipe_error;
  if (pipe (stdout_fd) < 0) goto pipe_error;
  if (pipe (stderr_fd) < 0) goto pipe_error;

  ALLOCATE (argv, char *, 2);
  argv[0] = cmd;
  argv[1] = 0;

  pid = fork ();
  if (!pid) { /* must be child process */
    gprint (GP_LOG, "starting controller connection\n");

    /* close the other ends of the pipes */
    close (stdin_fd[1]);
    close (stdout_fd[0]);
    close (stderr_fd[0]);

    /* tie our ends of the pipes to stdin, stdout, stderr */
    dup2 (stdin_fd[0],  STDIN_FILENO);
    dup2 (stdout_fd[1], STDOUT_FILENO);
    dup2 (stderr_fd[1], STDERR_FILENO);

    /* set all three unblocking */
    setvbuf (stdin,  (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stdout, (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stderr, (char *) NULL, _IONBF, BUFSIZ);

    status = execvp (argv[0], argv); 
    exit (1);
  }
  free (argv);

  /* close the other ends of the pipes */
  close (stdin_fd[0]);  stdin_fd[0]  = 0;
  close (stdout_fd[1]); stdout_fd[1] = 0;
  close (stderr_fd[1]); stderr_fd[1] = 0;

  /* make the pipes non-blocking */
  fcntl (stdin_fd[1],  F_SETFL, O_NONBLOCK);
  fcntl (stdout_fd[0], F_SETFL, O_NONBLOCK);
  fcntl (stderr_fd[0], F_SETFL, O_NONBLOCK);

  /* perform handshake with controller to verify alive & running */
  /** this handshake is similar to ControllerCommand, but has important differences **/
  InitIOBuffer (&buffer, 0x100);

  /* send handshake command */
  status = write_fmt (stdin_fd[1], "echo CONNECTED\n");
  if ((status == -1) && (errno == EPIPE)) goto pipe_error;

  /* try to get evidence connection is alive - wait upto a few seconds */
  /* connection is likely slow; don't bother with nanosleep here */
  p = NULL;
  status = -1;
  for (i = 0; (i < CONNECT_TIMEOUT) && (status != 0) && (p == NULL); i++) {
    status = ReadtoIOBuffer (&buffer, stdout_fd[0]);
    p = memstr (buffer.buffer, "CONNECTED", buffer.Nbuffer);
    usleep (50000); // wait for controller to start up
  }
  if (status == 0) goto pipe_error;
  if (status == -1) goto io_error;
  FreeIOBuffer (&buffer);

  /* set local static vars to pipe connections */
  stdin_cntl  = stdin_fd[1];
  stdout_cntl = stdout_fd[0];
  stderr_cntl = stderr_fd[0];

  InitIOBuffer (&stdout_buffer, 0x100);
  InitIOBuffer (&stderr_buffer, 0x100);

  ControllerPID = pid;
  ControllerStatus = TRUE;
  gprint (GP_LOG, "Connected\n");
  return (TRUE);

pipe_error:
  perror ("pipe error:");
  goto close_pipes;

io_error:
  gprint (GP_ERR, "timeout while connecting\n");
  goto close_pipes;

close_pipes:
  if (stdin_fd[0]  != 0) close (stdin_fd[0]);
  if (stdin_fd[1]  != 0) close (stdin_fd[1]);
  if (stdout_fd[0] != 0) close (stdout_fd[0]);
  if (stdout_fd[1] != 0) close (stdout_fd[1]);
  if (stderr_fd[0] != 0) close (stderr_fd[0]);
  if (stderr_fd[1] != 0) close (stderr_fd[1]);
  return (FALSE);
}

int ControllerCommand (char *cmd, char *response, IOBuffer *buffer) {

  int i, j, status;
  char *line;
  struct timespec request, remain;

  /* avoid blocking on waitpid, test every 100 usec, up to 50 msec */
  request.tv_sec = 0;
  request.tv_nsec = 100000;

  ReadtoIOBuffer (buffer, stdout_cntl);
  FlushIOBuffer (buffer);

  /* send command, is pipe still open? */
  status = write_fmt (stdin_cntl, "%s\n", cmd);
  if ((status == -1) && (errno == EPIPE)) {
    StopController ();
    fprintf (stderr, "controller is down (pipe closed), restarting\n");
    if (!RestartController ()) {
      return (FALSE);
    }
    return (FALSE);
  }
  
  /* for commands which don't return a prompt, don't look for one */
  if (response == NULL) {
    return (TRUE);
  }

  /* watch for response - wait up to 1 second */
  line = NULL;
  status = -1;
  for (j = 0; (status == -1) && (j < 10); j++) {
    for (i = 0; (i < CONTROLLER_TIMEOUT) && (status != 0) && (line == NULL); i++) {
      status = ReadtoIOBuffer (buffer, stdout_cntl);
      line = memstr (buffer[0].buffer, response, buffer[0].Nbuffer);
      if (status == -1) nanosleep (&request, &remain);
    }
    if (status ==  0) {
      StopController ();
      fprintf (stderr, "controller is down (EOF), restarting\n");
      if (!RestartController ()) {
	return (FALSE);
      }
      return (FALSE);
    }
    if (status == -1) {
      fprintf (stderr, "controller is not responding (%d tries)\n", j);
      gwrite (buffer[0].buffer, 1, buffer[0].Nbuffer, GP_ERR);
    }
  }
  if (status == -1) {
    fprintf (stderr, "controller still not responding, giving up\n");
    return (FALSE);
  }

  /* need to strip off the prompt */
  line = memstr (buffer[0].buffer, response, buffer[0].Nbuffer);
  if (line != NULL) {
    buffer[0].Nbuffer = line - buffer[0].buffer;
    bzero (buffer[0].buffer + buffer[0].Nbuffer, buffer[0].Nalloc - buffer[0].Nbuffer);
  }
  if (VerboseMode()) fprintf (stderr, "message received, %d cycles\n", i);
  return (TRUE);
}

int CheckControllerOutput () {

  int Nread;

  if (!ControllerStatus) return (TRUE);

  /* read stdout buffer */
  Nread = ReadtoIOBuffer (&stdout_buffer, stdout_cntl);
  switch (Nread) {
    case -2:  /* error in read (programming error?  system level error?) */
      gprint (GP_ERR, "serious IO error\n");
      exit (2);
    case -1:  /* no data in pipe */
    case 0:   /* pipe is closed, change child state? **/
    default:  /* data in pipe */
      break;
  }
  
  /* read stderr buffer */
  Nread = ReadtoIOBuffer (&stderr_buffer, stderr_cntl);
  switch (Nread) {
    case -2:  /* error in read (programming error?  system level error?) */
      gprint (GP_ERR, "serious IO error\n");
      exit (2);
    case -1:  /* no data in pipe */
    case 0:   /* pipe is closed, change child state? **/
    default:  /* data in pipe */
      break;
  }
  return (TRUE);
}

void PrintControllerBusyJobs () {

  int status;
  char command[1024];
  IOBuffer buffer;

  /* check if controller is running */
  status = CheckControllerStatus ();
  if (!status) {
    return;
  }

  sprintf (command, "jobstack busy");
  InitIOBuffer (&buffer, 0x100);

  status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);

  if (status) {
    gprint (GP_LOG, " jobs currently running remotely:\n");
    gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);
  } else {
    gprint (GP_LOG, "controller is not responding\n");
  }
  FreeIOBuffer (&buffer);
  return;
}

int PrintControllerOutput () {

  gprint (GP_LOG, "--- stdout ---\n");
  gwrite (stdout_buffer.buffer, 1, stdout_buffer.Nbuffer, GP_LOG);
  gprint (GP_LOG, "--- stderr ---\n");
  gwrite (stderr_buffer.buffer, 1, stderr_buffer.Nbuffer, GP_LOG);
  gprint (GP_LOG, "---  done  ---\n");
  return (TRUE);
}

int FlushControllerOutput () {

  FlushIOBuffer (&stdout_buffer);
  FlushIOBuffer (&stderr_buffer);

  return (TRUE);
}

int KillControllerJob (Job *job) {

  int status;
  char cmd[128];
  IOBuffer buffer;

  if (!ControllerStatus) return (TRUE);

  sprintf (cmd, "kill %d", job[0].pid);
  InitIOBuffer (&buffer, 0x100);
  status = ControllerCommand (cmd, CONTROLLER_PROMPT, &buffer);
  FreeIOBuffer (&buffer);
  return (status);

  /** need to interpret output message & free things **/
}

int QuitController () {

  char cmd[128];
  IOBuffer buffer;

  if (!ControllerStatus) return (TRUE);

  sprintf (cmd, "quit");
  InitIOBuffer (&buffer, 0x100);
  ControllerCommand (cmd, NULL, &buffer);
  FreeIOBuffer (&buffer);

  /* the quit command does not return a prompt, 
     check that the controller exited */
  StopController ();
  return (TRUE);
}

int StopController () {

  int i, waitstatus, result;

  if (!ControllerStatus) return (TRUE);

  ControllerStatus = FALSE;
  result = waitpid (ControllerPID, &waitstatus, WNOHANG);
  for (i = 0; (i < 10) && (result == 0); i++) {
    usleep (10000);  // wait for controller to exit
    result = waitpid (ControllerPID, &waitstatus, WNOHANG);
  }
  ControllerPID = 0;
  close (stdin_cntl);
  close (stdout_cntl);
  close (stderr_cntl);
  FreeIOBuffer (&stdout_buffer);
  FreeIOBuffer (&stderr_buffer);
  return (TRUE);
}

int RestartController () {

  int i, status;
  char command[256];
  IOBuffer buffer;

  gprint (GP_ERR, "attempting to restart pcontrol\n");
  if (!StartController ()) {
    gprint (GP_ERR, "failure to re-start pcontrol\n");
    return (FALSE);
  }

  InitIOBuffer (&buffer, 0x100);

  status = TRUE;

  // XXX lock the host table? no: that would risk a dead lock between client and controller threads:
  gprint (GP_ERR, "pcontrol restarted, reloading hosts\n");
  for (i = 0; i < Nhosts; i++) {
    snprintf (command, 256, "host add %s -threads %d\n", hosts[i].hostname, hosts[i].max_threads);
    gprint (GP_ERR, "sending: %s\n", command);
    status = ControllerCommand (command, CONTROLLER_PROMPT, &buffer);
  }

  if (status) gwrite (buffer.buffer, 1, buffer.Nbuffer, GP_LOG);

  FreeIOBuffer (&buffer);

  return (TRUE);
}
