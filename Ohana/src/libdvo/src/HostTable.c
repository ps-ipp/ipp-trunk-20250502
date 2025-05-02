# include "dvo.h"
# include <sys/types.h>
# include <sys/wait.h>

void InitHost (HostInfo *host) {
  host->hostname = NULL;
  host->pathname = NULL;
  host->results = NULL;
  host->stdio[HOST_STDIN] = -1;
  host->stdio[HOST_STDOUT] = -1;
  host->stdio[HOST_STDERR] = -1;
  host->pid = 0;
  host->status = 1024;
  return;
}

void InitHosts (HostInfo *hosts, int Nhosts, int NHOSTS) {

  int i;
  for (i = Nhosts; i < NHOSTS; i++) {
    InitHost (&hosts[i]);
  }
  return;
}

void FreeHosts (HostInfo *hosts, int Nhosts) {

  int i;
  for (i = 0; i < Nhosts; i++) {
    FREE (hosts[i].hostname);
    FREE (hosts[i].pathname);
    FREE (hosts[i].results);
    FreeIOBuffer (&hosts[i].stdout);
    FreeIOBuffer (&hosts[i].stderr);
  }
  free (hosts);
  return;
}

void FreeHostTable (HostTable *table) {

  if (!table) return;
  if (table->hosts) {
    FreeHosts (table->hosts, table->Nhosts);
  }
  if (table->index) free (table->index);

  free (table);
  return;
}

// a HostTableGroup is a pointer set to one or more HostTable entries
void FreeHostTableGroup (HostTableGroup *table) {

  if (!table) return;
  if (table->hosts) {
    free (table->hosts);
  }
  free (table);
  return;
}

int HostTableExists (char *catdir, char *rootname) {

  int Nchar = strlen(catdir) + strlen(rootname) + 16;
  ALLOCATE_PTR (filename, char, Nchar); // one slash and one EOL
  snprintf (filename, Nchar, "%s/%s", catdir, rootname);

  // can I access the file for read? (not backup, not readwrite, not verbose)
  if (!check_file_access (filename, FALSE, FALSE, FALSE)) {
    free (filename);
    return FALSE;
  }

  /* check permission to read file */
  struct stat filestat;

  int status = stat (filename, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    uid_t uid = getuid();
    gid_t gid = getgid();

    if (uid == filestat.st_uid) {
      if (filestat.st_mode & S_IRUSR) return TRUE;
    }
    if (gid == filestat.st_gid) {
      if (filestat.st_mode & S_IRGRP) return TRUE;
    }
    if (filestat.st_mode & S_IROTH) return TRUE;
  }

  return FALSE;
}

HostTable *HostTableLoad (char *catdir, char *rootname) {

  int i, Nline;

  char *filename = NULL;

  int Nchar = strlen(catdir) + strlen(rootname) + 16;
  ALLOCATE (filename, char, Nchar); // one slash and one EOL
  snprintf (filename, Nchar, "%s/%s", catdir, rootname);

  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "failed to open host table %s\n", filename);
    free (filename);
    return NULL;
  }

  // simple format: ID hostname pathname
  
  int NHOSTS = 16;
  int Nhosts = 0;
  HostInfo *hosts = NULL;
  ALLOCATE (hosts, HostInfo, NHOSTS);
  InitHosts (hosts, Nhosts, NHOSTS);

  int maxID = 0;

  for (Nline = 0; TRUE; Nline ++) {
    int ID;
    char tmphost[1024];
    char tmppath[1024];
    char line[1024];

    // XXXX use this for safety: int status = scan_line_maxlen (f, line, 1024);
    int status = scan_line_maxlen (f, line, 1024);
    if (status == EOF) break;

    // find first non-whitespace char & skip commented lines
    for (i = 0; OHANA_WHITESPACE (line[i]); i++);
    if (line[i] == '#') continue;
    if (line[i] == 0) continue;

    status = sscanf (line, "%d %1023s %1023s", &ID, tmphost, tmppath);
    if (status != 3) {
      fprintf (stderr, "error reading line %d of host table %s\n", Nline, filename);
      FreeHosts (hosts, Nhosts);
      free (filename);
      fclose (f);
      return NULL;
    }

    // check the validity of ID (0 < ID < MAX_SHORT)

    if (ID < 1) {
      fprintf (stderr, "invalid host ID %d\n", ID);
      exit (1);
    }
    if (ID > 255) {
      fprintf (stderr, "invalid host ID %d\n", ID);
      exit (1);
    }
    maxID = MAX(maxID, ID);

    hosts[Nhosts].hostID = ID;
    hosts[Nhosts].hostname = strcreate(tmphost);
    hosts[Nhosts].pathname = strcreate(tmppath);
    
    InitIOBuffer (&hosts[Nhosts].stdout, 1000);
    InitIOBuffer (&hosts[Nhosts].stderr, 1000);

    Nhosts ++;
    if (Nhosts >= NHOSTS) {
      NHOSTS += 16;
      REALLOCATE (hosts, HostInfo, NHOSTS);
      InitHosts (hosts, Nhosts, NHOSTS);
    }
  }    

  HostTable *table = NULL;
  ALLOCATE (table, HostTable, 1);
  table->Nhosts = Nhosts;
  table->hosts = hosts;

  ALLOCATE (table->index, short, maxID + 1);
  for (i = 0; i <= maxID; i++) table->index[i] = -1;

  for (i = 0; i < table->Nhosts; i++) {
    if (table->index[table->hosts[i].hostID] != -1) {
      fprintf (stderr, "error: duplicate hostID %d\n", table->hosts[i].hostID);
      exit (1);
    }
    table->index[table->hosts[i].hostID] = i;
  }

  free (filename);
  fclose (f);
  return table;
}

// split a host table into Ngroups, each with a unique set of hosts
HostTableGroup *HostTableGroupsUniqueMachines (HostTable *table, int *ngroups) {

  // identify the unique host names and the number of times they each exist
  // (Host tables are not very large; I can afford to do this in an inefficient way)
  int i, j, k;

  int Nunique = 0;
  char **unique_hosts = NULL; // an array of pointers to a set of the names
  int  *Nunique_hosts = NULL; // an array of pointers to the count of each
  ALLOCATE ( unique_hosts, char *, table->Nhosts);
  ALLOCATE (Nunique_hosts, int, table->Nhosts);
  for (i = 0; i < table->Nhosts; i++) Nunique_hosts[i] = 0;

  for (i = 0; i < table->Nhosts; i++) {
    int found = FALSE;
    for (j = 0; !found && (j < Nunique); j++) {
      if (!strcmp(table->hosts[i].hostname, unique_hosts[j])) {
	Nunique_hosts[j] ++;
	found = TRUE;
      }
    }
    if (!found) {
      unique_hosts[Nunique] = table->hosts[i].hostname;
      Nunique_hosts[Nunique] = 1;
      Nunique ++;
    }
  }

  int Ngroups = 0;
  for (i = 0; i < Nunique; i++) {
    Ngroups = MAX (Nunique_hosts[i], Ngroups);
  }

  HostTableGroup *groups = NULL;
  int *foundHost = NULL;
  
  ALLOCATE (foundHost, int, table->Nhosts);
  ALLOCATE (groups, HostTableGroup, Ngroups);

  for (i = 0; i < table->Nhosts; i++) foundHost[i] = FALSE;

  // in each group, attempt to add one of each unique host
  for (i = 0; i < Ngroups; i++) {
    groups[i].Nhosts = 0;
    ALLOCATE (groups[i].hosts, HostInfo *, Nunique);
    for (j = 0; j < Nunique; j++) {
      int found = FALSE;
      for (k = 0; !found && (k < table->Nhosts); k++) {
	if (foundHost[k]) continue;
	if (strcmp(unique_hosts[j], table->hosts[k].hostname)) continue;
	groups[i].hosts[groups[i].Nhosts] = &table->hosts[k];
	groups[i].Nhosts ++;
	found = TRUE;
	foundHost[k] = TRUE;
      }
    }
  }
  free (foundHost);
  free (unique_hosts);
  free (Nunique_hosts);
  *ngroups = Ngroups;
  return groups;
}

// split a host table into Ngroups, each with a unique set of hosts
HostTableGroup *HostTableGroupsMaxNumber (HostTable *table, int *ngroups, int Nmax) {

  int i, j;

  int Ngroups = (table->Nhosts % Nmax) ? (int)(table->Nhosts / Nmax + 1) : table->Nhosts / Nmax;

  HostTableGroup *groups = NULL;
  ALLOCATE (groups, HostTableGroup, Ngroups);

  // in each group, attempt to add one of each unique host
  int k = 0;
  for (i = 0; i < Ngroups; i++) {
    groups[i].Nhosts = 0;
    ALLOCATE (groups[i].hosts, HostInfo *, Nmax);
    for (j = 0; (j < Nmax) && (k < table->Nhosts); j++) {
      groups[i].hosts[groups[i].Nhosts] = &table->hosts[k];
      groups[i].Nhosts ++;
      k++;
    }
  }
  *ngroups = Ngroups;
  return groups;
}

// wait for all children to complete, report output to stdout
int HostTableWaitJobs (HostTable *table, char *file, int lineno) {

  int i;

  // we have launched table->Nhosts jobs; wait for all of them to complete...
  // if one (N) failed to launch, we will get an ECHILD error from the last (N) calls
  int done = FALSE;
  for (i = 0; !done  && (i < table->Nhosts); i++) {
    int status = 0;

    // XXX we'll need to pass in WNOHANG and keep retrying if we want to have a timeout...
    int pid = waitpid (-1, &status, 0);
    if (!pid) {
      // this should only occur if we called waitpid with the WNOHANG option
      fprintf (stderr, "programming error (1)? %s %d", file, lineno);
      exit (2);
    }
    if (pid == -1) {
      switch (errno) {
	case ECHILD:
	  done = TRUE;
	  break;
	default:
	  fprintf (stderr, "programming error (2)? %s %d", file, lineno);
	  exit (2);
      }
    }

    // when the host has finished, close the open sockets

    // find the host which has finished
    int Nout, j;
    int found = FALSE;
    for (j = 0; j < table->Nhosts; j++) {
      if (table->hosts[j].pid != pid) continue;
      found = TRUE;
      // check on the status of this and report any output?
      fprintf (stderr, "job finished for %s (%d)\n", table->hosts[j].hostname, pid);
      // read the stderr and stdout
      IOBuffer buffer;
      InitIOBuffer (&buffer, 100);
      EmptyIOBuffer (&buffer, 100, table->hosts[j].stdio[HOST_STDOUT]);
      fprintf (stderr, "--- stdout from %s ---\n", table->hosts[j].hostname);
      Nout = write (STDOUT_FILENO, buffer.buffer, buffer.Nbuffer);
      if (Nout != buffer.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
      fprintf (stderr, "\n");
	  
      InitIOBuffer (&buffer, 100);
      EmptyIOBuffer (&buffer, 100, table->hosts[j].stdio[HOST_STDERR]);
      fprintf (stderr, "--- stderr from %s ---\n", table->hosts[j].hostname);
      Nout = write (STDOUT_FILENO, buffer.buffer, buffer.Nbuffer);
      if (Nout != buffer.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
      fprintf (stderr, "\n");
      if (WIFEXITED(status)) {
	fprintf (stderr, "normal completion, exit status is %d\n", WEXITSTATUS(status));
	table->hosts[j].status = WEXITSTATUS(status);
	if (table->hosts[j].status) {
	  fprintf (stderr, "job failed on %s\n", table->hosts[j].hostname);
	  continue;
	}
      } else {
	table->hosts[j].status = -1;
	fprintf (stderr, "job exited abnormally on %s\n", table->hosts[j].hostname);
	continue;
      }
    }
    if (!found) {
      fprintf (stderr, "Programming error: failed to matched finished job to known host!\n");
      exit (2);
    }
  }
  return TRUE;
}

// wait for all children to complete, report output to stdout
int HostTableWaitJobsGetIO (HostTable *table, char *file, int lineno, int VERBOSE) {

  // we have launched table->Nhosts jobs; wait for all of them to complete...
  // if one (N) failed to launch, we will get an ECHILD error from the last (N) calls

  // we need to read any data waiting on stderr or stdout from these jobs, or the overfull
  // buffers can cause a problem.  we alternate between 'select' and 'waitpid' calls with
  // timeouts for both

  // add all hosts' sockets to the fd_sets
  fd_set rdSet, wtSet;
  FD_ZERO (&rdSet);
  FD_ZERO (&wtSet);

  // XXX can I set the fd_sets once, since I am not actually closing the fd's?

  int globalStatus = TRUE;

  int i;
  int Nmax = 0;
  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i].pid) continue; // any unconnected hosts should be skipped
    FD_SET (table->hosts[i].stdio[HOST_STDIN], &wtSet);
    Nmax = MAX (Nmax, table->hosts[i].stdio[HOST_STDIN]);
    FD_SET (table->hosts[i].stdio[HOST_STDOUT], &rdSet);
    Nmax = MAX (Nmax, table->hosts[i].stdio[HOST_STDOUT]);
    FD_SET (table->hosts[i].stdio[HOST_STDERR], &rdSet);
    Nmax = MAX (Nmax, table->hosts[i].stdio[HOST_STDERR]);
  }    
  Nmax ++;

  // need the list of connected hosts for exit test below
  int Nrunning = 0;
  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i].pid) continue; // any unconnected hosts should be skipped
    Nrunning ++;
  }

  int Nfound = 0;

  // this loop has 2 chunks: (a) check for I/O + (b) check for jobs done
  while (Nfound < Nrunning) {

    // Wait up to 0.5 second for host to provide I/O
    // timeout gets mucked: need to reset before each select
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 500000;

    int status = select (Nmax, NULL, &wtSet, NULL, &timeout);
    if (status == -1) {
      perror("select()");
      exit (2);
    }

    // we have some sockets to check, check sockets for all hosts
    for (i = 0; (status > 0) && (i < table->Nhosts); i++) {
      if (!table->hosts[i].pid) continue; // any unconnected hosts should be skipped

      if (FALSE && FD_ISSET (table->hosts[i].stdio[HOST_STDIN], &wtSet)) {
	// this host is waiting for input : this is an error, so exit
	fprintf (stderr, "host %s is waiting for input\n", table->hosts[i].hostname);
	abort();
      }
      
      if ((table->hosts[i].stdio[HOST_STDOUT] > 0) && FD_ISSET (table->hosts[i].stdio[HOST_STDOUT], &rdSet)) {
	// this host has waiting output : read to buffer, and dump if necessary
	ReadtoIOBuffer (&table->hosts[i].stdout, table->hosts[i].stdio[HOST_STDOUT]);
	// if (table->hosts[i].stdout.Nbuffer > 0x10000) {
	if (table->hosts[i].stdout.Nbuffer > 0x1000) {
	  int printHead = VERBOSE && (table->hosts[i].stdout.Nbuffer > 0);
	  if (printHead) fprintf (stdout, "--- stdout from %s --- (%d bytes, v1)\n", table->hosts[i].hostname, table->hosts[i].stdout.Nbuffer);
	  int Nout = write (STDOUT_FILENO, table->hosts[i].stdout.buffer, table->hosts[i].stdout.Nbuffer);
	  if (Nout != table->hosts[i].stdout.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	  FlushIOBuffer (&table->hosts[i].stdout);
	  if (printHead) fprintf (stdout, "\n");
	}
      }

      if ((table->hosts[i].stdio[HOST_STDERR] > 0) && FD_ISSET (table->hosts[i].stdio[HOST_STDERR], &rdSet)) {
	// this host has waiting output : read to buffer, and dump if necessary
	ReadtoIOBuffer (&table->hosts[i].stderr, table->hosts[i].stdio[HOST_STDERR]);
	// if (table->hosts[i].stderr.Nbuffer > 0x10000) {
	if (table->hosts[i].stderr.Nbuffer > 0x1000) {
	  int printHead = VERBOSE && (table->hosts[i].stderr.Nbuffer > 0);
	  if (printHead) fprintf (stdout, "--- stderr from %s --- (%d bytes, v1)\n", table->hosts[i].hostname, table->hosts[i].stderr.Nbuffer);
	  int Nout = write (STDOUT_FILENO, table->hosts[i].stderr.buffer, table->hosts[i].stderr.Nbuffer);
	  if (Nout != table->hosts[i].stderr.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	  FlushIOBuffer (&table->hosts[i].stderr);
	  if (printHead) fprintf (stdout, "\n");
	}
      }
    }

    // now check if any children have finished...
    while (TRUE) {
      int status = 0;
      int pid = waitpid (-1, &status, WNOHANG);
      if (!pid) {
	// fprintf (stderr, "no hosts to harvest\n");
	usleep (200000);
	break; // no outstanding jobs have finished 
      }
      if ((pid == -1) && (errno == ECHILD)) goto escape; // no more jobs on which to wait
      if ((pid == -1) && (errno != ECHILD)) {
	fprintf (stderr, "programming error (2)? %s %d", file, lineno);
	exit (2);
      }

      // find the host which has finished
      int found = FALSE;
      for (i = 0; (i < table->Nhosts) && !found; i++) {
	if (table->hosts[i].pid != pid) continue;
	found = TRUE;

	HostInfo *host = &table->hosts[i];

	// check on the status of this and report any output?
	if (VERBOSE) fprintf (stdout, "job finished for %s (%d)\n", host->hostname, pid);

	// read stdout
	int printHead;
	printHead = VERBOSE || (host->stdout.Nbuffer > 0);
	EmptyIOBuffer (&host->stdout, 100, host->stdio[HOST_STDOUT]);
	if (printHead) fprintf (stdout, "--- stdout from %s --- (%d bytes, v2)\n", host->hostname, host->stdout.Nbuffer);
	int Nout = write (STDOUT_FILENO, host->stdout.buffer, host->stdout.Nbuffer);
	if (Nout != host->stdout.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	FlushIOBuffer (&host->stdout);
	if (printHead) fprintf (stdout, "\n");
	    
	// read stderr
	printHead = VERBOSE || (host->stderr.Nbuffer > 0);
	EmptyIOBuffer (&host->stderr, 100, host->stdio[HOST_STDERR]);
	if (printHead) fprintf (stdout, "--- stderr from %s --- (%d bytes, v2)\n", host->hostname, host->stderr.Nbuffer);
	Nout = write (STDOUT_FILENO, host->stderr.buffer, host->stderr.Nbuffer);
	if (Nout != host->stderr.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	FlushIOBuffer (&host->stderr);
	if (printHead) fprintf (stdout, "\n");

	if (WIFEXITED(status)) {
	  if (VERBOSE) fprintf (stdout, "normal completion, exit status is %d\n", WEXITSTATUS(status));
	  host->status = WEXITSTATUS(status);
	  if (host->status) {
	    fprintf (stdout, "job failed on %s\n", host->hostname);
	    globalStatus = FALSE;
	  }
	} else {
	  host->status = -1;
	  fprintf (stdout, "job exited abnormally on %s\n", host->hostname);
	  globalStatus = FALSE;
	  continue;
	}
      }
      if (found) Nfound ++;
      // we sometimes harvest children not in the list (eg, closed kapa window).  just ignore
      if (Nfound == Nrunning) goto escape; // we've harvested all jobs
    }
  }

escape:

  // close all opened connections
  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i].pid) continue; // any unconnected hosts should be skipped
    close (table->hosts[i].stdio[HOST_STDIN]);
    close (table->hosts[i].stdio[HOST_STDOUT]);
    close (table->hosts[i].stdio[HOST_STDERR]);
  }

  return globalStatus;
}

// wait for all children to complete, report output to stdout
int HostTableGroupWaitJobsGetIO (HostTableGroup *table, char *file, int lineno, int VERBOSE) {

  // we have launched table->Nhosts jobs; wait for all of them to complete...
  // if one (N) failed to launch, we will get an ECHILD error from the last (N) calls

  // we need to read any data waiting on stderr or stdout from these jobs, or the overfull
  // buffers can cause a problem.  we alternate between 'select' and 'waitpid' calls with
  // timeouts for both

  // add all hosts' sockets to the fd_sets
  fd_set rdSet, wtSet;
  FD_ZERO (&rdSet);
  FD_ZERO (&wtSet);

  // XXX can I set the fd_sets once, since I am not actually closing the fd's?

  int globalStatus = TRUE;

  int i;
  int Nmax = 0;
  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i][0].pid) continue; // any unconnected hosts should be skipped
    FD_SET (table->hosts[i][0].stdio[HOST_STDIN], &wtSet);
    Nmax = MAX (Nmax, table->hosts[i][0].stdio[HOST_STDIN]);
    FD_SET (table->hosts[i][0].stdio[HOST_STDOUT], &rdSet);
    Nmax = MAX (Nmax, table->hosts[i][0].stdio[HOST_STDOUT]);
    FD_SET (table->hosts[i][0].stdio[HOST_STDERR], &rdSet);
    Nmax = MAX (Nmax, table->hosts[i][0].stdio[HOST_STDERR]);
  }    
  Nmax ++;

  // need the list of connected hosts for exit test below
  int Nrunning = 0;
  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i][0].pid) continue; // any unconnected hosts should be skipped
    Nrunning ++;
  }

  int Nfound = 0;

  // this loop has 2 chunks: (a) check for I/O + (b) check for jobs done
  while (1) {

    // Wait up to 0.5 second for host to provide I/O
    // timeout gets mucked: need to reset before each select
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 500000;

    int status = select (Nmax, NULL, &wtSet, NULL, &timeout);
    if (status == -1) {
      perror("select()");
      exit (2);
    }

    // we have some sockets to check, check sockets for all hosts
    for (i = 0; (status > 0) && (i < table->Nhosts); i++) {
      if (!table->hosts[i][0].pid) continue; // any unconnected hosts should be skipped

      if (FALSE && FD_ISSET (table->hosts[i][0].stdio[HOST_STDIN], &wtSet)) {
	// this host is waiting for input : this is an error, so exit
	fprintf (stderr, "host %s is waiting for input\n", table->hosts[i][0].hostname);
	abort();
      }
      
      if ((table->hosts[i][0].stdio[HOST_STDOUT] > 0) && FD_ISSET (table->hosts[i][0].stdio[HOST_STDOUT], &rdSet)) {
	// this host has waiting output : read to buffer, and dump if necessary
	ReadtoIOBuffer (&table->hosts[i][0].stdout, table->hosts[i][0].stdio[HOST_STDOUT]);
	// if (table->hosts[i][0].stdout.Nbuffer > 0x10000) {
	if (table->hosts[i][0].stdout.Nbuffer > 0x1000) {
	  int printHead = VERBOSE || (table->hosts[i][0].stdout.Nbuffer > 0);
	  if (printHead) fprintf (stdout, "--- stdout from %s --- (%d bytes, v1)\n", table->hosts[i][0].hostname, table->hosts[i][0].stdout.Nbuffer);
	  int Nout = write (STDOUT_FILENO, table->hosts[i][0].stdout.buffer, table->hosts[i][0].stdout.Nbuffer);
	  if (Nout != table->hosts[i][0].stdout.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	  FlushIOBuffer (&table->hosts[i][0].stdout);
	  if (printHead) fprintf (stdout, "\n");
	}
      }

      if ((table->hosts[i][0].stdio[HOST_STDERR] > 0) && FD_ISSET (table->hosts[i][0].stdio[HOST_STDERR], &rdSet)) {
	// this host has waiting output : read to buffer, and dump if necessary
	ReadtoIOBuffer (&table->hosts[i][0].stderr, table->hosts[i][0].stdio[HOST_STDERR]);
	// if (table->hosts[i][0].stderr.Nbuffer > 0x10000) {
	if (table->hosts[i][0].stderr.Nbuffer > 0x1000) {
	  int printHead = VERBOSE || (table->hosts[i][0].stderr.Nbuffer > 0);
	  if (printHead) fprintf (stdout, "--- stderr from %s --- (%d bytes, v1)\n", table->hosts[i][0].hostname, table->hosts[i][0].stderr.Nbuffer);
	  int Nout = write (STDOUT_FILENO, table->hosts[i][0].stderr.buffer, table->hosts[i][0].stderr.Nbuffer);
	  if (Nout != table->hosts[i][0].stderr.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	  FlushIOBuffer (&table->hosts[i][0].stderr);
	  if (printHead) fprintf (stdout, "\n");
	}
      }
    }

    // now check if any children have finished...
    while (TRUE) {
      int status = 0;
      int pid = waitpid (-1, &status, WNOHANG);
      if (!pid) {
	// fprintf (stderr, "no hosts to harvest\n");
	usleep (500000);
	break; // no outstanding jobs have finished 
      }
      if ((pid == -1) && (errno == ECHILD)) goto escape; // no more jobs on which to wait
      if ((pid == -1) && (errno != ECHILD)) {
	fprintf (stderr, "programming error (2)? %s %d", file, lineno);
	exit (2);
      }

      // find the host which has finished
      int found = FALSE;
      for (i = 0; (i < table->Nhosts) && !found; i++) {
	if (table->hosts[i][0].pid != pid) continue;
	found = TRUE;

	HostInfo *host = table->hosts[i];

	// check on the status of this and report any output?
	if (VERBOSE) fprintf (stdout, "job finished for %s (%d)\n", host->hostname, pid);

	// read stdout
	int printHead;
	printHead = VERBOSE || (host->stdout.Nbuffer > 0);
	EmptyIOBuffer (&host->stdout, 100, host->stdio[HOST_STDOUT]);
	if (printHead) fprintf (stdout, "--- stdout from %s --- (%d bytes, v2)\n", host->hostname, host->stdout.Nbuffer);
	int Nout = write (STDOUT_FILENO, host->stdout.buffer, host->stdout.Nbuffer);
	if (Nout != host->stdout.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	FlushIOBuffer (&host->stdout);
	if (printHead) fprintf (stdout, "\n");
	    
	// read stderr
	printHead = VERBOSE || (host->stderr.Nbuffer > 0);
	EmptyIOBuffer (&host->stderr, 100, host->stdio[HOST_STDERR]);
	if (printHead) fprintf (stdout, "--- stderr from %s --- (%d bytes, v2)\n", host->hostname, host->stderr.Nbuffer);
	Nout = write (STDOUT_FILENO, host->stderr.buffer, host->stderr.Nbuffer);
	if (Nout != host->stderr.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
	FlushIOBuffer (&host->stderr);
	if (printHead) fprintf (stdout, "\n");

	if (WIFEXITED(status)) {
	  if (VERBOSE) fprintf (stdout, "normal completion, exit status is %d\n", WEXITSTATUS(status));
	  host->status = WEXITSTATUS(status);
	  if (host->status) {
	    fprintf (stdout, "job failed on %s\n", host->hostname);
	    globalStatus = FALSE;
	  }
	} else {
	  host->status = -1;
	  fprintf (stdout, "job exited abnormally on %s\n", host->hostname);
	  globalStatus = FALSE;
	  continue;
	}
      }
      if (!found) {
	fprintf (stderr, "Programming error: failed to matched finished job to known host!\n");
	exit (2);
      }
      Nfound ++;
      if (Nfound == Nrunning) goto escape; // we've harvested all jobs
    }
  }

escape:

  // close all opened connections
  for (i = 0; i < table->Nhosts; i++) {
    if (!table->hosts[i][0].pid) continue; // any unconnected hosts should be skipped
    close (table->hosts[i][0].stdio[HOST_STDIN]);
    close (table->hosts[i][0].stdio[HOST_STDOUT]);
    close (table->hosts[i][0].stdio[HOST_STDERR]);
  }

  return globalStatus;
}

int HostTableTestHost (SkyRegion *region, int hostID) {

  myAssert (region, "oops");

  // if hostID is not set, then we are not in a remote client 
  if (!hostID) return TRUE;

  if (region->hostFlags & DATA_USE_BCK) {
    if (region->backupID == hostID) return TRUE;
  } else {
    if (region->hostID == hostID) return TRUE;
  }
  return FALSE;
}
