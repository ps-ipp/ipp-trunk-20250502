# include "pantasks.h"

void TimerMark (struct timeval *start);
float TimerElapsed (struct timeval *start, int reset);

int CheckController () {

  char *p, *q;
  int i, Njobs, status;
  IDtype JobID;
  Job *job;
  IOBuffer buffer;
  struct timeval start;

  /* get the list of completed jobs (exit / crash), update the job status */
  if (!CheckControllerStatus()) return (TRUE);

  /*** check EXIT jobs ***/
  InitIOBuffer (&buffer, 0x100);

  TimerMark (&start);
  FlushIOBuffer (&buffer);

  status = ControllerCommand ("jobstack exit", CONTROLLER_PROMPT, &buffer);
  if (VerboseMode()) gprint (GP_ERR, "check exit stack %f\n", TimerElapsed(&start, TRUE));
  if (!status) goto escape;

  /** programming error **/
  p = memstr (buffer.buffer, "USAGE: jobstack", buffer.Nbuffer);
  if (p != NULL) goto escape;

  /** skip past any leading garbage? ***/

  /** find "Njobs: **/
  p = memstr (buffer.buffer, "BEGIN BLOCK Njobs:", MIN(buffer.Nbuffer, 256));
  if (!p) goto escape;

  // check if buffer has data after BEGIN BLOCK Njobs:
  if (buffer.Nbuffer - (p - buffer.buffer) < strlen("BEGIN BLOCK Njobs:")) goto escape;

  // point to bit after Njobs:
  p += strlen("BEGIN BLOCK Njobs:");

  /** parse job list **/
  status = sscanf (p, "%d", &Njobs);
  if (status != 1) goto escape;
  if (VerboseMode()) gprint (GP_ERR, "parse %d jobs on stack %f\n", Njobs, TimerElapsed(&start, TRUE));

  /* output looks like:
     BEGIN BLOCK Njobs: NN\n
     ID name machine\n
     ID name machine\n
  */

  // p is currently pointing at "BEGIN BLOCK Njobs"

  for (i = 0; i < Njobs; i++) {
    q = strchr (p, '\n');
    if (q == NULL) {
      gprint (GP_ERR, "controller message error: incomplete job list\n");
      break;
    }
    p = q + 1;
    status = sscanf (p, "%d", &JobID);

    // the operations within this locked block only interact with the controller or
    // modify the properties of the selected job
    JobTaskLock();
    job = FindControllerJob (JobID);
    if (job == NULL) {
      gprint (GP_ERR, "misplaced job? %d not in EXIT job list\n", JobID);
      JobTaskUnlock();
      continue;
    }
    /* this checks the individual job status, grabs stdout/stderr */
    CheckControllerJob (job);
    JobTaskUnlock();
  }
  if (VerboseMode()) gprint (GP_ERR, "clear %d exit jobs %f\n", i, TimerElapsed(&start, TRUE));

  /*** check CRASH jobs ***/
  FlushIOBuffer (&buffer);
  status = ControllerCommand ("jobstack crash", CONTROLLER_PROMPT, &buffer);
  if (!status) goto escape;

  p = memstr (buffer.buffer, "USAGE: jobstack", buffer.Nbuffer);
  if (p != NULL) goto escape;

  /** skip past any leading garbage? ***/

  /** find "Njobs: **/
  p = memstr (buffer.buffer, "BEGIN BLOCK Njobs:", MIN(buffer.Nbuffer, 256));
  p += strlen("BEGIN BLOCK Njobs:");

  /** parse job list **/
  status = sscanf (p, "%d", &Njobs);
  if (status != 1) goto escape;
  if (VerboseMode()) gprint (GP_ERR, "check crash stack %f\n", TimerElapsed(&start, TRUE)); 

  /* output looks like:
     BEGIN BLOCK Njobs: NN\n
     ID name machine\n
     ID name machine\n
  */

  // p is currently pointing at "BEGIN BLOCK Njobs"

  for (i = 0; i < Njobs; i++) {
    q = strchr (p, '\n');
    if (q == NULL) {
      gprint (GP_ERR, "controller message error: incomplete job list\n");
      break;
    }
    p = q + 1;
    status = sscanf (p, "%d", &JobID);

    // the operations within this locked block only interact with the controller or
    // modify the properties of the selected job
    JobTaskLock();
    job = FindControllerJob (JobID);
    if (job == NULL) {
      gprint (GP_ERR, "misplaced job? %d not in CRASH job list\n", JobID);
      JobTaskUnlock();
      continue;
    }
    /* this checks the individual job status, grabs stdout/stderr */
    CheckControllerJob (job);
    JobTaskUnlock();
  }
  if (VerboseMode()) gprint (GP_ERR, "clear %d crash jobs %f\n", i, TimerElapsed(&start, TRUE)); 

  FlushIOBuffer (&buffer);
  FreeIOBuffer (&buffer);
  return (TRUE);

 escape:
  FlushIOBuffer (&buffer);
  FreeIOBuffer (&buffer);
  return (FALSE);
}

void TimerMark (struct timeval *start) {
    gettimeofday (start, (void *) NULL);
}

float TimerElapsed (struct timeval *start, int reset) {

  float dtime;
  struct timeval stop;

  gettimeofday (&stop, (void *) NULL);
  dtime = DTIME (stop, start[0]);
  if (reset) gettimeofday (start, (void *) NULL);
  return (dtime);
}
