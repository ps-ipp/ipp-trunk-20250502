# include "elixir.h"
# define MAXTIME 120

static double timeout;

void RegisterTimeout (double value) {
 timeout = value;
}

double GetTimeout () {
  return (timeout);
}

int CheckMachineStatus (Machine *machine) {

  unsigned int status;
  int Nread;
  struct timeval now;
  FILE *logfile;
  float dtime;

  status = machine[0].status;

  if (status == IDLE) return (status);
  if (status == DONE) return (status);
  if (status == DOWN) return (status);

  status &= ~MESSAGE; /* clear MESSAGE status */
  
  /* read from socket into fifo */
  ShiftFifo (&machine[0].fifo);

  Nread = ReadtoFifo (&machine[0].fifo, machine[0].rsock);
  
  /* evaluate data in message */
  switch (Nread) {
  case 0:
    status = DOWN;
    break;
  case -1:
    status |= BUSY;
    gettimeofday (&now, (void *) NULL);
    dtime = DTIME (now, machine[0].quiet);
    if (dtime > timeout) {
      logfile = LogOpen (machine[0].object[0].logfile);
      fprintf (logfile, "%s: process is taking too long, giving up\n", machine[0].hostname);
      if (logfile != stderr) fclose (logfile);
      fprintf (stderr, "process on %s hung?  killing and retrying...\n", machine[0].hostname);
      /* interrupt process which is running */
      kill (machine[0].pid, SIGINT);
      status = TIMEOUT;
      /* status = (JOBDONE | FAILURE); */
    }
    break;
  default:
    status |= MESSAGE;
    gettimeofday (&machine[0].quiet, (void *) NULL);
    if (memstr (machine[0].fifo.buffer, "ERROR",        machine[0].fifo.Nbuffer))
      status |= FAILURE;
    if (memstr (machine[0].fifo.buffer, "SUCCESS",      machine[0].fifo.Nbuffer))
      status |= SUCCESS;
    if (memstr (machine[0].fifo.buffer, "PROCESS DONE", machine[0].fifo.Nbuffer))
      status |= JOBDONE;
    break;
  }

  /* evaluate status completion-type status signals */
  switch (status & (JOBDONE | SUCCESS | FAILURE)) {
  case FAILURE:
  case SUCCESS:
  case JOBDONE:
    if (status & WAITING) break;
    status |= WAITING;
    gettimeofday (&machine[0].timer, (void *) NULL);
    break;
  case (JOBDONE | SUCCESS):
  case (JOBDONE | FAILURE):
    status |= DONE;
    break;
  case (FAILURE | SUCCESS):
  case (JOBDONE | FAILURE | SUCCESS):
    status |= ERROR;
    break;
  default:
    break;
  }

  /* check for completion timeout */
  if (status & WAITING) {
    gettimeofday (&now, (void *) NULL);
    if (DTIME (now, machine[0].timer) > MAXTIME) {
      switch (status & (JOBDONE | SUCCESS | FAILURE)) {
      case JOBDONE:
	status |= CRASH;
	break;
      case SUCCESS:
      case FAILURE:
      default:
	status |= ERROR;
	break;
      }
    }
  }

  machine[0].object[0].status = status;
  machine[0].status = status;

  return (status);

}


/* machine status:

   the machine can have several possible statuss, made up of specific bits
   in the status variable:

   IDLE 0x00 - no process running on host
   DOWN 0x01 - no connection to host
   BUSY 0x02 - no message on socket
   DONE 0x04 - process completed

   MESSAGE 0x08 - message on socket
   FAILURE 0x10 - process reported 'failure'
   SUCCESS 0x20 - process reported 'success'
   JOBDONE 0x40 - process reported 'jobdone'
   WAITING 0x80 - process reported 'jobdone'

   ERROR   0x010 - unexpected return values
   CRASH   0x020 - process crashed
*/
