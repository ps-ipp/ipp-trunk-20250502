# include "markstar.h"

check_permissions (char *basefile) {
  
  FILE *f;
  char *c, dir[256], filename[256];
  struct stat filestat;
  uid_t fuid, uid;
  gid_t fgid, gid;
  int status;

  uid = getuid();
  gid = getgid();

  /* check permission to write to directory */
  sprintf (filename, "%s\0", basefile);
  c = strrchr (filename, '/');
  if (c == (char *) NULL) {
    strcpy (dir, ".");
  } else {
    *c = 0;
    strcpy (dir, filename);
  }
  status = stat (dir, &filestat);
  if (status == -1) {
    fprintf (stderr, "ERROR: can't write to %s\n", dir);
    exit (0);
  } 
  if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRWXU)) ||
      ((gid == filestat.st_gid) && (filestat.st_mode & S_IRWXG)) || 
      (filestat.st_mode & S_IRWXO)) {
  } else {
    fprintf (stderr, "ERROR: can't write to %s\n", dir);
    exit (0);
  }
  
  /* check permission to write to file */
  sprintf (filename, "%s\0", basefile);
  status = stat (filename, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR) && (filestat.st_mode & S_IWUSR)) ||
	((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP) && (filestat.st_mode & S_IWGRP)) || 
	((filestat.st_mode & S_IROTH) && (filestat.st_mode & S_IWOTH))) {
    } else {
      fprintf (stderr, "ERROR: can't write to %s\n", filename);
      exit (0);
    }
  }
  
  /* check permission to write to backup file */
  sprintf (filename, "%s~\0", basefile);
  status = stat (filename, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR) && (filestat.st_mode & S_IWUSR)) ||
	((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP) && (filestat.st_mode & S_IWGRP)) || 
	((filestat.st_mode & S_IROTH) && (filestat.st_mode & S_IWOTH))) {
    } else {
      fprintf (stderr, "ERROR: can't write to %s\n", filename);
      exit (0);
    }
  }
  
}
