# include "nightd.h"
# include <time.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
# include <fcntl.h>

char **getargs (char *);

/* SetPID is always called outside of daemon loop, send errors to stderr */
int SetPID (pid_t *Xpid, char *Xuser, char *Xmachine) {
  
  int t1, t2, t3;
  pid_t pid;
  char *username, machine[256];
  FILE *f;

  f = fopen (PIDFile, "r");
  if (f == (FILE *) NULL) { 

    pid = getpid ();
    username = getenv ("USER");
    if (username == (char *) NULL) {
      fprintf (stderr, "error getting username\n");
      exit (2);
    }
    bzero (machine, 256);
    if (gethostname (machine, 256)) {
      fprintf (stderr, "error getting hostname\n");
      exit (2);
    }

    f = fopen (PIDFile, "w");
    if (f == (FILE *) NULL) { 
      fprintf (stderr, "can't write to PID file %s\n", PIDFile);
      exit (2);
    }

    fprintf (f, "PID:     %d\n", pid);
    fprintf (f, "USER:    %-s\n", username);
    fprintf (f, "MACHINE: %-s\n", machine);
    fclose (f);
    return (TRUE); 
  }

  t1 = fscanf (f, "%*s %d", Xpid);
  t2 = fscanf (f, "%*s %s", Xuser);
  t3 = fscanf (f, "%*s %s", Xmachine);
  if ((t1 != 1) || (t2 != 2) || (t3 != 1)) {
    fprintf (stderr, "error reading pid info\n");
  }
  fclose (f);
  return (FALSE);
}

/* date is the UT date at 09:00 local time for day starting at 12:00 */
int GetDateTime (char *datestr, char *timestr, float *time) {

  struct tm *local, *gmt;
  struct timeval now;
  time_t tsec, tref;
  int dsec;
  float hour;

  gettimeofday (&now, (struct timezone *) NULL);
  tsec = now.tv_sec;
  local = localtime (&tsec);

  sprintf (timestr, "%02dh%02dm", local[0].tm_hour, local[0].tm_min);
  hour = local[0].tm_hour + local[0].tm_min / 60.0 + local[0].tm_sec / 3600.0;
  if (hour < 12) hour += 24.0;

  dsec = 3600.0*(33.0 - hour);
  tref = tsec + dsec;

  gmt = gmtime (&tref);

  sprintf (datestr, "%04d%02d%02d", 1900 + gmt[0].tm_year, gmt[0].tm_mon + 1, gmt[0].tm_mday);
  *time = hour;
  
  return (TRUE);
}

int WaitForPeriod () {

  int Nsec;
  struct timeval now;
  
  gettimeofday (&now, (struct timezone *) NULL);
  Nsec = PERIOD - (now.tv_sec % PERIOD);
  sleep (Nsec);
  return (TRUE);
}

int pcommand (char *line, int timeout) {

  int i, status, eof, done, pidstat, Nread, exit_good, exit_stat;
  int rfd[2], wfd[2];
  pid_t pid;
  char buffer[0x4000];
  char **arglist;

  /* create pipes for communications */
  status = pipe (rfd);
  if (status < 0) {
    perror ("pipe");
    return (1);
  }
  status = pipe (wfd);
  if (status < 0) {
    perror ("pipe");
    return (1);
  }

  arglist = getargs (line);
  pid = fork ();
  if (!pid) { /* must be child process */
    dup2 (wfd[0], STDIN_FILENO);
    dup2 (rfd[1], STDOUT_FILENO);
    dup2 (rfd[1], STDERR_FILENO);
    setvbuf (stdin,  (char *) NULL, _IONBF, BUFSIZ); 
    setvbuf (stdout, (char *) NULL, _IONBF, BUFSIZ);
    setvbuf (stderr, (char *) NULL, _IONBF, BUFSIZ);

    status = execvp (arglist[0], arglist);
    fprintf (stderr, "error starting command %s\n", arglist[0]);
    exit (1);
  }

  eof = 0x04;
  if (write (wfd[1], &eof, 1) != 1) fprintf (stderr, "!\n");
  close (wfd[1]);
 
  /* ok to close these here? */
  close (rfd[1]);
  close (wfd[0]);

  fcntl (rfd[0], F_SETFL, O_NONBLOCK);

  done = FALSE;
  exit_good = exit_stat = FALSE;
  for (i = 0; !done && (i < 10*timeout); i++) {
    Nread = read (rfd[0], buffer, 0x4000);
    if (Nread > 0) {
      fwrite (buffer, 1, Nread, LogFile);
    }
    fflush (LogFile);
    pidstat = waitpid (pid, &status, WNOHANG);
    if (pidstat == pid) { 
      done = TRUE;
      exit_good = WIFEXITED (status);
      exit_stat = WEXITSTATUS (status);
    } else {
      usleep (100000);
    }
  }
  
  if (i == 10*timeout) {
    exit_good = FALSE;
    fprintf (LogFile, "timeout on %s\n", arglist[0]);
    fflush (LogFile);
    fprintf (stderr, "timeout on %s\n", arglist[0]);
    kill (pid, SIGKILL);

    done = FALSE;
    for (i = 0; !done && (i < 2*timeout); i++) {
      pidstat = waitpid (pid, &status, WNOHANG);
      if (pidstat == pid) { 
	done = TRUE;
      } else {
	usleep (100000);
      }
    }
  }
  
  close (rfd[0]);
  freeargs (arglist);
  if (exit_good) return (exit_stat);
  return (1);
}
    
/* split line into words by spaces, end with NULL */
char **getargs (char *input) {

  char **args, *line, *p, *e;
  int Nargs, N, Nchar;

  line = strcreate (input);
  stripwhite (line);
  p = line;

  /* count words in line */
  Nargs = 0;
  while (*p != 0) {
    for (; (*p != 0) && !isspace (*p); p++);
    for (; isspace (*p); p++);
    Nargs ++;
  }
  Nargs ++;
  ALLOCATE (args, char *, Nargs);

  /* extract words from line */
  N = 0;
  e = line;
  p = line;
  while (*p != 0) {
    for (; (*e != 0) && !isspace (*e); e++);
    Nchar = e - p;
    ALLOCATE (args[N], char, Nchar + 1);
    strncpy_nowarn (args[N], p, Nchar);
    p = e;
    for (; isspace (*p); p++);
    e = p;
    N ++;
  }
  args[N] = (char *) NULL;
  free (line);
  return (args);
}

int freeargs (char **arglist) {

  int i;

  for (i = 0; arglist[i] != (char *) NULL; i++) {
    free (arglist[i]);
  }
  free (arglist);
  return (TRUE);
}

char *ExpandWords (char *line) {

  int Nin, Nout, Ncpy, NBYTES;
  char *p1, *p2, word[256], value[256];
  char *outline;
  
  NBYTES = 256;
  ALLOCATE (outline, char, NBYTES);
  Nout = 0;
  Nin  = 0;
  while ((p1 = strchr (&line[Nin], '&')) != (char *) NULL) {
    Ncpy = p1 - line - Nin;
    if (Nout + Ncpy >= NBYTES) {
      NBYTES = Nout + Ncpy + 256;
      REALLOCATE (outline, char, NBYTES - 2);
    }
    memcpy (&outline[Nout], &line[Nin], Ncpy);
    Nout += Ncpy;
    
    p1 ++;
    for (p2 = p1; isalnum (*p2); p2++);
    memcpy (word, p1, p2 - p1);
    word[p2-p1] = 0;
    Nin += Ncpy + 1 + p2 - p1;
    
    /* substitute value for word */
    bzero (value, 256);
    if (!strcasecmp (word, "DATE")) strcpy (value, DateStr);
    if (!strcasecmp (word, "TIME")) strcpy (value, TimeStr);

    Ncpy = strlen(value);
    if (Nout + Ncpy >= NBYTES - 2) {
      NBYTES = Nout + Ncpy + 256;
      REALLOCATE (outline, char, NBYTES);
    }    
    memcpy (&outline[Nout], value, Ncpy);
    Nout += Ncpy;
  }
  Ncpy = strlen(&line[Nin]);
  if (Nout + Ncpy >= NBYTES - 2) {
    NBYTES = Nout + Ncpy + 256;
    REALLOCATE (outline, char, NBYTES);
  }  
  memcpy (&outline[Nout], &line[Nin], Ncpy);
  Nout += Ncpy;
  outline[Nout] = 0;
  free (line);
  return (outline);

}
