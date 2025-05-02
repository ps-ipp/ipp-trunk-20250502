# include "basic.h"
# define D_NLINES 100

/* module loads an opihi script files from the installed module tree */
int module (int argc, char **argv) {
  
  int i, NLINES, Nmodules, Nbytes, status;
  Macro inlist;
  char modname[16], *modpath, *filename;
  FILE *infile = NULL;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: module <filename>\n");
    return (FALSE);
  }

  Nmodules = get_int_variable ("MODULES:n", &status);
  if (!status) {
    gprint (GP_ERR, "MODULES list not found\n");
    return (FALSE);
  }

  /* search for requested file in MODULES:0 - MODULES:n */
  for (i = 0; i < Nmodules; i++) {
    snprintf_nowarn (modname, 16, "MODULES:%d", i);
    modpath = get_variable (modname);
    if (modpath == NULL) {
      gprint (GP_ERR, "MODULES list element %d not found\n", i);
      return (FALSE);
    }

    Nbytes = strlen(modpath) + strlen(argv[1]) + 2;
    ALLOCATE (filename, char, Nbytes);
    snprintf (filename, Nbytes, "%s/%s", modpath, argv[1]);
    
    infile = fopen (filename, "r");
    free (filename);

    if (infile != NULL) break;
  }
  if (infile == NULL) {
    gprint (GP_ERR, "module %s not found\n", argv[1]); 
    return (FALSE);
  }
    
  /* read file into the current list */
  NLINES = D_NLINES;
  ALLOCATE (inlist.line, char *, NLINES);
  ALLOCATE (inlist.line[0], char, 1024);
  for (i = 0; (scan_line (infile, inlist.line[i]) != EOF);) {
    stripwhite (inlist.line[i]);
    if (inlist.line[i][0] == 0) continue;
    if (inlist.line[i][0] == '#') continue;
    if (inlist.line[i][0] == '!') continue;

    REALLOCATE (inlist.line[i], char, strlen(inlist.line[i]) + 1);
    if (i == NLINES - 1) {
      NLINES += D_NLINES;
      REALLOCATE (inlist.line, char *, NLINES)
    }
    i++;
    ALLOCATE (inlist.line[i], char, 1024);
  }
  inlist.Nlines = i;
  fclose (infile);

  /* process this list */
  status = exec_loop (&inlist);

  /* cleanup list */
  for (i = 0; i < inlist.Nlines; i++) {
    free (inlist.line[i]);
  }
  free (inlist.line[i]); /* note that we always alloc one extra line */
  free (inlist.line);
  return (status);
}
