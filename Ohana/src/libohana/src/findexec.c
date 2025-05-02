# include <ohana.h>

int check_file_exec (char *filename) {
  
  struct stat filestat;
  uid_t uid;
  gid_t gid;
  int status;

  uid = getuid();
  gid = getgid();

  /* check permission to exec file */
  status = stat (filename, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR) && (filestat.st_mode & S_IXUSR)) ||
	((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP) && (filestat.st_mode & S_IXGRP)) || 
	(                            (filestat.st_mode & S_IROTH) && (filestat.st_mode & S_IXOTH))) {
      return (TRUE);
    } else {
      return (FALSE);
    }
  }
  return (FALSE);
}

/* check that:
   - dir exists
   - dir permissions OK
*/
int check_dir_access (char *path, int VERBOSE) {
  
  struct stat filestat;
  uid_t uid;
  gid_t gid;
  int status, cmode;

  uid = getuid();
  gid = getgid();

  /* check permission to write to directory */
  status = stat (path, &filestat);
  if (status == -1) {
    if (VERBOSE) fprintf (stderr, "directory %s does not exist, creating...\n", path);
    cmode = S_IRWXU | S_IRWXG | S_IRWXO;
    status = mkdirhier (path, cmode);
    if (status == -1) {
      if (VERBOSE) fprintf (stderr, "can't create %s\n", path);
      return (FALSE);
    }
  } 
  status = stat (path, &filestat);
  if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRWXU)) ||
      ((gid == filestat.st_gid) && (filestat.st_mode & S_IRWXG)) || 
      (filestat.st_mode & S_IRWXO)) {
  } else {
    if (VERBOSE) fprintf (stderr, "can't write to %s\n", path);
    return (FALSE);
  }
  return (TRUE);
}

/* check that file can be written to:
   - dir exists
   - dir permissions OK
   - file permissions OK, 
   - file backup permission OK (optional)
*/
int check_file_access (char *basefile, int BACKUP, int READWRITE, int VERBOSE) {
  
  char *path, *filename;
  struct stat filestat;
  uid_t uid;
  gid_t gid;
  int status;
  int valid;

  myAssert (basefile, "oops");
  myAssert (basefile[0], "oops");

  uid = getuid();
  gid = getgid();

  // XXX this function needs to call 'getgroups' to get the full list of the user's
  // groups.  we would then need to loop over all groups in the gid test below 
  // to see if any match the file.  test to see how slow this is.

  /* check permission to write to directory */
  path = pathname (basefile);
  status = check_dir_access (path, VERBOSE);
  free (path);
  if (!status) return (FALSE);
  
  /* check permission to write to file */
  status = stat (basefile, &filestat);
  if (status == 0) { /* file exists, are permissions OK? */
    valid = FALSE;
    if (!valid && (uid == filestat.st_uid)) {
      valid = (filestat.st_mode & S_IRUSR) != 0;
      valid &= !READWRITE || (filestat.st_mode & S_IWUSR);
    }
    if (!valid && (gid == filestat.st_gid)) {
      valid = (filestat.st_mode & S_IRGRP) != 0;
      valid &= !READWRITE || (filestat.st_mode & S_IWGRP);
    }
    if (!valid) {
      valid = (filestat.st_mode & S_IROTH) != 0;
      valid &= !READWRITE || (filestat.st_mode & S_IWOTH);
    }
    if (!valid) {
      if (VERBOSE) fprintf (stderr, "can't write to %s\n", basefile);
      return (FALSE);
    }
  }
  
  /* check permission to write to backup file */
  if (BACKUP) {
    int Nchar = strlen(basefile) + 16;
    ALLOCATE (filename, char, Nchar);
    snprintf (filename, Nchar, "%s~", basefile);
    status = stat (filename, &filestat);
    if (status == 0) { /* file exists, are permissions OK? */
      valid = FALSE;
      if (!valid && (uid == filestat.st_uid)) {
	valid = (filestat.st_mode & S_IRUSR) != 0;
	valid &= !READWRITE || (filestat.st_mode & S_IWUSR);
      }
      if (!valid && (gid == filestat.st_gid)) {
	valid = (filestat.st_mode & S_IRGRP) != 0;
	valid &= !READWRITE || (filestat.st_mode & S_IWGRP);
      }
      if (!valid) {
	valid = (filestat.st_mode & S_IROTH) != 0;
	valid &= !READWRITE || (filestat.st_mode & S_IWOTH);
      }
      if (!valid) {
	if (VERBOSE) fprintf (stderr, "can't write to %s\n", filename);
	return (FALSE);
      }
    }
    free (filename);
  }
  return (TRUE);
}

/* pathname:
   given path/filename, returns path
   given just filename, returns . 
   given path1/path2/,  returns path1
*/

char *pathname (char *infile) {
 
  int i;
  char *c, *file;

  myAssert (strlen(infile), "invalid zero-length infile");

  /* make working version */
  file = strcreate (infile);

  /* strip off trailing / */
  for (i = strlen (file); (i > 0) && (file[i] == '/'); i--) file[i] = 0;

  c = strrchr (file, '/');
  if (c == (char *) NULL) {
    strcpy (file, ".");
  } else {
    *c = 0;
  }
  
  return (file);
  
}

/* filerootname

   given /path/file.ext return file 
   given /path/file return file 
   given /path/file/ return file 
*/

char *filerootname (char *infile) {

  int i;
  char *file, *root, *p1, *p2;

  /* make working version */
  file = strcreate (infile);

  /* strip off trailing / */
  for (i = strlen (file); (i > 0) && (file[i] == '/'); i--) file[i] = 0;

  /* find last / */
  p1 = strrchr (file, '/');
  if (p1 == (char *) NULL) 
    p1 = file;
  else
  p1 ++;

  /* find last . */
  p2 = strrchr (file, '.');
  if (p2 == (char *) NULL) p2 = p1 + strlen(p1);

  /* create new string, free working space */
  root = strncreate (p1, p2-p1);
  free (file);

  return (root);

}  

/* fileextname

   given /path/file.ext return ext 
   given /path/file return NULL 
   given /path/file/ return NULL 
   given file.ext return ext
*/

char *fileextname (char *file) {

  char *root, *p1, *p2;

  /* find last / */
  p1 = strrchr (file, '/');
  if (p1 == (char *) NULL) p1 = file;

  /* find last . after p1 */
  p2 = strrchr (p1, '.');
  if (p2 == (char *) NULL) return ((char *) NULL);
  p2 ++;

  /* create new string, free working space */
  root = strncreate (p2, strlen(p2));
  return (root);

}  

/* given /path/file.ext return file.ext */
char *filebasename (char *name) {
 
  char *c, *file;

  ALLOCATE (file, char, strlen(name) + 16);

  c = strrchr (name, '/');
  if (c == (char *) NULL) {
    strcpy (file, name);
  } else {
    strcpy (file, c+1);
  }
  
  return (file);
  
}

# define OHANA_MAX_PATH 1024
# define OHANA_MAX_NAME 1280
char *findexec (int argc, char **argv) {

  int i, N, done, status;
  char *c, *e, *dir, path[OHANA_MAX_PATH], name[OHANA_MAX_NAME];

  /* if given an absolute or relative path, use it */
  if (strchr (argv[0], '/') != (char *) NULL) {
    status = check_file_exec (argv[0]);
    if (status) {
      if (realpath (argv[0], path) == (char *) NULL) return ((char *) NULL);
      dir = pathname (path);
      return (dir);
    } else {
      return ((char *) NULL);
    }
  }
  N = 0;
  for (i = argc+1; argv[i] != (char *) NULL; i++) {
    if (!strncmp (argv[i], "PATH", 4)) {
      N = i;
      break;
    }
  }

  if (N) {
    c = &argv[N][5];
    e = strchr (c, ':');
    done = FALSE;
    i = 0;
    while (!done) {
      bzero (path, OHANA_MAX_PATH);
      if (e == (char *) NULL) {
	done = TRUE;
	strncpy_nowarn (path, c, strlen(c));
      } else {
	strncpy_nowarn (path, c, e-c);
	c = e+1;
	e = strchr (c, ':');
      }
      snprintf (name, OHANA_MAX_NAME, "%s/%s", path, argv[0]);
      status = check_file_exec (name);

      if (status) {
	if (realpath (name, path) == (char *) NULL) continue;
	dir = pathname (path);
	return (dir);
      }
    }
  }
  return ((char *) NULL);
}

/* make directory hierarchy, 0: success, -1: failure (just like mkdir) */
int mkdirhier (char *path, int mode) {

  char *tpath;

  /* force addition of user exec/read/write */
  mode |= S_IRWXU;
  errno = 0; // If mkdirhier succeeds, errno must be 0
  if (mkdir (path, mode)) {
    if (errno == ENOENT) { 
      tpath = pathname (path);
      if (!mkdirhier (tpath, mode)) {
	free (tpath);
	if (mkdir (path, mode)) {
	  return (-1);
	} else {
	  errno = 0;
	  return (0);
	}
      } else {
	free (tpath);
	return (-1);
      }
    } else {
      return (-1);
    }
  } else {
    return (0);
  }
}

char *getcwd_cfht (char *path, int size) {
  
  char *hostname, *newpath, *p;

  path = getcwd (path, size);
  
  if (!strncmp (path, "/local/data", strlen ("/local/data"))) {
    
    ALLOCATE (hostname, char, size);

    if (gethostname (hostname, size-1)) {
      fprintf (stderr, "ERROR: can't get hostname\n");
      free (hostname);
      return ((char *) NULL);
    }
    if ((p = strchr (hostname, '.')) != (char *) NULL) *p = 0;
    
    ALLOCATE (newpath, char, size);
    /* path might be just /local/data or /local/data/ */
       
    p = path + strlen ("/local/data/");
    if (strlen (path) <= strlen ("/local/data/")) {
      snprintf (newpath, size, "/data/%s", hostname);
    } else {
      snprintf (newpath, size, "/data/%s/%s", hostname, p);
    }      

    free (hostname);
    strcpy (path, newpath);

    free (newpath);

  }

  return (path);

}

void make_backup (char *filename) {

  int status, cmode;
  struct stat filestat;
  char line[1024];

  status = stat (filename, &filestat);
  if (status == 0) { /* file exists, make backup copy */
    snprintf (line, 1024, "cp %s %s~", filename, filename);
    status = system (line);
    if (status) {
      fprintf (stderr, "ERROR: unable to create %s~, exiting\n", filename);
      exit (1);
    }
    cmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    snprintf (line, 1024, "%s~", filename);
    chmod (line, cmode);
  }
}

/* fseek with timeout - 0.5 sec */
int Fseek (FILE *f, off_t offset, int whence) {

  int status, k;

  status = fseeko (f, offset, whence);
  if (status == -1) {
    for (k = 0; (k < 10) && ((status = fseeko (f, 0, SEEK_SET)) == -1); k++) usleep (50000);
    if (status == -1) {
      return FALSE;
    }
  }
  return TRUE;
}

// ohana-based equivalent to 'realpath'.  realpath does not guarantee an absolute path (eg on Solaris), 
// just a 'cannonical path'.  I don't care about resolving out links, just ensuring an absolute path
// also, realpath is a bit ambiguous on the maxlength argument
char *abspath (char *oldpath, int maxlength) {

  if (!oldpath) return NULL;

  if (oldpath[0] == '/') {
    char *newpath = strcreate (oldpath);
    return newpath;
  }

  char *cwd  = getcwd(NULL, maxlength);
  if (cwd == NULL) {
    // XXX need an error reporting function...
    fprintf (stderr, "error getting cwd (longer than %d chars?)\n", maxlength);
    return NULL;
  }

  char *newpath = NULL;

  int Nbytes = strlen(cwd) + strlen(oldpath) + 2;
  ALLOCATE (newpath, char, Nbytes);
  snprintf (newpath, Nbytes, "%s/%s", cwd, oldpath);

  real_free (cwd);
  return newpath;
}
