# include "elixir.h"

Process **DefineProcesses (Process *global, int *nprocess, char *config) {

  int i, j;
  char procname[256], entry[256], field[256];
  Process **process;
  int Nprocess, NPROCESS;
  double timeout;

  global[0].success = InitQueue ();
  global[0].failure = InitQueue ();
  
  ALLOCATE (global[0].argv, char *, 8);
  ALLOCATE (global[0].argv[0], char, 256);
  ALLOCATE (global[0].argv[1], char, 256);
  ALLOCATE (global[0].argv[2], char, 256);
  ALLOCATE (global[0].argv[3], char, 256);
  ALLOCATE (global[0].argv[4], char, 256);
  ALLOCATE (global[0].argv[5], char, 256);
  ALLOCATE (global[0].argv[6], char, 256);
  ALLOCATE (global[0].argv[7], char, 256);
  
  GetConfig (config, "global.Nargs",   "%d",  0, &global[0].argc);
  GetConfig (config, "global.success", "%s",  0, global[0].argv[0]);
  GetConfig (config, "global.failure", "%s",  0, global[0].argv[1]);
  GetConfig (config, "global.pending", "%s",  0, global[0].argv[2]);
  GetConfig (config, "global.logfile", "%s",  0, global[0].argv[3]);
  GetConfig (config, "global.photcode",  "%s",  0, global[0].argv[4]);
  GetConfig (config, "global.msg",     "%s",  0, global[0].argv[5]);
  GetConfig (config, "global.end",     "%s",  0, global[0].argv[6]);
  GetConfig (config, "global.pid",     "%s",  0, global[0].argv[7]);

  ScanConfig (config, "global.timeout", "%lf", 0, &timeout);
  if (timeout < 1) {
    fprintf (stderr, "global.timeout is absurd: %f\n", timeout);
    Shutdown (1);
  }
  if (timeout < 100) {
    fprintf (stderr, "**** global.timeout is very short (%f) ****\n", timeout);
  }
  RegisterTimeout (timeout);
  
  Nprocess = 0;
  NPROCESS = 5;
  ALLOCATE (process, Process *, NPROCESS);

  /* find all entries in config file labeled 'process' */
  for (i = 1; ScanConfig (config, "process", "%s", i, procname); i++) {
    process[Nprocess] = ConfigProcess (config, procname);
    Nprocess ++;
    if (Nprocess == NPROCESS) {
      NPROCESS += 5;
      REALLOCATE (process, Process *, NPROCESS);
    }
  }
  if (Nprocess == 0) {
    fprintf (stderr, "no processes defined in config file\n");
    Shutdown (1);
  }

  /* make links between processes */
  for (i = 0; i < Nprocess; i++) {
    /* connect this process success queue */
    sprintf (field, "%s.success", process[i][0].name);
    GetConfig (config, field, "%s", 0, entry);
    if (!strcasecmp (entry, "global")) {
      process[i][0].success = global[0].success;
      goto stage1;
    }
    for (j = 0; j < Nprocess; j++) {
      if (!strcasecmp (entry, process[j][0].name)) {
	if (i == j) {
	  fprintf (stderr, "ERROR: can't connect a process to itself: %s\n", process[i][0].name);
	  Shutdown (1);
	}
	process[i][0].success = process[j][0].pending;
	goto stage1;
      }
    }
    fprintf (stderr, "ERROR: can't connect process %s to target %s\n", process[i][0].name, entry);
    Shutdown (1);
    
  stage1:
    /* connect this process failure queue */
    sprintf (field, "%s.failure", process[i][0].name);
    GetConfig (config, field, "%s", 0, entry);
    if (!strcasecmp (entry, "global")) {
      process[i][0].failure = global[0].failure;
      goto stage2;
    }
    for (j = 0; j < Nprocess; j++) {
      if (!strcasecmp (entry, process[j][0].name)) {
	if (i == j) {
	  fprintf (stderr, "ERROR: can't connect a process to itself: %s\n", process[i][0].name);
	  Shutdown (1);
	}
	process[i][0].failure = process[j][0].pending;
	goto stage2;
      }
    }
    fprintf (stderr, "ERROR: can't connect process %s to target %s\n", process[i][0].name, entry);
    Shutdown (1);
    
  stage2:
    continue;
  }

  /* link global process to first process */
  for (j = 0; j < Nprocess; j++) {
    if (!strcasecmp (global[0].argv[2], process[j][0].name)) {
      global[0].pending = process[j][0].pending;
      goto stage3;
    }
  }
  fprintf (stderr, "ERROR: can't connect global process to target %s\n", global[0].argv[2]);
  Shutdown (1);
 
stage3:
  *nprocess = Nprocess;
  return (process);

}

void GetConfig (char *config, char *field, char *format, int N, void *ptr) {

  char *status;

  status = ScanConfig (config, field, format, N, ptr);
  if (status == NULL) {
    fprintf (stderr, "error in config, cannot find required field %s\n", field);
    Shutdown (1);
  }
  return;
}
