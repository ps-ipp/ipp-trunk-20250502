# include "elixir.h"

/* this is in libohana -> should be in ohana.h */
char *getcwd_cfht (char *path, int size);

static Cluster *fullpool = (Cluster *) NULL;
static Cluster *idlepool = (Cluster *) NULL;
static Cluster *downpool = (Cluster *) NULL;
static char *ConfigFilename;

/* put machine on top of cluster */
void PushMachine (Machine *machine, Cluster *cluster) {

  int i;

  if (cluster[0].Nmachine == cluster[0].NMACHINE) {
    cluster[0].NMACHINE += 5;
    REALLOCATE (cluster[0].machine, Machine *, cluster[0].NMACHINE);
  }
  for (i = cluster[0].Nmachine; i > 0; i--) {
    cluster[0].machine[i] = cluster[0].machine[i-1];
  }
  cluster[0].machine[0] = machine;
  cluster[0].Nmachine ++;
}

/* put machine on bottom of cluster */
void PutMachine (Machine *machine, Cluster *cluster) {

  if (cluster[0].Nmachine == cluster[0].NMACHINE) {
    cluster[0].NMACHINE += 5;
    REALLOCATE (cluster[0].machine, Machine *, cluster[0].NMACHINE);
  }
  cluster[0].machine[cluster[0].Nmachine] = machine;
  cluster[0].Nmachine ++;
}

/* Get a machine from top of cluster */
Machine *GetMachine (Cluster *cluster) {

  int i;
  Machine *machine;

  if (cluster[0].Nmachine == 0) return ((Machine *) NULL);

  machine = cluster[0].machine[0];
  cluster[0].Nmachine --;
  for (i = 0; i < cluster[0].Nmachine; i++) {
    cluster[0].machine[i] = cluster[0].machine[i+1];
  }
  return (machine);
}

void IdleMachine (Machine *machine) {

  machine[0].status = IDLE;
  machine[0].object = (Object *) NULL;
  PutMachine (machine, idlepool);
 
}

void DownMachine (Machine *machine) {

  machine[0].status = DOWN;
  machine[0].object = (Object *) NULL;
  gettimeofday (&machine[0].start, (void *) NULL);
  gettimeofday (&machine[0].timer, (void *) NULL);
  machine[0].timer.tv_sec += 5;
  PutMachine (machine, downpool);
 
}

# define NRETRIES 50
int TestMachine (Machine *machine) {

  int i, status;
  char buffer[64];

  FlushFifo (&machine[0].fifo);

  if ((machine[0].wsock == 0) || (machine[0].rsock == 0)) return (FALSE);

  /* writes are non-blocking.  check for EPIPE in case pipe is closed */
  sprintf (buffer, "echo CONNECTION TEST\n");
  status = write (machine[0].wsock, buffer, strlen(buffer));
  if ((status == -1) && (errno == EPIPE)) {
    fprintf (stderr, "socket unexpectedly closed in test\n");
    CloseMachine (machine);
    return (FALSE);
  }

  status = -1;
  for (i = 0; (i < NRETRIES) && (status == -1); i++) {
    status = SockScan ("CONNECTION TEST", &machine[0].fifo, machine[0].rsock);
    if (status == 0) {
      fprintf (stderr, "socket unexpectedly closed in test\n");
      CloseMachine (machine);
      return (FALSE);
    }
  }
  if (i == NRETRIES) {
    fprintf (stderr, "no response from machine, shutting it down\n");
    CloseMachine (machine);
    return (FALSE);
  }
  FlushFifo (&machine[0].fifo);
  return (TRUE);
}

Machine *GrabMachine () {

  Machine *machine;

  machine = GetMachine (idlepool);
  if (machine == (Machine *) NULL) return (machine);

  if (!TestMachine (machine)) {
    DownMachine (machine);
    return ((Machine *) NULL);
  }
  return (machine);
}

Cluster *InitCluster () {

  Cluster *cluster;

  ALLOCATE (cluster, Cluster, 1);
  cluster[0].NMACHINE = 5;
  cluster[0].Nmachine = 0;
  ALLOCATE (cluster[0].machine, Machine *, cluster[0].NMACHINE);
  
  return (cluster);

}

void InitMachines (char *config) {

  int i, Nm, fd, Nout, status;
  char name[256];
  Machine *machine;
  char *cwd;

  fullpool = InitCluster ();
  idlepool = InitCluster ();
  downpool = InitCluster ();

  /* processes run on these machines need to have access to the
     exact config file used by elixir.  Therefore, we write the 
     config file to a unique filename in this directory and pass 
     that name to the processes which start machines.  it is 
     crucial that the cwd be visible on the other machines, and have
     the same path name.  when we exit (Shutdown(n)), we will delete
     this file (but not before!). */
  
  /* get cwd and create config file here */
  if ((cwd = getcwd_cfht (NULL, 1024)) == NULL) {
    fprintf (stderr, "error getting cwd\n");
    Shutdown (1);
  }
  sprintf (name, "%s/elixir.XXXXXX", cwd);
  if ((fd = mkstemp (name)) == -1) {
    fprintf (stderr, "can't store current config in cwd\n");
    Shutdown (1);
  }
  Nout = write (fd, config, strlen (config));
  if (Nout != strlen (config)) {
    fprintf (stderr, "can't store current config in cwd\n");
    Shutdown (1);
  }
  status = close (fd);
  if (status == -1) {
    fprintf (stderr, "can't store current config in cwd\n");
    Shutdown (1);
  }
  ConfigFilename = strcreate (name);

  /* create connection to each of the machines */
  Nm = 0;
  for (i = 1; ScanConfig (config, "MACHINE", "%s", i, name); i++) {
    ALLOCATE (machine, Machine, 1);
    machine[0].hostname = strcreate (name);
    InitFifo (&machine[0].fifo, 0x4000, 0x1000);
    if (ConnectMachine (machine)) {
      IdleMachine (machine);
    } else {
      DownMachine (machine);
    }
    PutMachine (machine, fullpool);
    Nm ++;
  }
  if (Nm == 0) {
    fprintf (stderr, "no available machines, exiting\n");
    Shutdown (1);
  }
      
}

int ConnectMachine (Machine *machine) {

  int rsock, wsock, pid;
  char line[256];

  pid = rconnect_elixir (machine[0].hostname, CONNECT, &rsock, &wsock);
  if (pid) {
    machine[0].rsock = rsock;
    machine[0].wsock = wsock;
    machine[0].pid   = pid;
    /* we can set up the shell here */
    sprintf (line, "setenv PTOLEMY %s\n", ConfigFilename);
    if (write (wsock, line, strlen (line)) != strlen(line)) {
      return FALSE;
    }
    sprintf (line, "umask 002\n");
    if (write (wsock, line, strlen (line)) != strlen(line)) {
      return FALSE;
    }
    return (TRUE);
  } else {
    fprintf (stderr, "can't make connection to %s, skipping for now\n", machine[0].hostname);
    machine[0].rsock = 0;
    machine[0].wsock = 0;
    machine[0].pid   = 0;
    return (FALSE);
  }
}

/* machine which are claimed as down need to be restarted.  
   first, check that they really are down, then restart as needed */
void RestartMachines () {
  
  int i;
  Machine *machine;
  double dtime;
  struct timeval now;

  for (i = 0; i < downpool[0].Nmachine; i++) {
    machine = GetMachine (downpool);
    if (machine == (Machine *) NULL) return;

    /* we only try to reconnect if timer is expired */
    gettimeofday (&now, (void *) NULL);
    dtime = DTIME (machine[0].timer, now);
    if (dtime > 0) {
      PutMachine (machine, downpool);
      continue;
    }

    fprintf (stderr, "restarting machine %s\n", machine[0].hostname);
    if (TestMachine (machine)) {
      /* machine is still alive, return to idlepool */
      fprintf (stderr, "%s is alive\n", machine[0].hostname);
      IdleMachine (machine);
      continue;
    }

    if (ConnectMachine (machine)) {
      fprintf (stderr, "connection to %s successfully restarted\n", machine[0].hostname);
      IdleMachine (machine);
    } else {
      /* advance dtime so restarts happen later and later */
      dtime = DTIME (machine[0].timer, machine[0].start);
      dtime = dtime * 2;
      dtime = MIN (600, dtime);
      gettimeofday (&machine[0].start, (void *) NULL);
      gettimeofday (&machine[0].timer, (void *) NULL);
      machine[0].timer.tv_sec += dtime;
      PutMachine (machine, downpool);
    }
  }

}

void Shutdown (int status) {

  int i;

  if (unlink (ConfigFilename) == -1) {
    fprintf (stderr, "trouble deleting config file: %s\n", ConfigFilename);
  }

  RemovePID ();

  if (fullpool == (Cluster *) NULL) exit (status);
    
  for (i = 0; i < fullpool[0].Nmachine; i++) {
    CloseMachine (fullpool[0].machine[i]);
  }

  exit (status);

}

void Restart (char **argv) {

  int i;

  if (unlink (ConfigFilename) == -1) {
    fprintf (stderr, "trouble deleting config file: %s\n", ConfigFilename);
  }

  RemovePID ();

  if (fullpool == (Cluster *) NULL) execvp (argv[0], argv);
    
  for (i = 0; i < fullpool[0].Nmachine; i++) {
    CloseMachine (fullpool[0].machine[i]);
  }

  execvp (argv[0], argv);
}


int InitMsgFile (char *file) {
  
  char line[256];
  int status;
  struct stat filestat;

  if (stat (file, &filestat) == -1) return (TRUE);
  sprintf (line, "mv -f %s %s~", file, file);
  status = system (line);
  return (status);
}

void CloseMachine (Machine *machine) {

  char buffer[128];

  sprintf (buffer, "exit\n");
  if (write (machine[0].wsock, buffer, strlen(buffer)) != strlen(buffer)) {
    fprintf (stderr, "can't shutdown %s\n", machine[0].hostname); 
  }
  close (machine[0].wsock);
  close (machine[0].rsock);
  fprintf (stderr, "shutdown machine %s, pid %d\n", machine[0].hostname, machine[0].pid); 

}

void DumpMachineStatus (FILE *f) {
  
  int i;

  if (fullpool == (Cluster *) NULL) return;

  fprintf (f, "machine status:\n");
  fprintf (f, "          name  process  status\n");
  fprintf (f, "----------------------------------------------------------------\n");
  for (i = 0; i < fullpool[0].Nmachine; i++) {
    if (fullpool[0].machine[i][0].object == (Object *) NULL) {
      fprintf (f, "%14s %8s      %2d\n", 
	       fullpool[0].machine[i][0].hostname,
	       "(none)",
	       fullpool[0].machine[i][0].status);
      
    } else {      
      fprintf (f, "%14s %8s      %2d\n", 
	       fullpool[0].machine[i][0].hostname,
	       fullpool[0].machine[i][0].object[0].lastproc,
	       fullpool[0].machine[i][0].status);
    }

  }

}

static int Nprocs, Nmachine;
static ProcessTimers *proctimers;

/* this must be called after machines have been loaded, registered */
void InitProcessTimers (Process **process, int Nprocess) {
  int i, j;

  Nmachine = fullpool[0].Nmachine;
  Nprocs = Nprocess;
  ALLOCATE (proctimers, ProcessTimers, Nmachine);

  for (i = 0; i < Nmachine; i++) {
    ALLOCATE (proctimers[i].timer,   struct timeval, Nprocs);
    ALLOCATE (proctimers[i].timesum, double,         Nprocs);
    ALLOCATE (proctimers[i].Njobs,   int,            Nprocs);
    ALLOCATE (proctimers[i].active,  int,            Nprocs);
    ALLOCATE (proctimers[i].process, Process *,      Nprocs);
    
    for (j = 0; j < Nprocess; j++) {
      proctimers[i].timer[j].tv_sec = 0;
      proctimers[i].timer[j].tv_usec = 0;
      proctimers[i].timesum[j] = 0;
      proctimers[i].Njobs[j] = 0;
      proctimers[i].active[j] = FALSE;
      proctimers[i].process[j] = process[j];
    }

    proctimers[i].machine = fullpool[0].machine[i];

  }
  
}

void StartProcessTimer (Process *process, Machine *machine) {
  
  int i, j;

  /* identify the machine */
  for (i = 0; i < Nmachine; i++) {
    if (proctimers[i].machine == machine) {
      for (j = 0; j < Nprocs; j++) {
	if (proctimers[i].process[j] == process) {
	  gettimeofday (&proctimers[i].timer[j], (void *) NULL);
	  proctimers[i].active[j] = TRUE;
	  return;
	}
      }
    }
  }

  fprintf (stderr, "can't find this machine, process combination!\n");
  fprintf (stderr, "machine: %p process: %p\n", machine, process);
  return;
}

void StopProcessTimer (Machine *machine) {
  
  int i, j;
  struct timeval stop;
  double dtime;

  /* identify the machine */
  for (i = 0; i < Nmachine; i++) {
    if (proctimers[i].machine == machine) {
      for (j = 0; j < Nprocs; j++) {
	if (proctimers[i].active[j]) {
 	  gettimeofday (&stop, (void *) NULL);
	  dtime = DTIME (stop, proctimers[i].timer[j]);
	  proctimers[i].timesum[j] += dtime;
	  proctimers[i].active[j] = FALSE;
	  proctimers[i].Njobs[j] ++;
	  return;
	}
      }
    }
  }

  fprintf (stderr, "can't find an active process for this machine!\n");
  fprintf (stderr, "machine: %p\n", machine);

}

int DumpProcessTimes (char *filename) {
  
  int i, j, state, mode;
  FILE *f;
  double dt;

  if (filename == (char *) NULL) {
    if (system ("tput clear") == -1) {
      fprintf (stderr, "--------\n");
    }
    f = stderr;
  } else {
    /* check lockfile */
    f = fsetlockfile (filename, 0.1, LCK_XCLD, &state);
    if (f == NULL) return (2);
    fseeko (f, 0, SEEK_END);
  }  

  fprintf (f, "               .");
  for (j = 0; j < Nprocs; j++) {
    fprintf (f, "   %-14s", proctimers[0].process[j][0].name);
  }
  fprintf (f, "\n");
  for (i = 0; i < Nmachine; i++) {
    fprintf (f, "%16s ", proctimers[i].machine[0].hostname);
    for (j = 0; j < Nprocs; j++) {
      if (proctimers[i].Njobs[j] > 0) {
	dt = proctimers[i].timesum[j] / proctimers[i].Njobs[j];
      } else {
	dt = 0;
      }
      fprintf (f, "%7.3f %-6d   ", dt, proctimers[i].Njobs[j]);
    }
    fprintf (f, "\n");
  }

  if (f != stderr) {
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    chmod (filename, mode);
    fclearlockfile (filename, f, LCK_XCLD, &state);
  }
  return (1);
}

