# include "markstar.h"

check_lockfile ()
{
  
  FILE *f;
  char filename[128];
  struct stat filestat;

  sprintf (filename, "%s/lock\0", CATDIR);
  if (stat (filename, &filestat) != -1) {
    fprintf (stderr, "ERROR: catalog %s is locked, try again later\n", CATDIR);
    exit (0);
  }

  f = fopen (filename, "w");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't set lock file %s\n", filename);
    exit (0);
  }
  fclose (f);

}

clear_lockfile ()
{
  
  char filename[128], line[256];
  struct stat filestat;

  sprintf (filename, "%s/lock\0", CATDIR);
  if (stat (filename, &filestat) != -1) {
    sprintf (line, "rm %s\0", filename);
    system (line);
  } else {
    fprintf (stderr, "can't remove lockfile, why not?\n");
  }

}
