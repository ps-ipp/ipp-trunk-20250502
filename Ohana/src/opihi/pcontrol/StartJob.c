# include "pcontrol.h"

// job and host are bound together (why pass in both?)
int StartJob (Job *job, Host *host) {

  int  i, Nline, status;
  char *line;

  /* job must have assigned host */
  ASSERT (job != NULL, "missing job");
  ASSERT (host != NULL, "missing host");
  ASSERT (host == (Host *) job[0].host, "invalid host");
  ASSERT (job  == (Job *) host[0].job, "invalid job");

  ResetJobOutput (&job[0].stdout_buf);
  ResetJobOutput (&job[0].stderr_buf);

  /* construct command line : job arg0 arg1 ... argN\n */
  // arguments of the form @MAX_THREADS@ are replaced here
  Nline = 10 + job[0].argc;
  for (i = 0; i < job[0].argc; i++) {
    Nline += strlen (job[0].argv[i]);
  }
  ALLOCATE (line, char, Nline);
  bzero (line, Nline);
  strcpy (line, "job");
  if (job[0].nicelevel) {
    char tmp[64];
    snprintf (tmp, 64, " -nice %d", job[0].nicelevel);
    strcat (line, tmp);
  }
  for (i = 0; i < job[0].argc; i++) {
    strcat (line, " ");
    if (!strcmp (job[0].argv[i], "@MAX_THREADS@")) {
      char threads[10];
      snprintf (threads, 10, "%5d", host[0].max_threads);
      strcat (line, threads);
      continue;
    } 
    strcat (line, job[0].argv[i]);
  }

  // fprintf (stderr, "command: %s\n", line);

  status = PclientCommand (host, line, PCLIENT_PROMPT, PCONTROL_RESP_START_JOB);
  free (line);

  /* check on success of pclient command */
  switch (status) {
    case PCLIENT_DOWN:
      // unlink host & job
      if (VerboseMode()) gprint (GP_ERR, "host %s is down\n", host[0].hostname);
      job[0].host = NULL;
      host[0].job = NULL;
      HarvestHost (host[0].pid);
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_PENDING, STACK_BOTTOM);
      return (FALSE);

    case PCLIENT_GOOD:
      job[0].realhost = strcreate (host[0].hostname);
      job[0].pid = -1;
      gettimeofday (&job[0].start, (void *) NULL);

      if (VerboseMode()) gprint (GP_ERR, "started job on host %s\n", host[0].hostname);  
      PutHost (host, PCONTROL_HOST_RESP, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_RESP, STACK_BOTTOM);
      return (TRUE);

    default:
      ABORT ("unknown status for pclient command");  
  }
}

// message has been received from the host, interpret results
int StartJobResponse (Host *host) {
  
  int status;
  char *p;
  IOBuffer *buffer;
  Job *job;

  /* job must have assigned host */
  ASSERT (host, "missing host");
  ASSERT (host[0].job, "missing job");
  buffer = &host[0].comms_buffer;
  job = (Job *) host[0].job;

  /* check on result of pclient command */
  p = memstr (buffer[0].buffer, "PCLIENT_PID:", buffer[0].Nbuffer);
  if (p == NULL) {
      // failed to get a valid response.  kill the job and try again, 
      // or accept a running process without a PID?
      if (VerboseMode()) gprint (GP_ERR, "failed to get a valid PID, trying to continue without\n");
      PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_BUSY, STACK_BOTTOM);
      gettimeofday (&job[0].start, NULL);
      return (TRUE);
  }

  sscanf (p, "%*s %d", &status);
  switch (status) {
    case -1:
      if (VerboseMode()) gprint (GP_ERR, "error in pclient child\n");
      // unlink host & job
      job[0].host = NULL;
      host[0].job = NULL;
      HarvestHost (host[0].pid);
      PutHost (host, PCONTROL_HOST_DOWN, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_PENDING, STACK_BOTTOM);
      return (FALSE);

    case -2:
      ABORT ("syntax error in pclient command");

    case -3:
      ABORT ("existing child on pclient");

    default:
      if (VerboseMode()) gprint (GP_ERR, "message received (StartJobResponse)\n");  
      job[0].pid = status;
      PutHost (host, PCONTROL_HOST_BUSY, STACK_BOTTOM);
      PutJob (job, PCONTROL_JOB_BUSY, STACK_BOTTOM);
      gettimeofday (&job[0].start, NULL);
      return (TRUE);
  }

  /* we should never reach here */
  ABORT ("should not reach here (StartJob)");
}
