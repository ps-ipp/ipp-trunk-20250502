# include "elixir.h"

static int Nsuccess = 0;
static int Nfailure = 0;
static double timeout;
struct timeval start;
static int Reload = FALSE;

int CheckEndingState (Process *global, int Nobjects, int Dynamic) {
  
  int Ndone;
  struct timeval now;
  double dtime;
  
  /* check the success and failure queues for newly added objects */
  
  if (global[0].success[0].Nobject > Nsuccess) {
    DumpFinished (global[0].success, Nsuccess, global[0].argv[0]);
    Nsuccess = global[0].success[0].Nobject;
  }
  if (global[0].failure[0].Nobject > Nfailure) {
    DumpFinished (global[0].failure, Nfailure, global[0].argv[1]);
    Nfailure = global[0].failure[0].Nobject;
  }

  if (Dynamic) return  (TRUE);
  
  Ndone = 0;
  Ndone += global[0].success[0].Nobject;
  Ndone += global[0].failure[0].Nobject;

  if (Reload) {
    gettimeofday (&now, (void *) NULL);
    dtime = DTIME (now, start);
    if (dtime > timeout) return (FALSE);
  }
  
  if (Ndone == Nobjects)  
    return (FALSE);
  else
    return (TRUE);
}

void DumpFinished (Queue *queue, int Nstart, char *filename) {
  
  int i, j;
  FILE *f;
  
  f = fopen (filename, "a");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't output status of finished objects to %s\n", filename);
    f = stderr;
  }
  
  for (i = Nstart; i < queue[0].Nobject; i++) {
    
    for (j = 0; j < queue[0].object[i][0].argc; j++) {
      fprintf (f, "%s ", queue[0].object[i][0].argv[j]);
    }
    fprintf (f, "%08x %s\n", queue[0].object[i][0].status, queue[0].object[i][0].lastproc);
  }
  
  if (f != stderr) fclose (f);
  
}

int SetExitTimer () {

  timeout = GetTimeout ();
  gettimeofday (&start, (void *) NULL);
  Reload = TRUE;
  return (TRUE);
}

/* 
   
   global.success = global[0].argv[0]
   global.failure = global[0].argv[1]
   
*/
