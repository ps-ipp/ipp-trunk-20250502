# include "dvo.h"
# include <sys/types.h>
# include <sys/wait.h>

void InitRegionHosts (RegionHostInfo *hosts, int Nhosts, int NHOSTS) {

  int i;
  for (i = Nhosts; i < NHOSTS; i++) {
    hosts[i].Rmin = NAN;
    hosts[i].Rmax = NAN;
    hosts[i].Dmin = NAN;
    hosts[i].Dmax = NAN;

    hosts[i].RminCat = NAN;
    hosts[i].RmaxCat = NAN;
    hosts[i].DminCat = NAN;
    hosts[i].DmaxCat = NAN;

    hosts[i].hostname = NULL;
    hosts[i].stdio[HOST_STDIN] = -1;
    hosts[i].stdio[HOST_STDOUT] = -1;
    hosts[i].stdio[HOST_STDERR] = -1;
    hosts[i].pid = 0;

    hosts[i].image = NULL;
    hosts[i].imseq = NULL;
    hosts[i].Nimage = 0;
    hosts[i].NIMAGE = 0;

    hosts[i].neighbors = NULL;
    hosts[i].Nneighbors = 0;
    hosts[i].isNeighbor = FALSE;

    hosts[i].astromTable = NULL;
  }
  return;
}

void FreeRegionHosts (RegionHostInfo *hosts, int Nhosts) {

  int i;
  for (i = 0; i < Nhosts; i++) {
    free (hosts[i].hostname);
    FreeIOBuffer (&hosts[i].stdout);
    FreeIOBuffer (&hosts[i].stderr);
    FREE (hosts[i].image);
    FREE (hosts[i].imseq);
    FREE (hosts[i].neighbors);
    if (hosts[i].astromTable) {
      FREE (hosts[i].astromTable->map);
      FREE (hosts[i].astromTable->imageIDtoTableSeq);
      FREE (hosts[i].astromTable);
    }
    // do NOT use AstromOffsetTableFree : an astromTable is
    // only set here by relastro:assign_images, and it uses
    // AstromOffsetTableAddMapFromImage to assign pointers, not copies
    // AstromOffsetTableFree(hosts[i].astromTable);
  }
  free (hosts);
  return;
}

void FreeRegionHostTable (RegionHostTable *table) {

  if (!table) return;
  if (table->hosts) {
    FreeRegionHosts (table->hosts, table->Nhosts);
  }
  if (table->index) free (table->index);
  free (table);

  return;
}

# define MAX_LINE_LENGTH 1024
RegionHostTable *RegionHostTableLoad (char *catdir, char *rootname) {

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
  RegionHostInfo *hosts = NULL;
  ALLOCATE (hosts, RegionHostInfo, NHOSTS);
  InitRegionHosts (hosts, 0, NHOSTS);

  int maxID = 0;

  RegionHostTable *table = NULL;
  ALLOCATE (table, RegionHostTable, 1);
  table->Rmin = 360;
  table->Rmax =   0;
  table->Dmin = +90;
  table->Dmax = -90;

  for (Nline = 0; TRUE; Nline ++) {
    int ID;
    char tmphost[MAX_LINE_LENGTH];
    char line[MAX_LINE_LENGTH];

    int status = scan_line_maxlen (f, line, MAX_LINE_LENGTH);
    if (status == EOF) break;

    // find first non-whitespace char & skip commented lines
    for (i = 0; OHANA_WHITESPACE (line[i]); i++);
    if (line[i] == '#') continue;
    if (line[i] == 0) continue;

    double Rmin, Rmax, Dmin, Dmax;
    status = sscanf (line, "%d %1023s %lf %lf %lf %lf", &ID, tmphost, &Rmin, &Rmax, &Dmin, &Dmax);
    if (status != 6) {
      fprintf (stderr, "error reading line %d of region host table %s\n", Nline, filename);
      FreeRegionHosts (hosts, Nhosts);
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

    InitIOBuffer (&hosts[Nhosts].stdout, 1000);
    InitIOBuffer (&hosts[Nhosts].stderr, 1000);

    hosts[Nhosts].Rmin = Rmin;
    hosts[Nhosts].Rmax = Rmax;
    hosts[Nhosts].Dmin = Dmin;
    hosts[Nhosts].Dmax = Dmax;
    
    hosts[Nhosts].RminCat = Rmin;
    hosts[Nhosts].RmaxCat = Rmax;
    hosts[Nhosts].DminCat = Dmin;
    hosts[Nhosts].DmaxCat = Dmax;
    
    table->Rmin = MIN(Rmin, table->Rmin);
    table->Rmax = MAX(Rmax, table->Rmax);
    table->Dmin = MIN(Dmin, table->Dmin);
    table->Dmax = MAX(Dmax, table->Dmax);

    // InitIOBuffer (&hosts[Nhosts].stdout, 1000);
    // InitIOBuffer (&hosts[Nhosts].stderr, 1000);

    Nhosts ++;
    if (Nhosts >= NHOSTS) {
      NHOSTS += 16;
      REALLOCATE (hosts, RegionHostInfo, NHOSTS);
      InitRegionHosts (hosts, Nhosts, NHOSTS);
    }
  }    

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

int RegionHostFindNeighbors (RegionHostTable *table, int Nhost) {

  int i;

  // given a specific host (by table sequence), find all of its neighbors
  // (eg, (host->Rmin == althost->Rmax && (host->Dmin <= althost->Dmax) && (host->Dmax >= althost->Dmin)

  RegionHostInfo *myhost = &table->hosts[Nhost];
  myAssert (!myhost->neighbors, "myhost neighbors already allocated");
  myAssert (!myhost->Nneighbors, "myhost Nneighbors not zero?");
  ALLOCATE (myhost->neighbors, int, 1); // always allocate 1 extra

  for (i = 0; i < table->Nhosts; i++) {

    if (i == Nhost) continue;
    RegionHostInfo *altHost = &table->hosts[i];
    
    int onBorder;

    onBorder = 
      (myhost->Rmin == altHost->Rmax) && 
      (myhost->Dmin <= altHost->Dmax) &&
      (myhost->Dmax >= altHost->Dmin);
    if (onBorder) goto add_neighbor;

    onBorder = 
      (myhost->Rmax == altHost->Rmin) && 
      (myhost->Dmin <= altHost->Dmax) &&
      (myhost->Dmax >= altHost->Dmin);
    if (onBorder) goto add_neighbor;

    // handle the 0,360 boundary (Rmin,Rmax are in the range 0,360, but 0 == 360)
    onBorder = 
      (myhost->Rmin == 0.0) && 
      (altHost->Rmax == 360.0) && 
      (myhost->Dmin <= altHost->Dmax) &&
      (myhost->Dmax >= altHost->Dmin);
    if (onBorder) goto add_neighbor;

    onBorder = 
      (myhost->Rmax == 360.0) && 
      (altHost->Rmin == 0.0) && 
      (myhost->Dmin <= altHost->Dmax) &&
      (myhost->Dmax >= altHost->Dmin);
    if (onBorder) goto add_neighbor;

    onBorder = 
      (myhost->Dmin == altHost->Dmax) && 
      (myhost->Rmin <= altHost->Rmax) &&
      (myhost->Rmax >= altHost->Rmin);
    if (onBorder) goto add_neighbor;

    onBorder = 
      (myhost->Dmax == altHost->Dmin) && 
      (myhost->Rmin <= altHost->Rmax) &&
      (myhost->Rmax >= altHost->Rmin);
    if (onBorder) goto add_neighbor;
    
    continue;

  add_neighbor:
      myhost->neighbors[myhost->Nneighbors] = i;
      myhost->Nneighbors ++;
      REALLOCATE (myhost->neighbors, int, myhost->Nneighbors + 1);
      altHost->isNeighbor = TRUE;
  }
  return TRUE;
}

// wait for all children to complete, report output to stdout
int RegionHostTableWaitJobs (RegionHostTable *table, char *file, int lineno) {

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
int RegionHostTableWaitJobsGetIO (RegionHostTable *table, char *file, int lineno, int VERBOSE) {

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
	  int printHead = VERBOSE || (table->hosts[i].stdout.Nbuffer > 0);
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
	  int printHead = VERBOSE || (table->hosts[i].stderr.Nbuffer > 0);
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
	if (table->hosts[i].pid != pid) continue;
	found = TRUE;

	RegionHostInfo *host = &table->hosts[i];

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
    if (!table->hosts[i].pid) continue; // any unconnected hosts should be skipped
    close (table->hosts[i].stdio[HOST_STDIN]);
    close (table->hosts[i].stdio[HOST_STDOUT]);
    close (table->hosts[i].stdio[HOST_STDERR]);
  }

  return globalStatus;
}
