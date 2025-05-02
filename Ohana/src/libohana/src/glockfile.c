# include <ohana.h>

int fchecklockfile (char *filename, int type, int *state) {
  
  int status;
  char mode[16];
  int fd;
  FILE *f;
  struct stat filestat;
  struct flock filelock;

  f = NULL;

  /* define lock type */
  filelock.l_start  = 0;
  filelock.l_whence = SEEK_SET;
  filelock.l_len    = 0;
  filelock.l_pid    = 0;
  switch (type) {
  case LCK_HARD:
  case LCK_XCLD:
    filelock.l_type   = F_WRLCK;  /* set an exclusive lock */
    strcpy (mode, "r+");
    break;
  case LCK_SOFT:
    filelock.l_type   = F_RDLCK;  /* set a shared lock */
    strcpy (mode, "r");
    break;
  default:
    *state = LCK_INVALID;
    goto failure;
  }

  /* check if file exists */
  status = stat (filename, &filestat);
  if ((status == -1) && (errno == ENOENT)) {
    if (type == LCK_SOFT) {
      *state = LCK_MISSING;
      goto failure;
    } 
    /* otherwise, we need to be able to create file */ 
    strcpy (mode, "w+");
  }
  
  /* try to open file (create if it does not exist) */
  f = fopen (filename, mode);
  if (f == NULL) {
    *state = LCK_ACCESS;
    goto failure;
  }
  fd = fileno (f);

  /* check the lock */
  status = fcntl (fd, F_GETLK, &filelock);
  if (status) {
    *state = LCK_ACCESS;
    goto failure;
  }

  if (filelock.l_pid) {
    if (filelock.l_type == F_WRLCK) {
      fprintf (stderr, "lock (hard) is held by pid %d\n", filelock.l_pid);
    } else {
      fprintf (stderr, "lock (soft) is held by pid %d\n", filelock.l_pid);
    }
  } else {
    fprintf (stderr, "lock is NOT held\n");
  }

  fclose (f);
  return TRUE;

failure:
  fprintf (stderr, "failed to check lock\n");
  if (f        != NULL) fclose (f);
  return FALSE;
}

FILE *fsetlockfile (char *filename, double timeout, int type, int *state) {
  
  int i, nbytes, status;
  char *lockname, buffer[64];
  char *file, *path, mode[3];
  int fd;
  FILE *f, *flock;
  struct stat filestat;
  struct flock filelock;
  struct timeval now, then;

  f = flock = NULL;
  file = path = lockname = NULL;

  /* define lock type */
  filelock.l_start  = 0;
  filelock.l_whence = SEEK_SET;
  filelock.l_len    = 0;
  filelock.l_pid    = getpid ();
  switch (type) {
  case LCK_HARD:
  case LCK_XCLD:
    filelock.l_type   = F_WRLCK;  /* set an exclusive lock */
    strcpy (mode, "r+");
    break;
  case LCK_SOFT:
    filelock.l_type   = F_RDLCK;  /* set a shared lock */
    strcpy (mode, "r");
    break;
  default:
    *state = LCK_INVALID;
    goto failure;
  }

  /* check if file exists */
  status = stat (filename, &filestat);
  if ((status == -1) && (errno == ENOENT)) {
    /* if soft, return LCK_ACCESS */
    if (type == LCK_SOFT) {
      *state = LCK_MISSING;
      goto failure;
    } 
    /* otherwise, we need to be able to create file */ 
    strcpy (mode, "w+");
  }
  
  /* try to open file (create if it does not exist) */
  f = fopen (filename, mode);
  if (f == NULL) {
    *state = LCK_ACCESS;
    goto failure;
  }
  fd = fileno (f);

  /* we first try to set a FS level lock on the file */
  gettimeofday (&then, (void *) NULL);
  while (1) {
    /* try to lock file */
    if (fcntl (fd, F_SETLK, &filelock) != -1) goto got_lock;

    /* check for timeout */
    gettimeofday (&now, (void *) NULL);
    if (DTIME (now, then) > timeout) {
      *state = LCK_TIMEOUT;
      goto failure;
    }
    usleep (10000); /* 10 ms is min utime */
  }
got_lock:

  /* check if blocking hardlock exists */
  if (type == LCK_HARD) {
    /* set up name to lockfile */
    path = pathname (filename);
    file = filebasename (filename);
    int Nchar = strlen (path) + strlen (file) + 16;
    ALLOCATE (lockname, char, Nchar);
    snprintf (lockname, Nchar, "%s/.%s.lck", path, file);

    status = stat (lockname, &filestat);
    if ((status == -1) && (errno == ENOENT)) {
      strcpy (mode, "w+");
    } else {
      strcpy (mode, "r+");
    }

    /* try to open lockfile */
    flock = fopen (lockname, mode);
    if (flock == NULL) {
      *state = LCK_HARDOPEN;
      goto failure;
    }
    fd = fileno (flock);
    
    /* try a few times to lock lockfile (locking the lockfile before checking
       the contents will ensure the data is synced across NFS) */
    for (i = 0; (i < 20) && (fcntl (fd, F_SETLK, &filelock) == -1); i++) usleep (10000);
    if (i == 20) {
      *state = LCK_HARDLOCK;
      goto failure;
    }
    
    /* we've locked the lockfile, now read the contents */
    nbytes = fread (buffer, 1, 4, flock);
    if (nbytes == 4) { /* lock file has a word in it */
      buffer[4] = 0;
      if (!strcmp (buffer, "BUSY")) { 
	*state = LCK_HARDLOCKHARD;
	goto failure;
      }
      /* note that we don't care if the lockfile has random garbage */
    }
  }
    
  /* set blocking hardlock */
  if (type == LCK_HARD) {
    /* we've really got the lock, write BUSY to protect it */
    fseeko (flock, 0LL, SEEK_SET);
    nbytes = fprintf (flock, "BUSY\n");

    /* 
    if (nbytes != 5) {
      *state = LCK_HARDCLOSE;
      goto failure;
      } */
    
    /* now fclose lockfile (also unlocks file) */
    if (fclose (flock)) {
      *state = LCK_HARDCLOSE;
      goto failure;
    }
    flock = NULL;
  }

  /* check if file is empty or not */
  fd = fileno (f);
  if (fstat (fd, &filestat)) {
    *state = LCK_UNKNOWN;
  } else {
    if (filestat.st_size == 0) {
      *state = LCK_EMPTY;
    } else {
      *state = LCK_FULL;
    }
  }

  if (path     != NULL) free (path);
  if (file     != NULL) free (file);
  if (lockname != NULL) free (lockname);
  if (flock    != NULL) fclose (flock);
  return (f);

failure:
  if (f        != NULL) fclose (f);
  if (flock    != NULL) fclose (flock);
  if (path     != NULL) free (path);
  if (file     != NULL) free (file);
  if (lockname != NULL) free (lockname);
  return (NULL);
}
  
/* clears lock. removes hardlock even if file pointer is not supplied */
int fclearlockfile (char *filename, FILE *f, int type, int *state) {

  int i, fd, status, nbytes;
  char *lockname, *path, *file;
  FILE *flock;
  struct stat filestat;
  struct flock filelock;

  file = path = lockname = (char *) NULL;

  /* define lock */
  filelock.l_type   = F_UNLCK;
  filelock.l_start  = 0;
  filelock.l_whence = SEEK_SET;
  filelock.l_len    = 0;
  filelock.l_pid    = getpid ();

  /* first clear hard lockfile */
  if (type == LCK_HARD) {

    /* define lockfile */
    path = pathname (filename);
    file = filebasename (filename);
    int Nchar = strlen (path) + strlen (file) + 16;
    ALLOCATE (lockname, char, Nchar);
    snprintf (lockname, Nchar, "%s/.%s.lck", path, file);
    
    /* check for lockfile existance */
    status = stat (lockname, &filestat);
    if (status == -1) {
      *state = LCK_HARDLOCK;
      goto failure;
    } 
  
    /* try to open lockfile */
    flock = fopen (lockname, "w+");
    if (flock == NULL) {
      *state = LCK_HARDOPEN;
      goto failure;
    }
    fd = fileno (flock);

    /* try a few times to lock lockfile */
    filelock.l_type = F_WRLCK;
    for (i = 0; (i < 20) && (fcntl (fd, F_SETLK, &filelock) == -1); i++) usleep (10000);
    if (i == 20) {
      *state = LCK_HARDLOCKHARD;
      goto failure;
    }

    /* set value to IDLE */
    if (fseeko (flock, 0LL, SEEK_SET)) {
      *state = LCK_HARDCLOSE;
      goto failure;
    }

    nbytes = fprintf (flock, "IDLE\n");
    if (nbytes != 5) {
      *state = LCK_HARDCLOSE;
      goto failure;
    }

    if (fclose (flock)) {
      *state = LCK_HARDCLOSE;
      goto failure;
    }
    flock = NULL;
  }
  
  /* now unlock the file */
  if (fclose (f)) {
    *state = LCK_UNKNOWN;
    goto failure;
  }

  *state = LCK_UNLOCK;
  if (path != NULL)     free (path);
  if (file != NULL)     free (file);
  if (lockname != NULL) free (lockname);
  return (1);

failure:
  if (path != NULL)     free (path);
  if (file != NULL)     free (file);
  if (lockname != NULL) free (lockname);
  return (0);
}
