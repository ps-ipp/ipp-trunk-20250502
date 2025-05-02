# include "addstar.h"

void check_permissions (char *basefile) {
  
  char *c, dir[256], filename[256];
  struct stat filestat;
  uid_t uid;
  gid_t gid;
  int status, cmode;

  uid = getuid();
  gid = getgid();

  /* check permission to write to directory */
  sprintf (filename, "%s", basefile);
  c = strrchr (filename, '/');
  if (c == (char *) NULL) {
    strcpy (dir, ".");
  } else {
    *c = 0;
    strcpy (dir, filename);
  }
  status = stat (dir, &filestat);
  if (status == -1) {
    fprintf (stderr, "directory %s does not exist, creating...\n", dir);
    cmode = S_IRWXU | S_IRWXG | S_IRWXO;
    status = mkdir (dir, cmode);
    if (status == -1) {
      fprintf (stderr, "ERROR: can't create %s\n", dir);
      exit (1);
    }
  } 
  status = stat (dir, &filestat);
  if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRWXU)) ||
      ((gid == filestat.st_gid) && (filestat.st_mode & S_IRWXG)) || 
      (filestat.st_mode & S_IRWXO)) {
  } else {
    fprintf (stderr, "ERROR: can't write to %s\n", dir);
    exit (1);
  }
  
  /* check permission to write to file */
  sprintf (filename, "%s", basefile);
  status = stat (filename, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR) && (filestat.st_mode & S_IWUSR)) ||
	((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP) && (filestat.st_mode & S_IWGRP)) || 
	((filestat.st_mode & S_IROTH) && (filestat.st_mode & S_IWOTH))) {
    } else {
      fprintf (stderr, "ERROR: can't write to %s\n", filename);
      exit (1);
    }
  }
  
  /* check permission to write to backup file */
  sprintf (filename, "%s~", basefile);
  status = stat (filename, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR) && (filestat.st_mode & S_IWUSR)) ||
	((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP) && (filestat.st_mode & S_IWGRP)) || 
	((filestat.st_mode & S_IROTH) && (filestat.st_mode & S_IWOTH))) {
    } else {
      fprintf (stderr, "ERROR: can't write to %s\n", filename);
      exit (1);
    }
  }
}
