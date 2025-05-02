# include "elixir.h"

int CheckCluster (Cluster *cluster, Queue *success, Queue *failure, Queue *pending) {

  int i, j, status;
  Machine *machine, **tmachine;
  Object *object;
  FILE *logfile;

  /* check current status of all machines in cluster, idle those finished */
  for (i = 0; i < cluster[0].Nmachine; i++) {
    
    machine = cluster[0].machine[i];
    status = CheckMachineStatus (machine);

    object = machine[0].object;
    logfile = LogOpen (object[0].logfile);

    if (status & MESSAGE) {
      fprintf (logfile, "%s @ %s:", object[0].argv[0], machine[0].hostname);
      fwrite (&machine[0].fifo.buffer[machine[0].fifo.Nlast], 1, machine[0].fifo.Nbuffer - machine[0].fifo.Nlast, logfile);
    }      

    if (status & DOWN) {
      fprintf (stderr, "%s @ %s is down\n", object[0].argv[0], machine[0].hostname);
      fprintf (logfile, "%s @ %s is down\n", object[0].argv[0], machine[0].hostname);
      object[0].timer.tv_sec = 0; /* reset timer */
      StopProcessTimer (machine);
      /* PutObject (failure, object); */
      /* if machine crashes, retry object later */
      PutObject (pending, object);
      DownMachine (machine);
      cluster[0].machine[i] = (Machine *) NULL;
      goto escape;
    }
    if (status & DONE) {
      object[0].timer.tv_sec = 0; /* reset timer */
      StopProcessTimer (machine);
      fprintf (logfile, "%s @ %s is done\n", object[0].argv[0], machine[0].hostname);
      if (status & SUCCESS) {
	PutObject (success, object);
      } 
      if (status & FAILURE) {
	PutObject (failure, object);
      } 
      IdleMachine (machine);
      cluster[0].machine[i] = (Machine *) NULL;
      goto escape;
    }
    if (status & CRASH) {
      object[0].timer.tv_sec = 0; /* reset timer */
      StopProcessTimer (machine);
      fprintf (logfile, "%s @ %s had process crash\n", object[0].argv[0], machine[0].hostname);
      PutObject (failure, object);
      IdleMachine (machine);
      cluster[0].machine[i] = (Machine *) NULL;
      goto escape;
    }
    if (status & ERROR) {
      object[0].timer.tv_sec = 0; /* reset timer */
      StopProcessTimer (machine);
      fprintf (logfile, "%s @ %s has an odd status\n", object[0].argv[0], machine[0].hostname);
      PutObject (failure, object);
      DownMachine (machine);
      cluster[0].machine[i] = (Machine *) NULL;
      goto escape;
    }
    if (status & TIMEOUT) {
      object[0].timer.tv_sec = 0; /* reset timer */
      /* don't include TIMEOUT in timer stats StopProcessTimer (machine); */
      fprintf (logfile, "%s @ %s timed out, retrying\n", object[0].argv[0], machine[0].hostname);
      PutObject (pending, object);
      IdleMachine (machine);
      cluster[0].machine[i] = (Machine *) NULL;
      goto escape;
    }

  escape:
    if (logfile != stderr) fclose (logfile);
  }

  /* remove idle machines from this cluster's list */
  ALLOCATE (tmachine, Machine *, cluster[0].NMACHINE);
  for (j = i = 0; i < cluster[0].Nmachine; i++) {
    if (cluster[0].machine[i] != (Machine *) NULL) {
      tmachine[j] = cluster[0].machine[i];
      j++;
    }
  }
  free (cluster[0].machine);
  cluster[0].machine = tmachine;
  cluster[0].Nmachine = j;

  return (TRUE);
}


/*

  possible ending states:

  DONE & SUCCESS = process finished normally & succeeded
  DONE & FAILURE = process finished normally & failed

  CRASH = process crashed
  DOWN  = machine crashed
  IDLE  = no process (should never show up here!)
  ERROR = unexpected state!

  whenever an object is done, in any of the possible states, the
  arg dependency timer must be reset 

*/
