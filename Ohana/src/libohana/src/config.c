# include <ohana.h>

# define D_NBYTES 4096
# define NXTRA 16

static char *ConfigVariable = (char *) NULL;
static int NDefineVariable;
static char **DefineVariable;
static char **DefineValue;

void FreeConfigFile (void) {
  int i;
  for (i = 0; i < NDefineVariable; i++) {
    free (DefineVariable[i]);
    free (DefineValue[i]);
  }
  free (DefineVariable);
  free (DefineValue);
  free (ConfigVariable);
}

char *SelectConfigFile (int *argc, char **argv, char *progname) {
  
  /* 
     config file selection rules (first ones override later ones):

     1) -c Filename   : use Filename
     2) PROGNAME      : use environment variable as config file
     3) progname.rc   : use alternate name in local dir as config file
     4) .prognamerc   : use rc file in local dir as config file
     5) ~/.prognamerc : use rc file in homedir as config file

     special variable definitions:
     1) -C WORD       : set CONFIG variable to WORD
     2) -D NAME WORD  : set NAME variable to WORD
        -D overrides variables in param file

     these command-line options are removed and a complete arg list left behind
  */

  char *filename, *find, *home;
  struct stat filestat;
  uid_t uid;
  gid_t gid;
  int unsigned i, N, NDEF, status;
  
  /* first look for -C CONFIG variable */
  if ((N = get_argument (*argc, argv, "-C"))) {
    remove_argument (N, argc, argv);
    if (ConfigVariable != (char *) NULL) free (ConfigVariable);
    ConfigVariable = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
    
  /* next look for -D NAME WORD variables */
  NDEF = 10;
  ALLOCATE (DefineVariable, char *, NDEF);
  ALLOCATE (DefineValue, char *, NDEF);
  for (i = 0; (N = get_argument (*argc, argv, "-D")); i++) {
    remove_argument (N, argc, argv);
    DefineVariable[i] = strcreate (argv[N]);
    remove_argument (N, argc, argv);
    DefineValue[i] = strcreate (argv[N]);
    remove_argument (N, argc, argv);
    if (i == NDEF - 1) {
      NDEF += 10;
      REALLOCATE (DefineVariable, char *, NDEF);
      REALLOCATE (DefineValue, char *, NDEF);
    }      
  }    
  NDefineVariable = i;
  REALLOCATE (DefineVariable, char *, MAX (1, i));
    
  /* look for -c FILENAME for config file */
  if ((N = get_argument (*argc, argv, "-c"))) {
    remove_argument (N, argc, argv);
    filename = strcreate (argv[N]);
    remove_argument (N, argc, argv);
    return (filename);
  }
  
  /* look for PROGNAME env var */
  find = strcreate (progname);
  for (i = 0; i < strlen(find); i++) find[i] = toupper (find[i]);
  filename = getenv (find);
  free (find);
  if (filename != (char *) NULL) {
    find = strcreate (filename);
    return (find);
  }

  uid = getuid();
  gid = getgid();

  int Nchar = strlen(progname) + 32;
  ALLOCATE (find, char, Nchar);

  /* look for progname.rc */
  snprintf (find, Nchar, "%s.rc", progname);
  status = stat (find, &filestat);
  if (status == 0) { 
    /* file exists, can we read it? */
    if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR)) ||
	((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP)) || 
	(                            (filestat.st_mode & S_IROTH))) {
      return (find);
    }
  }
  free (find);

  /* look for ~/.prognamerc */
  home = getenv ("HOME");
  if (home == (char *) NULL) { return ((char *) NULL); }
  ALLOCATE (find, char, 1024);
  snprintf (find, 1024, "%s/.%src", home, progname);
  status = stat (find, &filestat);
  if (status == 0) { 
    /* file exists, can we read it? */
    if (((uid == filestat.st_uid) && (filestat.st_mode & S_IRUSR)) ||
	((gid == filestat.st_gid) && (filestat.st_mode & S_IRGRP)) || 
	(                            (filestat.st_mode & S_IROTH))) {
      return (find);
    }
  }
  free (find);
  return ((char *) NULL);
}

char *LoadConfigFile (char *filename) {
  
  FILE *f;
  int i, Nbytes, NBYTES, nbytes, Nout, Ncpy, INPUT, Nlevel;
  char *ibuffer, *obuffer, *tbuffer;
  char *last, *next;
  char infile[256], line[256];
  
  /* open file */
  f = fopen (filename, "r");
  // if (f == NULL) {
  // return ((char *) NULL);
  // }
 
  /* allocate tmp space, 2 extra bytes for a final return and EOL */
  Nbytes = 0;
  NBYTES = D_NBYTES;
  ALLOCATE (ibuffer, char, NBYTES + NXTRA);
  memset (ibuffer, 0, NBYTES + NXTRA);
    
  /* load data from file */
  if (f) {
    while ((nbytes = fread (&ibuffer[Nbytes], sizeof(char), D_NBYTES, f)) == D_NBYTES) {
      Nbytes += nbytes;
      NBYTES += D_NBYTES;
      REALLOCATE (ibuffer, char, NBYTES + NXTRA);
    }
    Nbytes += nbytes;
    fclose (f);
  }

  /* add final return & EOL if non-existent */
  if (ibuffer[Nbytes-1] != '\n') {
    ibuffer[Nbytes] = '\n';
    Nbytes ++;
  }
  if (ibuffer[Nbytes]) ibuffer[Nbytes] = 0;
  myAssert (Nbytes < NBYTES, "oops");

  /* add the optional variables to the end of the input buffer */
  if (ConfigVariable != (char *) NULL) {
    snprintf (line, 256, "%s %s\n", "CONFIG", ConfigVariable);
    Ncpy = strlen (line);
    if (Nbytes + Ncpy >= NBYTES - NXTRA) {
      NBYTES = Nbytes + Ncpy + D_NBYTES;
      REALLOCATE (ibuffer, char, NBYTES);
    }    
    memcpy (&ibuffer[Nbytes], line, Ncpy);
    Nbytes += Ncpy;
    ibuffer[Nbytes] = 0;
  }
  for (i = 0; i < NDefineVariable; i++) {
    snprintf (line, 256, "%s %s\n", DefineVariable[i], DefineValue[i]);
    Ncpy = strlen (line);
    if (Nbytes + Ncpy >= NBYTES - NXTRA) {
      NBYTES = Nbytes + Ncpy + D_NBYTES;
      REALLOCATE (ibuffer, char, NBYTES);
    }    
    memcpy (&ibuffer[Nbytes], line, Ncpy);
    Nbytes += Ncpy;
    ibuffer[Nbytes] = 0;
  }
  myAssert (Nbytes < NBYTES, "oops");
  
  /* loop over input buffer, interpolating 'input' lines until none are added */
  Nlevel = 0;
  do {
    INPUT = FALSE;

    /* allocate output buffer & set counter */
    NBYTES = Nbytes + D_NBYTES;
    ALLOCATE (obuffer, char, NBYTES);
    Nout = 0;
    
    /* copy from ibuffer to obuffer, interpolating 'input' lines */
    last = next = ibuffer;
    for (i = 1; (next = ScanConfig (ibuffer, "input", "%s", i, infile)) != (char *) NULL; i++) {
      /* copy data from last point to before 'input' */ 
      Ncpy = next - last - 5;
      if (Nout + Ncpy >= NBYTES - NXTRA) {
	NBYTES = Nout + Ncpy + D_NBYTES;
	REALLOCATE (obuffer, char, NBYTES);
      }      
      memcpy (&obuffer[Nout], last, Ncpy);
      Nout += Ncpy;
      obuffer[Nout] = 0;
      
      /* insert data from 'input' file */
      tbuffer = LoadRawConfigFile (infile, FALSE);
      if (tbuffer != (char *) NULL) {
	Ncpy = strlen (tbuffer);
	if (Nout + Ncpy >= NBYTES - NXTRA) {
	  NBYTES = Nout + Ncpy + D_NBYTES;
	  REALLOCATE (obuffer, char, NBYTES);
	}      
	memcpy (&obuffer[Nout], tbuffer, Ncpy);
	free (tbuffer);
	Nout += Ncpy;
	obuffer[Nout] = 0;
      }
      
      /* pointer goes to end of input line */
      last = strchr (next, '\n');
      if (last == (char *) NULL) break;
      last ++;
      INPUT = TRUE;
    }
    /* last set of bytes after last input */
    Ncpy = strlen (last);
    if (Nout + Ncpy >= NBYTES - NXTRA) {
      NBYTES = Nout + Ncpy + D_NBYTES;
      REALLOCATE (obuffer, char, NBYTES);
    }      
    memcpy (&obuffer[Nout], last, Ncpy);
    Nout += Ncpy;
    obuffer[Nout] = 0;
    free (ibuffer);
    ibuffer = obuffer;
    Nlevel ++;
  } while (INPUT && (Nlevel < 20));
  if (Nlevel == 20) {
    fprintf (stderr, "warning: config reached max depth of 20\n");
  }

  /* 'obuffer' now has complete set of interpolated lines from 'filename' */
  return (obuffer);
}

char *ScanConfig (char *config, char *field, char *mode, int Nentry, ...) {
  
  int i;
  char *p, *p2, *tmp, *tfield, *start, *expandline();
  va_list argp;
  double value;
  
  if (config == (char *) NULL) return ((char *) NULL);
  va_start (argp, Nentry);

  int Nchar = strlen (field) + NXTRA;
  ALLOCATE (tfield, char, Nchar);
  snprintf (tfield, Nchar, "\n%s", field);

  /* we search for Nentry matching fields,
     or until the end if Nentry == 0 */
  p = (char *) NULL;
  p2 = config;
  for (i = 0; (i < Nentry) || !Nentry;) {
    tmp = strstr (p2, tfield);
    if (tmp == (char *) NULL) {
      break;
    }
    p2 = tmp + strlen (tfield);
    if (OHANA_WHITESPACE (*p2)) {
      p = p2;
      i++;
    }
  }
  free (tfield);
  if (Nentry && (i != Nentry)) {
    p = va_arg (argp, char *);
    p[0] = 0;
    return ((char *) NULL);
  }
  if (p == (char *) NULL) {
    p = va_arg (argp, char *);
    p[0] = 0;
    return ((char *) NULL);
  }

  start = p;
  if (!strcmp (mode, "%s")) {
    p2 = strchr (p, '\n');
    if (p2 == (char *) NULL) p2 = config + strlen(config);
    ALLOCATE (tmp, char, p2-p + 2);
    memcpy (tmp, p, (p2-p));
    tmp[(p2-p)] = 0;
    stripwhite (tmp);
    tmp = expandline (tmp, config);
    p2 = va_arg (argp, char *);
    strcpy (p2, tmp);
    free (tmp);
  } else {
 
    /* try to get a numerical value from the field */
    value = strtod (p, &p2);
    if ((*p2 == 'd') || (*p2 == 'D')) 
      value *= pow (10.0, atof (p2 + 1));
    
    if (!strcmp (mode, "%d"))  *va_arg (argp, int *)       = value;
    if (!strcmp (mode, "%u"))  *va_arg (argp, unsigned *)  = value;
    if (!strcmp (mode, "%ld")) *va_arg (argp, long *)      = value;
    if (!strcmp (mode, "%hd")) *va_arg (argp, short *)     = value;
    if (!strcmp (mode, "%f"))  *va_arg (argp, float *)     = value;
    if (!strcmp (mode, "%lf")) *va_arg (argp, double *)    = value;
    
  }
 
  va_end (argp);
  return (start);
  
}

char *expandline (char *line, char *config) {

  int Nin, Nout, Ncpy;
  char *p1, *p2, word[256], value[256];
  char *outline;
  
  int DBYTES = 256;
  int NBYTES = 256;
  ALLOCATE (outline, char, NBYTES);
  Nout = 0;
  Nin  = 0;
  while ((p1 = strchr (&line[Nin], '$')) != (char *) NULL) {
    Ncpy = p1 - line - Nin;
    if (Nout + Ncpy >= NBYTES - NXTRA) {
      NBYTES = Nout + Ncpy + DBYTES;
      REALLOCATE (outline, char, NBYTES);
    }
    memcpy (&outline[Nout], &line[Nin], Ncpy);
    Nout += Ncpy;
    
    p1 ++;
    for (p2 = p1; isalnum (*p2) || (*p2 == '_') || (*p2 == '-'); p2++);
    memcpy (word, p1, p2 - p1);
    word[p2-p1] = 0;
    Nin += Ncpy + 1 + p2 - p1;
    
    /* search for last entry of word */
    if (!ScanConfig (config, word, "%s", 0, value)) {
      fprintf (stderr, "variable %s not found in config file\n", word);
      return (line);
    }
    Ncpy = strlen(value);
    if (Nout + Ncpy >= NBYTES - NXTRA) {
      NBYTES = Nout + Ncpy + DBYTES;
      REALLOCATE (outline, char, NBYTES);
    }    
    memcpy (&outline[Nout], value, Ncpy);
    Nout += Ncpy;
  }
  Ncpy = strlen(&line[Nin]);
  if (Nout + Ncpy >= NBYTES - NXTRA) {
    NBYTES = Nout + Ncpy + DBYTES;
    REALLOCATE (outline, char, NBYTES);
  }  
  memcpy (&outline[Nout], &line[Nin], Ncpy);
  Nout += Ncpy;
  outline[Nout] = 0;
  free (line);
  return (outline);

}

char *LoadRawConfigFile (char *filename, int options) {
  
  FILE *f;
  int i, Nbytes, NBYTES, nbytes, Ncpy;
  char *ibuffer;
  char line[256];
  
  /* open file */
  f = fopen (filename, "r");
  if (f == NULL) {
    return ((char *) NULL);
  }
 
  /* allocate tmp space, 2 extra bytes for a final return and EOL */
  Nbytes = 0;
  NBYTES = D_NBYTES;
  ALLOCATE_ZERO (ibuffer, char, NBYTES + NXTRA);

  /* load data from file */
  while ((nbytes = fread (&ibuffer[Nbytes], sizeof(char), D_NBYTES, f)) == D_NBYTES) {
    Nbytes += nbytes;
    NBYTES += D_NBYTES;
    REALLOCATE (ibuffer, char, NBYTES + NXTRA);
    memset (&ibuffer[NBYTES - D_NBYTES], 0, NBYTES - D_NBYTES);
  }
  Nbytes += nbytes;
  fclose (f);

  /* add final return & EOL, if non-existent */
  if (ibuffer[Nbytes-1] != '\n') {
    ibuffer[Nbytes] = '\n';
    Nbytes ++;
  }
  if (ibuffer[Nbytes]) ibuffer[Nbytes] = 0;
  myAssert (Nbytes < NBYTES, "oops");

  if (options) {
    /* write optional variables to bottom of buffer, overriding entries in the file */
    if (ConfigVariable != (char *) NULL) {
      snprintf (line, 256, "%s %s\n", "CONFIG", ConfigVariable);
      Ncpy = strlen (line);
      if (Nbytes + Ncpy >= NBYTES - NXTRA) {
	int Nbytes_old = NBYTES;
	NBYTES = Nbytes + Ncpy + D_NBYTES;
	REALLOCATE (ibuffer, char, NBYTES + NXTRA);
	memset (&ibuffer[Nbytes_old], 0, NBYTES - Nbytes_old);
      }    
      memcpy (&ibuffer[Nbytes], line, Ncpy);
      Nbytes += Ncpy;
      ibuffer[Nbytes] = 0;
    }
    for (i = 0; i < NDefineVariable; i++) {
      snprintf (line, 256, "%s %s\n", DefineVariable[i], DefineValue[i]);
      Ncpy = strlen (line);
      if (Nbytes + Ncpy >= NBYTES - NXTRA) {
	int Nbytes_old = NBYTES;
	NBYTES = Nbytes + Ncpy + D_NBYTES;
	REALLOCATE (ibuffer, char, NBYTES + NXTRA);
	memset (&ibuffer[Nbytes_old], 0, NBYTES - Nbytes_old);
      }    
      memcpy (&ibuffer[Nbytes], line, Ncpy);
      Nbytes += Ncpy;
      ibuffer[Nbytes] = 0;
    }
  }

  return (ibuffer);
}

/* 
char *LoadConfigFile (char *filename) {
  
  char *config;

  config = LoadSubConfigFile (filename, TRUE);

  return (config);

}

*/
