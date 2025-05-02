# include "basic.h"
# define D_NLINES 100

int input (int argc, char **argv) {
  
  int i, NLINES, status;
  FILE *infile;
  Macro inlist;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: input <filename>\n");
    return (FALSE);
  }

  infile = fopen (argv[1], "r");
  if (infile == NULL) {
    gprint (GP_ERR, "no file %s\n", argv[1]); 
    return (FALSE);
  }

  /* read file into the current list */
  NLINES = D_NLINES;
  ALLOCATE (inlist.line, char *, NLINES);
  ALLOCATE (inlist.line[0], char, 4096);
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
    ALLOCATE (inlist.line[i], char, 4096);
  }
  inlist.Nlines = i;
  fclose (infile);

  if (!inlist.Nlines) gprint (GP_ERR, "WARNING: input file (%s) was empty\n", argv[1]);

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
