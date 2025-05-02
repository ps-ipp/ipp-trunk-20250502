# include <ohana.h>
# define D_NBYTES 4096

char *LoadConfigFile (filename) 
char *filename; 
{
  
  FILE *f;
  int i, done, Nbytes, NBYTES, size;
  char *config;
  
  f = fopen (filename, "r");
  if (f == NULL) {
    fprintf (stderr, "couldn't find %s\n", filename);
    return ((char *) NULL);
  }
 
  NBYTES = D_NBYTES;
  ALLOCATE (config, char, NBYTES);
 
  size = 0;
  done = FALSE;
  for (i = 0; !done; i++) {
    Nbytes = Fread (&config[i*D_NBYTES], sizeof(char), D_NBYTES, f, "char");
    size += Nbytes;
    if (Nbytes < D_NBYTES) 
      done = TRUE;
    else {
      NBYTES += D_NBYTES;
      REALLOCATE (config, char, NBYTES);
    }
  }
  
  config[size] = '\n';
  config[size+1] = 0;
  REALLOCATE (config, char, size + 2);
 
  fclose (f);

  return (config);
}

int ScanConfig /* we expect one more field: the pointer to the value requested */
# ifndef ANSI
(config, field, mode, va_alist) 
 char *config; char *field, *mode; va_dcl
# else
(char       config[],
 char       field[],
 char       mode[],...)
# endif
{
 
  int i, j;
  char *p, *p2, tmp[256];
  va_list argp;
  double value;
  
# ifndef ANSI
  va_start (argp);
# else
  va_start (argp, N);
# endif
  
  /* find the correct line with field */
  p2 = config;
  for (i = 0; ; i++) {
    if (!strncmp (field, p2, strlen(field))) {
      p = p2 + strlen (field);
      break;
    }
    else {
      p2 = strchr (p2, '\n');
      if (p2 == (char *) NULL) {
	fprintf (stderr, "entry %s not found in config file\n", field);
	return (FALSE);
      }
      else p2++;
    }
  }
  if (!strcmp (mode, "%s")) {
    p2 = strchr (p, '\n');
    if (p2 == (char *) NULL)
      p2 = config + strlen(config);
    bcopy (p, tmp, (p2-p));
    tmp[(p2-p)] = 0;
    stripwhite (tmp);
    p2 = va_arg (argp, char *);
    strcpy (p2, tmp);
  }
  else {
 
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
  return (TRUE);
  
}
