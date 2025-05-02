# include "elixir.h"

/* take entry and return char with filled out value,
   perform recursively */

char *ExpandEntry (char *entry, int argc, char **argv) {

  char *p1, *p2, *p3;
  int Nbyte, N;
  char *function, *operand, *value;
  int Nout, Ncpy;

  if (entry[0] == '!') {
    p1 = strchr (entry, '!'); p1++;
    p2 = strchr (entry, '(');
    p3 = strchr (entry, ')');
    if ((p2 == (char *) NULL) || (p3 == (char *) NULL)) {
      fprintf (stderr, "syntax error in process defs\n");
      Shutdown (1);
    }
    Nbyte = p2 - p1;
    function = strncreate (p1, Nbyte);

    p2 ++;
    Nbyte = p3 - p2;
    operand = strncreate (p2, Nbyte);
    value = ExpandEntry (operand, argc, argv); 
    free (operand);
    operand = value; 
    
    value = (char *) NULL;
    if (!strcasecmp (function, "buildname")) {
      value = BuildName (operand);
    }
    if (!strcasecmp (function, "buildcode")) {
      value = BuildCode (operand);
    }
    if (!strcasecmp (function, "photcode")) {
      value = GetPhotcode (operand);
    }
    if (!strcasecmp (function, "photcodemef")) {
      value = GetPhotcodeMef (operand);
    }
    if (!strcasecmp (function, "root")) {
      value = RootFilename (operand);
    }
    if (!strcasecmp (function, "path")) {
      value = PathFilename (operand);
    }
    if (!strcasecmp (function, "base")) {
      value = BaseFilename (operand);
    }
    if (value == (char *) NULL) {
      fprintf (stderr, "unknown process command %s\n", function);
      Shutdown (1);
    }
    free (function); 
    free (operand); 
  } else {
    Nbyte = strlen (entry) + 50;
    ALLOCATE (value, char, Nbyte);

    Nout = 0;
    p1 = entry;
    while ((p2 = strchr (p1, '&')) != (char *) NULL) {
      Ncpy = p2 - p1;
      if (Nout + Ncpy >= Nbyte - 1) {
	Nbyte = Nout + Ncpy + 50;
	REALLOCATE (value, char, Nbyte);
      }    
      strncpy_nowarn (&value[Nout], p1, Ncpy);
      Nout += Ncpy;
      p2 ++;
      N = strtod (p2, &p1);
      if ((N > argc - 1) || (N < 0)) {
	fprintf (stderr, "ERROR: command expects too many object arguments\n");
	Shutdown (1);
      }
      Ncpy = strlen (argv[N]);
      if (Nout + Ncpy >= Nbyte - 1) {
	Nbyte = Nout + Ncpy + 50;
	REALLOCATE (value, char, Nbyte);
      }    
      strcpy (&value[Nout], argv[N]);
      Nout += Ncpy;
    }
    Ncpy = strlen (p1);
    if (Nout + Ncpy >= Nbyte - 1) {
      Nbyte = Nout + Ncpy + 50;
      REALLOCATE (value, char, Nbyte);
    }    
    strncpy_nowarn (&value[Nout], p1, Ncpy);
    Nout += Ncpy;
  }

  return (value);

}

Process *ConfigProcess (char *config, char *procname) {
  
  int j, NPAR;
  char argname[256], argline[256];
  Process *process;

  NPAR = 10;
  ALLOCATE (process, Process, 1);
  ALLOCATE (process[0].argv, char *, NPAR);

  process[0].name = strcreate (procname);

  /* find all lines of form procname.arg - the argument lines */
  sprintf (argname, "%s.arg", procname);
  for (j = 0; ScanConfig (config, argname, "%s", j+1, argline); j++) {
    process[0].argv[j] = strcreate (argline);
    if (j == NPAR) {
      NPAR += 10;
      REALLOCATE (process[0].argv, char *, NPAR);
    }
  }
  process[0].argc = j;
  REALLOCATE (process[0].argv, char *, MAX (j,1));
  if (j == 0) {
    fprintf (stderr, "ERROR: process %s has no arguments defined\n", procname);
    Shutdown (1);
  }
    
  process[0].pending = InitQueue ();
  process[0].failure = (Queue *) NULL;
  process[0].success = (Queue *) NULL;
  process[0].cluster = InitCluster ();

  return (process);
  
}

/* need a return value */
int MakeArgs (Process *process, Object *object, int *cargc, char ***cargv, int **cargd) {

  int i;
  int Cargc, *Cargd;
  char **Cargv;

  Cargc = process[0].argc;
  ALLOCATE (Cargv, char *, Cargc);
  ALLOCATE (Cargd, int, Cargc);
  
  /* convert par to entry */
  for (i = 0; i < Cargc; i++) {
    ParseLine (process[0].argv[i], object[0].argc, object[0].argv, &Cargd[i], &Cargv[i]);
  }
  *cargc = Cargc;
  *cargv = Cargv;
  *cargd = Cargd;

  return (TRUE);

}

/* 'process' carries around a list of strings which MakeArgs uses to create the command line
   arguments and the dependencies (cargc, cargv, cargd) */

void FreeArgs (int argc, char **argv, int *argd) {

  int i;

  for (i = 0; i < argc; i++) {
    free (argv[i]);
  }

  free (argv);
  free (argd);

}


void ParseLine (char *testline, int argc, char **argv, int *depend, char **outline) {

  int i, k;
  char format[256], *line, *value;
  int NTERMS, Nterms;
  char **terms;
  

  NTERMS = 8;
  ALLOCATE (terms, char *, NTERMS);
  for (i = 0; i < NTERMS; i++) {
    ALLOCATE (terms[i], char, 256);
  }

  Nterms = sscanf (testline, "%d%s%s%s%s%s%s%s%s%s", depend, format, 
		   terms[0], terms[1], terms[2], terms[3], terms[4], terms[5], terms[6], terms[7]);
  Nterms -= 2;
      
  for (k = 0; k < Nterms; k++) {
    value = ExpandEntry (terms[k], argc, argv);
    strcpy (terms[k], value);
    free (value);
  }
      
  ALLOCATE (line, char, 256);
  switch (Nterms) {
  case 0:
    sprintf (line, "%s", format); 
    break;
  case 1:
    sprintf (line, format, terms[0]); 
    break;
  case 2:
    sprintf (line, format, terms[0], terms[1]); 
    break;
  case 3:
    sprintf (line, format, terms[0], terms[1], terms[2]); 
    break;
  case 4:
    sprintf (line, format, terms[0], terms[1], terms[2], terms[3]); 
    break;
  case 5:
    sprintf (line, format, terms[0], terms[1], terms[2], terms[3], terms[4]); 
    break;
  case 6:
    sprintf (line, format, terms[0], terms[1], terms[2], terms[3], terms[4], terms[5]); 
    break;
  case 7:
    sprintf (line, format, terms[0], terms[1], terms[2], terms[3], terms[4], terms[5], terms[6]); 
    break;
  case 8:
    sprintf (line, format, terms[0], terms[1], terms[2], terms[3], terms[4], terms[5], terms[6], terms[7]); 
    break;
  default: 
    fprintf (stderr, "too many terms for command line argument (lim = 5)\n");
    Shutdown (1);
  }
  *outline = line;

  for (i = 0; i < NTERMS; i++) { free (terms[i]); }
  free (terms);

}
