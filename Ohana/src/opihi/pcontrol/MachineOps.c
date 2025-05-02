# include "pcontrol.h"
# define DEBUG 0

static int MAX_UNWANTED_HOST_JOBS = 5;

static Machine **machines = NULL;
static int      Nmachines = 0;
static int      NMACHINES = 0;

void InitMachines () {

  if (machines != NULL) return;

  NMACHINES = 10;
  Nmachines =  0;

  ALLOCATE (machines, Machine *, NMACHINES);
  memset (machines, 0, NMACHINES*sizeof(Machine *));

  return;
}

void FreeMachines () {

  int i;

  for (i = 0; i < NMACHINES; i++) {
    FREE (machines[i]);
  }
  FREE (machines);

  return;
}

// find a machine matching the given name (if not found, return NULL)
Machine *FindMachineByName (char *name) {

  int i;

  for (i = 0; i < Nmachines; i++) {
    if (!strcmp (machines[i][0].name, name)) {
      return machines[i];
    }
  }
  return NULL;
}

// add a new machine (if not new, return TRUE)
Machine *AddMachine (char *name) {

  Machine *machine;

  machine = FindMachineByName (name);
  if (machine) return machine;

  ALLOCATE (machines[Nmachines], Machine, 1);
  machines[Nmachines][0].name = strcreate (name);
  machines[Nmachines][0].Nhosts = 0;
  machines[Nmachines][0].NjobsRealhost = 0;
  machines[Nmachines][0].NjobsWanthost = 0;

  machine = machines[Nmachines];
  Nmachines ++;

  if (Nmachines >= NMACHINES - 1) {
    NMACHINES += 10;
    REALLOCATE (machines, Machine *, NMACHINES);
    memset (&machines[Nmachines], 0, (NMACHINES - Nmachines)*sizeof(Machine *));
  }

  return machine;
}

// delete a new machine (if not new, return TRUE)
int DelMachine (char *name) {
  OHANA_UNUSED_PARAM(name);

  return TRUE;
}

int AddMachineHost (Host *host) {

  Machine *machine;

  // find or add machine matching this host name
  machine = AddMachine (host[0].hostname); // Can this fail?
  machine[0].Nhosts ++;
  // XXX do we need this? host[0].machine = machine;
  return (TRUE);
}

int DelMachineHost (Host *host) {

  Machine *machine;

  // machine = host[0].machine;
  machine = FindMachineByName (host[0].hostname);
  if (machine == NULL) return (FALSE);

  machine[0].Nhosts --;
  if (machine[0].Nhosts < 0) {
    fprintf (stderr, "warning: mis-match in host count for machine %s\n", machine[0].name);
    machine[0].Nhosts = 0;
  }

  return (TRUE);
}

int AddMachineJob (Host *host, Job *job) {

  int i;
  Machine *machine;

  // find machine matching the real host name
  machine = FindMachineByName (host[0].hostname); // Can this fail?
  ASSERT (machine, "cannot find machine associated with host");
  machine[0].NjobsRealhost ++;

  // skip jobs that do not have a targeted host or any xhosts
  if (!job[0].hostname && !job[0].Nxhosts) {
    return (TRUE);
  }

  for (i = 0; i < job[0].Nxhosts; i++) {
    // find machine matching the xhost name (these count against the unwanted host total)
    machine = FindMachineByName (job[0].xhosts[i]); // Can this fail?
    if (!machine) continue;
    machine[0].NjobsWanthost ++;
  }

  // do not double count jobs on the wanted host
  if (!strcmp (job[0].hostname, host[0].hostname)) {
    return (TRUE);
  }

  // find machine matching the want host name (these are running on an unwanted host)
  machine = FindMachineByName (job[0].hostname); // Can this fail?
  if (machine == NULL) return (TRUE);
  machine[0].NjobsWanthost ++;

  return (TRUE);
}

int DelMachineJob (Host *host, Job *job) {

  int i;
  Machine *machine;

  // find machine matching the real host name
  machine = FindMachineByName (host[0].hostname); // Can this fail?
  ASSERT (machine, "cannot find machine associated with host");
  machine[0].NjobsRealhost --;

  // skip jobs that do not have a targeted host or any xhosts
  if (!job[0].hostname && !job[0].Nxhosts) {
    return (TRUE);
  }

  for (i = 0; i < job[0].Nxhosts; i++) {
    // find machine matching the xhost name (these count against the unwanted host total)
    machine = FindMachineByName (job[0].xhosts[i]); // Can this fail?
    if (!machine) continue;
    machine[0].NjobsWanthost --;
  }

  // do not double count jobs on the wanted host
  if (!strcmp (job[0].hostname, host[0].hostname)) {
    return (TRUE);
  }

  // find machine matching the want host name
  machine = FindMachineByName (job[0].hostname); // Can this fail?
  if (machine == NULL) return (TRUE);
  machine[0].NjobsWanthost --;

  return (TRUE);
}

int PrintMachines () {

  int i;
  Machine *machine;

  gprint (GP_LOG, "Nmachines: %d\n", Nmachines);
  for (i = 0; i < Nmachines; i++) {
    machine = machines[i];
    gprint (GP_LOG, "%s : %d : %d : %d\n", machine[0].name, machine[0].Nhosts, machine[0].NjobsRealhost, machine[0].NjobsWanthost);
  }

  return (TRUE);
}

int CheckMachineJobs (Host *host, Job *job) {

  int i;
  Machine *machine;

  machine = FindMachineByName (job[0].hostname);
  if (machine) {
    if (DEBUG) fprintf (stderr, "wanthost: %s, Ntotal: %d, Nmax: %d\n", machine[0].name, machine[0].NjobsWanthost + machine[0].NjobsRealhost, machine[0].Nhosts + MAX_UNWANTED_HOST_JOBS);
    if (machine[0].NjobsWanthost >= MAX_UNWANTED_HOST_JOBS) {
      if (DEBUG) fprintf (stderr, "too many outstanding jobs wanting host %s, delay job %s for now\n", machine[0].name, job[0].argv[0]);
      return (FALSE);
    }
  }

  for (i = 0; i < job[0].Nxhosts; i++) {
    machine = FindMachineByName (job[0].xhosts[i]);
    if (machine) {
      if (DEBUG) fprintf (stderr, "xhost: %s, Ntotal: %d, Nmax: %d\n", machine[0].name, machine[0].NjobsWanthost + machine[0].NjobsRealhost, machine[0].Nhosts + MAX_UNWANTED_HOST_JOBS);
      if (machine[0].NjobsWanthost >= MAX_UNWANTED_HOST_JOBS) {
	if (DEBUG) fprintf (stderr, "too many outstanding jobs wanting host %s, delay job %s for now\n", machine[0].name, job[0].argv[0]);
	return (FALSE);
      }
    }
  }    

  machine = FindMachineByName (host[0].hostname);
  if (machine) {
    if (DEBUG) fprintf (stderr, "realhost: %s, Ntotal: %d, Nmax: %d\n", machine[0].name, machine[0].NjobsWanthost + machine[0].NjobsRealhost, machine[0].Nhosts + MAX_UNWANTED_HOST_JOBS);
    if (machine[0].NjobsWanthost >= MAX_UNWANTED_HOST_JOBS) {
      if (DEBUG) fprintf (stderr, "too many outstanding jobs wanting host %s, delay job %s for now\n", machine[0].name, job[0].argv[0]);
      return (FALSE);
    }
  }

  return (TRUE);
}

void SetMaxUnwantedHostJobs (int value) {

  MAX_UNWANTED_HOST_JOBS = value;
  return;
}

int GetMaxUnwantedHostJobs (void) {

  return MAX_UNWANTED_HOST_JOBS;
}

