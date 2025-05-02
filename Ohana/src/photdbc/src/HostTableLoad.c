# include "dvodist.h"
# define DEBUG 1

int CheckHostsAndPaths (HostTable *table) {

  int i;
  char command[64];
  char shell[64];

  strcpy (command, "ssh");
  strcpy (shell, "pclient");

  for (i = 0; i < table->Nhosts; i++) {

    // verify that I can ssh to this machine
    if (DEBUG) fprintf (stderr, "starting host within thread\n");

    int errorInfo;
    int pid = rconnect (command, table->hosts[i].hostname, shell, table->hosts[i].stdio, &errorInfo, TRUE);
    if (!pid) {     
      /** failure to start: extend retry period **/
      if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", table->hosts[i].hostname, errorInfo);
      exit (1);
    }
    table->hosts[i].pid = pid;
    // keep the connection open to use for md5sum check

    int status = check_dir_access (table->hosts[i].pathname, DEBUG);
    if (!status) {
      fprintf (stderr, "failed to find / create target path\n");
      exit (2);
    }

    fprintf (stderr, "success : %s, %s\n", table->hosts[i].hostname, table->hosts[i].pathname);
  }
  return (TRUE);
}
