# include "elixir.h"

int CheckDepend (Object *object, int argc, char **argv, int *argd) {

  int i;
  struct stat filestat;
  uid_t uid;
  gid_t gid;
  int status;
  struct timeval now;
  FILE *logfile;
  double dtime;

  uid = getuid();
  gid = getgid();

  for (i = 0; i < argc; i++) {
    if (argd[i]) {
      /* check permission to read file */
      status = stat (argv[i], &filestat);
      
      /* continue if file exists and is accessible */
      if (!status) {
	if ((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR)) continue;
	if ((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP)) continue;
	if (filestat.st_mode & S_IROTH) continue;
      }

      /* if the file doesn't exist or is inaccessible, check the timer on this object */
      if (object[0].timer.tv_sec == 0) {
	gettimeofday (&object[0].timer, (void *) NULL);
	return (0);
      } 
      
      gettimeofday (&now, (void *) NULL);
      dtime = DTIME (now, object[0].timer);
      if (dtime > argd[i]) {
	logfile = LogOpen (object[0].logfile);
	fprintf (logfile, "timeout on %s: %f > %d\n", argv[i], dtime, argd[i]);
	if (logfile != stderr) fclose (logfile);
	object[0].status &= TIMEOUT;
	return (2);
      } else {
	return (0);
      }
    }
  }

  return (TRUE);

}

