# include "addstar.h"
# include "loadICRF.h"
# include <sys/types.h>
# include <sys/wait.h>

# define DEBUG 1

// we are running N parallel remote jobs on the remote hosts.  In the usual remote
// processing (eg, relphot or relastro), we launch one job per remote host.  In this case,
// we are launching one job foreach table (catalog.cpt) being touched.  individual hosts
// may have more than one job active at a time.  

int       Nhosts = 0;
HostInfo **hosts = NULL;

int init_remote_hosts () {

  Nhosts = 10;
  ALLOCATE (hosts, HostInfo *, Nhosts);

  int i;
  for (i = 0; i < Nhosts; i++) {
    hosts[i] = NULL;
  }
  return TRUE;
}

int find_empty_slot () {

  int i;
  for (i = 0; i < Nhosts; i++) {
    if (hosts[i] == NULL) return i;
  }
  return -1;
}

int harvest_all () {

  int slot = -1;

  while (slot != -2) {
    slot = harvest_host ();
    usleep (50000);
  }
  return TRUE;
}
 
// put the host in the list in a free slot. NOTE: this should never be called if we do not
// already have a free slot.
int save_remote_host (HostInfo *host) {

  int i;
  for (i = 0; i < Nhosts; i++) {
    if (hosts[i] == NULL) {
      hosts[i] = host;
      return TRUE;
    }
  }
  myAbort ("failed to find an empty slot: this is a programming error");
  return FALSE;
}

// wait for all children to complete, report output to stdout
// possible states when function is finished:
// * no child was harvested, but children still exist
// * no outstanding children
// * a child was harvested
int harvest_host () {

  // check if any children have finished...
  int status = 0;
  int pid = waitpid (-1, &status, WNOHANG);

  if (!pid) return -1; // there are outstanding children, but none have exited, no slot was opened
  if ((pid == -1) && (errno == ECHILD)) return -2; // there are no outstanding children
  if ((pid == -1) && (errno != ECHILD)) myAbort ("programming error?");

  if (DEBUG) fprintf (stdout, "PID %d done\n", pid);

  // find the host which has finished
  int i;
  int found = FALSE;
  for (i = 0; (i < Nhosts) && !found; i++) {
    if (!hosts[i]) continue; // unassigned slot
    if (hosts[i][0].pid != pid) continue;
    found = TRUE;
    break;
  }
  myAssert (found, "Programming error: failed to matched finished job to known host!");
  int slot = i;

  HostInfo *host = hosts[slot];

  // check on the status of this job and report any output
  if (DEBUG) fprintf (stdout, "job finished for %s (%d)\n", host->hostname, pid);

  int Nout, printHead;

  // read stdout
  EmptyIOBuffer (&host->stdout, 100, host->stdio[HOST_STDOUT]);
  printHead = VERBOSE || (host->stdout.Nbuffer > 0);
  if (printHead) fprintf (stdout, "--- stdout from %s --- (%d bytes, v2)\n", host->hostname, host->stdout.Nbuffer);
  Nout = write (STDOUT_FILENO, host->stdout.buffer, host->stdout.Nbuffer);
  if (Nout != host->stdout.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
  FlushIOBuffer (&host->stdout);
  if (printHead) fprintf (stdout, "\n");
	    
  // read stderr
  EmptyIOBuffer (&host->stderr, 100, host->stdio[HOST_STDERR]);
  printHead = VERBOSE || (host->stderr.Nbuffer > 0);
  if (printHead) fprintf (stdout, "--- stderr from %s --- (%d bytes, v2)\n", host->hostname, host->stderr.Nbuffer);
  Nout = write (STDOUT_FILENO, host->stderr.buffer, host->stderr.Nbuffer);
  if (Nout != host->stderr.Nbuffer) { fprintf (stderr, "(error writing log?)\n"); }
  FlushIOBuffer (&host->stderr);
  if (printHead) fprintf (stdout, "\n");

  if (WIFEXITED(status)) {
    if (DEBUG) fprintf (stdout, "normal completion, exit status is %d\n", WEXITSTATUS(status));
    host->status = WEXITSTATUS(status);
    if (host->status) {
      fprintf (stdout, "job failed on %s\n", host->hostname);
    }
  } else {
    host->status = -1;
    fprintf (stdout, "job exited abnormally on %s\n", host->hostname);
  }

  // close opened connections
  close (host->stdio[HOST_STDIN]);
  close (host->stdio[HOST_STDOUT]);
  close (host->stdio[HOST_STDERR]);

  // free data associated with the host
  free (host->hostname);
  free (host->pathname);
  FreeIOBuffer (&host->stdout);
  FreeIOBuffer (&host->stderr);
  free (host);

  // free the slot
  hosts[slot] = NULL;
  return slot; 
}

