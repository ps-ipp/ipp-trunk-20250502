# include <stdio.h>
# include <strings.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <stdlib.h>

# define TRUE 1
# define FALSE 0

# ifndef ALLOCATE
# define ALLOCATE(X,T,S)  \
  X=(T *)malloc((unsigned) ((S)*sizeof(T)));\
  if(X==NULL) \
    { \
      fprintf(stderr,"failed to malloc X\n");\
        exit(0);\
    } 
# define REALLOCATE(X,T,S) \
  X=(T *)realloc(X,(unsigned) ((S)*sizeof(T))); \
  if(X==NULL) \
    { \
       fprintf(stderr,"failed to realloc X\n"); \
       exit(0); \
    }
# endif /* ALLOCATE */

int _check_permissions (char *filename) {
  
  FILE *f;
  struct stat filestat;
  uid_t fuid, uid;
  gid_t fgid, gid;
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
      fprintf (stderr, "can't exec %s\n", filename);
      return (FALSE);
    }
  }
  fprintf (stderr, "no such file %s\n", filename);
  return (FALSE);
}

char *pathname (char *name) {
 
  char *c, *path;

  ALLOCATE (path, char, strlen(name) + 1);
  strcpy (path, name);
  c = strrchr (path, '/');
  if (c == (char *) NULL) {
    strcpy (path, ".");
  } else {
    *c = 0;
  }
  
  return (path);
  
}

char *findexec (int argc, char **argv) {

  int i, N, done, status;
  char *c, *e, *dir, path[1024], name[1024];
  struct stat state;

  if (argv[0][0] == '/') {
    status = _check_permissions (argv[0]);
    if (status) {
      realpath (argv[0], path);
      dir = pathname (path);
      return (dir);
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
    while (!done && (i < 50)) {
      if (e == (char *) NULL) {
	done = TRUE;
	bzero (path, 256);
	strncpy_nowarn (path, c, strlen(c));
      } else {
	bzero (path, 256);
	strncpy_nowarn (path, c, e-c);
	c = e+1;
	e = strchr (c, ':');
      }
      sprintf (name, "%s/%s", path, argv[0]);
      status = _check_permissions (name);

      if (status) {
	realpath (name, path);
	dir = pathname (path);
	return (dir);
      }
    }
    i++;
  }
}
