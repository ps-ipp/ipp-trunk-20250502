# include "dvodist.h"
# define DEBUG 0

// examine all of the filenames in the catdir directories and update the host table to note the correct locations
int FixSkyForHost (char *catdir, SkyList *skylist, HostTable *table) {

  int i, j;

  for (i = 0; i < skylist->Nregions; i++) {
    
    if (i && (i % 1000 == 0)) fprintf (stderr, ".");

    struct stat filestat;

    // filename of copy at CATDIR
    char rawfile[1024];
    snprintf (rawfile, 1024, "%s/%s.cpt", catdir, skylist[0].regions[i]->name);
    
    int status = lstat (rawfile, &filestat);
    if (status == -1) {
      if (errno == ENOENT) continue;
      fprintf (stderr, "failed to STAT file %s\n", rawfile);
      continue;
    }

    if (!S_ISLNK(filestat.st_mode)) continue;

    char linkfile[1024];
    ssize_t Nbytes = readlink (rawfile, linkfile, 1024);
    if  (Nbytes == -1) {
      fprintf (stderr, "error reading link %s\n", rawfile);
      continue;
    }
       
    int found = FALSE;
    for (j = 0; !found && (j < table->Nhosts); j++) {
      if (strncmp (linkfile, table->hosts[j].pathname, strlen(table->hosts[j].pathname))) continue;
      // found the host
      HostInfo *host = &table->hosts[j];
      skylist->regions[i]->hostID = host->hostID;
      skylist->regions[i]->hostFlags |= DATA_ON_TGT;
      found = TRUE;

      if (DEBUG) fprintf (stderr, "found %s on %s\n", rawfile, host->hostname);
    }  
      
    if (!found) {
      fprintf (stderr, "failed to find host for %s (%s)\n", rawfile, linkfile);
    }
  }      
  return (TRUE);
}
